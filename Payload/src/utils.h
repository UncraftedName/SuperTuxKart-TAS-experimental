#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>


// unload the dll from anywhere
__declspec(noreturn) void Exit(DWORD exitCode);

namespace utils {
	bool GetModuleInfo(const std::wstring& mName, void** hModule, void** mBase, size_t* mSize);
}
