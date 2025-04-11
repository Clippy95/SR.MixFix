module;
#include "framework.h"
export module widescreen;
import general;
class widescreen {
public:
	widescreen() {
		MixFix::onAttach() += []() {
			CIniReader ini;
			if (ini.ReadBoolean("Graphics", "DisableBlackBars", true)) {
#if SRTTR
				auto pattern = hook::pattern("C6 05 ? ? ? ? ? 84 C9 74 ? F3 0F 10 05");
#else SRTT
				auto pattern = hook::pattern("0F 57 C0 C6 05 ? ? ? ? ? F3 0F 11 05 ? ? ? ? C7 05");
				Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
				pattern = hook::pattern("80 7C 24 ? ? C6 05 ? ? ? ? ? 74 ? F3 0F 10 05");
#endif
				if(!pattern.empty())
				Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
			}
			};
	}
} widescreen;