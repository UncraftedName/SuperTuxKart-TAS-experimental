#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>

#include "ipc.h"

#pragma comment(lib, "Ws2_32.lib")

IPC::IPC() {
	client_socket = INVALID_SOCKET;

	WSADATA wsaData;

	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		std::cerr << "IPC: Failed to initialize Winsock2" << std::endl;
		throw 1;
	}

	struct addrinfo* result = nullptr;
	struct addrinfo* ptr = nullptr;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	res = getaddrinfo("127.0.0.1", PORT, &hints, &result);
	if (res != 0) {
		std::cerr << "IPC: Failed to get localhost info" << std::endl;
		WSACleanup();
		throw 1;
	}

	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		std::cerr << "IPC: Error initializing listen socket" << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		throw 1;
	}

	res = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);
	if (res == SOCKET_ERROR) {
		std::cerr << "IPC: Error binding listen socket" << std::endl;
		closesocket(listen_socket);
		WSACleanup();
		throw 1;
	}
}

IPC::~IPC() {

}

void IPC::start() {
	int res = listen(listen_socket, 1);
	if (res == SOCKET_ERROR) {
		std::cerr << "IPC: Listen failed" << std::endl;
		closesocket(listen_socket);
		WSACleanup();
		throw 1;
	}

	// Launch thread
	std::thread t(&accept_loop);
}

void IPC::stop() {

}

void IPC::accept_loop() {
	while (true) {
		client_socket = accept(listen_socket, nullptr, nullptr);
		if (client_socket == INVALID_SOCKET) {
			std::cerr << "IPC: Failed to accept client" << std::endl;
			closesocket(listen_socket);
			WSACleanup();
			throw 1;
		}

		// handle connection
	}
}