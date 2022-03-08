#include "utils.h"
#include "hooks.h"
#include "./ipc.h"


GlobalInfo* g_pInfo;
const char* g_szExitReason = nullptr; // if set, we'll display this in a message box as we're about to unload ourself


// stuff used in asm
extern "C" {
	bool g_bQueueExit = false; // we should exit on the next engine loop


	HMODULE GetSelfModuleHandle() {
		return g_pInfo->hModule;
	}


	// Called from asm when we want to unload this dll - right before FreeLibrary.
	// IPC cleanup is handled in its destructor in DllMain.
	void PreExitCleanup() {
		g_pInfo->script_mgr.stopScript(); // must be called before we unhook so we can clear keys
		MH_Uninitialize();
		if (g_szExitReason)
			MessageBoxA(0, g_szExitReason, nullptr, MB_OK);
	}
}


void QueueExit(const char* reason) {
	g_bQueueExit = true;
	g_szExitReason = reason;
}


void __stdcall Main(void* _) {

	if (!GetModuleInfo(L"supertuxkart.exe", nullptr, &g_mBase, nullptr)) {
		MessageBoxA(0, "Failed to get module info for supertuxkart.exe", nullptr, MB_OK);
		FreeLibraryAndExitThread(g_pInfo->hModule, 1);
	}

	const char* ipcFailReason = nullptr;
	g_pInfo->ipc.init(ipcFailReason);
	if (ipcFailReason) {
		MessageBoxA(0, ipcFailReason, nullptr, MB_OK);
		FreeLibraryAndExitThread(g_pInfo->hModule, 1);
	}

	if (hooks::HookAll() != MH_OK) {
		MessageBoxA(0, "Failed to hook one or more functions", nullptr, MB_OK);
		MH_Uninitialize();
		FreeLibraryAndExitThread(g_pInfo->hModule, 1);
	}
}


// There's some limitations on what you can do from here cuz of the loader lock,
// summoning another thread for our "real" main seems to be the simplest solution.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			g_pInfo = new GlobalInfo(hModule);
			DisableThreadLibraryCalls(hModule);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
			break;
		case DLL_PROCESS_DETACH:
			delete g_pInfo;
			g_pInfo = nullptr;
			break;
		default:
			break;
	}
	return true;
}
