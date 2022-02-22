#include "script_data.h"

void ScriptData::fill_framebulk_data(char* buf, int bufLen) {
	for (int off = 0; bufLen - off >= Framebulk::FB_SIZE_BYTES; off += Framebulk::FB_SIZE_BYTES)
		framebulks.push_back(Framebulk(buf + off));
}
