module;
#include "framework.h"
export module InGameConfig;
#if SRIV_HV
import general;

typedef double(__fastcall* lua_tonumberT)(uintptr_t L, int idx);
lua_tonumberT lua_tonumberG;

typedef __int64(__fastcall* lua_gettopT)(uintptr_t L);
lua_gettopT lua_gettopG;

typedef __int64(__fastcall* lua_typeT)(uintptr_t L, int idx);
lua_typeT lua_typeG;

typedef __int64(__fastcall* lua_pushnumberT)(uintptr_t L, double value);
lua_pushnumberT lua_pushnumberG;

static SafetyHookInline lua_load_dynamic_script_buffer_hook{};
static char* replacement_buffer = nullptr;

static char __fastcall lua_load_dynamic_script_buffer(
    const char* LuaBuffer,
    unsigned __int64 buff_size,
    const char* filename,
    __int64 root_state,
    __int64 temp_mempool) {

    std::string_view sfilename(filename);

    // Get module directory
    char module_path[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(nullptr), module_path, MAX_PATH);
    std::string module_dir = module_path;
    size_t last_slash = module_dir.find_last_of("\\/");
    if (last_slash != std::string::npos) {
        module_dir = module_dir.substr(0, last_slash + 1);
    }

    // Check if replacement file exists
    std::string replacement_file = module_dir + std::string(filename);

    FILE* file = nullptr;
    if (fopen_s(&file, replacement_file.c_str(), "rb") == 0 && file != nullptr) {
        printf("Found replacement file: %s\n", replacement_file.c_str());

        // Delete old buffer if it exists
        if (replacement_buffer) {
            delete[] replacement_buffer;
            replacement_buffer = nullptr;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size > 0) {
            // Allocate new buffer
            replacement_buffer = new char[file_size];

            // Read file into buffer
            size_t bytes_read = fread(replacement_buffer, 1, file_size, file);
            fclose(file);

            if (bytes_read == file_size) {
                printf("Loaded %zu bytes from %s\n", bytes_read, filename);
                return lua_load_dynamic_script_buffer_hook.fastcall<char>(
                    replacement_buffer,
                    static_cast<unsigned __int64>(file_size),
                    filename,
                    root_state,
                    temp_mempool
                );
            }
            else {
                printf("Failed to read complete file: %s\n", filename);
                delete[] replacement_buffer;
                replacement_buffer = nullptr;
            }
        }
        else {
            fclose(file);
            printf("File is empty: %s\n", filename);
        }
    }

    // Fallback to original if file doesn't exist or failed to load
    return lua_load_dynamic_script_buffer_hook.fastcall<char>(LuaBuffer, buff_size, filename, root_state, temp_mempool);
}

bool Testing = false;

static SafetyHookInline vint_get_avg_processing_time_hook{};
static __int64 __fastcall vint_get_avg_processing_timeD(uintptr_t lua_state) {


	int argc = (int)lua_gettopG(lua_state);
	if (argc == 1) {
		int option_index = (int)lua_tonumberG(lua_state, 1);
		if (option_index == 0) {
			int value = Testing;
			lua_pushnumberG(lua_state, static_cast<double>(value + 1));
			return 1;
		}
		
	}
	return 0;
}



void __fastcall game_gds_log_option_hook(SafetyHookContext& ctx) {
	int which_menu = static_cast<int>(ctx.rbp & 0xFFFFFFFF);        // EBP is lower 32 bits of RBP
	int control_index = static_cast<int>(ctx.rsi & 0xFFFFFFFF);     // ESI is lower 32 bits of RSI
	bool we_dont_need = static_cast<bool>(ctx.rdi & 0xFFFFFFFF);    // EDI is lower 32 bits of RDI
	float control_amount = ctx.xmm6.f32[0];
	printf("menu %d index %d control amount %f\n",which_menu,control_index ,control_amount);

	if (which_menu == 0 && control_index == 8) {

		Testing = (control_amount == 2.f);
		printf("testing is %d\n", Testing);

	}
	

}


class InGameConfig {
public:
	InGameConfig() {
		MixFix::onAttach() += []() {
			CIniReader ini;
			if (ini.ReadBoolean("Misc", "InGameConfig", true)) {
				auto pattern = hook::pattern("40 53 55 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 48 8B AC 24");
				if(!pattern.empty())
				lua_load_dynamic_script_buffer_hook = safetyhook::create_inline(pattern.get_first(), lua_load_dynamic_script_buffer);

				pattern = hook::pattern("E8 ? ? ? ? 8B D3 49 8B CE F7 DA");

				if (!pattern.empty()) {
					Memory::VP::ReadCall(pattern.get_first(), lua_tonumberG);
					printf("lua_to_number addr %p\n", lua_tonumberG);

					Memory::VP::ReadCall(pattern.get_first(-0xF), lua_gettopG);
					printf("lua_gettopG addr %p\n", lua_gettopG);


					pattern = hook::pattern("E8 ? ? ? ? 41 8B 46 ? 48 8B CF");

					Memory::VP::ReadCall(pattern.get_first(), lua_pushnumberG);
					printf("lua_pushnumberG addr %p\n", lua_pushnumberG);

				}

			}
			vint_get_avg_processing_time_hook = safetyhook::create_inline(hook::pattern("48 83 EC ? E8 ? ? ? ? 33 C0 48 83 C4 ? C3 40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B 05").get_first(), vint_get_avg_processing_timeD);

			static auto gds_hook = safetyhook::create_mid(hook::pattern("E8 ? ? ? ? 83 FD ? 0F 87").get_first(), &game_gds_log_option_hook);
		};
	}
} InGameConfig;
#endif