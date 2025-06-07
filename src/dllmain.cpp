// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
import general;
static SafetyHookInline game_init_stage1_hook;
SAFETYHOOK_NOINLINE void game_init_stage1_hookF() {
    game_init_stage1_hook.call();
    //MixFix::onGameInitStage_1().executeAll();
}

SafetyHookInline gameplay_simulate_frame_hook;
void gameplay_simulate_frame_hookF() {
    gameplay_simulate_frame_hook.call();
    //MixFix::onGameplaySimulateFrame().executeAll();
}

void SRTT_services() {
#if SRTT
    auto pattern = hook::pattern("6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC ? A1 ? ? ? ? 33 C4 89 44 24 ? 53 56 57 A1 ? ? ? ? 33 C4 50 8D 44 24 ? 64 A3 ? ? ? ? 68");
    Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
#endif
}
void OpenConsole()
{

    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    SetConsoleTitleA("Debug Console");

}
void start() {
#if _DEBUG
    OpenConsole();
#endif
    //auto pattern = hook::pattern("4C 8B DC 49 89 5B ? 55 49 8D AB");
    //game_init_stage1_hook = safetyhook::create_inline(pattern.get_first<void>(0),&game_init_stage1_hookF);
    CIniReader ini;
#if SRTT
    if (ini.ReadBoolean("Misc", "DisableMySteelportService", true))
    SRTT_services();
#endif
    MixFix::onAttach().executeAll();
    
    auto pattern = hook::pattern("48 89 5C 24 ? 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? E8 ? ? ? ? 0F B6 D8");
    //if (!pattern.empty())
        //gameplay_simulate_frame_hook = safetyhook::create_inline(pattern.get_first(0), &gameplay_simulate_frame_hookF);
        
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        start();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

