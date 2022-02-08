#pragma once
#include <Windows.h>
#include <string>


// unload the dll from anywhere
__declspec(noreturn) void Exit(DWORD exitCode);

namespace utils {
	bool GetModuleInfo(const std::wstring& mName, void** hModule, void** mBase, size_t* mSize);
}
