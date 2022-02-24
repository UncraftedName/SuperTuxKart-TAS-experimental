#include "utils.h"
#include "hooks.h"
#include "./ipc.h"


GlobalInfo* g_Info;


__declspec(noreturn) void Exit(DWORD exitCode, const wchar_t* reason) {

	// SPIIIIIIIIIIIIIIIIIIIIIIIN
	while (g_Info->detour_thread_count.load() > 0) {
		if (g_Info->script_mgr.running_script())
			g_Info->script_mgr.stop_script();
		Sleep(1);
	}

	MH_Uninitialize(); // this should unhook everything, pray that game threads are not in detours
	if (reason)
		MessageBox(0, reason, L"", MB_OK);
	FreeLibraryAndExitThread(g_Info->hModule, exitCode);
}


__declspec(noreturn) void __stdcall Main(void* _) {

	if (!utils::GetModuleInfo(L"supertuxkart.exe", nullptr, &g_mBase, nullptr))
		Exit(1, L"failed to get module info for main exe");
	if (hooks::HookAll() != MH_OK)
		Exit(1, L"Failed to hook one or more functions");

	g_Info->ipc.accept_loop();

	Exit(1, L"IPC accept loop exited");
}


// There's some limitations on what you can do from here cuz of the loader lock,
// summoning another thread for our "real" main seems to be the simplest solution.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			g_Info = new GlobalInfo(hModule);
			DisableThreadLibraryCalls(hModule);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
			break;
		case DLL_PROCESS_DETACH:
			delete g_Info;
			break;
		default:
			break;
	}
	return true;
}
