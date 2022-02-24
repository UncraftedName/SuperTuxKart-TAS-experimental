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

	enum class MessageType : uint8_t {
		Script = 0,
		Unload
	};

	SOCKET listen_socket;
	SOCKET client_socket;

	void recv_next(char* dest, int numBytes);

	bool process_msg(const char* buf, size_t size, MessageType type);
};