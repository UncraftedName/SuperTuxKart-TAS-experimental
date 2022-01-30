#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>


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
					wprintf(L"%s process ID: %lu\n", processName, pE.th32ProcessID);
					return pE.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &pE));
		}
	}
	CloseHandle(hSnap);
	return procId;
}


bool InjectDLL(const wchar_t* processName, const char* dllPath) {

	DWORD procID = GetProcessId(processName);
	if (!procID) {
		std::cout << "Could not get target process ID, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procID);
	if (hProc == INVALID_HANDLE_VALUE) {
		std::cout << "Could not open target process, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!loc) {
		std::cout << "Could not allocate memory in target process, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	if (!WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0)) {
		// doesn't seem to return errors if the dll path is wrong...
		std::cout << "Could not write to target process memory, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
	if (!hThread) {
		// this will give access denied if the architecture doesn't match (e.g. x64 vs x86)
		std::cout << "Could not create remote thread, reason: " << GetLastErrorAsStr() << "\n";
		return false;
	}

	CloseHandle(hThread);
	CloseHandle(hProc);
	return true;
}


bool does_file_exist(const char* filePath) {
	struct stat buffer;
	return (stat(filePath, &buffer) == 0);
}


int main(void) {

	const wchar_t* processName = L"supertuxkart.exe";
	const wchar_t* payloadName = L"Payload.dll";

	// Get full path of dll as char array, most functions need wchars
	// but WriteProcessMemory needs chars, which is just silly.
	wchar_t wdllPath[MAX_PATH];
	char dllPath[MAX_PATH];
	GetFullPathName(payloadName, MAX_PATH, wdllPath, 0);
	size_t ret;
	wcstombs_s(&ret, dllPath, wdllPath, MAX_PATH);

	if (!does_file_exist(dllPath)) {
		std::cout << ("Could not find dll file.\n");
		system("pause");
		return EXIT_FAILURE;
	}

	// TODO - check if the dll is already loaded, attempting to load it twice is :/
	if (InjectDLL(processName, dllPath))
		std::cout << "DLL successfully injected.\n";
	else
		std::cout << "Could not inject DLL.\n";

	system("pause");
	return EXIT_SUCCESS;
}
