#include "script_data.h"
#include "hooks.h"

void ScriptData::fill_framebulk_data(const char* buf, size_t size) {
	framebulks.clear();
	framebulks.push_back(Framebulk()); // to unpress all keys before & after running
	for (size_t off = 0; size - off >= Framebulk::FB_SIZE_BYTES; off += Framebulk::FB_SIZE_BYTES)
		framebulks.push_back(Framebulk(buf + off));
	framebulks.push_back(Framebulk());
}


void ScriptManager::set_new_script(ScriptData* data) {
	script_mutex.lock();
	delete script_data;
	script_data = data;
	has_active_script = true;
	map_loaded = false;
	fb_tick = 0;
	fb_idx = 0;
	script_mutex.unlock();
}


void ScriptManager::tick_signal() {
	script_mutex.lock();
	
	if (!has_active_script || !script_data) {
		script_mutex.unlock();
		return;
	}
	
	if (!map_loaded) {
		map_loaded = true;
		load_map();
		script_mutex.unlock();
		return;
	}

	for (;;) {
		Framebulk& fb = script_data->framebulks[fb_idx];

		// hard coded keys for each flag
		const EKEY_CODE flag_keys[] = {IRR_KEY_UP, IRR_KEY_DOWN, IRR_KEY_SPACE, IRR_KEY_N, IRR_KEY_V};

		for (int i = 0; i < Framebulk::NUM_FLAGS; i++)
			send_game_input(flag_keys[i], fb.flags & (1 << i));

		// TODO: send joystick inputs instead of just hard left/right
		bool r_key = fb.turn_angle > 0;
		bool l_key = fb.turn_angle < 0;
		send_game_input(IRR_KEY_RIGHT, r_key);
		send_game_input(IRR_KEY_LEFT, l_key);

		// increment tick
		if (++fb_tick >= fb.num_ticks) {
			fb_tick = 0;
			if (++fb_idx >= script_data->framebulks.size()) {
				has_active_script = false; // we're done
				break;
			}
		}
		// break if we just processed a framebulk with at least one tick
		if (fb.num_ticks > 0)
			break;
	}

	script_mutex.unlock();
}


void ScriptManager::send_game_input(EKEY_CODE key, bool key_pressed) {
	SEvent e = {};
	e.EventType = EET_KEY_INPUT_EVENT;
	e.KeyInput.Char = U'\0'; // might need to set this for actual keys
	e.KeyInput.Key = key;
	e.KeyInput.SystemKeyCode = 0; // hope this doesn't matter
	e.KeyInput.PressedDown = key_pressed;
	e.KeyInput.Shift = false;
	e.KeyInput.Control = false;
	hooks::ORIG_InputManager__input(*hooks::input_manager, e);
}


void ScriptManager::load_map() {
	/*
	* This is equivalent to the following game code:
	*
	* input_manager->getDeviceManager()->setAssignMode(ASSIGN);
	* auto device = input_manager->getDeviceManager()->getLatestUsedDevice();
	* auto profile = PlayerManager::getCurrentPlayer();
	* StateManager::get()->createActivePlayer(profile, device);
	* RaceManager::get()->setPlayerKart(0, "tux");
	* RaceManager::get()->setNumKarts(0);
	* RaceManager::get()->setNumLaps(1);
	* RaceManager::get()->startSingleRace("abyss", 1, false);
	*/
	using namespace hooks;

	ORIG_DeviceManager__setAssignMode((**input_manager).m_device_manager, ASSIGN);
	auto device = ORIG_DeviceManager__getLatestUsedDevice((**input_manager).m_device_manager);
	auto profile = (**m_player_manager).m_current_player;
	ORIG_StateManager__createActivePlayer(*state_manager_singleton, profile, device);
	ORIG_RaceManager__setPlayerKart(*g_race_manager, 0, script_data->player_name.c_str());
	// (**g_race_manager).setNumKarts(0); // <-- fix this
	// TODO set num laps, difficulty
	ORIG_RaceManager__startSingleRace(*g_race_manager, script_data->map_name.c_str(), 1, false);
}
