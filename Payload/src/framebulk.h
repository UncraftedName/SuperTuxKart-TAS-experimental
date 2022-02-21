#pragma once

class Framebulk {
public:
	bool accel : 1;
	bool brake : 1;
	bool fire : 1;
	bool nitro : 1;
	bool skid : 1;
	int ticks : 16;
	float angle;

	Framebulk(__int64);

private:
	const __int64 mask_accel = 0x0010000000000000;
	const __int64 mask_brake = 0x0008000000000000;
	const __int64 mask_fire  = 0x0004000000000000;
	const __int64 mask_nitro = 0x0002000000000000;
	const __int64 mask_skid  = 0x0001000000000000;
	const __int64 mask_ticks = 0x0000FFFF00000000;
	const __int64 mask_angle = 0x00000000FFFFFFFF;
};