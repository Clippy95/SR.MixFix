module;
#include "framework.h"
export module InGameConfig;
#if SRIV_HV
import general;

// Lua function pointers
typedef double(__fastcall* lua_tonumberT)(uintptr_t L, int idx);
lua_tonumberT lua_tonumberG;

typedef __int64(__fastcall* lua_gettopT)(uintptr_t L);
lua_gettopT lua_gettopG;

typedef __int64(__fastcall* lua_typeT)(uintptr_t L, int idx);
lua_typeT lua_typeG;

typedef __int64(__fastcall* lua_pushnumberT)(uintptr_t L, double value);
lua_pushnumberT lua_pushnumberG;

// Config system types
export enum class MenuType {
    GAMEPLAY = 0,
    GRAPHICS = 1,
    AUDIO = 2
};

export enum class OptionType {
    TYPE_TOGGLE,    // bool options
    TYPE_SLIDER,    // float/int ranges (stub for now)
    TYPE_LIST       // multiple choice (stub for now)
};

// Base class for type erasure
class GameOptionBase {
public:
    std::string label;
    std::string tooltip;
    std::vector<std::string> options;
    OptionType type;
    std::string ini_section;
    std::string ini_key;
    bool use_ini;

    virtual ~GameOptionBase() = default;
    virtual void setValueFromFloat(float value) = 0;
    virtual float getValueAsFloat() const = 0;
};

template<typename T>
class GameOption : public GameOptionBase {
private:
    T* value_ptr = nullptr;
    std::function<T()> getter = nullptr;
    std::function<void(T)> setter = nullptr;
    T default_value;
    bool use_ini_with_pointer = false;

public:
    GameOption(const std::string& label, const std::string& tooltip,
        const std::vector<std::string>& options, OptionType type,
        T default_val, const std::string& section = "", const std::string& key = "",
        T* ptr = nullptr, bool use_ini_and_pointer = false,
        std::function<T()> get_func = nullptr,
        std::function<void(T)> set_func = nullptr)
        : default_value(default_val), value_ptr(ptr), getter(get_func), setter(set_func),
        use_ini_with_pointer(use_ini_and_pointer) {
        this->label = label;
        this->tooltip = tooltip;
        this->options = options;
        this->type = type;
        this->ini_section = section.empty() ? "Misc" : section;
        this->ini_key = key.empty() ? label : key;
        this->use_ini = use_ini_and_pointer || (ptr == nullptr && get_func == nullptr && set_func == nullptr);

        // Auto-initialize pointer from INI if both are provided
        if (use_ini_and_pointer && value_ptr) {
            CIniReader ini;
            if constexpr (std::is_same_v<T, bool>) {
                *value_ptr = ini.ReadBoolean(ini_section.c_str(), ini_key.c_str(), default_value);
            }
            else if constexpr (std::is_same_v<T, int>) {
                *value_ptr = ini.ReadInteger(ini_section.c_str(), ini_key.c_str(), default_value);
            }
            else if constexpr (std::is_same_v<T, float>) {
                *value_ptr = ini.ReadFloat(ini_section.c_str(), ini_key.c_str(), default_value);
            }
            printf("Auto-initialized %s to %s from INI\n", label.c_str(),
                (std::is_same_v<T, bool> ? (*value_ptr ? "true" : "false") : "value"));
        }
    }

    T getValue() const {
        if (use_ini) {
            CIniReader ini;
            T ini_value;
            if constexpr (std::is_same_v<T, bool>) {
                ini_value = ini.ReadBoolean(ini_section.c_str(), ini_key.c_str(), default_value);
            }
            else if constexpr (std::is_same_v<T, int>) {
                ini_value = ini.ReadInteger(ini_section.c_str(), ini_key.c_str(), default_value);
            }
            else if constexpr (std::is_same_v<T, float>) {
                ini_value = ini.ReadFloat(ini_section.c_str(), ini_key.c_str(), default_value);
            }

            // If we have both INI and pointer, sync pointer with INI value
            if (use_ini_with_pointer && value_ptr) {
                *value_ptr = ini_value;
            }
            return ini_value;
        }

        if (value_ptr) return *value_ptr;
        if (getter) return getter();
        return default_value;
    }

    void setValue(T val) {
        if (use_ini) {
            CIniReader ini;
            if constexpr (std::is_same_v<T, bool>) {
                ini.WriteBoolean(ini_section.c_str(), ini_key.c_str(), val);
            }
            else if constexpr (std::is_same_v<T, int>) {
                ini.WriteInteger(ini_section.c_str(), ini_key.c_str(), val);
            }
            else if constexpr (std::is_same_v<T, float>) {
                ini.WriteFloat(ini_section.c_str(), ini_key.c_str(), val);
            }
        }

        // Always update pointer and setter if they exist
        if (value_ptr) *value_ptr = val;
        if (setter) setter(val);
    }

    void setValueFromFloat(float value) override {
        if constexpr (std::is_same_v<T, bool>) {
            setValue(value == 2.0f); // Lua uses 2 for true, 1 for false
        }
        else if constexpr (std::is_same_v<T, int>) {
            setValue(static_cast<int>(value) - 1); // Convert 1,2,3 to 0,1,2
        }
        else if constexpr (std::is_same_v<T, float>) {
            setValue(value);
        }
    }

    float getValueAsFloat() const override {
        T val = getValue();
        if constexpr (std::is_same_v<T, bool>) {
            return val ? 2.0f : 1.0f; // Convert bool to Lua display values
        }
        else if constexpr (std::is_same_v<T, int>) {
            return static_cast<float>(val + 1); // Convert 0,1,2 to 1,2,3
        }
        else if constexpr (std::is_same_v<T, float>) {
            return val;
        }
        return 0.0f;
    }
};

class InGameConfigSystem {
private:
    std::unordered_map<MenuType, std::vector<std::unique_ptr<GameOptionBase>>> options;
    std::unordered_map<MenuType, int> menu_base_indices = {
        {MenuType::GAMEPLAY, 8},  // Gameplay custom options start at index 8
        {MenuType::GRAPHICS, 50}, // Graphics could start at 50 (placeholder)
        {MenuType::AUDIO, 100}    // Audio could start at 100 (placeholder)
    };

public:
    template<typename T>
    void addOption(MenuType menu, const std::string& label, const std::string& tooltip,
        const std::vector<std::string>& option_labels, OptionType type,
        T default_value, const std::string& ini_section = "", const std::string& ini_key = "") {
        auto option = std::make_unique<GameOption<T>>(
            label, tooltip, option_labels, type, default_value, ini_section, ini_key);
        options[menu].push_back(std::move(option));
    }

    template<typename T>
    void addOption(MenuType menu, const std::string& label, const std::string& tooltip,
        const std::vector<std::string>& option_labels, OptionType type,
        T default_value, T* value_ptr) {
        auto option = std::make_unique<GameOption<T>>(
            label, tooltip, option_labels, type, default_value, "", "", value_ptr);
        options[menu].push_back(std::move(option));
    }

    template<typename T>
    void addOption(MenuType menu, const std::string& label, const std::string& tooltip,
        const std::vector<std::string>& option_labels, OptionType type,
        T default_value, std::function<T()> getter, std::function<void(T)> setter) {
        auto option = std::make_unique<GameOption<T>>(
            label, tooltip, option_labels, type, default_value, "", "", nullptr, getter, setter);
        options[menu].push_back(std::move(option));
    }

    template<typename T>
    void addOption(MenuType menu, const std::string& label, const std::string& tooltip,
        const std::vector<std::string>& option_labels, OptionType type,
        T default_value, T* value_ptr, const std::string& ini_section, const std::string& ini_key) {
        auto option = std::make_unique<GameOption<T>>(
            label, tooltip, option_labels, type, default_value, ini_section, ini_key, value_ptr, true);
        options[menu].push_back(std::move(option));
    }

    GameOptionBase* getOption(MenuType menu, int custom_index) {
        if (options[menu].size() > custom_index) {
            return options[menu][custom_index].get();
        }
        return nullptr;
    }

    int getCustomOptionIndex(MenuType menu, int lua_control_index) {
        return lua_control_index - menu_base_indices[menu];
    }

    bool isCustomOption(MenuType menu, int lua_control_index) {
        return lua_control_index >= menu_base_indices[menu];
    }

    void handleOptionChange(int which_menu, int control_index, float control_amount) {
        MenuType menu = static_cast<MenuType>(which_menu);

        if (isCustomOption(menu, control_index)) {
            int custom_index = getCustomOptionIndex(menu, control_index);
            auto* option = getOption(menu, custom_index);

            if (option) {
                option->setValueFromFloat(control_amount);
                //printf("Set custom option %d in menu %d to %f\n", custom_index, which_menu, control_amount);
            }
        }
    }

    // Debug the insertion points
    std::string modifyLuaScriptForFile(MenuType menu, const std::string& original_script) {
        if (options[menu].empty()) {
            return original_script;
        }

        std::string modified_script = original_script;
        std::stringstream additions;

        // Generate custom options to add inside options_difficulty_add_pc_options()
        additions << "\n\t-- Custom options added by InGameConfig system\n";
        for (size_t i = 0; i < options[menu].size(); i++) {
            auto& option = options[menu][i];
            additions << "\tData[#Data + 1] = {\n";
            additions << "\t\ttype = " << (option->type == OptionType::TYPE_TOGGLE ? "TYPE_TOGGLE" : "TYPE_SLIDER") << ",\n";
            additions << "\t\tlabel = \"" << option->label << "\",\n";
            additions << "\t\toptions = {";
            for (size_t j = 0; j < option->options.size(); j++) {
                additions << "\"" << option->options[j] << "\"";
                if (j < option->options.size() - 1) additions << ", ";
            }
            additions << "},\n";
            additions << "\t\tcurrent_value = 2,\n";
            additions << "\t\ttool_tip_text = \"" << option->tooltip << "\",\n";
            additions << "\t\tid = #Data\n";
            additions << "\t}\n";
            additions << "\t\n";
        }

        // Find the end of options_difficulty_add_pc_options() function
        size_t func_start = modified_script.find("function options_difficulty_add_pc_options()");
        if (func_start != std::string::npos) {
            // Find the matching "end" for this function by counting braces/function depth
            size_t pos = func_start + 38; // Skip past "function options_difficulty_add_pc_options()"
            int depth = 1; // We're inside one function

            while (pos < modified_script.length() && depth > 0) {
                if (modified_script.substr(pos, 8) == "function") {
                    depth++;
                    pos += 8;
                }
                else if (modified_script.substr(pos, 3) == "end") {
                    depth--;
                    if (depth == 0) {
                        // Found the matching end
                        modified_script.insert(pos, additions.str());
                        //printf("Inserted custom options at end of options_difficulty_add_pc_options()\n");
                        break;
                    }
                    pos += 3;
                }
                else {
                    pos++;
                }
            }
        }
        else {
            printf("Could not find options_difficulty_add_pc_options() function\n");
            modified_script += "\n" + additions.str();
        }

        // Add populate calls to the populate function
        std::stringstream populate_additions;
        populate_additions << "\t-- Custom option population\n";
        for (size_t i = 0; i < options[menu].size(); i++) {
            int lua_index = menu_base_indices[menu] + static_cast<int>(i);
            populate_additions << "\tData[" << lua_index << "].previous_value = Data[" << lua_index << "].current_value\n";
            populate_additions << "\tData[" << lua_index << "].current_value = vint_get_avg_processing_time(" << i << ")\n";
        }

        // Find the end of the populate function
        size_t populate_func = modified_script.find("function options_difficulty_populate(");
        if (populate_func != std::string::npos) {
            size_t pos = populate_func + 37; // Skip past function name
            int depth = 1;

            while (pos < modified_script.length() && depth > 0) {
                if (modified_script.substr(pos, 8) == "function") {
                    depth++;
                    pos += 8;
                }
                else if (modified_script.substr(pos, 3) == "end") {
                    depth--;
                    if (depth == 0) {
                        modified_script.insert(pos, populate_additions.str());
                        //printf("Inserted populate code at end of options_difficulty_populate()\n");
                        break;
                    }
                    pos += 3;
                }
                else {
                    pos++;
                }
            }
        }
        else {
            printf("Could not find options_difficulty_populate() function\n");
            modified_script += "\n" + populate_additions.str();
        }

        return modified_script;
    }
};

// Global instance
export InGameConfigSystem configSystem;

static SafetyHookInline lua_load_dynamic_script_buffer_hook{};
static char* replacement_buffer = nullptr;

void DebugDumpLua(const std::string& buffer, const std::string& stage) {
    // Get current module directory
    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(nullptr), modulePath, MAX_PATH);
    std::string moduleDir = std::string(modulePath);
    size_t lastSlash = moduleDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        moduleDir = moduleDir.substr(0, lastSlash + 1);
    }

    std::string filename = moduleDir + "pause_menu_" + stage + ".lua";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << buffer;
        file.close();
    }
}

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
    std::string final_lua_content;
    bool file_found = false;

    FILE* file = nullptr;
    if (fopen_s(&file, replacement_file.c_str(), "rb") == 0 && file != nullptr) {
        //printf("Found replacement file: %s\n", replacement_file.c_str());

        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size > 0) {
            // Read file into string
            std::string file_content(file_size, '\0');
            size_t bytes_read = fread(file_content.data(), 1, file_size, file);
            fclose(file);

            if (bytes_read == file_size) {
                final_lua_content = file_content;
                file_found = true;
               // printf("Loaded %zu bytes from file: %s\n", bytes_read, filename);
            }
            else {
                printf("Failed to read complete file: %s\n", filename);
                fclose(file);
            }
        }
        else {
            fclose(file);
            printf("File is empty: %s\n", filename);
        }
    }

    // If no file found, use original buffer
    if (!file_found) {
        final_lua_content = std::string(LuaBuffer, buff_size);
    }

    // Apply config system modifications if this is the gameplay menu
    if (sfilename == "pause_options_difficulty.lua") {
        final_lua_content = configSystem.modifyLuaScriptForFile(MenuType::GAMEPLAY, final_lua_content);
        printf("Applied config system modifications to %s\n", filename);
    }

    // Delete old buffer if it exists
    if (replacement_buffer) {
        delete[] replacement_buffer;
        replacement_buffer = nullptr;
    }

    // Create new buffer with final content
    replacement_buffer = new char[final_lua_content.size()];
    memcpy(replacement_buffer, final_lua_content.data(), final_lua_content.size());



    return lua_load_dynamic_script_buffer_hook.fastcall<char>(
        replacement_buffer,
        static_cast<unsigned __int64>(final_lua_content.size()),
        filename,
        root_state,
        temp_mempool
    );
}

static SafetyHookInline vint_get_avg_processing_time_hook{};
static __int64 __fastcall vint_get_avg_processing_timeD(uintptr_t lua_state) {
    int argc = (int)lua_gettopG(lua_state);

    if (argc == 1) {
        // GET: lua call with 1 argument (option_index)
        int option_index = (int)lua_tonumberG(lua_state, 1);
        auto* option = configSystem.getOption(MenuType::GAMEPLAY, option_index);

        if (option) {
            float value = option->getValueAsFloat();
            lua_pushnumberG(lua_state, static_cast<double>(value));
            return 1; // Return 1 value
        }
    }
    else if (argc == 2) {
        // SET: lua call with 2 arguments (option_index, new_value)
        int option_index = (int)lua_tonumberG(lua_state, 1);
        double new_value = lua_tonumberG(lua_state, 2);
        auto* option = configSystem.getOption(MenuType::GAMEPLAY, option_index);

        if (option) {
            option->setValueFromFloat(static_cast<float>(new_value));
        }
        return 0; // No return values
    }

    return 0; // Invalid number of arguments
}

void __fastcall game_gds_log_option_hook(SafetyHookContext& ctx) {
    int which_menu = static_cast<int>(ctx.rbp & 0xFFFFFFFF);
    int control_index = static_cast<int>(ctx.rsi & 0xFFFFFFFF);
    bool we_dont_need = static_cast<bool>(ctx.rdi & 0xFFFFFFFF);
    float control_amount = ctx.xmm6.f32[0];

    // Let the config system handle it
    configSystem.handleOptionChange(which_menu, control_index, control_amount);
}

class InGameConfig {
public:
    InGameConfig() {
        MixFix::onAttach() += []() {
            CIniReader ini;
            if (ini.ReadInteger("Misc", "InGameConfig", 1) == 0)
                return;
                auto pattern = hook::pattern("40 53 55 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 48 8B AC 24");
                if (!pattern.empty())
                    lua_load_dynamic_script_buffer_hook = safetyhook::create_inline(pattern.get_first(), lua_load_dynamic_script_buffer);

                pattern = hook::pattern("E8 ? ? ? ? 8B D3 49 8B CE F7 DA");
                if (!pattern.empty()) {
                    Memory::VP::ReadCall(pattern.get_first(), lua_tonumberG);
                    printf("lua_to_number addr %p\n", lua_tonumberG);

                    Memory::VP::ReadCall(pattern.get_first(-0xF), lua_gettopG);
                    printf("lua_gettopG addr %p\n", lua_gettopG);

                    pattern = find_pattern("E8 ? ? ? ? 41 8B 46 ? 48 8B CF","E8 ? ? ? ? 41 8B C5 0F 57 C9");
                    Memory::VP::ReadCall(pattern.get_first(), lua_pushnumberG);
                    printf("lua_pushnumberG addr %p\n", lua_pushnumberG);
                }

                vint_get_avg_processing_time_hook = safetyhook::create_inline(
                    hook::pattern("48 83 EC ? E8 ? ? ? ? 33 C0 48 83 C4 ? C3 40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B 05").get_first(),
                    vint_get_avg_processing_timeD);

                static auto gds_hook = safetyhook::create_mid(
                    hook::pattern("E8 ? ? ? ? 83 FD ? 0F 87").get_first(),
                    &game_gds_log_option_hook);
            
            };
    }
} InGameConfig;
#endif