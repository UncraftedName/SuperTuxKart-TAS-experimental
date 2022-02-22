#pragma once
#include <string>
#include <vector>
#include <mutex>

class Framebulk {
public:
	union {
		struct {
			float angle;
			short numTicks;
			bool accel : 1;
			bool brake : 1;
			bool fire  : 1;
			bool nitro : 1;
			bool skid  : 1;
		};
		__int64 __val;
	};

	Framebulk(char* buf) {
		this->__val = *(__int64*)buf;
	}

	static const int FB_SIZE_BYTES = 8;

private:
	// probably won't be used
	static const __int64 mask_accel = 0x0010000000000000;
	static const __int64 mask_brake = 0x0008000000000000;
	static const __int64 mask_fire  = 0x0004000000000000;
	static const __int64 mask_nitro = 0x0002000000000000;
	static const __int64 mask_skid  = 0x0001000000000000;
	static const __int64 mask_ticks = 0x0000FFFF00000000;
	static const __int64 mask_angle = 0x00000000FFFFFFFF;
};


class ScriptData {
public:
	std::string map_name;
	std::string player_name;
	int ai_count = 0;
	int laps = 0;
	std::vector<Framebulk> framebulks;

	void fill_framebulk_data(char* buf, int bufLen);
};


class ScriptStatus {

private:

	// header/framebulks
	ScriptData* script_data = nullptr;
	// use when accessing anything in this class, since that might happen from game threads
	std::mutex script_mutex;
	// we have a script that is currently being executed
	volatile bool has_active_script = false;
	// we've loaded the map in the TAS script
	volatile bool map_loaded = false;
	// the tick that we're on in the script
	volatile int tick = 0;

public:

	~ScriptStatus() {
		/*
		* These locks are probably redundant, this destructor can in theory be called at any
		* time from Exit(), but the game can acquire these locks at any time which is
		* undefined behavior or something anyways.
		*/
		script_mutex.lock();
		delete script_data;
		script_mutex.unlock();
	}

	void set_new_script(ScriptData* data) {
		script_mutex.lock();
		delete script_data;
		script_data = data;
		has_active_script = true;
		map_loaded = false;
		tick = 0;
		script_mutex.unlock();
	}

	// signal that we're on a new game tick
	void tick_signal();
};
