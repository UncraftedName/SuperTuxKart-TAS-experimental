#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>

#include "ipc.h"
#include "utils.h"

#pragma comment(lib, "Ws2_32.lib")

IPC::IPC() {
	client_socket = INVALID_SOCKET;

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

IPC::~IPC() {
	if (listen_socket != INVALID_SOCKET)
		closesocket(listen_socket);
}

void IPC::start() {
	int res = listen(listen_socket, 1);
	if (res == SOCKET_ERROR) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Listen failed");
	}
	accept_loop();
}

void IPC::stop() {

}

void IPC::accept_loop() {
	while (true) {
		client_socket = accept(listen_socket, nullptr, nullptr);
		if (client_socket == INVALID_SOCKET) {
			closesocket(listen_socket);
			WSACleanup();
			Exit(1, L"IPC: Failed to accept client");
		}

		// handle connection
		
	}
}