#pragma once

#include <WinSock2.h>
#include <string>

class IPC {
public:
	~IPC();

	// Initialize WinSock and the listen_socket, on failure sets the failReason.
	void init(const char*& failReason /*out*/);

	// Check if we have data to read, process it if so. Should only be called once we have the hook set up.
	void try_accept();

private:
	const char* PORT = "27015";

	enum class MessageType : uint8_t {
		Script = 0,
		Unload
	};

	SOCKET listen_socket = INVALID_SOCKET;
	SOCKET client_socket = INVALID_SOCKET;
	// how many ticks we've been holding on to the client socket
	int cl_sock_hold_count = 0;

	// max number of ticks to hold on the client socket for
	const int MAX_CL_SOCK_HOLD_COUNT = 2;

	// process buffer, queue unload on error
	void process_msg(const char* buf, size_t size, MessageType type);
};