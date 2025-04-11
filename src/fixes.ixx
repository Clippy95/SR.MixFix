module;
#include "framework.h"
export module fixes;

import general;

class fixes {
public:
	fixes() {
		MixFix::onAttach() += []() {
			CIniReader ini;
#if SRTT
			// Motion blur fix, display.ini claims # MotionBlur can override motion blur off (even when post process detail is up.)
			// This was incorrect, it turned motion blur off for a single frame before the post process sets it back to true!
			{ 
				
				auto pattern = hook::pattern("A2 ? ? ? ? A1 ? ? ? ? 3B C3");
				Memory::VP::Nop(pattern.get_first(), 5);
			}
#endif
			};
	}
}fixes;