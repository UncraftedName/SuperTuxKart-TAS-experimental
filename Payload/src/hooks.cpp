#include "hooks.h"


void* g_mBase = nullptr;


namespace hooks {

	// globals
	
	RaceManager** g_race_manager = nullptr;
	InputManager** input_manager = nullptr;
	PlayerManager** m_player_manager = nullptr;
	StateManager** state_manager_singleton = nullptr;

	// pointers to game functions

	_InputManager__input ORIG_InputManager__input = nullptr;
	_InputManager__update ORIG_InputManager__update = nullptr;
	_RaceManager__startSingleRace ORIG_RaceManager__startSingleRace = nullptr;
	_DeviceManager__getLatestUsedDevice ORIG_DeviceManager__getLatestUsedDevice = nullptr;
	_StateManager__createActivePlayer ORIG_StateManager__createActivePlayer = nullptr;
	_RaceManager__setPlayerKart ORIG_RaceManager__setPlayerKart = nullptr;
	_DeviceManager__setAssignMode ORIG_DeviceManager__setAssignMode = nullptr;


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

		#define FAILED_HOOK(name, offset) (stat = QueueFunctionHook(FROM_BASE(offset), &DETOUR_##name, ORIG_##name)) != MH_OK
		#define SET_FUNC_PTR(name, offset) ORIG_##name = (_##name)FROM_BASE(offset);


		// hook functions
		MH_STATUS stat;
		if (
			(stat = MH_Initialize()) != MH_OK ||
			FAILED_HOOK(InputManager__input, 0x17d690) ||
			FAILED_HOOK(InputManager__update, 0x17ad70) ||
			(stat = MH_ApplyQueued()) != MH_OK
		) return stat;


		// get plain function pointers (not hooks)
		SET_FUNC_PTR(RaceManager__startSingleRace, 0x2f41b0);
		SET_FUNC_PTR(DeviceManager__getLatestUsedDevice, 0x172020);
		SET_FUNC_PTR(StateManager__createActivePlayer, 0x437640);
		SET_FUNC_PTR(RaceManager__setPlayerKart, 0x2ed210);
		SET_FUNC_PTR(DeviceManager__setAssignMode, 0x171990);


		// init global pointers
		g_race_manager = (RaceManager**)FROM_BASE(0xc8f340);
		input_manager = (InputManager**)FROM_BASE(0xc77e20);
		m_player_manager = (PlayerManager**)FROM_BASE(0xc672e0);
		state_manager_singleton = (StateManager**)FROM_BASE(0xca3400);


		#undef FAILED_HOOK
		#undef GET_FUNC_PTR

		return MH_OK;
	}


	// just inverts left/right arrow key movement (even in the menu)
	EventPropagation DETOUR_InputManager__input(InputManager* thisptr, SEvent& event) {
		if (event.EventType == EET_KEY_INPUT_EVENT) {
			if (event.KeyInput.Key == IRR_KEY_LEFT)
				event.KeyInput.Key = IRR_KEY_RIGHT;
			else if (event.KeyInput.Key == IRR_KEY_RIGHT)
				event.KeyInput.Key = IRR_KEY_LEFT;
		}
		return ORIG_InputManager__input(thisptr, event);
	}


	void LoadMap() {
		/*
		* This is equivalent to the following game code:
		*
		* auto device = input_manager->getDeviceManager()->getLatestUsedDevice();
		* auto profile = PlayerManager::getCurrentPlayer();
		* StateManager::get()->createActivePlayer(profile, device);
		* RaceManager::get()->setPlayerKart(0, "tux");
		* RaceManager::get()->setNumKarts(0);
		* RaceManager::get()->setNumLaps(1);
		* RaceManager::get()->startSingleRace("abyss", 1, false);
		*/
		ORIG_DeviceManager__setAssignMode((**input_manager).m_device_manager, ASSIGN);
		auto device = ORIG_DeviceManager__getLatestUsedDevice((**input_manager).m_device_manager);
		auto profile = (**m_player_manager).m_current_player;
		ORIG_StateManager__createActivePlayer(*state_manager_singleton, profile, device);
		ORIG_RaceManager__setPlayerKart(*g_race_manager, 0, "tux");
		(**g_race_manager).setNumKarts(0);
		// TODO set num laps
		ORIG_RaceManager__startSingleRace(*g_race_manager, "abyss", 1, false);
	}


	static int tick = 0;

	/*
	* It doesn't really matter what this function does, what matters is when it's called. It's in the
	* main engine loop before some inputs are handled. This is probably a nice place to send inputs
	* to the game, as well as loading new maps and stuff.
	*/
	void DETOUR_InputManager__update(InputManager* thisptr, float dt) {
		ORIG_InputManager__update(thisptr, dt);
		if (tick++ < 1)
			LoadMap();
		else {
			SEvent e;
			e.EventType = EET_KEY_INPUT_EVENT;
			auto& eki = e.KeyInput;
			eki.Char = 0;
			eki.Key = IRR_KEY_UP;
			eki.SystemKeyCode = 0;
			eki.PressedDown = 1;
			eki.Shift = 0;
			eki.Control = 0;
			ORIG_InputManager__input(*input_manager, e);
		}
	}
}
