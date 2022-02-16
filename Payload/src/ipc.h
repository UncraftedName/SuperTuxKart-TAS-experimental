#pragma once

#include <WinSock2.h>

class IPC {
public:
	IPC();
	~IPC();
	void start();
	const auto& get_fbref() { return output; }
	bool next_framebulk();

private:
	struct IPCResult {
		bool accel = false;
		bool brake = false;
		bool fire  = false;
		bool nitro = false;
		bool skid  = false;
		int ticks = 0;
		float angle = 0.0;
	};

	const char* PORT = "27015";
	const int FB_SIZE = 8;
	const __int64 mask_accel = 0x0010000000000000;
	const __int64 mask_brake = 0x0008000000000000;
	const __int64 mask_fire  = 0x0004000000000000;
	const __int64 mask_nitro = 0x0002000000000000;
	const __int64 mask_skid  = 0x0001000000000000;
	const __int64 mask_ticks = 0x0000FFFF00000000;
	const __int64 mask_angle = 0x00000000FFFFFFFF;

	SOCKET listen_socket;
	SOCKET client_socket;
	IPCResult output;
	char* buf = nullptr;
	int pos = -1;
	int buflen = -1;
};