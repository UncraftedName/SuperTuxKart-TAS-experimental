#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <string>

#include "framebulk.h"
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
void IPC::start() {
	if (buf != nullptr) {
		delete[] buf;
		buf = nullptr;
		pos = -1;
		buflen = -1;
	}

	int res = listen(listen_socket, 1);
	if (res == SOCKET_ERROR) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Listen failed");
	}

	client_socket = accept(listen_socket, nullptr, nullptr);
	if (client_socket == INVALID_SOCKET) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Failed to accept client");
	}

	int header_len = 0;
	res = recv(client_socket, (char *) &header_len, 4, 0);
	if (res != 4) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Failed to receive data from client");
	}

	char* header = new char[header_len];
	res = recv(client_socket, header, header_len, 0);
	if (res != header_len) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Failed to receive data from client");
	}

	char* header_pos = header;
	map_name.assign(header_pos);
	while (*header_pos != '\0') {
		header_pos++;
	}
	header_pos++;

	player_name.assign(header_pos);
	while (*header_pos != '\0') {
		header_pos++;
	}
	header_pos++;

	memcpy(&ai_count, header_pos, 4);
	header_pos += 4;

	memcpy(&laps, header_pos, 4);
	header_pos += 4;

	memcpy(&buflen, header_pos, 4);
	delete[] header;

	buf = new char[buflen];
	res = recv(client_socket, buf, buflen, 0);
	if (res != buflen) {
		closesocket(listen_socket);
		WSACleanup();
		delete[] buf;
		Exit(1, L"IPC: Failed to receive data from client");
	}
	pos = 0;
}

// Get the name of the map
const std::string &IPC::get_map_name() {
	return map_name;
}

// Get the name of the player's kart
const std::string& IPC::get_player_name() {
	return player_name;
}

// Get the number of AI opponents
int IPC::get_ai_count() {
	return ai_count;
}

// Get the number of laps
int IPC::get_num_laps() {
	return laps;
}

// Get all framebulks. Should only be called after start() has finished successfully.
// Returns a pointer to a vector of Framebulks. It is the user's responsibility to free
// this memory once done with it. If there is no framebulk data to return (start has not been called/
// this function was already called before), returns null. 
const auto *IPC::get_framebulks() {


	return (std::vector<Framebulk> *)nullptr;
}