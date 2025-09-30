module;
#include "framework.h"
export module camera;

import general;
import InGameConfig;

class camera {
public:
	camera() {
		MixFix::onAttach() += []() {
		CIniReader ini;
#if SRTTR || SRIV_HV
		if (ini.ReadBoolean("QOL", "BetterDriveCam", true)) {
			auto pattern = hook::pattern("E8 ? ? ? ? 44 0F B6 C5 0F 28 CF");
			if (!pattern.empty())
			Memory::VP::Nop(pattern.get_first(), 5);
		}
#endif

		if (ini.ReadBoolean("QOL", "BetterHandbrakeCam", false)) {
#if SRTTR || SRIV_HV
			auto pattern = get_pattern("X", "80 3D ? ? ? ? ? F3 0F 10 0A 0F 16 4A","80 3D ? ? ? ? ? F3 0F 10 77");
#elif SRTT
			auto pattern = hook::pattern("80 3D ? ? ? ? ? 74 ? 83 3D ? ? ? ? ? 75 ? 8B 4C 24");
#endif
			if (!pattern.empty()) {
				bool* allow_handbrake_cam = NULL;
				uintptr_t instructionAddr = (uintptr_t)pattern.get_first();
#if SRTTR || SRIV_HV
				Memory::VP::ReadOffsetValue<1>(instructionAddr + 2, allow_handbrake_cam);
#else SRTT
				allow_handbrake_cam = (bool*)*(uintptr_t*)(instructionAddr + 0x2);
#endif
				printf("handbrake %p addr %p \n", allow_handbrake_cam, instructionAddr);
				if(allow_handbrake_cam)
				*allow_handbrake_cam = false;
			}
		}

#ifdef SRIV_HV
		static int Camera_velocity_offset_disable = 1;
#define Camera_velocity_offset_disable_name "Camera_velocity_offset_disable"
		configSystem.addOption<int>(
			MenuType::GAMEPLAY,
			"Disable Camera velocity offset",
			"MIXFIX: Disables camera 'lag' effect behind player",
			{ "OPTION_NO", "Disabled when Script Camera","Always Disabled"},
			OptionType::TYPE_TOGGLE,
			1,
			&Camera_velocity_offset_disable,
			"Fixes",
			Camera_velocity_offset_disable_name
		);
		Camera_velocity_offset_disable = ini.ReadInteger("Fixes", Camera_velocity_offset_disable_name, 1);
			//auto pattern = hook::pattern("80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 8D 55 ? 0F 29 65");
			//if (!pattern.empty()) {
			//	bool* Camera_velocity_offset_disable = nullptr;
			//	Memory::VP::ReadOffsetValue<1>(pattern.get_first(2), Camera_velocity_offset_disable);
			//	if (Camera_velocity_offset_disable) {
			//		printf("CAMERA %p\n", Camera_velocity_offset_disable);
			//		*Camera_velocity_offset_disable = true;
			//	}
			//}

			auto pattern = hook::pattern("80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 8D 55 ? 0F 29 65");
			auto Script_camera_enabled_pattern = hook::pattern("C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? C3 CC CC CC CC CC CC 0F B6 05");
			static bool* Script_camera_enabled;
			static uintptr_t velocity_exit_addr;
			if (!pattern.empty() && !Script_camera_enabled_pattern.empty()) {
				//Memory::VP::Nop(pattern.get_first(), 13);
				Memory::VP::ReadOffsetValue<1>(Script_camera_enabled_pattern.get_first(2), Script_camera_enabled);
				printf("script camera %p\n", Script_camera_enabled);

				velocity_exit_addr = (uintptr_t)pattern.get_first(0x2D9);

				static auto new_velocity_check = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {
					if (Camera_velocity_offset_disable == 1 && *Script_camera_enabled) {
						ctx.rip = velocity_exit_addr;
						return;
					}
					else if (Camera_velocity_offset_disable == 2) {
						ctx.rip = velocity_exit_addr;
					}
					});


			}

		

#endif
			};
	}

}camera;