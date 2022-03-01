#pragma once
#include <stdint.h>

/*
* Suppose we hook a function foo(SEvent& event). Internally, that just gets
* converted to foo(SEvent* event). However, since we don't have access to the game
* source code in this project, we have two options:
*
*   Option 1: foo(void* ptr)
*     Then if we wanted to access say the y position of the mouse cursor, we would
*     do something like (int)((uintptr_t)ptr + 20).
*
*   Option 2: partially/fully copy the game's implementation of the SEvent struct.
*     Then if we wanted to access the y position of the mouse cursor, we could just
*     do event.MouseInput.Y like normal citizens of society.
*
* Option 1 is quicker if you don't have full access to the structure in question.
* Option 2 is less abhorrent.
*/


namespace std {

	/*
	* So... strings and vectors (and probably other std concoctions) don't work for
	* us. For some reason which I can't really be bothered to figure out, the field
	* offsets that we get when using good ol' std::string, etc. don't match up with
	* what the game has, and are therefore not compatible. Even worse, sometimes
	* the offsets are different in debug vs. release builds!! There are all manner
	* of profanities I could use to express my dissatisfaction, but we'll just say
	* that it makes me want to stand awkwardly close to a substantially tall cliff
	* edge.
	* 
	* Anyways, these are meant to serve as replacements, and should be used in
	* place of the normal std constructs whenever you call game functions or
	* reconstruct game structures.
	*/


	// std::string replacement
	struct str_wrap {

		char* alias;
		size_t size;
		union {
			size_t unknown; // seems to copy from size when buf is not used (might be capacity)
			char buf[16];
		};

		str_wrap(const str_wrap& s) : str_wrap(s.alias, s.size) {}

		str_wrap(const char* str) : str_wrap(str, strlen(str)) {}

		// std::string will store the string locally if it's less than 16 chars long
		str_wrap(const char* str, size_t size) {
			this->size = size;
			size_t capacity = size | 15; // rounds up to nearest 16 (-1) since alignment exists
			if (size < 16) {
				alias = buf;
			} else {
				alias = new char[capacity + 1]; // +1 for null terminator
				unknown = size;
			}
			memcpy(alias, str, size);
		}

		~str_wrap() {
			if (size >= 16)
				delete alias;
		}
	};


	// std::vector replacement, does not work for std::vector<bool>
	template <class T>
	struct vec_wrap {

		T *first, *last;
		size_t capacity;

		void clear() {
			if (!empty())
				delete[] first; // TODO call destructor
			first = last = nullptr;
			capacity = 0;
		}

		bool empty() const {
			return capacity == 0;
		}

		void push_back(T& val) {
			if (empty() || (last - first) / sizeof(T) + 1 > capacity) {
				// grow
				size_t new_capacity = empty() ? 4 : capacity * 2;
				T* new_arr = new T[new_capacity];
				memcpy(new_arr, first, capacity);
				delete[] first;
				last = new_arr + (last - first);
				first = new_arr;
				capacity = new_capacity;
			}
			*++last = val;
		}
	};
}


enum Difficulty: uint32_t {
	DIFFICULTY_EASY = 0,
	DIFFICULTY_FIRST = DIFFICULTY_EASY,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,
	DIFFICULTY_BEST,
	DIFFICULTY_LAST = DIFFICULTY_BEST,
	DIFFICULTY_COUNT,
	DIFFICULTY_NONE
};

enum MajorRaceModeType: uint32_t {
	MAJOR_MODE_GRAND_PRIX = 0,
	MAJOR_MODE_SINGLE
};

enum MinorRaceModeType: int32_t {
	MINOR_MODE_NONE = -1,

	MINOR_MODE_NORMAL_RACE = 1100,
	MINOR_MODE_TIME_TRIAL,
	MINOR_MODE_FOLLOW_LEADER = 1002,

	MINOR_MODE_3_STRIKES = 2000,
	MINOR_MODE_FREE_FOR_ALL,
	MINOR_MODE_CAPTURE_THE_FLAG,
	MINOR_MODE_SOCCER,

	MINOR_MODE_EASTER_EGG = 3000,

	MINOR_MODE_OVERWORLD = 4000,
	MINOR_MODE_TUTORIAL,
	MINOR_MODE_CUTSCENE
};

enum AISuperPower: uint8_t {
	SUPERPOWER_NONE = 0,
	SUPERPOWER_NOLOK_BOSS = 1
};

enum PlayerAssignMode {
	NO_ASSIGN,  //!< react to all devices
	DETECT_NEW, //!< notify the manager when an inactive device is being asked to activate with fire
	ASSIGN      //!< only react to assigned devices
};


struct DeviceManager {
	char __pad[0x78];
	void* m_single_player;
};

struct StateManager;
struct InputDevice;
struct PlayerProfile;

struct InputManager {
	char __pad[48];
	DeviceManager* m_device_manager; // rel w/ debug puts this at 16
};

struct PlayerManager {
	char __pad[24];
	PlayerProfile* m_current_player;
};

// pretty much anything that's a vector in here is (probably) a list of what will happen for the next map(s), we only care about 1
struct RaceManager {
	char __pad0[32];
	Difficulty m_difficulty;
	MajorRaceModeType m_major_mode;
	MinorRaceModeType m_minor_mode;
	char __pad1[28];
	std::vec_wrap<std::str_wrap> m_tracks;
	char __pad2[8];
	std::vec_wrap<int> m_num_laps;
	char __pad3[64]; // this has m_reverse_track list in it, might need custom vector<bool> implementation
	std::str_wrap m_ai_kart_override;
	AISuperPower m_ai_superpower;
	char __pad4[7];
	std::vec_wrap<std::str_wrap> m_ai_kart_list; // we might be able to use this to set the specific AI karts
	char __pad5[216];
	int m_num_karts;

	void setupBasicRace(Difficulty difficulty, int num_laps) {
		m_difficulty = difficulty;
		// TODO
		// m_num_laps.clear();
		// m_num_laps.push_back(num_laps);
		m_num_karts = 0;
		m_ai_kart_override = "";
		m_ai_superpower = SUPERPOWER_NONE;
		m_major_mode = MAJOR_MODE_SINGLE;
		m_minor_mode = MINOR_MODE_TIME_TRIAL;
	}
};

struct MainLoop;

struct STKConfig {
	char __pad0[0x354];
	int m_physics_fps;
};


typedef unsigned __int32 u32;
typedef __int32 s32;
typedef __int8 s8;
typedef char c8;
typedef __int16 s16;
typedef float f32;
typedef double f64;


enum EGUI_EVENT_TYPE;
enum EMOUSE_INPUT_EVENT;
enum EKEY_CODE;
enum ETOUCH_INPUT_EVENT;
enum ELOG_LEVEL;
enum ESYSTEM_EVENT_TYPE;
enum EAPPLICATION_EVENT_TYPE;

enum EventPropagation {
	EVENT_BLOCK,
	EVENT_BLOCK_BUT_HANDLED,
	EVENT_LET
};


enum EEVENT_TYPE {
	//! An event of the graphical user interface.
	/** GUI events are created by the GUI environment or the GUI elements in response
	to mouse or keyboard events. When a GUI element receives an event it will either
	process it and return true, or pass the event to its parent. If an event is not absorbed
	before it reaches the root element then it will then be passed to the user receiver. */
	EET_GUI_EVENT = 0,

	//! A mouse input event.
	/** Mouse events are created by the device and passed to IrrlichtDevice::postEventFromUser
	in response to mouse input received from the operating system.
	Mouse events are first passed to the user receiver, then to the GUI environment and its elements,
	then finally the input receiving scene manager where it is passed to the active camera.
	*/
	EET_MOUSE_INPUT_EVENT,

	//! A key input event.
	/** Like mouse events, keyboard events are created by the device and passed to
	IrrlichtDevice::postEventFromUser. They take the same path as mouse events. */
	EET_KEY_INPUT_EVENT,

	//! A touch input event.
	EET_TOUCH_INPUT_EVENT,

	//! A accelerometer event.
	EET_ACCELEROMETER_EVENT,

	//! A gyroscope event.
	EET_GYROSCOPE_EVENT,

	//! A device motion event.
	EET_DEVICE_MOTION_EVENT,

	//! A joystick (joypad, gamepad) input event.
	/** Joystick events are created by polling all connected joysticks once per
	device run() and then passing the events to IrrlichtDevice::postEventFromUser.
	They take the same path as mouse events.
	Windows, SDL: Implemented.
	Linux: Implemented, with POV hat issues.
	MacOS / Other: Not yet implemented.
	*/
	EET_JOYSTICK_INPUT_EVENT,

	//! A log event
	/** Log events are only passed to the user receiver if there is one. If they are absorbed by the
	user receiver then no text will be sent to the console. */
	EET_LOG_TEXT_EVENT,

	//! A user event with user data.
	/** This is not used by Irrlicht and can be used to send user
	specific data though the system. The Irrlicht 'window handle'
	can be obtained from IrrlichtDevice::getExposedVideoData()
	The usage and behavior depends on the operating system:
	Windows: send a WM_USER message to the Irrlicht Window; the
		wParam and lParam will be used to populate the
		UserData1 and UserData2 members of the SUserEvent.
	Linux: send a ClientMessage via XSendEvent to the Irrlicht
		Window; the data.l[0] and data.l[1] members will be
		casted to s32 and used as UserData1 and UserData2.
	MacOS: Not yet implemented
	*/
	EET_USER_EVENT,

	//! Pass on raw events from the OS
	EET_SYSTEM_EVENT,

	//! Application state events like a resume, pause etc.
	EET_APPLICATION_EVENT,

	//! SDL text event
	EET_SDL_TEXT_EVENT,

	//! This enum is never used, it only forces the compiler to
	//! compile these enumeration values to 32 bit.
	EGUIET_FORCE_32_BIT = 0x7fffffff
};


struct SEvent {
	//! Any kind of GUI event.
	struct SGUIEvent {
		//! IGUIElement who called the event
		void* Caller;
		//! If the event has something to do with another element, it will be held here.
		void* Element;
		//! Type of GUI Event
		EGUI_EVENT_TYPE EventType;

	};

	//! Any kind of mouse event.
	struct SMouseInput {
		//! X position of mouse cursor
		s32 X;
		//! Y position of mouse cursor
		s32 Y;
		//! mouse wheel delta, often 1.0 or -1.0, but can have other values < 0.f or > 0.f;
		/** Only valid if event was EMIE_MOUSE_WHEEL */
		f32 Wheel;
		//! True if shift was also pressed
		bool Shift : 1;
		//! True if ctrl was also pressed
		bool Control : 1;
		//! A bitmap of button states. You can use isButtonPressed() to determine
		//! if a button is pressed or not.
		//! Currently only valid if the event was EMIE_MOUSE_MOVED
		u32 ButtonStates;
		//! Type of mouse event
		EMOUSE_INPUT_EVENT Event;
	};

	//! Any kind of keyboard event.
	struct SKeyInput {
		//! Character corresponding to the key (0, if not a character, value undefined in key releases)
		char32_t Char;
		//! Key which has been pressed or released
		EKEY_CODE Key;
		//! System dependent code. Only set for systems which are described below, otherwise undefined.
		//! Android: int32_t with physical key as returned by AKeyEvent_getKeyCode
		u32 SystemKeyCode;
		//! If not true, then the key was left up
		bool PressedDown : 1;
		//! True if shift was also pressed
		bool Shift : 1;
		//! True if ctrl was also pressed
		bool Control : 1;
	};

	//! Any kind of touch event.
	struct STouchInput {
		// Touch ID.
		size_t ID;
		// X position of simple touch.
		s32 X;
		// Y position of simple touch.
		s32 Y;
		//! Type of touch event.
		ETOUCH_INPUT_EVENT Event;
	};


	//! Any kind of accelerometer event.
	struct SAccelerometerEvent {
		f64 X, Y, Z;
	};

	//! Any kind of gyroscope event.
	struct SGyroscopeEvent {
		f64 X, Y, Z;
	};

	//! Any kind of device motion event.
	struct SDeviceMotionEvent {
		f64 roll, pitch, yaw;
	};

	//! A joystick event.
	/** Unlike other events, joystick events represent the result of polling
	 * each connected joystick once per run() of the device. Joystick events will
	 * not be generated by default.  If joystick support is available for the
	 * active device, _IRR_COMPILE_WITH_JOYSTICK_EVENTS_ is defined, and
	 * @ref irr::IrrlichtDevice::activateJoysticks() has been called, an event of
	 * this type will be generated once per joystick per @ref IrrlichtDevice::run()
	 * regardless of whether the state of the joystick has actually changed. */
	struct SJoystickEvent {
		enum {
			NUMBER_OF_BUTTONS = 32,

			AXIS_X = 0, // e.g. analog stick 1 left to right
			AXIS_Y,		// e.g. analog stick 1 top to bottom
			AXIS_Z,		// e.g. throttle, or analog 2 stick 2 left to right
			AXIS_R,		// e.g. rudder, or analog 2 stick 2 top to bottom
			AXIS_U,
			AXIS_V,
			NUMBER_OF_AXES = 32
		};

		/** For AXIS_X, AXIS_Y, AXIS_Z, AXIS_R, AXIS_U and AXIS_V
		 * Values are in the range -32768 to 32767, with 0 representing
		 * the center position.  You will receive the raw value from the
		 * joystick, and so will usually want to implement a dead zone around
		 * the center of the range. Axes not supported by this joystick will
		 * always have a value of 0. On Linux, POV hats are represented as axes,
		 * usually the last two active axis.
		 */
		s16 Axis[NUMBER_OF_AXES];

		//! The ID of the joystick which generated this event.
		/** This is an internal Irrlicht index; it does not map directly
		 * to any particular hardware joystick. */
		u32 AxisChanged;

		/** A bitmap of button states.  You can use IsButtonPressed() to
		 ( check the state of each button from 0 to (NUMBER_OF_BUTTONS - 1) */
		u32 ButtonStates;

		//! The ID of the joystick which generated this event.
		/** This is an internal Irrlicht index; it does not map directly
		 * to any particular hardware joystick. */
		u32 Joystick;

		//! A helper function to check if a button is pressed.
		bool IsAxisChanged(u32 axis) const
		{
			if (axis >= (u32)NUMBER_OF_AXES)
				return false;

			return (AxisChanged & (1 << axis)) ? true : false;
		}

		//! A helper function to check if a button is pressed.
		bool IsButtonPressed(u32 button) const
		{
			if (button >= (u32)NUMBER_OF_BUTTONS)
				return false;

			return (ButtonStates & (1 << button)) ? true : false;
		}
	};

	//! Any kind of log event.
	struct SLogEvent {
		//! Pointer to text which has been logged
		const c8* Text;
		//! Log level in which the text has been logged
		ELOG_LEVEL Level;
	};

	//! Any kind of user event.
	struct SUserEvent {
		//! Some user specified data as int
		s32 UserData1;
		//! Another user specified data as int
		s32 UserData2;
	};

	// Raw events from the OS
	struct SSystemEvent {
		//! Android command handler native activity messages.
		struct SAndroidCmd {
			//!  APP_CMD_ enums defined in android_native_app_glue.h from the Android NDK
			s32 Cmd;
		};

		ESYSTEM_EVENT_TYPE EventType;
		union {
			struct SAndroidCmd AndroidCmd;
		};
	};

	// Application state event
	struct SApplicationEvent {
		EAPPLICATION_EVENT_TYPE EventType;
	};

	// Application state event
	struct SSDLTextEvent {
		u32 Type; // SDL_TEXTEDITING or SDL_TEXTINPUT
		c8 Text[32];
		s32 Start; // SDL_TEXTEDITING usage
		s32 Length; // SDL_TEXTEDITING usage
	};

	EEVENT_TYPE EventType;
	union {
		struct SGUIEvent GUIEvent;
		struct SMouseInput MouseInput;
		struct SKeyInput KeyInput;
		struct STouchInput TouchInput;
		struct SAccelerometerEvent AccelerometerEvent;
		struct SGyroscopeEvent GyroscopeEvent;
		struct SDeviceMotionEvent DeviceMotionEvent;
		struct SJoystickEvent JoystickEvent;
		struct SLogEvent LogEvent;
		struct SUserEvent UserEvent;
		struct SSystemEvent SystemEvent;
		struct SApplicationEvent ApplicationEvent;
		struct SSDLTextEvent SDLTextEvent;
	};

};


enum EKEY_CODE {
	IRR_KEY_UNKNOWN = 0x0,
	IRR_KEY_LBUTTON = 0x01,  // Left mouse button
	IRR_KEY_RBUTTON = 0x02,  // Right mouse button
	IRR_KEY_CANCEL = 0x03,  // Control-break processing
	IRR_KEY_MBUTTON = 0x04,  // Middle mouse button (three-button mouse)
	IRR_KEY_XBUTTON1 = 0x05,  // Windows 2000/XP: X1 mouse button
	IRR_KEY_XBUTTON2 = 0x06,  // Windows 2000/XP: X2 mouse button
	IRR_KEY_BACK = 0x08,  // BACKSPACE key
	IRR_KEY_TAB = 0x09,  // TAB key
	IRR_KEY_CLEAR = 0x0C,  // CLEAR key
	IRR_KEY_RETURN = 0x0D,  // ENTER key
	IRR_KEY_SHIFT = 0x10,  // SHIFT key
	IRR_KEY_CONTROL = 0x11,  // CTRL key
	IRR_KEY_MENU = 0x12,  // ALT key
	IRR_KEY_PAUSE = 0x13,  // PAUSE key
	IRR_KEY_CAPITAL = 0x14,  // CAPS LOCK key
	IRR_KEY_KANA = 0x15,  // IME Kana mode
	IRR_KEY_HANGUEL = 0x15,  // IME Hanguel mode (maintained for compatibility use KEY_HANGUL)
	IRR_KEY_HANGUL = 0x15,  // IME Hangul mode
	IRR_KEY_JUNJA = 0x17,  // IME Junja mode
	IRR_KEY_FINAL = 0x18,  // IME final mode
	IRR_KEY_HANJA = 0x19,  // IME Hanja mode
	IRR_KEY_KANJI = 0x19,  // IME Kanji mode
	IRR_KEY_ESCAPE = 0x1B,  // ESC key
	IRR_KEY_CONVERT = 0x1C,  // IME convert
	IRR_KEY_NONCONVERT = 0x1D,  // IME nonconvert
	IRR_KEY_ACCEPT = 0x1E,  // IME accept
	IRR_KEY_MODECHANGE = 0x1F,  // IME mode change request
	IRR_KEY_SPACE = 0x20,  // SPACEBAR
	IRR_KEY_PRIOR = 0x21,  // PAGE UP key
	IRR_KEY_NEXT = 0x22,  // PAGE DOWN key
	IRR_KEY_END = 0x23,  // END key
	IRR_KEY_HOME = 0x24,  // HOME key
	IRR_KEY_LEFT = 0x25,  // LEFT ARROW key
	IRR_KEY_UP = 0x26,  // UP ARROW key
	IRR_KEY_RIGHT = 0x27,  // RIGHT ARROW key
	IRR_KEY_DOWN = 0x28,  // DOWN ARROW key
	IRR_KEY_SELECT = 0x29,  // SELECT key
	IRR_KEY_PRINT = 0x2A,  // PRINT key
	IRR_KEY_EXECUT = 0x2B,  // EXECUTE key
	IRR_KEY_SNAPSHOT = 0x2C,  // PRINT SCREEN key
	IRR_KEY_INSERT = 0x2D,  // INS key
	IRR_KEY_DELETE = 0x2E,  // DEL key
	IRR_KEY_HELP = 0x2F,  // HELP key
	IRR_KEY_0 = 0x30,  // 0 key
	IRR_KEY_1 = 0x31,  // 1 key
	IRR_KEY_2 = 0x32,  // 2 key
	IRR_KEY_3 = 0x33,  // 3 key
	IRR_KEY_4 = 0x34,  // 4 key
	IRR_KEY_5 = 0x35,  // 5 key
	IRR_KEY_6 = 0x36,  // 6 key
	IRR_KEY_7 = 0x37,  // 7 key
	IRR_KEY_8 = 0x38,  // 8 key
	IRR_KEY_9 = 0x39,  // 9 key
	IRR_KEY_A = 0x41,  // A key
	IRR_KEY_B = 0x42,  // B key
	IRR_KEY_C = 0x43,  // C key
	IRR_KEY_D = 0x44,  // D key
	IRR_KEY_E = 0x45,  // E key
	IRR_KEY_F = 0x46,  // F key
	IRR_KEY_G = 0x47,  // G key
	IRR_KEY_H = 0x48,  // H key
	IRR_KEY_I = 0x49,  // I key
	IRR_KEY_J = 0x4A,  // J key
	IRR_KEY_K = 0x4B,  // K key
	IRR_KEY_L = 0x4C,  // L key
	IRR_KEY_M = 0x4D,  // M key
	IRR_KEY_N = 0x4E,  // N key
	IRR_KEY_O = 0x4F,  // O key
	IRR_KEY_P = 0x50,  // P key
	IRR_KEY_Q = 0x51,  // Q key
	IRR_KEY_R = 0x52,  // R key
	IRR_KEY_S = 0x53,  // S key
	IRR_KEY_T = 0x54,  // T key
	IRR_KEY_U = 0x55,  // U key
	IRR_KEY_V = 0x56,  // V key
	IRR_KEY_W = 0x57,  // W key
	IRR_KEY_X = 0x58,  // X key
	IRR_KEY_Y = 0x59,  // Y key
	IRR_KEY_Z = 0x5A,  // Z key
	IRR_KEY_LWIN = 0x5B,  // Left Windows key (Microsoft Natural keyboard)
	IRR_KEY_RWIN = 0x5C,  // Right Windows key (Natural keyboard)
	IRR_KEY_APPS = 0x5D,  // Applications key (Natural keyboard)
	IRR_KEY_SLEEP = 0x5F,  // Computer Sleep key
	IRR_KEY_NUMPAD0 = 0x60,  // Numeric keypad 0 key
	IRR_KEY_NUMPAD1 = 0x61,  // Numeric keypad 1 key
	IRR_KEY_NUMPAD2 = 0x62,  // Numeric keypad 2 key
	IRR_KEY_NUMPAD3 = 0x63,  // Numeric keypad 3 key
	IRR_KEY_NUMPAD4 = 0x64,  // Numeric keypad 4 key
	IRR_KEY_NUMPAD5 = 0x65,  // Numeric keypad 5 key
	IRR_KEY_NUMPAD6 = 0x66,  // Numeric keypad 6 key
	IRR_KEY_NUMPAD7 = 0x67,  // Numeric keypad 7 key
	IRR_KEY_NUMPAD8 = 0x68,  // Numeric keypad 8 key
	IRR_KEY_NUMPAD9 = 0x69,  // Numeric keypad 9 key
	IRR_KEY_MULTIPLY = 0x6A,  // Multiply key
	IRR_KEY_ADD = 0x6B,  // Add key
	IRR_KEY_SEPARATOR = 0x6C,  // Separator key
	IRR_KEY_SUBTRACT = 0x6D,  // Subtract key
	IRR_KEY_DECIMAL = 0x6E,  // Decimal key
	IRR_KEY_DIVIDE = 0x6F,  // Divide key
	IRR_KEY_F1 = 0x70,  // F1 key
	IRR_KEY_F2 = 0x71,  // F2 key
	IRR_KEY_F3 = 0x72,  // F3 key
	IRR_KEY_F4 = 0x73,  // F4 key
	IRR_KEY_F5 = 0x74,  // F5 key
	IRR_KEY_F6 = 0x75,  // F6 key
	IRR_KEY_F7 = 0x76,  // F7 key
	IRR_KEY_F8 = 0x77,  // F8 key
	IRR_KEY_F9 = 0x78,  // F9 key
	IRR_KEY_F10 = 0x79,  // F10 key
	IRR_KEY_F11 = 0x7A,  // F11 key
	IRR_KEY_F12 = 0x7B,  // F12 key
	IRR_KEY_F13 = 0x7C,  // F13 key
	IRR_KEY_F14 = 0x7D,  // F14 key
	IRR_KEY_F15 = 0x7E,  // F15 key
	IRR_KEY_F16 = 0x7F,  // F16 key
	IRR_KEY_F17 = 0x80,  // F17 key
	IRR_KEY_F18 = 0x81,  // F18 key
	IRR_KEY_F19 = 0x82,  // F19 key
	IRR_KEY_F20 = 0x83,  // F20 key
	IRR_KEY_F21 = 0x84,  // F21 key
	IRR_KEY_F22 = 0x85,  // F22 key
	IRR_KEY_F23 = 0x86,  // F23 key
	IRR_KEY_F24 = 0x87,  // F24 key
	IRR_KEY_NUMLOCK = 0x90,  // NUM LOCK key
	IRR_KEY_SCROLL = 0x91,  // SCROLL LOCK key
	IRR_KEY_LSHIFT = 0xA0,  // Left SHIFT key
	IRR_KEY_RSHIFT = 0xA1,  // Right SHIFT key
	IRR_KEY_LCONTROL = 0xA2,  // Left CONTROL key
	IRR_KEY_RCONTROL = 0xA3,  // Right CONTROL key
	IRR_KEY_LMENU = 0xA4,  // Left MENU key
	IRR_KEY_RMENU = 0xA5,  // Right MENU key
	IRR_KEY_BROWSER_BACK = 0xA6,  // Browser Back key
	IRR_KEY_BROWSER_FORWARD = 0xA7,  // Browser Forward key
	IRR_KEY_BROWSER_REFRESH = 0xA8,  // Browser Refresh key
	IRR_KEY_BROWSER_STOP = 0xA9,  // Browser Stop key
	IRR_KEY_BROWSER_SEARCH = 0xAA,  // Browser Search key
	IRR_KEY_BROWSER_FAVORITES = 0xAB,  // Browser Favorites key
	IRR_KEY_BROWSER_HOME = 0xAC,  // Browser Start and Home key
	IRR_KEY_VOLUME_MUTE = 0xAD,  // Volume Mute key
	IRR_KEY_VOLUME_DOWN = 0xAE,  // Volume Down key
	IRR_KEY_VOLUME_UP = 0xAF,  // Volume Up key
	IRR_KEY_MEDIA_NEXT_TRACK = 0xB0,  // Next Track key
	IRR_KEY_MEDIA_PREV_TRACK = 0xB1,  // Previous Track key
	IRR_KEY_MEDIA_STOP = 0xB2,  // Stop Media key
	IRR_KEY_MEDIA_PLAY_PAUSE = 0xB3,  // Play/Pause Media key
	IRR_KEY_OEM_1 = 0xBA,  // for US    ";:"
	IRR_KEY_PLUS = 0xBB,  // Plus Key   "+"
	IRR_KEY_COMMA = 0xBC,  // Comma Key  ","
	IRR_KEY_MINUS = 0xBD,  // Minus Key  "-"
	IRR_KEY_PERIOD = 0xBE,  // Period Key "."
	IRR_KEY_OEM_2 = 0xBF,  // for US    "/?"
	IRR_KEY_OEM_3 = 0xC0,  // for US    "`~"
	IRR_KEY_OEM_4 = 0xDB,  // for US    "[{"
	IRR_KEY_OEM_5 = 0xDC,  // for US    "\|"
	IRR_KEY_OEM_6 = 0xDD,  // for US    "]}"
	IRR_KEY_OEM_7 = 0xDE,  // for US    "'""
	IRR_KEY_OEM_8 = 0xDF,  // None
	IRR_KEY_OEM_AX = 0xE1,  // for Japan "AX"
	IRR_KEY_OEM_102 = 0xE2,  // "<>" or "\|"
	IRR_KEY_ATTN = 0xF6,  // Attn key
	IRR_KEY_CRSEL = 0xF7,  // CrSel key
	IRR_KEY_EXSEL = 0xF8,  // ExSel key
	IRR_KEY_EREOF = 0xF9,  // Erase EOF key
	IRR_KEY_PLAY = 0xFA,  // Play key
	IRR_KEY_ZOOM = 0xFB,  // Zoom key
	IRR_KEY_PA1 = 0xFD,  // PA1 key
	IRR_KEY_OEM_CLEAR = 0xFE,   // Clear key

	IRR_KEY_BUTTON_LEFT = 0x100,
	IRR_KEY_BUTTON_RIGHT = 0x101,
	IRR_KEY_BUTTON_UP = 0x102,
	IRR_KEY_BUTTON_DOWN = 0x103,
	IRR_KEY_BUTTON_A = 0x104,
	IRR_KEY_BUTTON_B = 0x105,
	IRR_KEY_BUTTON_C = 0x106,
	IRR_KEY_BUTTON_X = 0x107,
	IRR_KEY_BUTTON_Y = 0x108,
	IRR_KEY_BUTTON_Z = 0x109,
	IRR_KEY_BUTTON_L1 = 0x10A,
	IRR_KEY_BUTTON_R1 = 0x10B,
	IRR_KEY_BUTTON_L2 = 0x10C,
	IRR_KEY_BUTTON_R2 = 0x10D,
	IRR_KEY_BUTTON_THUMBL = 0x10E,
	IRR_KEY_BUTTON_THUMBR = 0x10F,
	IRR_KEY_BUTTON_START = 0x110,
	IRR_KEY_BUTTON_SELECT = 0x111,
	IRR_KEY_BUTTON_MODE = 0x112,

	// For Azerty layout
	IRR_KEY_AMPERSAND = 0x113,
	IRR_KEY_EACUTE = 0x114,
	IRR_KEY_QUOTEDBL = 0x115,
	IRR_KEY_PARENLEFT = 0x116,
	IRR_KEY_EGRAVE = 0x117,
	IRR_KEY_CCEDILLA = 0x118,
	IRR_KEY_AGRAVE = 0x119,
	IRR_KEY_PARENRIGHT = 0x120,
	IRR_KEY_UGRAVE = 0x121,
	IRR_KEY_COLON = 0x122,
	IRR_KEY_DOLLAR = 0x123,
	IRR_KEY_EXCLAM = 0x124,
	IRR_KEY_TWOSUPERIOR = 0x125,
	IRR_KEY_MU = 0x126,
	IRR_KEY_SECTION = 0x127,

	IRR_KEY_CODES_COUNT = 0x128 // this is not a key, but the amount of keycodes there are.
};
