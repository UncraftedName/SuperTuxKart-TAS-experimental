#include "framebulk.h"

Framebulk::Framebulk(__int64 i) {
	accel = (i & mask_accel) >> 52;
	brake = (i & mask_brake) >> 51;
	fire = (i & mask_fire) >> 50;
	nitro = (i & mask_nitro) >> 49;
	skid = (i & mask_skid) >> 48;
	ticks = (i & mask_ticks) >> 32;
	angle = (i & mask_angle);
}