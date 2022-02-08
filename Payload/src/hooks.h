#pragma once
#include "minhook\include\MinHook.h"
#include "game_structures.h"


namespace hooks {

	// return true on success, false on failure
	// basePtr - pointer to the base of supertuxkart.exe
	MH_STATUS HookAll(void* basePtr);

	// the main input function for the game
	typedef EventPropagation (*_InputManager__input)(void* thisptr, SEvent& event);
	extern _InputManager__input ORIG_InputManager__input;
	EventPropagation DETOUR_InputManager__input(void* thisptr, SEvent& event);
}
