#pragma once

#include <WinSock2.h>
#include <string>

class IPC {
public:
	IPC();
	~IPC();
	void start();
	const std::string& get_map_name();
	const std::string& get_player_name();
	int get_ai_count();
	int get_num_laps();
	const auto *get_framebulks();

private:
	const char* PORT = "27015";

	SOCKET listen_socket;
	SOCKET client_socket;

	std::string map_name;
	std::string player_name;
	int ai_count = 0;
	int laps = 0;

	const int FB_SIZE = 8;
	char* buf = nullptr;
	int pos = -1;
	int buflen = -1;
};