module;
#include "framework.h"
export module fixes;

import general;

#if SRIV_HV
import InGameConfig;
#endif

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

#if SRIV_HV
			auto pattern = hook::pattern("0F B6 1D ? ? ? ? E8 ? ? ? ? 83 3D");

			if (!pattern.empty()) {
				bool* Pause_for_weapon_screen;
				Memory::ReadOffsetValue(pattern.get_first(3), Pause_for_weapon_screen);
				printf("Pause for weapon %p\n", Pause_for_weapon_screen);
				configSystem.addOption<bool>(
					MenuType::GAMEPLAY,
					"Inventory Wheel Pause",
					"MIXFIX: PAUSES GAME WHILE INVENTORY IS OPEN",
					{ "OPTION_NO", "OPTION_YES" },
					OptionType::TYPE_TOGGLE,
					false,
					Pause_for_weapon_screen,
					"Gameplay",
					"InventoryWheelPause"
				);


			}
#endif
#ifdef SRIV_HV
			if (ini.ReadBoolean("Misc", "SkipIntros", false)) {
				auto pattern = hook::pattern("83 3D ? ? ? ? ? 0F 94 C0 C3 CC CC CC CC CC 48 83 EC ? 80 3D");
				if (!pattern.empty()) {
					static auto IntroSkip1 = safetyhook::create_inline(pattern.get_first(), ReturnTrue);
				}
				pattern = hook::pattern("75 ? C7 05 ? ? ? ? ? ? ? ? 48 83 C4 ? C3 E8");
				if (!pattern.empty()) {
					Memory::VP::Nop(pattern.get_first(), 2);
				}

				pattern = hook::pattern("49 8B 5B ? 33 C0 41 0F 28 73");
				if (!pattern.empty()) {
					Memory::VP::Patch(pattern.get_first(54 + 4),{ 0x43, 0x4B, 0x36, 0x00 });
					//Memory::VP::Patch(pattern.get_first(-0x1CB), { 0x89, 0xC7, 0x90 });
				}
				pattern = hook::pattern("48 83 EC ? 48 8D 15 ? ? ? ? B9 ? ? ? ? FF 15");
				if (!pattern.empty()) {
					//Memory::VP::Patch(pattern.get_first(), 0xC3);
				}

			}
#endif

			auto vintpattern = hook::pattern("40 38 3D ? ? ? ? 0F 85 ? ? ? ? 40 38 2D");
			if (!vintpattern.empty() && ini.ReadBoolean("Fixes","FixMouseFlicker",false)) {
				bool* Vint_hardware_cursor; 
				Memory::VP::ReadOffsetValue(vintpattern.get_first(3), Vint_hardware_cursor);
				if (Vint_hardware_cursor)
					*Vint_hardware_cursor = false;

			}
			};
	}
}fixes;