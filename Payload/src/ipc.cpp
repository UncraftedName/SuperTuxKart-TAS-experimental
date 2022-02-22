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


// Listen for a client, and accept a single connection. Note that this function only accepts
// one client at a time before returning. To accept multiple clients, it is the user's 
// responsibility to place this start() call in a loop. 
void IPC::accept_loop() {

	// TODO: would be nice to have an exit condition for this
	for (;;) {

		// Wait for an incoming client socket
		client_socket = accept(listen_socket, nullptr, nullptr);
		if (client_socket == INVALID_SOCKET) {
			closesocket(listen_socket);
			WSACleanup();
			Exit(1, L"IPC: Failed to accept client");
		}

		int header_len = 0;
		recv_next((char*)&header_len, 4);

		char* header = new char[header_len];
		recv_next(header, header_len);
		char* header_pos = header;

		// read header fields 

		ScriptData* script = g_Info->script_data = new ScriptData();

		script->map_name.assign(header_pos);
		header_pos += script->map_name.length() + 1;

		script->player_name.assign(header_pos);
		header_pos += script->player_name.length() + 1;

		script->ai_count = *(int*)header_pos;
		header_pos += 4;

		script->laps = *(int*)header_pos;
		header_pos += 4;

		delete[] header;
		
		// read framebulk data

		int bufLen = 0;
		recv_next((char*)&bufLen, 4);
		char* buf = new char[bufLen];
		recv_next(buf, bufLen);

		script->fill_framebulk_data(buf, bufLen);
		delete[] buf;

		g_Info->script_status.set_new_script(script);
		g_Info->script_data = nullptr;
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
