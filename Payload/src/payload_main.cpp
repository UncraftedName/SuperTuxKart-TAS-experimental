#include "utils.h"
#include "hooks.h"


// any sort of global stuff we might need to keep track of so that we can cleanup in Exit()
struct InfoMain {
	HMODULE hModule; // handle to this dll
	InfoMain(HMODULE hModule) : hModule(hModule) {}
};


InfoMain* g_Info;


__declspec(noreturn) void Exit(DWORD exitCode) {
	MH_Uninitialize(); // this should unhook everything
	HMODULE thisMod = g_Info->hModule;
	delete g_Info; // if we don't delete this it technically results in a memory leak
	FreeLibraryAndExitThread(thisMod, exitCode);
}


DWORD __stdcall Main(void* _) {

	void* mBase = nullptr;
	if (!utils::GetModuleInfo(L"supertuxkart.exe", nullptr, &mBase, nullptr)) {
		MessageBox(0, L"failed to get module info for main exe", L"", MB_OK);
		Exit(1);
	}
	if (hooks::HookAll(mBase) != MH_OK) {
		MessageBox(0, L"Failed to hook one or more functions", L"", MB_OK);
		Exit(1);
	}

	// TODO: @ryan replace this with your ipc stuff
	MessageBox(0, L"When you close this the dll will unload", L"", MB_OK);
	Exit(0);
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
