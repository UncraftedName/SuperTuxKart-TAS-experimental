#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "game_structures.h"

class Framebulk {
public:
	union {
		// flags that correspond to buttons should come first
		struct {
			bool accel     : 1;
			bool brake     : 1;
			bool fire      : 1;
			bool nitro     : 1;
			bool skid      : 1;
			bool set_speed : 1;
		};
		uint16_t flags;
	};
	uint16_t num_ticks;
	union {
		float turn_angle;
		float new_play_speed; // only applies if the set_speed flag is true
	};

	// size of a framebulk when set over network
	static const int FB_SIZE_BYTES = 8;

	static const int NUM_BUTTON_FLAGS = 5; // flags that correspond to single buttons

	Framebulk() = default;

	Framebulk(const char* buf) {
		flags = *(uint16_t*)buf;
		num_ticks = *(uint16_t*)(buf + 2);
		turn_angle = *(float*)(buf + 4);
	}
};


class ScriptData {
public:
	std::string map_name;
	std::string player_name;
	int ai_count = 0;
	int laps = 0;
	Difficulty difficulty = DIFFICULTY_EASY;
	std::vector<Framebulk> framebulks;
	// can we restart a map without reloading?
	bool quick_reset;

	void fill_framebulk_data(const char* buf, size_t size);
};


class ScriptManager {

private:

	// header/framebulks
	ScriptData* script_data = nullptr;
	// we have a script that is currently being executed
	bool has_active_script = false;
	// we've loaded the map in the TAS script
	bool map_loaded = false;
	// the tick that we're on in current framebulk
	int fb_tick = 0;
	// the framebulk index that we're on
	int fb_idx = 0;
	// current playspeed, negative values means as fast as possible
	float play_speed = 1;

	// loads the map in script data
	void load_map();
	// convert framebulk to key/controller inputs
	void send_framebulk_inputs(const Framebulk&);
	// only handles key codes
	void send_keyboard_input(EKEY_CODE key, bool key_pressed);

public:

	~ScriptManager() {
		delete script_data;
	}

	bool running_script() {return has_active_script;}

	float get_play_speed() {return play_speed;}

	// we've just parsed a new script via IPC, stops the existing script
	void set_new_script(ScriptData* data);

	void stop_script();

	// signal that we're on a new game tick
	void tick_signal();
};
