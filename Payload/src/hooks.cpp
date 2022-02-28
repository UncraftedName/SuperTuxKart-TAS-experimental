#include "hooks.h"
#include "utils.h"


void* g_mBase = nullptr;


// create a struct that decrements the detour thread count once it goes out of scope
#define DETOUR_BEGIN() \
	struct { \
		struct Inner { \
			 Inner() {g_Info->detour_thread_count.fetch_add(1);} \
			~Inner() {g_Info->detour_thread_count.fetch_sub(1);} \
		} __inner; \
	} __detour_scope;



namespace hooks {

	// globals
	
	RaceManager** g_race_manager = nullptr;
	InputManager** input_manager = nullptr;
	PlayerManager** m_player_manager = nullptr;
	StateManager** state_manager_singleton = nullptr;
	MainLoop** main_loop = nullptr;
	STKConfig** stk_config = nullptr;
	bool* g_is_no_graphics = nullptr;

	// pointers to game functions

	_InputManager__input ORIG_InputManager__input = nullptr;
	_RaceManager__startSingleRace ORIG_RaceManager__startSingleRace = nullptr;
	_DeviceManager__getLatestUsedDevice ORIG_DeviceManager__getLatestUsedDevice = nullptr;
	_StateManager__createActivePlayer ORIG_StateManager__createActivePlayer = nullptr;
	_RaceManager__setPlayerKart ORIG_RaceManager__setPlayerKart = nullptr;
	_DeviceManager__setAssignMode ORIG_DeviceManager__setAssignMode = nullptr;
	_MainLoop__getLimitedDt ORIG_MainLoop__getLimitedDt = nullptr;
	_RaceManager__exitRace ORIG_RaceManager__exitRace = nullptr;

	static uint64_t prev_time = 0;


	// (copied doc string from MH_CreateHook)
	/*
	* Creates and queues a Hook for the specified target function.
	* Parameters:
	*   pTarget    [in]  A pointer to the target function, which will be
	*                    overridden by the detour function.
	*   pDetour    [in]  A pointer to the detour function, which will override
	*                    the target function.
	*   pOriginal  [out] A pointer to the trampoline function, which will be
	*                    used to call the original target function.
	*                    This parameter can be NULL.
	*/
	template <typename T>
	inline MH_STATUS QueueFunctionHook(LPVOID pTarget, LPVOID pDetour, T*& pOriginal) {
		MH_STATUS stat = MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(&pOriginal));
		if (stat != MH_OK)
			return stat;
		return MH_QueueEnableHook(pTarget);
	}


	MH_STATUS HookAll() {

		#define MH_FAILED(try_func) ((stat = (try_func)) != MH_OK)
		#define MH_FAILED_HOOK(name, offset) MH_FAILED(QueueFunctionHook(FROM_BASE(offset), &DETOUR_##name, ORIG_##name))


		// hook functions
		MH_STATUS stat;
		if (
			MH_FAILED(MH_Initialize()) ||
			MH_FAILED_HOOK(InputManager__input,    0x17d690) ||
			MH_FAILED_HOOK(MainLoop__getLimitedDt, 0x219770) ||
			MH_FAILED(MH_ApplyQueued())
		) return stat;


		// get plain function pointers (not hooks)
		#define SET_FUNC_PTR(name, offset) ORIG_##name = (_##name)FROM_BASE(offset);

		SET_FUNC_PTR(RaceManager__startSingleRace,       0x2f41b0);
		SET_FUNC_PTR(DeviceManager__getLatestUsedDevice, 0x172020);
		SET_FUNC_PTR(StateManager__createActivePlayer,   0x437640);
		SET_FUNC_PTR(RaceManager__setPlayerKart,         0x2ed210);
		SET_FUNC_PTR(DeviceManager__setAssignMode,       0x171990);
		SET_FUNC_PTR(RaceManager__exitRace,              0x2f4330);


		// init global pointers
		g_race_manager          =   (RaceManager**) FROM_BASE(0xc8f340);
		input_manager           =  (InputManager**) FROM_BASE(0xc77e20);
		m_player_manager        = (PlayerManager**) FROM_BASE(0xc672e0);
		state_manager_singleton =  (StateManager**) FROM_BASE(0xca3400);
		main_loop               =      (MainLoop**) FROM_BASE(0xc83d80);
		stk_config              =     (STKConfig**) FROM_BASE(0xc67720);
		g_is_no_graphics        =           (bool*) FROM_BASE(0xc72498);


		#undef FAILED_HOOK
		#undef GET_FUNC_PTR
		#undef MH_FAILED

		return MH_OK;
	}


	EventPropagation DETOUR_InputManager__input(InputManager* thisptr, SEvent& event) {
		DETOUR_BEGIN();
		// don't accept inputs if we're running a script
		if (event.EventType == EET_KEY_INPUT_EVENT &&
			event.KeyInput.Key != IRR_KEY_ESCAPE &&
			g_Info->script_mgr.running_script()
		) return EVENT_BLOCK_BUT_HANDLED;

		return ORIG_InputManager__input(thisptr, event);
	}


	float DETOUR_MainLoop__getLimitedDt(MainLoop* thisptr) {
		DETOUR_BEGIN();
		uint64_t cur_time = GetTickCount64();
		float dt;
		if (g_Info->script_mgr.running_script()) {
			/*
			* When a script is running, we want to signal the script manager when exactly
			* one physics step has been taken, (otherwise our scripts won't be consistent).
			* This is the function that sleeps if the framerate is too fast; obviously, it
			* would normally cap the speed of the game, so we sleep the exact amount
			* necessary to sync the tickrate and framerate. In theory if we wanted to keep
			* the framerate high when the playspeed is low we could probably just hook a
			* physics step function.
			*/
			float play_speed = g_Info->script_mgr.get_play_speed();
			int target_frametime_ms;
			if (play_speed == 0) {
				// don't step, but cap framerate because we care about our carbon footprint
				dt = 0;
				target_frametime_ms = int(1.0f / 120.0f * 1000.0f);
			} else {
				dt = 1.0f / (**stk_config).m_physics_fps;
				target_frametime_ms = int(dt / play_speed * 1000);
				g_Info->script_mgr.tick_signal();
			}
			int sleep_time_ms = target_frametime_ms - int(cur_time - prev_time);
			if (play_speed >= 0 && sleep_time_ms > 0)
				Sleep(sleep_time_ms);
		} else {
			dt = ORIG_MainLoop__getLimitedDt(thisptr);
		}
		prev_time = GetTickCount64();
		return dt;
	}
}
