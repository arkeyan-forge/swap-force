// Swap Force's ultrawide hooks (d3d/ultrawide.h, docs/runtime/gpu/ultrawide.md "Swap Force"). The scene's size is the
// render-target pass's (the display's size times the pass's scales); the world camera takes the display's aspect from its
// viewport object. The scene's size is widened after the game's own code has run, only for the 16:9 values.
#include "d3d/ultrawide.h"
#include "game_hooks.h"
#include "memory.h"
#include <atomic>
#include <cstdio>
#include <ppc_config.h>
#include <ppc_context.h>

#ifdef GAME_HOOK_Ultrawide_ScenePass
PPC_FUNC_IMPL(GAME_IMP(Ultrawide_ScenePass));
// (r3: a render-target pass object): +88 and +92 are its width and height once computed. The scene's pass is the one that
// comes out 1040x620; its children (half, quarter, the reductions) read these two when they are computed.
PPC_FUNC(GAME_HOOK(Ultrawide_ScenePass))
{
    const uint32_t pass = ctx.r3.u32;
    GAME_IMP(Ultrawide_ScenePass)(ctx, base);
    if (!ultrawide::Active() || pass < 0x10000)
        return;
    if (g_memory.Read<uint32_t>(pass + 88) != ultrawide::SceneWidth16x9 || g_memory.Read<uint32_t>(pass + 92) != ultrawide::SceneHeight16x9)
        return;
    static std::atomic<bool> logged;
    if (!logged.exchange(true))
        fprintf(stderr, "[ultrawide] scene %ux%u -> %ux%u\n", ultrawide::SceneWidth16x9, ultrawide::SceneHeight16x9, ultrawide::SceneWidth(),
            ultrawide::SceneHeight());
    g_memory.Write<uint32_t>(pass + 88, ultrawide::SceneWidth());
    g_memory.Write<uint32_t>(pass + 92, ultrawide::SceneHeight());
}
#endif

#ifdef GAME_HOOK_Ultrawide_WorldCamera
PPC_FUNC_IMPL(GAME_IMP(Ultrawide_WorldCamera));
// (r3: the camera object, r4: its view): builds the world camera's projection. Its viewport object (r3 +44) holds the display's
// aspect at +8; the original reads it from there, so the window's aspect is put there for the call only (the composite quad
// and the HUD cameras read the same field and need the 16:9 one). Hor+, as SSA's and Giants': the vertical field of view stays.
PPC_FUNC(GAME_HOOK(Ultrawide_WorldCamera))
{
    const uint32_t viewport = ultrawide::Active() && ctx.r3.u32 >= 0x10000 ? g_memory.Read<uint32_t>(ctx.r3.u32 + 44) : 0;
    if (viewport < 0x10000)
    {
        GAME_IMP(Ultrawide_WorldCamera)(ctx, base);
        return;
    }
    const float original = g_memory.Read<float>(viewport + 8);
    static std::atomic<bool> logged;
    if (!logged.exchange(true))
        fprintf(stderr, "[ultrawide] camera aspect %.3f -> %.3f\n", original, ultrawide::Aspect());
    g_memory.Write<float>(viewport + 8, ultrawide::Aspect());
    GAME_IMP(Ultrawide_WorldCamera)(ctx, base);
    g_memory.Write<float>(viewport + 8, original);
}
#endif
