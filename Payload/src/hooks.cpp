#include "hooks.h"
#include "utils.h"


void* g_mBase = nullptr;


namespace hooks {

	// globals
	
	RaceManager** g_race_manager = nullptr;
	InputManager** input_manager = nullptr;
	PlayerManager** m_player_manager = nullptr;
	StateManager** state_manager_singleton = nullptr;
	MainLoop** main_loop = nullptr;
	STKConfig** stk_config = nullptr;
	World** m_world = nullptr;
	bool* g_is_no_graphics = nullptr;

	// pointers to game functions

	#define DEFINE_GAME_FUNC(name) _##name ORIG_##name = nullptr;

	DEFINE_GAME_FUNC(InputManager__input);
	DEFINE_GAME_FUNC(RaceManager__startSingleRace);
	DEFINE_GAME_FUNC(DeviceManager__getLatestUsedDevice);
	DEFINE_GAME_FUNC(StateManager__createActivePlayer);
	DEFINE_GAME_FUNC(RaceManager__setPlayerKart);
	DEFINE_GAME_FUNC(DeviceManager__setAssignMode);
	DEFINE_GAME_FUNC(MainLoop__getLimitedDt);
	DEFINE_GAME_FUNC(RaceManager__exitRace);
	DEFINE_GAME_FUNC(StateManager__resetActivePlayers);

	#undef DEFINE_GAME_FUNC

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
			MH_FAILED_HOOK(RaceManager__exitRace,  0x2f4330) ||
			MH_FAILED(MH_ApplyQueued())
		) return stat;


		// get plain function pointers (not hooks)
		#define SET_FUNC_PTR(name, offset) ORIG_##name = (_##name)FROM_BASE(offset);

		SET_FUNC_PTR(RaceManager__startSingleRace,       0x2f41b0);
		SET_FUNC_PTR(DeviceManager__getLatestUsedDevice, 0x172020);
		SET_FUNC_PTR(StateManager__createActivePlayer,   0x437640);
		SET_FUNC_PTR(RaceManager__setPlayerKart,         0x2ed210);
		SET_FUNC_PTR(DeviceManager__setAssignMode,       0x171990);
		SET_FUNC_PTR(StateManager__resetActivePlayers,   0x437510);


		// init global pointers
		g_race_manager          =   (RaceManager**) FROM_BASE(0xc8f340);
		input_manager           =  (InputManager**) FROM_BASE(0xc77e20);
		m_player_manager        = (PlayerManager**) FROM_BASE(0xc672e0);
		state_manager_singleton =  (StateManager**) FROM_BASE(0xca3400);
		main_loop               =      (MainLoop**) FROM_BASE(0xc83d80);
		stk_config              =     (STKConfig**) FROM_BASE(0xc67720);
		m_world                 =         (World**) FROM_BASE(0xc87100);
		g_is_no_graphics        =           (bool*) FROM_BASE(0xc72498);


		#undef FAILED_HOOK
		#undef SET_FUNC_PTR
		#undef MH_FAILED

		return MH_OK;
	}


	EventPropagation DETOUR_InputManager__input(InputManager* thisptr, SEvent& event) {
		// don't accept inputs if we're running a script
		if (event.EventType == EET_KEY_INPUT_EVENT &&
			event.KeyInput.Key != IRR_KEY_ESCAPE &&
			g_pInfo->script_mgr.runningScript()
		) return EVENT_BLOCK_BUT_HANDLED;

		return ORIG_InputManager__input(thisptr, event);
	}


	// called from asm if we don't want to exit
	extern "C" float DETOUR_MainLoop__getLimitedDt_Func(MainLoop* thisptr) {
		g_pInfo->ipc.try_accept();
		float dt;
		if (g_pInfo->script_mgr.runningScript()) {
			/*
			* When a script is running, we want to signal the script manager when exactly
			* one physics step has been taken, (otherwise our scripts won't be consistent).
			* This is the function that sleeps if the framerate is too fast; obviously, it
			* would normally cap the speed of the game, so we sleep the exact amount
			* necessary to sync the tickrate and framerate. In theory if we wanted to keep
			* the framerate high when the playspeed is low we could probably just hook a
			* physics step function.
			*/
			float play_speed = g_pInfo->script_mgr.getPlaySpeed();
			int target_frametime_ms;
			if (play_speed == 0) {
				// don't step, but cap framerate because we care about our carbon footprint
				dt = 0;
				target_frametime_ms = int(1.0f / 120.0f * 1000.0f);
			} else {
				dt = 1.0f / (**stk_config).m_physics_fps;
				target_frametime_ms = int(dt / play_speed * 1000);
				g_pInfo->script_mgr.tickSignal();
			}
			// uint64_t cur_time = GetTickCount64();
			int sleep_time_ms = target_frametime_ms - int(GetTickCount64() - prev_time);
			if (play_speed >= 0 && sleep_time_ms > 0)
				Sleep(sleep_time_ms);
		} else {
			dt = ORIG_MainLoop__getLimitedDt(thisptr);
		}
		prev_time = GetTickCount64();
		return dt;
	}


	void DETOUR_RaceManager__exitRace(RaceManager* thisptr, bool delete_world) {
		g_pInfo->script_mgr.stopScript();
		ORIG_RaceManager__exitRace(thisptr, delete_world);
	}
}
