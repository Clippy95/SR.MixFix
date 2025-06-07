module;
#include "framework.h"
export module camera;

import general;

class camera {
public:
	camera() {
		MixFix::onAttach() += []() {
		CIniReader ini;
#if SRTTR
		if (ini.ReadBoolean("QOL", "BetterDriveCam", true)) {
			auto pattern = hook::pattern("E8 ? ? ? ? 44 0F B6 C5 0F 28 CF");
			if (!pattern.empty())
			Memory::VP::Nop(pattern.get_first(), 5);
		}
#endif
		if (ini.ReadBoolean("QOL", "BetterHandbrakeCam", false)) {
#if SRTTR
			auto pattern = hook::pattern("80 3D ? ? ? ? ? F3 0F 10 0A 0F 16 4A");
#else SRTT
			auto pattern = hook::pattern("80 3D ? ? ? ? ? 74 ? 83 3D ? ? ? ? ? 75 ? 8B 4C 24");
#endif
			if (!pattern.empty()) {
				bool* allow_handbrake_cam = NULL;
				uintptr_t instructionAddr = (uintptr_t)pattern.get_first();
#if SRTTR
				Memory::VP::ReadOffsetValue<1>(instructionAddr + 2, allow_handbrake_cam);
#else SRTT
				allow_handbrake_cam = (bool*)*(uintptr_t*)(instructionAddr + 0x2);
#endif
				printf("handbrake %p addr %p \n", allow_handbrake_cam, instructionAddr);
				if(allow_handbrake_cam)
				*allow_handbrake_cam = false;
			}
		}
			};
	}

}camera;