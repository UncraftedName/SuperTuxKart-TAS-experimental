#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "minhook\include\MinHook.h"
#include "game_structures.h"

// pointer to start of supertuxkart.exe, not initialized until HookAll()
extern void* g_mBase;


#define FROM_BASE(offset) ((void*)((uintptr_t)g_mBase + offset))


namespace hooks {

	MH_STATUS HookAll();


	// globals
	extern RaceManager** g_race_manager;
	extern InputManager** input_manager;
	extern PlayerManager** m_player_manager;
	extern StateManager** state_manager_singleton;


	// use this if you just want a function pointer
	#define DECLARE_FUNC(name, ret, ...) \
		typedef ret (*_##name)(__VA_ARGS__); \
		extern _##name ORIG_##name;


	// use this if you actually want to hook a function and implement a detour
	#define DECLARE_HOOK(name, ret, ...) \
		DECLARE_FUNC(name, ret, __VA_ARGS__) \
		ret DETOUR_##name(__VA_ARGS__);


	// the main input function for the game
	DECLARE_HOOK(InputManager__input, EventPropagation, InputManager* thisptr, SEvent& event);

	// called once a tick(?) near GUI/Input stuff
	DECLARE_HOOK(InputManager__update, void, InputManager* thisptr, float dt);

	// starts a new track
	DECLARE_FUNC(RaceManager__startSingleRace, void, RaceManager* thisptr, const std::str_wrap& track_ident, const int num_laps, bool from_overworld);

	// probably returns keyboard, we need this to create an active player
	DECLARE_FUNC(DeviceManager__getLatestUsedDevice, InputDevice*, DeviceManager* thisptr);

	// adds a new entry to m_active_players in the state manager
	DECLARE_FUNC(StateManager__createActivePlayer, int, StateManager* thisptr, PlayerProfile* profile, InputDevice* device);

	// sets the kart for the given player profile
	DECLARE_FUNC(RaceManager__setPlayerKart, void, RaceManager* thisptr, uint32_t player_id, const std::str_wrap& kart_name);

	// sets assign mode for input devices (we need to set this so that the game can map our inputs to a specific player)
	DECLARE_FUNC(DeviceManager__setAssignMode, void, DeviceManager* thisptr, const PlayerAssignMode assignMode);
}
