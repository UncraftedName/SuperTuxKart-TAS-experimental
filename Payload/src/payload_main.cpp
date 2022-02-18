#include "utils.h"
#include "hooks.h"
#include "./ipc.h"


// any sort of global stuff we might need to keep track of so that we can cleanup in Exit()
struct InfoMain {

	HMODULE hModule; // handle to this dll
	IPC* ipc; // global ipc object

	InfoMain(HMODULE hModule) : hModule(hModule), ipc(nullptr) {}

	~InfoMain() {
		delete ipc;
	}
};


InfoMain* g_Info;


__declspec(noreturn) void Exit(DWORD exitCode, const wchar_t* reason) {
	MH_Uninitialize(); // this should unhook everything
	HMODULE thisMod = g_Info->hModule;
	delete g_Info; // if we don't delete this it technically results in a memory leak
	if (reason)
		MessageBox(0, reason, L"", MB_OK);
	FreeLibraryAndExitThread(thisMod, exitCode);
}


__declspec(noreturn) void __stdcall Main(void* _) {

	if (!utils::GetModuleInfo(L"supertuxkart.exe", nullptr, &g_mBase, nullptr))
		Exit(1, L"failed to get module info for main exe");
	if (hooks::HookAll() != MH_OK)
		Exit(1, L"Failed to hook one or more functions");

	g_Info->ipc = new IPC();
	g_Info->ipc->start();
	Exit(1, L"I'm not supposed to get here...");
}


// There's some limitations on what you can do from here cuz of the loader lock,
// summoning another thread for our "real" main seems to be the simplest solution.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		g_Info = new InfoMain(hModule);
		DisableThreadLibraryCalls(hModule);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
	}
	return true;
}
