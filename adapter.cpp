// Swap Force's adapter: what the runtime asks of the game (runtime/game.h). Its portal prompt is read from the
// CMagicMoment object (swapforce_portal_prompt.h); ultrawide is ultrawide.cpp's; its button glyphs are pictures its CPU text renderer
// copies into strings, redrawn in guest memory (glyphs::MemoryImage; games/swapforce/docs/art.md, "Button pictures").
#include "game.h"
#include "host/portal.h"
#include "memory.h"
#include "swapforce.h"
#include "swapforce_portal_prompt.h"
#include <cstdio>
#include <string_view>

namespace
{
    using namespace portalprompt;

    // Three guest reads a frame, no scan: the instance pointer is a global of the XEX.
    Prompt ReadPortalPrompt()
    {
        if (g_memory.base == nullptr)
            return {};
        uint32_t instance = g_memory.Read<uint32_t>(SwapForceMagicMomentInstance);
        uint32_t state = 0, player = ~0u;
        if (instance >= 0x10000 && uint64_t(instance) + SwapForcePlayerOffset + 4 <= 0xE0000000ull)
        {
            state = g_memory.Read<uint32_t>(instance + SwapForceStateOffset);
            player = g_memory.Read<uint32_t>(instance + SwapForcePlayerOffset);
        }
        static uint32_t lastState = ~0u, lastPlayer = ~0u;
        if (PortalLogEnabled() && (state != lastState || player != lastPlayer))
        {
            uint32_t reason = state != 0 ? g_memory.Read<uint32_t>(instance + SwapForceReasonOffset) : 0;
            fprintf(stderr, "[toybox] portal prompt: magic moment state %u, player %d, reason %u\n", state, int32_t(player), reason);
        }
        lastState = state;
        lastPlayer = player;
        return ClassifySwapForce(state, player);
    }

    // The three start-up logos (\archives\GSBP_ATVI.bik, GSBP_Alchemy.bik, GSBP_VVisions.bk2); the story and level movies
    // (fmv01..14, <zone>_intro, ...) always play.
    constexpr std::string_view IntroMovies[] = { "gsbp_atvi", "gsbp_alchemy", "gsbp_vvisions" };

    // The fonts' "button" images (fonts\\proximanova-body.igz and -info.igz, 32-bit tiled ARGB), found by the XXH3 of their
    // first 4096 bytes; the cells are the buttons' bounding boxes in the stored rows. The info font's holds three more
    // pictures (keyhole, bottle cap, "!") that stay.
    using glyphs::Cell;
    using glyphs::Glyph;
    constexpr Cell BodyButtons[] = { // 256x83
        { Glyph::A, 5, 1, 42, 39 }, { Glyph::Y, 47, 1, 85, 39 }, { Glyph::X, 91, 1, 129, 39 },
        { Glyph::LeftBumper, 138, 8, 201, 35 }, { Glyph::LeftTrigger, 208, 1, 245, 39 }, { Glyph::RightTrigger, 4, 42, 41, 80 },
        { Glyph::RightBumper, 47, 49, 109, 76 }, { Glyph::LeftStickIcon, 117, 43, 154, 80 },
        { Glyph::RightStickIcon, 164, 43, 201, 80 }, { Glyph::B, 206, 42, 244, 80 },
    };
    constexpr Cell InfoButtons[] = { // 256x168
        { Glyph::A, 3, 7, 45, 49 }, { Glyph::B, 51, 6, 92, 49 }, { Glyph::Y, 97, 7, 139, 49 }, { Glyph::X, 144, 7, 186, 49 },
        { Glyph::LeftTrigger, 82, 58, 122, 110 }, { Glyph::RightTrigger, 132, 58, 174, 110 },
        { Glyph::LeftBumper, 5, 71, 74, 101 }, { Glyph::RightBumper, 182, 71, 250, 101 },
        { Glyph::LeftStickIcon, 144, 117, 188, 160 }, { Glyph::RightStickIcon, 201, 117, 245, 160 },
    };
    constexpr glyphs::MemoryImage GlyphImages[] = {
        { 0xC72144049EB62788ull, "body font buttons 256x83", 256, 83, 98304, BodyButtons },
        { 0x7360B650D459A892ull, "info font buttons 256x168", 256, 168, 196608, InfoButtons },
    };

    // A whole texture: the Turbo race HUD's green A (juicestandard_turbo_a_10, 128x128 DXT5, stored upside down), found from
    // the archive's bytes (the hash method of the other textures), not seen in game yet.
    constexpr glyphs::Texture GlyphTextures[] = { { 0x0758AAD79A30A2C8ull, "turbo_a 128", Glyph::A, {}, true } };

    // fullScene: the scene at the frame's size (games/swapforce/ultrawide.cpp: the scene chain's size, its tiling and EDRAM plan).
    const game::Adapter g_adapter = { g_gameInfo, ReadPortalPrompt, true, swapforce::SetVoiceLanguage, GlyphTextures, IntroMovies, false,
        nullptr, {}, GlyphImages, true, { 1280, 720 } };
}

const game::Adapter& game::Current() { return g_adapter; }
