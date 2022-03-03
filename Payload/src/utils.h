#pragma once

// This is required when using WinSock2.h & Windows.h,
// so this must come before Windows.h & ipc.h :/.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>
#include "script_data.h"
#include "ipc.h"


// any sort of stuff we might need to keep track of so that we can cleanup in Exit()
struct GlobalInfo {
public:
	// handle to this dll, we'll need this to unload
	HMODULE hModule;
	// singleton ipc object
	IPC ipc;
	// single object to keep track of where we are in the TAS script
	ScriptManager script_mgr;

	GlobalInfo(HMODULE hModule) : hModule(hModule) {}
};


extern GlobalInfo* g_pInfo;


// Signal that we want to exit as soon as possible (will exit during next engine loop).
// Only usable after hooks have been initialized. If reason is specified, will display
// a message box with the reason on exit.
void QueueExit(const char* reason = nullptr);

bool GetModuleInfo(const std::wstring& mName, void** hModule, void** mBase, size_t* mSize);
