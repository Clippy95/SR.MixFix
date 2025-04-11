module;
#include "framework.h"
export module camera;

import general;

class camera {
public:
	camera() {
#if SRTTR
		MixFix::onAttach() += []() {
		CIniReader ini;
		if (ini.ReadBoolean("QOL", "BetterDriveCam", true)) {
			auto pattern = hook::pattern("E8 ? ? ? ? 44 0F B6 C5 0F 28 CF");
			if (!pattern.empty())
			Memory::VP::Nop(pattern.get_first(), 5);
		}
			};
#endif
	}

}camera;