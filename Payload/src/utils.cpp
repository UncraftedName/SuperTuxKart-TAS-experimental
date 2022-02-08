#include "utils.h"
#include <Psapi.h> // must come after Windows.h

namespace utils {

	bool GetModuleInfo(void* hModule, void** mBase, size_t* mSize) {
		if (!hModule)
			return false;
		MODULEINFO Info;
		GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(hModule), &Info, sizeof(Info));
		if (mBase)
			*mBase = Info.lpBaseOfDll;
		if (mSize)
			*mSize = (size_t)Info.SizeOfImage;
		return true;
	}


	bool GetModuleInfo(const std::wstring& mName, void** hModule, void** mBase, size_t* mSize) {
		HMODULE Handle = GetModuleHandleW(mName.c_str());
		auto ret = GetModuleInfo(Handle, mBase, mSize);
		if (ret && hModule)
			*hModule = Handle;
		return ret;
	}
}
