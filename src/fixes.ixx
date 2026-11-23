module;
#include "framework.h"
export module fixes;

import general;

#if SRIV_HV
import InGameConfig;
#endif

#if SRTT
SafetyHookInline particle_get_box_def_h;
void __fastcall particle_get_box_def(uintptr_t sp, void*, void* box)
{
	if (sp == 0x40)
	{
		printf("[crashfix] prevented crash: particle_get_box_def call with bad ptr\n");
		return;
	}

	particle_get_box_def_h.thiscall(sp, box);
}

SafetyHookInline particle_unk1_h;
void __fastcall particle_unk1(uintptr_t ecx, void* edx, void* a1, void* a2)
{
	if (ecx == 0)
	{
		printf("[crashfix] prevented crash: particle_unk1 call with ecx == nullptr\n");
		return;
	}

	if (*reinterpret_cast<uintptr_t*>(ecx + 0xD8) == 0)
	{
		printf("[crashfix] prevented crash: particle_unk1 call with invalid object\n");
		return;
	}

	particle_unk1_h.fastcall(ecx, edx, a1, a2);
}

SafetyHookInline particle_unk2_h;
void __fastcall particle_unk2(uintptr_t ecx, void*, bool a1)
{
	if (ecx == 0)
	{
		printf("[crashfix] prevented crash: particle_unk2 call with ecx == nullptr\n");
		return;
	}

	if (*reinterpret_cast<uintptr_t*>(ecx + 0xD8) == 0)
	{
		printf("[crashfix] prevented crash: particle_unk2 call with invalid object\n");
		return;
	}

	particle_unk2_h.thiscall(ecx, a1);
}

SafetyHookInline particle_unk3_h;
void __fastcall particle_unk3(uintptr_t ecx, void*)
{
	if (ecx == 0)
	{
		printf("[crashfix] prevented crash: particle_unk3 call with ecx == nullptr\n");
		return;
	}

	if (*reinterpret_cast<uintptr_t*>(ecx + 0xD8) == 0)
	{
		printf("[crashfix] prevented crash: particle_unk3 call with invalid object\n");
		return;
	}

	particle_unk3_h.thiscall(ecx);
}
#endif

#if SRTTR
// Hides the "slow connection" HUD warning (UI element "hud_connection_warning").
//
// How it works:
//  - the game keeps every UI element in a linked list; an element is drawn only while its
//    "active" byte (element + 0x5F3) is non-zero.
//  - the function that draws all active elements is hooked, and right before it runs we look up
//    "hud_connection_warning" by name (using the game's own lookup function) and clear that byte.
//    The game can switch the element back on at any time, we just switch it off again every frame.
namespace hud_connection_warning_fix
{
	constexpr size_t UI_ELEMENT_ACTIVE_OFFSET = 0x5F3;

	using FindUIElementByName_t = uint8_t* (__fastcall*)(const char* name);
	FindUIElementByName_t FindUIElementByName = nullptr;
	SafetyHookInline draw_ui_elements_h;

	void __fastcall draw_ui_elements_hook()
	{
		if (FindUIElementByName)
		{
			if (uint8_t* element = FindUIElementByName("hud_connection_warning"))
				element[UI_ELEMENT_ACTIVE_OFFSET] = 0;
		}

		draw_ui_elements_h.call();
	}

	void install()
	{
		// UI element lookup by name: returns UI element* or nullptr
		auto find_element_pattern = hook::pattern("48 83 EC 28 48 85 C9 74 2D 33 D2 E8 ? ? ? ? 48 8B 0D ? ? ? ? 8B D0 48 8B C1 48 85 C9 74 15 39 90 C8 05 00 00");

		// draws all active UI elements (reads the active byte at +0x5F3)
		auto draw_pattern = hook::pattern("48 89 5C 24 08 57 48 81 EC 30 02 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 20 02 00 00 48 8B 15 ? ? ? ? 33 DB 89 1D ? ? ? ? 48 8B C2 48 85 D2 74 1D 80 B8 F3 05 00 00 00");

		if (find_element_pattern.size() != 1 || draw_pattern.size() != 1)
		{
			printf("[hud_connection_warning] patterns not found, option skipped\n");
			return;
		}

		FindUIElementByName = reinterpret_cast<FindUIElementByName_t>(find_element_pattern.get_first(0));
		draw_ui_elements_h = safetyhook::create_inline(draw_pattern.get_first(0), &draw_ui_elements_hook);
	}
}
#endif


class fixes {
public:
	fixes() {
		MixFix::onAttach() += []() {
			CIniReader ini;
#if SRTTR
			if (ini.ReadBoolean("HUD", "disable_hud_connection_warning", true))
				hud_connection_warning_fix::install();
#endif
#if SRTT
			// Motion blur fix, display.ini claims # MotionBlur can override motion blur off (even when post process detail is up.)
			// This was incorrect, it turned motion blur off for a single frame before the post process sets it back to true!
			{ 
				
				auto pattern = hook::pattern("A2 ? ? ? ? A1 ? ? ? ? 3B C3");
				Memory::VP::Nop(pattern.get_first(), 5);
			}

			if (ini.ReadBoolean("Misc", "ParticleCrashFix", true))
			{
				auto p1 = hook::pattern("83 EC ? 56 8B F1 8B 44 24");
				particle_get_box_def_h = safetyhook::create_inline(p1.get_first(0), particle_get_box_def);

				auto p2 = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? 80 7D ? ? 53 56 8B F1");
				particle_unk1_h = safetyhook::create_inline(p2.get_first(0), particle_unk1);

				auto p3 = hook::pattern("53 8B 5C 24 ? 57 8B F9 38 9F");
				particle_unk2_h = safetyhook::create_inline(p3.get_first(0), particle_unk2);

				auto p4 = hook::pattern("A1 ? ? ? ? 8B 50 ? 53 56 8B F1");
				particle_unk3_h = safetyhook::create_inline(p4.get_first(0), particle_unk3);
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
