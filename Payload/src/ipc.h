#pragma once

#include <WinSock2.h>

class IPC {
public:
	IPC();
	~IPC();
	void start();
	void stop();

private:
	const char* PORT = "27015";
	SOCKET listen_socket;
	SOCKET client_socket;

	void accept_loop();
};