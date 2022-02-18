#pragma once
#include "minhook\include\MinHook.h"
#include "game_structures.h"

// pointer to start of supertuxkart.exe, not initialized until HookAll()
extern void* g_mBase;


#define FROM_BASE(offset) ((void*)((uintptr_t)g_mBase + offset))


namespace hooks {

	MH_STATUS HookAll();


	// use this if you just want a function pointer
	#define DECLARE_FUNC(name, ret, ...) \
		typedef ret (*_##name)(__VA_ARGS__); \
		extern _##name ORIG_##name;


	// use this if you actually want to hook a function and implement a detour
	#define DECLARE_HOOK(name, ret, ...) \
		DECLARE_FUNC(name, ret, __VA_ARGS__) \
		ret DETOUR_##name(__VA_ARGS__);


	// the main input function for the game
	DECLARE_HOOK(InputManager__input, EventPropagation, void* thisptr, SEvent& event);
}
