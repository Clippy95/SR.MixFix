#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <stdint.h>
#include <safetyhook.hpp>
#include "Hooking.Patterns.h"
#include "..\include\MemoryMgr.h"
#include "IniReader.h"
#if _WIN64
#define UPPER_32BIT_MASK 0xFFFFFFFF00000000ULL
#endif