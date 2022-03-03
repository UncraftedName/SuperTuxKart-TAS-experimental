#pragma once

#include <WinSock2.h>
#include <string>

class IPC {
public:
	IPC() : listen_socket(INVALID_SOCKET) {};
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

	SOCKET listen_socket;

	// process buffer, queue unload on error
	void process_msg(const char* buf, size_t size, MessageType type);
};