#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "minhook\include\MinHook.h"
#include "game_structures.h"

// pointer to start of supertuxkart.exe, not initialized until HookAll()
extern void* g_mBase;


#define FROM_BASE(offset) ((void*)((uintptr_t)g_mBase + offset))


// dereference object to get vtable address, get entry in vtable, call it
#define CALL_VIRTUAL_FUNC(func_type, obj, vtable_off, ...) \
	(*(func_type*)(*(uintptr_t*)obj + vtable_off * sizeof(func_type*)))(__VA_ARGS__)


namespace hooks {

	MH_STATUS HookAll();


	// globals
	extern RaceManager** g_race_manager;
	extern InputManager** input_manager;
	extern PlayerManager** m_player_manager;
	extern StateManager** state_manager_singleton;
	extern MainLoop** main_loop;
	extern STKConfig** stk_config;
	extern World** m_world;
	extern bool* g_is_no_graphics;


	// Use this if you just want a function pointer. Creates a typedef of the function pointer
	// called _name and declares the function pointer to the "original" game function.
	#define DECLARE_FUNC(name, ret, ...) \
		typedef ret (*_##name)(__VA_ARGS__); \
		extern _##name ORIG_##name;


	// Use this if you actually want to hook a function and implement a detour. Same macro as
	// above, but also declares a function called DETOUR_name which MinHook will jump to.
	#define DECLARE_HOOK(name, ret, ...) \
		DECLARE_FUNC(name, ret, __VA_ARGS__) \
		ret DETOUR_##name(__VA_ARGS__);


	// the main input function for the game
	DECLARE_HOOK(InputManager__input, EventPropagation, InputManager* thisptr, SEvent& event);

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
	
	// exits track
	DECLARE_HOOK(RaceManager__exitRace, void, RaceManager* thisptr, bool delete_world);

	// removes all players from the player list
	DECLARE_FUNC(StateManager__resetActivePlayers, void, StateManager* thisptr);

	// called once per engine loop to determine the next timestep to take, detour is partially implemented in asm
	DECLARE_FUNC(MainLoop__getLimitedDt, float, MainLoop* thisptr);
	extern "C" float DETOUR_MainLoop__getLimitedDt(MainLoop* thisptr);

	// virtual function for World, called when reloading/restarting a world/race
	typedef void (*_World__reset)(World* thisptr, bool restart);
}
