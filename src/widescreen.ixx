module;
#include "framework.h"
export module widescreen;
import general;
SafetyHookInline fullscreen_borderless_handlerT;
uintptr_t fullscreen_borderless_handler(bool fullscreen, bool borderless) {
	return fullscreen_borderless_handlerT.fastcall<uintptr_t>(fullscreen, true);
}

SafetyHookInline os_create_windowT;

char __fastcall os_create_window_hook(LONG nWidth, LONG nHeight, bool fullscreen, bool borderless, HMONITOR hMonitor) {
	if (!fullscreen)
		borderless = true;
	return os_create_windowT.unsafe_fastcall<char>(nWidth, nHeight, fullscreen, borderless, hMonitor);
}

class widescreen {
public:
	widescreen() {
		MixFix::onAttach() += []() {
			CIniReader ini;
			if (ini.ReadBoolean("Graphics", "DisableBlackBars", true)) {
#if SRTTR || SRIV_HV
				auto pattern = hook::pattern("C6 05 ? ? ? ? ? 84 C9 74 ? F3 0F 10 05");
#else SRTT
				auto pattern = hook::pattern("0F 57 C0 C6 05 ? ? ? ? ? F3 0F 11 05 ? ? ? ? C7 05");
				Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
				pattern = hook::pattern("80 7C 24 ? ? C6 05 ? ? ? ? ? 74 ? F3 0F 10 05");
#endif
				if(!pattern.empty())
				Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
			}
#ifdef SRIV_HV
			auto pattern = hook::pattern("84 C9 74 ? B8");
			if (ini.ReadBoolean("Misc","ForceBorderless", SR4_BUILD) && !pattern.empty()) {
				fullscreen_borderless_handlerT = safetyhook::create_inline(pattern.get_first(), fullscreen_borderless_handler);
			}
			 pattern = hook::pattern("40 55 53 56 57 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 48 8B 5D");

			if (!pattern.empty()) {
				os_create_windowT = safetyhook::create_inline(pattern.get_first(), os_create_window_hook);
			}

#endif
			};

	}
} widescreen;