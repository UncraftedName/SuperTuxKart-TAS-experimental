#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <string>

#include "script_data.h"
#include "ipc.h"
#include "utils.h"

#pragma comment(lib, "Ws2_32.lib")


// Constructs an IPC object. Handles creating and binding the socket. 
// For accepting clients, see the IPC::start() function
IPC::IPC() {
	client_socket = INVALID_SOCKET;
	listen_socket = INVALID_SOCKET;

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		Exit(1, L"IPC: Failed to initialize Winsock2");

	struct addrinfo* result = nullptr;
	struct addrinfo* ptr = nullptr;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo("127.0.0.1", PORT, &hints, &result) != 0) {
		WSACleanup();
		Exit(1, L"IPC: Failed to get localhost info");
	}

	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		freeaddrinfo(result);
		WSACleanup();
		Exit(1, L"IPC: Error initializing listen socket");
	}

	int res = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);
	if (res == SOCKET_ERROR) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Error binding listen socket");
	}

	// Initializes buffer supporting 1 incoming connection
	res = listen(listen_socket, 1);
	if (res == SOCKET_ERROR) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Listen failed");
	}
}


// IPC destructor. Cleans up memory and closes socket
IPC::~IPC() {
	WSACleanup();
	if (listen_socket != INVALID_SOCKET)
		closesocket(listen_socket);
}


// accept and process all connections
void IPC::accept_loop() {

	for (;;) {

		// Wait for an incoming client socket
		client_socket = accept(listen_socket, nullptr, nullptr);
		if (client_socket == INVALID_SOCKET) {
			closesocket(listen_socket);
			WSACleanup();
			Exit(1, L"IPC: Failed to accept client");
		}

		int size = 0;
		recv_next((char*)&size, 4);
		if (size == 0)
			Exit(1, L"IPC: Could not deduce message type");

		// set the buffer in g_Info so that it can be cleaned up in case we decide to exit
		char* buf = g_Info->tmp_ipc_buf = new char[size];
		recv_next(buf, size);

		MessageType type = (MessageType)*buf;
		process_msg(buf + 1, (size_t)size - 1, type);

		g_Info->tmp_ipc_buf = nullptr;
		delete[] buf;
	}
}

void IPC::recv_next(char* dest, int numBytes) {
	int bytesRead = recv(client_socket, dest, numBytes, 0);
	if (bytesRead != numBytes) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Failed to receive data from client");
	}
}

bool IPC::process_msg(const char* buf, size_t size, MessageType type) {
	const char* buf_orig = buf;
	switch (type) {
		case MessageType::Script: {

			ScriptData* script = new ScriptData();

			// read header fields

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
			g_Info->script_mgr.set_new_script(script);
			break;
		}
		case MessageType::Unload:
			Exit(0);
		default:
			return false;
	}
	return true;
}
