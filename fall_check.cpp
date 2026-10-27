// Swap Force's "stuck falling" check at a frame rate above 30 (games/swapforce/docs/boot.md, "Long falls at 60 fps") [SwapForce].
//
// Every frame a falling player's jump component (sub_82A4A828) compares how far the player moved since the last frame with a
// threshold (the config's "stuck falling distance threshold", times a per-component scale at +104); moving less for long
// enough puts the player back on a safe spot. The threshold is a distance per frame, tuned for the console's 30 fps: at 60
// fps or more a normal fall moves less than it every frame and the player is put back. The threshold is scaled here by
// 30 / fps_limit (at most 1), like a frame at 30 fps, for the call only (the scale is restored after it).
#include "game_hooks.h"
#include "gpu/gpu.h"
#include "memory.h"
#include <algorithm>
#include <cstdio>
#include <ppc_config.h>
#include <ppc_context.h>

#ifdef GAME_HOOK_FallCheck
PPC_FUNC_IMPL(GAME_IMP(FallCheck));
// (jump component) -> void.
PPC_FUNC(GAME_HOOK(FallCheck))
{
    const uint32_t fps = std::max<uint32_t>(gpu::g_settings.fpsLimit, 1);
    const uint32_t component = ctx.r3.u32;
    if (fps <= 30 || component == 0)
    {
        GAME_IMP(FallCheck)(ctx, base);
        return;
    }
    const float scale = g_memory.Read<float>(component + game::FallCheckScaleOffset);
    static bool logged = false;
    if (!logged)
    {
        logged = true;
        fprintf(stderr, "[fall] the stuck-falling threshold is scaled by 30/%u (component scale %g)\n", fps, scale);
    }
    g_memory.Write<float>(component + game::FallCheckScaleOffset, scale * (30.0f / float(fps)));
    GAME_IMP(FallCheck)(ctx, base);
    g_memory.Write<float>(component + game::FallCheckScaleOffset, scale);
}
#endif
