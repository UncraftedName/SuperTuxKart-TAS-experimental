#include <Windows.h>

struct MainParams {
	HMODULE hModule;
	MainParams(HMODULE hModule) : hModule(hModule) {}
};


DWORD __stdcall Main(MainParams* args) {
	MessageBox(0, L"I am a message", L"Message Box", MB_OK);
	HMODULE thisMod = args->hModule;
	delete args;
	FreeLibraryAndExitThread(thisMod, 0); // unload self from game, don't do anything after this
	return 0;
}


// There's some limitations on what you can do from here cuz of the loader lock,
// summoning another thread for our "real" main seems to be the simplest solution.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		MainParams* params = new MainParams(hModule); // allocate on heap (since this thread will be destroyed)
		DisableThreadLibraryCalls(hModule);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, (LPVOID)params, 0, 0);
	}
	return true;
}
