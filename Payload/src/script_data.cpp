#include "script_data.h"
#include "hooks.h"

void ScriptData::fillFramebulkData(const char* buf, size_t size) {
	framebulks.clear();
	framebulks.push_back(Framebulk()); // to unpress all keys before & after running
	for (size_t off = 0; size - off >= Framebulk::FB_SIZE_BYTES; off += Framebulk::FB_SIZE_BYTES)
		framebulks.push_back(Framebulk(buf + off));
	framebulks.push_back(Framebulk());
}


void ScriptManager::setNewScript(ScriptData* data) {
	stopScript();
	script_data = data;
	has_active_script = true;
	map_loaded = false;
	fb_tick = 0;
	fb_idx = 0;
}


void ScriptManager::stopScript() {
	if (!has_active_script)
		return;
	delete script_data;
	script_data = nullptr;
	has_active_script = false;
	play_speed = 1;
	*hooks::g_is_no_graphics = false;
	sendFramebulkInputs(Framebulk()); // clear keys
}


void ScriptManager::tickSignal() {
	
	if (!has_active_script || !script_data)
		return;
	
	if (!map_loaded) {
		loadMap();
		map_loaded = true;
		return;
	}

	for (;;) {
		Framebulk& fb = script_data->framebulks[fb_idx];
		sendFramebulkInputs(fb);

		if (fb.set_speed) {
			play_speed = fb.new_play_speed;
			*hooks::g_is_no_graphics = play_speed < 0;
		}

		// increment tick
		if (++fb_tick >= fb.num_ticks) {
			fb_tick = 0;
			if (++fb_idx >= script_data->framebulks.size()) {
				stopScript(); // we're done
				break;
			}
		}
		// break if we just processed a framebulk with at least one tick or if we set the speed to 0
		if (fb.num_ticks > 0 || (fb.set_speed && fb.new_play_speed == 0))
			break;
	}
}


void ScriptManager::sendFramebulkInputs(const Framebulk& fb) {
	// if we're running a script, don't send keypresses/releases from a 0-tick framebulk unless it's the first/last one
	if (script_data) {
		if (fb.num_ticks <= 0 && &fb != &script_data->framebulks.back() && &fb != &script_data->framebulks.front())
			return;
	}

	// TODO: send joystick inputs instead of just hard left/right
	// send direction inputs BEFORE skid flag!
	sendKeyboardInput(IRR_KEY_RIGHT, fb.turn_angle > 0);
	sendKeyboardInput(IRR_KEY_LEFT, fb.turn_angle < 0);

	// hard coded keys for each flag
	const EKEY_CODE flag_keys[] = {IRR_KEY_UP, IRR_KEY_DOWN, IRR_KEY_SPACE, IRR_KEY_N, IRR_KEY_V};

	for (int i = 0; i < Framebulk::NUM_BUTTON_FLAGS; i++)
		sendKeyboardInput(flag_keys[i], fb.flags & (1 << i));
}


void ScriptManager::sendKeyboardInput(EKEY_CODE key, bool key_pressed) {
	// Fills an SEvent struct and calls InputManager::input in game code
	SEvent e = {};
	e.EventType = EET_KEY_INPUT_EVENT;
	e.KeyInput.Char = U'\0';
	e.KeyInput.Key = key;
	e.KeyInput.SystemKeyCode = 0;
	e.KeyInput.PressedDown = key_pressed;
	e.KeyInput.Shift = false;
	e.KeyInput.Control = false;
	hooks::ORIG_InputManager__input(*hooks::input_manager, e);
}


void ScriptManager::loadMap() {
	/*
	* This is roughly equivalent to the following game code:
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

	// can't quick reset if the world isn't loaded yet
	if (*m_world && script_data->quick_reset) {
		CALL_VIRTUAL_FUNC(_World__reset, *m_world, 2, *m_world, true);
		/*
		* When loading a map normally, there's 1 tick that gets triggered during the world load.
		* To make scripts consistent when using quick reload, 1 tick is subtracted from the first
		* framebulk with at least 1 tick. So technically, if there's any tricks that require inputs
		* during the map load, they won't work with quick reload.
		*/
		for (auto it = script_data->framebulks.begin(); it != script_data->framebulks.end(); it++) {
			if (it->num_ticks > 0) {
				it->num_ticks -= 1;
				break;
			}
		}
	} else {
		ORIG_RaceManager__exitRace(*g_race_manager, true);
		ORIG_DeviceManager__setAssignMode((**input_manager).m_device_manager, ASSIGN);
		auto device = ORIG_DeviceManager__getLatestUsedDevice((**input_manager).m_device_manager);
		auto profile = (**m_player_manager).m_current_player;
		ORIG_StateManager__resetActivePlayers(*state_manager_singleton);
		(**input_manager).m_device_manager->m_single_player = nullptr;
		ORIG_StateManager__createActivePlayer(*state_manager_singleton, profile, device);
		ORIG_RaceManager__setPlayerKart(*g_race_manager, 0, script_data->player_name.c_str());
		(**g_race_manager).setupBasicRace(script_data->difficulty, script_data->laps);
		ORIG_RaceManager__startSingleRace(*g_race_manager, script_data->map_name.c_str(), 1, false);
	}
}
