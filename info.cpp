#include "host/launcher.h"

// Swap Force, for the disc check (host/launcher.cpp). The hashes identify the player's own default.xex; they are not game
// content.
namespace
{
    const GameRelease SwapForceReleases[] =
    {
        // Redump "Skylanders - Swap Force (USA, Australia) (En,Fr)", 8738846720 bytes: the reference disc.
        // SHA-256 of its default.xex (18563072 bytes), computed from the player's image.
        { "USA, Australia", 0x415608E9, 0x22F1FB91, 0x00000021, "5d9a3bc3c2d85ed59684a9c5f94daf6bffcfdfedcef7650a866737edbf3f4ac3" },
    };
}

extern const GameInfo g_gameInfo = { "swapforce", "Skylanders: Swap Force", SwapForceReleases, "SSFRecomp" };
