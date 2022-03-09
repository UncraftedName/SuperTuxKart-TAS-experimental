#include <iostream>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>


std::string GetLastErrorAsStr() {
	DWORD dw = GetLastError();
	LPSTR szMsg = nullptr;
	// convert the error to a string - Win32 allocates and creates the message
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, 0, (LPSTR)&szMsg, 0, NULL);

	std::string str(szMsg, size);
	// free Win32's buffer
	LocalFree(szMsg);
	return str;
}


// most of this code is yoinked from (or rather, inspired by) https://github.com/saeedirha/DLL-Injector


// go through all processes and find the ID of the first one that matches the given string
DWORD GetProcessId(const wchar_t* processName) {
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pE = {0};
		pE.dwSize = sizeof(pE);
		if (Process32First(hSnap, &pE)) {
			if (!pE.th32ProcessID)
				Process32Next(hSnap, &pE);
			do {
				if (!wcscmp(pE.szExeFile, processName)) {
					procId = pE.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &pE));
		}
	}
	CloseHandle(hSnap);
	return procId;
}


bool IsModuleLoaded(HANDLE hProc, const wchar_t* modName) {
	
	HMODULE hMods[1024] = {0};
	DWORD bytesNeeded;

	// get modules in process
	if (!EnumProcessModules(hProc, hMods, sizeof(hMods), &bytesNeeded)) {
		// this should never happen... premature exit
		std::cout << "Could not enumerate modules in process, ";
		if (bytesNeeded > sizeof(hMods) / sizeof(HMODULE))
			std::cout << "too few entries\n";
		else
			std::cout << "reason: " << GetLastErrorAsStr() << "\n";
		exit(EXIT_FAILURE);
	}

	/*
	 * Compare each module to the name given; this only compares the base name, so if this process
	 * has already loaded e.g. Payload.dll (even if it's not our dll) this would return true.
	 * It would be better to compare against the full path, but for the time being this will be fine
	 * since I don't want someone trying to load a release and debug dll together. It would probably
	 * be better if the dll somehow detected that some version of it has already been loaded.
	 */
	wchar_t curModName[MAX_PATH];
	for (int i = 0; i < bytesNeeded / sizeof(HMODULE); i++)
		if (GetModuleBaseName(hProc, hMods[i], curModName, MAX_PATH) && !wcscmp(modName, curModName))
			return true;
	return false;
}


bool InjectDLL(const wchar_t* processName, const wchar_t* dllPath, const wchar_t* dllName) {

	DWORD procID = GetProcessId(processName);
	if (!procID) {
		std::cout << "Could not find target process ID (is the game open?)\n";
		return false;
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procID);
	if (hProc == INVALID_HANDLE_VALUE) {
		std::cout << "Could not open target process, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	if (IsModuleLoaded(hProc, dllName)) {
		// premature exit
		std::cout << "DLL already loaded\n";
		return true;
	}

	void* loc = VirtualAllocEx(hProc, 0, MAX_PATH * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!loc) {
		std::cout << "Could not allocate memory in target process, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	if (!WriteProcessMemory(hProc, loc, dllPath, wcslen(dllPath) * sizeof(wchar_t) + 1, 0)) {
		// doesn't seem to return errors if the dll path is wrong...
		std::cout << "Could not write to target process memory, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, loc, 0, 0);
	if (!hThread) {
		// this will give access denied if the architecture doesn't match (e.g. x64 vs x86)
		std::cout << "Could not create remote thread, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	CloseHandle(hThread);
	CloseHandle(hProc);
	return true;
}


bool DoesFileExist(const wchar_t* filePath) {
	struct _stat64 buffer;
	return (_wstat64(filePath, &buffer) == 0);
}


// gets dll path in current dir
void GetDllPath(const wchar_t* dllName, wchar_t* dllPath /* out */) {
	wchar_t buffer[MAX_PATH];
	// Stores in buffer the current exe path (...\Injector.exe)
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	std::wstring path = buffer;
	// Replaces "Injector.exe" in path with payloadName and copy to wdllPath
	path = path.substr(0, path.find_last_of(L"\\") + 1).append(dllName);
	path.copy(dllPath, MAX_PATH);
	dllPath[path.length()] = L'\0';
}


int main(void) {

	const wchar_t* processName = L"supertuxkart.exe";
	const wchar_t* payloadName = L"Payload.dll";

	wchar_t dllPath[MAX_PATH];
	GetDllPath(payloadName, dllPath);

	if (!DoesFileExist(dllPath)) {
		std::cout << "Could not find dll file.\n";
		return EXIT_FAILURE;
	}

	if (InjectDLL(processName, dllPath, payloadName)) {
		std::cout << "DLL successfully injected.\n";
		return EXIT_SUCCESS;
	} else {
		std::cout << "Could not inject DLL.\n";
		return EXIT_FAILURE;
	}
}
