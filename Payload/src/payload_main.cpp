#include <Windows.h>

struct MainParams {
	MainParams(HMODULE hModule) : hModule(hModule) {}
	HMODULE hModule;
};


DWORD __stdcall Main(MainParams* args) {
	MessageBox(0, L"I am a message", L"Message Box", MB_OK);
	FreeLibraryAndExitThread(args->hModule, 0); // unload self from game
	return 0;
}


// There's some limitations on what you can do from here cuz of the loader lock,
// summoning another thread for our "real" main seems to be the simpliest solution.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		MainParams* params = new MainParams(hModule); // allocate on heap (since this thread will be destroyed)
		DisableThreadLibraryCalls(hModule);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, (LPVOID)params, 0, 0);
	}
	return true;
}
