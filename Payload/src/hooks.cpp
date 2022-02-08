#include "hooks.h"

namespace hooks {

	_InputManager__input ORIG_InputManager__input = nullptr;

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


	MH_STATUS HookAll(void* basePtr) {

		MH_STATUS stat;
		if (
			(stat = MH_Initialize()) != MH_OK ||
			(stat = QueueFunctionHook((LPVOID)((uintptr_t)basePtr + 0x17d690), &DETOUR_InputManager__input, ORIG_InputManager__input)) != MH_OK ||
			// add more hooks here
			(stat = MH_ApplyQueued()) != MH_OK
		) return stat;

		return MH_OK;
	}


	// just inverts left/right arrow key movement (even in the menu)
	EventPropagation DETOUR_InputManager__input(void* thisptr, SEvent& event) {
		if (event.EventType == EET_KEY_INPUT_EVENT) {
			if (event.KeyInput.Key == IRR_KEY_LEFT)
				event.KeyInput.Key = IRR_KEY_RIGHT;
			else if (event.KeyInput.Key == IRR_KEY_RIGHT)
				event.KeyInput.Key = IRR_KEY_LEFT;
		}
		return ORIG_InputManager__input(thisptr, event);
	}
}
