#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

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
	WSACleanup();
	if (listen_socket != INVALID_SOCKET)
		closesocket(listen_socket);
}

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

	int len = 0;
	res = recv(client_socket, (char *) &len, 4, 0);
	if (res != 4) {
		closesocket(listen_socket);
		WSACleanup();
		Exit(1, L"IPC: Failed to receive data from client");
	}

	buflen = ntohl(len);
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

bool IPC::next_framebulk() {
	if (buf == nullptr) {
		return false;
	}

	__int64 fb = 0;
	memcpy(&fb, buf + pos, FB_SIZE);
	fb = ntohll(fb);
	pos += FB_SIZE;
	if (pos == buflen) {
		delete[] buf;
		buf = nullptr;
		pos = -1;
		buflen = -1;
	}

	output.accel = fb & mask_accel;
	output.brake = fb & mask_brake;
	output.fire = fb & mask_fire;
	output.nitro = fb & mask_nitro;
	output.skid = fb & mask_skid;
	output.angle = (float)(fb & mask_angle);
	output.ticks = (int)((fb & mask_ticks) >> 32);
	return true;
}
