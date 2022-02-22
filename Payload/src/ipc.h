#pragma once

#include <WinSock2.h>
#include <string>

class IPC {
public:
	IPC();
	~IPC();
	void accept_loop();

private:
	const char* PORT = "27015";

	SOCKET listen_socket;
	SOCKET client_socket;

	void recv_next(char* dest, int numBytes);
};