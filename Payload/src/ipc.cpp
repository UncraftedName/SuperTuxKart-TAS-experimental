#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <string>

#include "script_data.h"
#include "ipc.h"
#include "utils.h"

#pragma comment(lib, "Ws2_32.lib")


// Initializes WSA & the listen_socket. For accepting clients, see IPC::try_accept().
// WSACleanup & closesocket are NOT called on failure, as they are called in the destructor.
void IPC::init(const char*& failReason) {

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		failReason = "IPC: Failed to initialize Winsock2";
		return;
	}

	struct addrinfo* result = nullptr;
	struct addrinfo* ptr = nullptr;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo("127.0.0.1", PORT, &hints, &result) != 0) {
		failReason = "IPC: Failed to get localhost info";
		return;
	}

	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		freeaddrinfo(result);
		failReason = "IPC: Error initializing listen socket";
		return;
	}

	int res = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);
	if (res == SOCKET_ERROR) {
		failReason = "IPC: Error binding listen socket";
		return;
	}

	// Initializes buffer supporting 1 incoming connection
	res = listen(listen_socket, 1);
	if (res == SOCKET_ERROR) {
		failReason = "IPC: Listen failed";
		return;
	}

	// set socket to be non-blocking
	u_long block_mode = 1;
	if (ioctlsocket(listen_socket, FIONBIO, &block_mode) == SOCKET_ERROR) {
		failReason = "IPC: Could not set socket to be non-blocking";
		return;
	}
}


// IPC destructor. Cleans up memory and closes socket
IPC::~IPC() {
	if (listen_socket != INVALID_SOCKET)
		closesocket(listen_socket);
	WSACleanup();
}


void IPC::try_accept() {

	// since the socket is non-blocking, this just polls to see if we have any connections
	SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
	if (client_socket == INVALID_SOCKET) {
		// do we not have any connections or is this an actual error?
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			QueueExit("IPC: accept() failed");
		return;
	}

	FD_SET read_set = {};
	FD_ZERO(&read_set);
	FD_SET(client_socket, &read_set);
	const timeval tval = {0, 10};

	int ret;
	if ((ret = select(0, &read_set, nullptr, nullptr, &tval)) == SOCKET_ERROR) {
		QueueExit("IPC: select() returned with error");
		return;
	}

	if (ret == 0) {
		closesocket(client_socket); // no data
		return;
	}

	// you've got mail!

	uint32_t size = 0;
	if (recv(client_socket, (char*)&size, 4, 0) != 4 || size == 0) {
		closesocket(client_socket);
		QueueExit("IPC: Could not deduce message size & message type");
		return;
	}

	char* buf = new char[size];
	if (recv(client_socket, buf, size, 0) != size) {
		closesocket(client_socket);
		QueueExit("IPC: bad message format");
		return;
	}

	MessageType type = (MessageType)*buf;
	process_msg(buf + 1, (size_t)size - 1, type);
	delete[] buf;
	closesocket(client_socket);
}

void IPC::process_msg(const char* buf, size_t size, MessageType type) {
	const char* buf_orig = buf;
	switch (type) {
		case MessageType::Script: {

			ScriptData* script = new ScriptData();

			// read header fields, would be a good idea to check for overflow here

			script->map_name.assign(buf);
			buf += script->map_name.length() + 1;

			script->player_name.assign(buf);
			buf += script->player_name.length() + 1;

			script->ai_count = *(int*)buf;
			buf += 4;

			script->laps = *(int*)buf;
			buf += 4;

			script->difficulty = *(Difficulty*)buf;
			buf += 4;

			// read framebulk data

			script->fill_framebulk_data(buf, size - (buf - buf_orig));
			g_pInfo->script_mgr.set_new_script(script);
			break;
		}
		case MessageType::Unload:
			QueueExit();
			break;
		default:
			QueueExit("IPC: bad message type");
			break;
	}
	return;
}
