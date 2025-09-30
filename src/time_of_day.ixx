module;
#include "framework.h"
export module time_of_day;
import general;
import InGameConfig;
class time_of_day {
public:
	time_of_day() {
		MixFix::onAttach() += []() {
			CIniReader ini;

			auto pattern = hook::pattern("80 3D ? ? ? ? ? 74 ? 44 0F B6 05");
			bool* Dynamic_tod;
			bool* Semi_dynamic_tod;
			if (!pattern.empty()) {
				Memory::ReadOffsetValue<1>(pattern.get_first(2), Dynamic_tod);
				Memory::ReadOffsetValue<1>(pattern.get_first(0x2F), Semi_dynamic_tod);
				printf("Dynamic TOD %p %p\n", Dynamic_tod, Semi_dynamic_tod);


				if (Dynamic_tod && Semi_dynamic_tod) {
					*Dynamic_tod = ini.ReadBoolean("Debug", "Dynamic_tod", false);
					*Semi_dynamic_tod = ini.ReadBoolean("Debug", "Dynamic_tod", false);

				}
#if SRIV_HV

				configSystem.addOption<bool>(
					MenuType::GAMEPLAY,
					"Dynamic_tod",
					"MIXFIX DEBUG: Dynamic_tod",
					{ "OPTION_NO", "OPTION_YES" },
					OptionType::TYPE_TOGGLE,
					false,
					Dynamic_tod,
					"Debug",
					"Dynamic_tod"
				);

				configSystem.addOption<bool>(
					MenuType::GAMEPLAY,
					"Semi_dynamic_tod",
					"MIXFIX DEBUG: Semi_dynamic_tod",
					{ "OPTION_NO", "OPTION_YES" },
					OptionType::TYPE_TOGGLE,
					false,
					Semi_dynamic_tod,
					"Debug",
					"Semi_dynamic_tod"
				);


#endif

		}

			};
	}
} time_of_day;