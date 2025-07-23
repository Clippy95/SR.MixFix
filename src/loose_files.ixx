
module;
#include "framework.h"
export module loose_files;
#if SRTTR
import general;

SafetyHookInline search_files{};
__int64 __fastcall search_files_hook(int search, int media, bool do_first) {

	if (search == 0)
		do_first = true;
	else do_first = false;
	return search_files.unsafe_fastcall<__int64>(search, media, do_first);
}
class loose_files {
public:
	loose_files() {
		MixFix::onAttach() += []() {
			CIniReader ini;


			if(ini.ReadBoolean("QOL","ImproveLooseFiles",true)) {
				auto pattern = hook::pattern("40 57 48 83 EC ? 48 C7 44 24 ? ? ? ? ? 48 89 5C 24 ? 48 89 74 24 ? 41 0F B6 F0");
				search_files = safetyhook::create_inline(pattern.get_first(), &search_files_hook);

			}

			};
	}
}loose_files;
#endif