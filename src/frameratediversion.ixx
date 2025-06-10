module;
#include "framework.h"
export module frameratediversion;

import general;
export float* frametime;
float frametime_30_divide_frametime() {
    return (1.f / 30.f) / *frametime;
}
static SafetyHookInline disable_FPS_cap;
SAFETYHOOK_NOINLINE void 
#if SRTT
__cdecl
#else SRTTR
__fastcall
#endif
FPSUncapHandler(float min, float max) {

#if SRTT
    disable_FPS_cap.ccall(10.f, 0.0000099999997f);
#else SRTTR
    disable_FPS_cap.fastcall(10.f, 0.0000099999997f);
#endif
}
class frameratediversion {
public:

    frameratediversion() {
        MixFix::onAttach() += [this]() {
            CIniReader ini;
            if (ini.ReadBoolean("Fixes", "FixEngineAudioFPS", true)) {
#if SRTTR
                auto pattern = hook::pattern("F3 0F 11 15 ? ? ? ? F3 0F 5E 15");
#else SRTT
                auto pattern = hook::pattern("D9 05 ? ? ? ? 83 C4 ? D9 1C 24 E8 ? ? ? ? 83 C4 ? E8");
#endif
                if (!pattern.empty()) {
                    uintptr_t instructionAddr = (uintptr_t)pattern.get_first();


                    uintptr_t frameTimeAddr = 0;
                    

                    
                       
#if SRTTR
                    pattern = hook::pattern("48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 83 ? ? ? ? 0F B7 88");
                    Memory::ReadOffsetValue<4>(instructionAddr + 4, frameTimeAddr);
#else SRTT
                    pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 86 ? ? ? ? 0F B7 88");
                    frameTimeAddr = *(uint32_t*)(instructionAddr + 2);
#endif
                    frametime = (float*)(frameTimeAddr);
                    printf("frametime addr %p", frametime);
                    static auto engine_audio_fix = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& ctx) {
#if _WIN64 && SRTTR

                        ctx.rdx = (ctx.rdx & UPPER_32BIT_MASK) | (int)(40 * frametime_30_divide_frametime());
#else SRTT
                        *(int*)(ctx.esp) = (int)(40 * frametime_30_divide_frametime());
                        //printf("SRTT %d \n", *(int*)(ctx.esp));
#endif
                        });
                }
            }

            if (ini.ReadBoolean("Fixes", "UncapFPS", true)) {
#
                auto fps_pattern = get_pattern("F3 0F 10 44 24 ? F3 0F 10 4C 24 ? 0F 5A D0 0F 5A D9 66 0F 2F DA 76 ? F3 0F 11 05","0F 2F C8 76 ? F3 0F 11 05 ? ? ? ? F3 0F 11 0D");
                auto vint_pattern = get_pattern("75 ? A1 ? ? ? ? 8B F0 3B C3", "76 ? 48 8B 05 ? ? ? ? 48 8B D8");
                //Memory::VP::Patch<char>(pattern.get_first(), 0xC3);
                if(!fps_pattern.empty())
                disable_FPS_cap = safetyhook::create_inline(fps_pattern.get_first(), &FPSUncapHandler);
                // vint_document::process_all, if (frametime v1 <= 0.001), fixes menu slow down at FPS above 1000
                if(!vint_pattern.empty())
                Memory::VP::Nop(vint_pattern.get_first(), 2);
                if(disable_FPS_cap.enabled())
                    //FPSUncapHandler(0.f,0.f);
            }

             };
    }
} frameratediversion;