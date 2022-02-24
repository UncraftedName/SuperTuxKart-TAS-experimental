#pragma once

// This is required when using WinSock2.h & Windows.h,
// so this must come before Windows.h & ipc.h :/.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>
#include <atomic>
#include "script_data.h"
#include "ipc.h"


// unload the dll from anywhere, reason can be null
__declspec(noreturn) void Exit(DWORD exitCode, const wchar_t* reason = nullptr);


// any sort of stuff we might need to keep track of so that we can cleanup in Exit()
struct GlobalInfo {
public:
	// handle to this dll
	HMODULE hModule;
	// singleton ipc object
	IPC ipc;
	// single object to keep track of where we are in the TAS script
	ScriptManager script_mgr;
	// we might get a premature exit before we can delete this
	char* tmp_ipc_buf = nullptr;
	// do our best to prevent unloading while a game thread is in a hooked function
	std::atomic_int detour_thread_count;

	GlobalInfo(HMODULE hModule) : hModule(hModule) {}

	~GlobalInfo() {
		delete tmp_ipc_buf;
	}
};


extern GlobalInfo* g_Info;


namespace utils {
	bool GetModuleInfo(const std::wstring& mName, void** hModule, void** mBase, size_t* mSize);
}
