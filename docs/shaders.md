# Swap Force's shaders: the disc and the cache

> **Purpose:** where Swap Force's shaders are on the disc and how the kit builds the cache · **Status:** current (2026-10-28) · **Tags:** [Xbox] [XDK] [Alchemy] [SwapForce]

## On the disc

### Where the shaders are [SwapForce] [V]
**All of them are in one file: `/archives/shaders_xenon.pak`** (592025 bytes, IGA v10 big-endian, 62 entries): 61 IGZ v7
files, one per shader family (`shaders\characteroutdoor.igz`, `ubershaderoutdoor.igz`, `water.igz`, `fxaa.igz`,
`tiledlight_<n>p.igz`...), plus one `shaders_xenon.pkgb` (a package list, no shader). They hold **1074 distinct
containers (580 vertex, 494 pixel)**. None of the other 2230 `.pak` files of the disc has any (all scanned,
2026-10-27). The game opens it at start, before `permanent_xenon.pak`; like Giants, Alchemy builds the shader objects
itself and calls `D3DVertexShader_Init`/`D3DPixelShader_Init` (both hooked: `games/swapforce/addresses.h`).

Not in the archive: **7 containers compiled into `default.xex`** (their physical parts are in the loaded image, their
virtual parts are assembled at run time, as Giants' Bink ones), named by the debug-database paths next to them:
- 5 from Alchemy's own movie module, `movie/binkMovie` (`binkVertexShader`, `bink1PixelShader`, `bink2PixelShader`,
  `bink1AlphaPixelShader`, `bink2AlphaPixelShader`): `2C7D434B8344C7F7.vs`, `4D79576E97BF424C.vs`,
  `2BE3ED3FB609BD71.ps`, `515F525572999F76.ps`, `811CCF44C1A9FC42.ps` (dump names);
- 2 from Alchemy's `gfx` defaults (`defaultVertexShader`, `defaultPixelShader`): `8D91FA8A6A2C9429.vs`,
  `73A2EEDD9631994F.ps`.

So Swap Force has 7 run-time shaders where Giants has 50 (Giants' come from the Bink 2 library itself) [V for the
dump; whether a movie of another kind adds more is [?]].

### Format facts [Alchemy] [SwapForce] [V]
IGA v10 and the IGZ v7 header: [art.md](art.md#formats-alchemy-swapforce-v). What the shaders add:
- IGZ v7 sections (at 0x14, `{u32 ?, offset, size, alignment}`): section 0 the tagged records; the object pools; the
  last one, 0x20-aligned, the **physical memory** (the microcode).
- The record `TMHN` (stored `NHMT`) lists the memory handles, 8 bytes each: `{u32 kind<<24 | size, u32 pool<<27 |
  offset}`, `pool` n being section n + 1 (v6 put the pool in the top 8 bits). Kind 0x18 = physical memory, as in v6.
- A container's virtual part (`102A1100` pixel / `102A1101` vertex, virtual size, physical size) sits in an object
  pool and its physical part in the physical pool: **not next to each other** (Giants' v6 test "within 0x40 bytes
  after" does not apply). The containers, in file order, and the 0x18 handles, in table order, go together: each
  container takes the next handle of its physical size. On the disc every one of the 1074 finds its handle with the
  same size, in step, and the 549 that a dump also has are byte-identical to it.
- What the game hashes is again `virtual part + physical part`, so the extractor's files are XenosRecomp's input as is.

## How we extract it

### From the disc (the kit's path)
- **Python** (developers): `python3 -I tools/shaders/swapforce_extract.py <iso> private/swapforce/shaders/disc
  [<dump dir>]`: reads `shaders_xenon.pak` (1 s; `ARCHIVES=all` scans all 2231 `.pak` files, 70 s with 4 processes),
  writes `<sha1 prefix>.vs|.ps`, `manifest.txt` and, with a dump dir, `missing.txt`.
- **SkyBuild** (C++, the player kit): `shaders=igz:/archives/shaders_xenon.pak` in `games/swapforce/game.txt`.
  `tools/skybuild/igz_shaders.cpp` reads IGZ v7 too (`ForEachContainerV7`), through the runtime's IGA reader, which
  reads v10 since 2026-10-27 (`runtime/host/art/alchemy_archive.cpp`). It writes the same 1074 containers as the Python
  tool (compared by content). Then, as for Giants, the short headless run dumps what the game builds at run time and
  the cache is rebuilt ([kit_contract.md](https://github.com/arkeyan-forge/forge/blob/main/docs/release/launcher/kit_contract.md)). The two `gfx` defaults come
  at +0 s, four of the five Bink ones with the start-up logos at +1-3 s (`2C7D434B8344C7F7.vs`, `4D79576E97BF424C.vs`,
  `2BE3ED3FB609BD71.ps`, `515F525572999F76.ps`), and **`811CCF44C1A9FC42.ps`** only on the tutorial's loading screen
  (`[render] no translated pixel shader 811CCF44C1A9FC42 (primitive 6)` right after `open \archives\tutorial_a.pak`
  and `fmv01`), after the menus.
- **The movie pass reaches it** [V] (2026-10-27): `games/swapforce/game.txt` gives SkyBuild `movie_portal=emulated`
  (an empty emulated portal), `movie_press=` (A every 2 s from 10 to 110 s: the title, Story Mode, the first NEW slot,
  the difficulty) and `movie_shaders=7` (the run ends once 7 new shaders are dumped, at the latest after 120 s), so
  the installed game has **1081** shaders. Until then the pass ran with `SKY_PORTAL=none` and stopped 8 s after the
  first new shader (1080 shaders): with no portal, Swap Force stops after the title on its own **"PORTAL
  DISCONNECTED"** screen (two `SL_LOAD_SLOTS Thread` lines, then nothing; [status.md](status.md), "Open problems"),
  so presses alone could not reach the loading screen. Headless with the empty emulated portal, the installed game
  dumps `811CCF44C1A9FC42.ps` ~34 s in at a load of 4. If a slow machine does not get there in 120 s, the install
  still ends with the shaders found (a `warning: 7 expected` line) and only that screen's draws are skipped.
- The cache: `SSA_GAME=swapforce SKY_SHADER_OUT=<dir> tools/shaders/build_cache.sh <iso> <containers dir>` writes
  `<dir>/shader_cache.cpp` (input in `<dir>/input`) without touching `private/swapforce/ppc/`. From the 1074 disc
  containers and the 7 dump-only ones: **1081 of 1081 translated, 0 without DXIL** (5 min on an M1 Pro), and the 556
  hashes of the dump cache are all in it.

### Coverage
Against the 556 containers of the run-time dumps (title, menus, tutorial level):

| | |
|---|---|
| found on the disc | **1074** (580 vertex, 494 pixel) |
| of the dump, on the disc (byte-identical) | **549** |
| in the dump, not on the disc | **7** (compiled into the XEX, above) |
| on the disc, never dumped | 525 (levels and effects the runs did not reach) |

### From a run-time dump (the first cache, still useful to find run-time shaders)
As Giants' first cache [V]: `SKY_DUMP_SHADERS=<dir>` on a run through the title and the tutorial level
(`private/swapforce/run.sh`, A presses, `SKY_PORTAL_PLACE=new:9`), then `SSA_GAME=swapforce tools/shaders/build_cache.sh
<disc .iso> <dump dir>` writes `private/swapforce/ppc/shader_cache.cpp`: two runs gave **556 containers, 556 of 556
translated** (~1 min on an M1 Pro); a third run with that cache dumped no new one, and every draw of the title, menus
and tutorial has its shader (`skipped 0 (shader)`).

19 of the 556 use **unnormalized texture coordinates** (the deferred composite, the shadow-mask blur, FXAA): the cache must come from a XenosRecomp with `tfetch2DDenorm` (2026-10-27, [shaders.md](https://github.com/arkeyan-forge/forge/blob/main/docs/runtime/gpu/shaders.md)); an older cache draws the lit scene black.

## The post-processing passes and the blur [SwapForce] [Alchemy]

### The user's report and the cause (2026-10-27) [V]
"A kind of general blur over the whole game" and "rendering problems with the lava at the start". **Both came from the
`anisotropy` setting** (the launcher's "Texture filtering", `--anisotropy`) above 1: the runtime gave that anisotropy to
every sampler with linear filters, also to textures without mipmaps, i.e. the render targets the post passes read at
computed coordinates and the colour tables. The tutorial at `--anisotropy 16` showed the whole 3D view smeared, the lava
sky with green speckled contours and Eruptor green; with the fix (anisotropy only for textures with mipmaps,
`SamplerFor` in `runtime/d3d/render.cpp`, [textures_formats.md](https://github.com/arkeyan-forge/forge/blob/main/docs/runtime/gpu/textures_formats.md)) the same
moment is sharp and orange, and the ground keeps the anisotropic filtering (sand ripples stay sharp in the distance in
`barrens/brn1`). The game itself never asks for anisotropy: the aniso field of its fetch constants is 0 in every draw
of a tutorial frame. Which taps did the damage is inferred from the shaders [?]: the motion blur (`3DF3E533`) samples the
scene at 7 jittered points and the tone map looks up a 256x16 colour table, both without mips. [Xbox] (the same code
served SSA and Giants: any colour ramp they read is fixed the same way.)

Shots (untracked, `private/sfblur/shots/`): `tutorial_t60_aniso16_before.png` / `_after.png` (and `t80`),
`brn1_crop_aniso16_before.png` (Eruptor green) / `_after.png`, and at anisotropy 1 `tutorial_t60_aniso1_main.png`,
`brn1_crop_aniso1.png`. **Check it**: the tutorial recipe of [testing.md](https://github.com/arkeyan-forge/forge/blob/main/docs/getting-started/testing.md) with
`--anisotropy 16` and `SKY_GPU_SNAPSHOT_SECONDS=60,80`: no green, no smear.

### The chain at the end of a level frame (tutorial, `cap` of 2026-10-27, scale 2)
Passes 14-16 of [boot.md](boot.md#the-tutorials-level-frame-the-pass-list-2026-10-27-swapforce-alchemy-v), by pixel
shader (first 8 hex digits of the hash), with the constants the game sets:

| Pass | Pixel shader | What it does |
|---|---|---|
| Bloom | `20FCED23`, `9F067E57`, `EAC5877F` | bright pass and a separable blur, at 520x310 and again at 260x155 (mean value ~3/255 in the tutorial: no haze) |
| Bloom composite | `9B8EE4E4` | scene + 260x155 x 2.467 + 520x310 x 2.051 (c0); writes the **depth of field weight to alpha**: `saturate(abs(linear depth) x 1/3500 - 0.857)` (c1), 0 up to ~3000 units, 1 beyond ~6500 |
| Lava layers | `45500B15` | translucent lava sheets (a 512x128 DXT5 with mips), blended over the scene, depth tested |
| Depth of field | `786D32D7`, `90459BAF`, then `F6354FA3` | the scene halved to 520x620 then 520x310 (taps weighted by their alpha), then `scene + (blurred - scene) x scene.alpha`: only the far background is blurred, by design |
| Heat distortion | `B974E1A3` | particles that read the scene shifted by a DXN normal map |
| Motion blur and tone map | `3DF3E533` (+ `E441188B` under the character's stencil) | 7 taps along a per-object velocity, then the 256x16 colour table. The velocity is in the alpha of two G-buffer targets, packed by pass 6 (`1D4DD81F`: RT0 = normal.a, third.a, depth in z/w; the sky gets 0). Standing still in `brn1`, 98.4 % of the pixels have zero velocity; on the tutorial's flying ship the background streams radially, by design |
| FXAA | `123B15C5` | FXAA 3.11, Xbox 360 variant: c0 = +-0.5/size, c1 = (8, 8, -4, -4)/size, sharpness 8, threshold 0.125, minimum 0.05, luma in alpha (written by the tone map). It changes 1.7 % of the pixels (edges) |
| Frame | `124F67B6` | the 1040x620 scene stretched to 1280x720 with bilinear filtering, then the HUD |

**At anisotropy 1 every pass checked is faithful** [V]: the depth of field's composite copies the scene exactly where its
weight is 0 (error 0.00 over the deck); the unnormalized fetches land on texel centres at scale 2 (a shift of half a
guest texel, as on the console, not a blur); no half-resolution pass is stretched by mistake (no 2x2 blocks in the
G-buffer or the scene). What softness is left is the game's own: the 3D at 1040x620 stretched x1.23 / x1.16 to the
frame (at scale 2 on a 2560x1440 window: 2080 scene pixels over 2560), FXAA, the depth of field on the far background,
the motion blur while the ship flies. Since 2026-10-27 the scene is rendered at the frame's size by default (no stretch; the console's
1040x620 with `original_resolution=1`: [boot.md](boot.md#the-scene-at-full-resolution-2026-10-27-alchemy-swapforce-v)).
The table above is the 1040x620 frame; at full resolution every size is the same fraction of 1280x720 and the game
computes the constants from the passes.

### The lava's smear and the soft backgrounds (2026-10-27) [V]
The user's recording (tutorial, `main` at 1040x620): "the lava bugs a lot", smeared like a strong motion blur; the
Skylander menu's clouds and the backdrops look soft. Checked on frame captures (`SKY_DUMP_VS_CONSTANTS`,
[debugging.md](https://github.com/arkeyan-forge/forge/blob/main/docs/porting/debugging.md)); nothing of ours was found:
- **The velocity is the game's own, per vertex.** The G-buffer materials (vertex shaders `2EED9EE6`, skinned
  `359FC863`) project each vertex with this frame's matrices and with the previous frame's (static meshes: previous
  world-view c14-c17 and projection c18-c21; skinned: previous bones from c90, previous world-view c165-c168), scale
  the difference by c22 / c173 = (-224, 126), clamp it to 7 and write it to two G-buffer alphas; pass 6 (`1D4DD81F`)
  packs them, zero only where nothing was drawn. The depth range does not enter (positions only). In the captured
  frame the previous matrices are the current ones moved by the ship's motion (the deck: 0.1 unit; the bones: under
  1 unit), so the far canyon walls stream radially because the ship flies forward through them: the game's motion
  blur, by design. [?] Whether the console blurs as much cannot be told without a console capture.
- **The backgrounds are the disc's textures.** The pause and Skylander menus draw 256x512, 512x512, 256x144 and
  512x1080 textures without mipmaps, base level, linear filter, no LOD bias, stretched over the screen; the canyon
  walls use 256x256 and 512x512 DXT1 textures with mipmaps (no bias; at scale 2 we pick finer mips than the console),
  then the depth of field and the motion blur soften them. A higher-resolution look needs texture packs
  ([texture_packs.md](https://github.com/arkeyan-forge/forge/blob/main/docs/runtime/gpu/texture_packs.md)) or less post-processing (a product choice).

### The lava and the crystals flicker (2026-10-28): frame pacing, not a draw [SwapForce] [Alchemy] [V]
The user's recording (kit 0.6.0, macOS, game window 1392x840): "the lava is broken, the crystal-like things too, the
picture is epileptic, the lava sparkles at the start". The recording has 44-48 distinct frames a second, so the game ran
at `fps_limit=60` below 60: each frame was shown 1, 2 or 3 refreshes while the scene moved by the same step from one frame
to the next (correlation of step and display time 0.0 over 10 s). That judder, on the two fastest scenes of the
tutorial (the ship flies through the lava canyon, then through a tunnel of cyan shards at ~265 units a frame), is the
"epileptic" picture. The fix is in the runtime's pacing: at 60 the frames are shown every 2 blanks while they miss their
blank ([video_vblank.md](https://github.com/arkeyan-forge/forge/blob/main/docs/runtime/xbox360/video_vblank.md#pacing-waitforflippresentinterval-in-the-swap-hook)).
Headless at the user's settings the present intervals went from median 38.9 ms (std 7.2) to 33.3 ms (std 3.9).

What was checked and found right, frame by frame (`SKY_DUMP_FRAME` with `SKY_DUMP_FRAMES=<n>`, consecutive snapshots at
30 fps, where each present is one game step):
- **The canyon** (G-buffer, vertex shader `9183BAD7`, pixel shader `35F4F3BF`, 1024/2048 textures): its wobble is
  `sin(frequency x time + phase)` with the material time in c3.x, which advances by the measured frame time (one frame
  behind the present, as the render thread is); c3.y is the time two frames back. Its albedo changes little between
  frames; the flow overlays (`1A20887C` with `C3EFBD6E` / `614DDFBC`) have static texture matrices.
- **The lava sheets** (`45500B15`): the scroll is c8.x = time x 0.01, smooth. **The 256x256 ping-pong** (pass 2 of
  [boot.md](boot.md#the-tutorials-level-frame-the-pass-list-2026-10-27-swapforce-alchemy-v)) is a ripple simulation whose
  normal map feeds the overlays (texture 5); it is flat in the tutorial (127, 127, 255).
- **The tunnel and the swirl sky** are skinned meshes (`4292A997`, `E5509D27`, bones from c8 / c17) drawn in the forward
  pass, after the G-buffer: their bones change smoothly. They have **no motion blur by design**: the velocity the blur
  reads is packed in pass 6 from the G-buffer, where the sky is 0. So at 30 fps two frames in the tunnel differ by ~45 of
  255 on average, on the console as here [?] (no console capture).
- **The motion blur's inputs**: the previous-frame matrices of a frame are exactly the current ones of the frame before.
- No out-of-order or alternating frames (frame i+2 always further from i than i+1), no 2x2-quad pattern (the ratio of the
  luma step across quad edges to the step inside a quad is 0.94, as in a normal picture), no excess of per-pixel flicker
  over the lava (2.4 % of the pixels a frame, the same at 30 and 60 fps).
- One translation gap found on the way, without a visible effect: [`SetTextureLod`](https://github.com/arkeyan-forge/forge/blob/main/docs/runtime/gpu/shaders.md).

**At 30 fps** (2026-10-28, after the user said the game was set to 30) [V]: the same places at `--fps-limit 30`, scale 1,
every present saved (each one a game step of 1/30 s): the canyon and the tunnel draw as above, with no flicker beyond the
scene's own motion (`c30_lava_zoom.png`, four consecutive canyon frames; `seq_vtx_shards.png`, three consecutive tunnel
frames). Also checked: the literal constants of the canyon and tunnel vertex shaders (`c253` = (-pi, 1/14, 2 pi, 0) for
the sine range reduction, `c255.x` = 3 for the bone stride) are in the constant buffer at their draws, as the translated
code reads them (`g_C(253)`, `g_C(255)`); the post passes' constants follow the full-resolution scene (1280x720: bloom at
640x360, offsets 1/1280) as they follow 1040x620 with `original_resolution=1`. **The recording cannot come from a game
at 30**: at `fps_limit=30` Swap Force keeps its interval 2 (`[video] present interval 0x2 seen (fps limit 30)`), so at
most 30 different pictures a second (presents 33.3 ms apart headless), and the recording has 44-47 (`mpdecimate`), in
equal steps of about 1/60 s of game time. So the game the user saw ran at 60: the launcher's 30 did not reach it [?]
(the game reads `fps_limit` only from `launcher.ini`'s `[video]`, which Skylauncher fills from the game's
`[swapforce.video]` before a launch; the game log's `[settings] ... fps limit N` line says which value it used).

Shots and data (untracked, `private/sflava/`): `seq_vtx_shards.png` (three consecutive tunnel frames), `seq_f60s1_crop.png`
and `z_seq30_1100.png` (consecutive lava frames), `dump6_alb.png` / `dump6_fin.png` (albedo and final over 4 frames),
`capt_mb.png` (tunnel frame before and after the motion blur, the packed G-buffer), `runs/pace_before` and
`runs/pace_after3` (`frames.txt`: the present times before and after).

## Tests
- The extraction: `python3 -I tools/shaders/swapforce_extract.py <iso> <out> private/swapforce/shaders/all` prints
  `1074 distinct (580 vertex)` and `found by the extractor: 549; missing: 7`. SkyBuild's C++ extractor on the same
  archive writes the same 1074 files (by content), and on Giants' `global.bld` the same 4394 files as before the v10/v7
  change (byte-identical set, 2026-10-27). `SSAArtTest` reads a synthetic IGA v10 fixture (big-endian, 16-byte entries,
  0x800-aligned chunks).
- Not yet: a game run on the disc-made cache (`private/swapforce/shaders/cache_disc/`, 1081 shaders).
- The frame snapshots of the runs (`SKY_GPU_SNAPSHOT`): the title screen, menus, loading screen, the HUD and, since 2026-10-27, the tutorial's 3D scene draw like the game ([boot.md](boot.md), "What broke on the way to a level").

## Is it engine-generic?
The dump path is: any game whose shader Init or Create functions are hooked. The disc path is per archive format:
Giants IGA v8 / IGZ v6 (`global.bld`), Swap Force IGA v10 / IGZ v7 (`shaders_xenon.pak`). The idea carries over
[Alchemy]: one archive of shader IGZ files, containers found by their D3D header, the physical part found through the
memory-handle table. The "in order, same size" pairing is checked on Swap Force only; Trap Team and later (IGA v11,
IGZ v8+) have to be checked again.
