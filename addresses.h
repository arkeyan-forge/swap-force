// Skylanders: Swap Force: the guest functions the runtime hooks and the guest data it reads. Same names as
// games/ssa/addresses.h and games/giants/addresses.h. It starts empty: no hook exists in a Swap Force build until its
// line is added here, e.g.
//     #define GAME_HOOK_D3DDevice_DrawVertices sub_XXXXXXXX
// A data global is a constexpr in namespace game, 0 while the address is not known. Never reuse another game's address:
// every one is re-found in Swap Force's own binary (docs/porting/day1.md).
#pragma once

// D3D (runtime/d3d/*.cpp): XDK D3D9 2.0.21250 (not LTCG), found with tools/xmatch.py against Giants (`.:giants`) and SSA
// (games/swapforce/docs/boot.md, "Direct3D"). As in Giants: no command buffers, no Create*Shader (the shader Init
// functions instead), no BeginVertices (it exists, sub_82F64CF8, shape 1.00 against SSA's, but only D3D's own Swap
// helper sub_82F5C4B0 calls it).
#define GAME_HOOK_D3DDevice_NextRingSegment sub_82F58760 // same code as Giants' sub_823D0DB8 (asm side by side)
#define GAME_HOOK_D3DDevice_KickOff sub_82F59840         // call graph from VdInitializeRingBuffer's caller (SSA symbols)
#define GAME_HOOK_D3DDevice_BlockOnFence sub_82F58460    // call graph (SSA symbols); takes one more argument (r7) than Giants'
#define GAME_HOOK_D3DDevice_InsertCallback sub_82F58EF0  // shape 1.00 against Giants', same callees
#define GAME_HOOK_D3DDevice_DrawVertices sub_82F651B8    // mapped callees, same size 0x418
#define GAME_HOOK_D3DDevice_DrawIndexedVertices sub_82F655D0 // same size 0x478; base vertex back in r5 (VGT_INDX_OFFSET)
#define GAME_HOOK_D3DDevice_Resolve sub_82F64AB0         // the public Resolve: SSA's arguments, forwarded to the worker sub_82F63958
#define GAME_HOOK_D3DDevice_ClearF_Worker sub_82F626B0   // shape 1.00 against Giants' and SSA's, same size 0x680
#define GAME_HOOK_D3DDevice_Swap sub_82F5D018            // only caller of VdSwap
#define GAME_HOOK_D3DVertexShader_Init sub_82F527F8      // identical to Giants' sub_823DC7E8
#define GAME_HOOK_D3DPixelShader_Init sub_82F51A10       // identical to Giants' sub_823DC5F0

// XMA (runtime/kernel/xma.cpp): the same two engines as SSA and Giants (shape similarity 1.00 each). Unlike Giants, engine
// 2 has its own init again (sub_82F9FA20 reads 0x7FEA1800 like engine 1's; called from the voice set-up sub_8268B9F0,
// Giants' sub_823561C0).
#define GAME_HOOK_Xma1_Init sub_82FAB058
#define GAME_HOOK_Xma2_Init sub_82F9FA20
#define GAME_HOOK_Xma1_VoiceClear sub_82FAB0B8
#define GAME_HOOK_Xma1_VoiceKick sub_82FABA50
#define GAME_HOOK_Xma2_VoiceClear sub_82F9FAF0
#define GAME_HOOK_Xma2_VoiceLock sub_82F9FCF0
#define GAME_HOOK_Xma2_VoiceKick sub_82F9FED8
#define GAME_HOOK_Xma1_LockAll sub_82FAB8E0

// Bink 2 (runtime/d3d/bink_display.cpp): BinkOpen calls the header reader sub_83081CE8 (shape 1.00 against Giants');
// BinkClose sets HBINK+76, then frees (0.94 against Giants', the same first 30 instructions).
#define GAME_HOOK_BinkOpen sub_83083260
#define GAME_HOOK_BinkClose sub_83084128

// SSA_LEVEL (engines/alchemy/zone.cpp): the zone list's lookup by name, (list, const char* name) -> index or -1; it
// hashes the name (sub_8295FE60) and looks it up (sub_82461980). Found from the level loader sub_829EC0F0, which reads
// the zone's archives (the "generated/maps/%s_characters" format) after this lookup.
#define GAME_HOOK_Zone_IndexByName sub_829EA688

// The save tasks (games/swapforce/save_tasks.cpp): the storage object's poll, (storage) -> busy; it finishes a task once
// its worker thread set the done flag (game::SaveTaskDone). Its start, sub_829CF218, clears that flag after starting
// the thread, so a worker that ends first loses it (the stop at the save slots).
#define GAME_HOOK_SaveTask_Poll sub_829CF1C0

// Languages (games/swapforce/language.cpp, boot.md "Text and voice language"): Language_Get is the title's one reader of the
// console language, XGetLanguage mapped to an IG_CORE_LANGUAGE value (6 English, 8 French), returned in r3; Language_SoundPak
// opens one sound package of a sound set ("<set>_<abbreviation>.pak", the abbreviation from that same value); Movie_Open
// starts a movie (r3: the movie; its description at +32, the language's Bink track id at +56 of the description), calling
// BinkSetSoundTrack sub_8307EC40 then BinkOpen. Found by hand (the strings "%s/%s_%s.pak", "LocalizationData.xmlb" and the
// call graph); Giants' counterparts are not in the same place (tools/xmatch.py shape: 0.55 for Movie_Open, none for the rest).
#define GAME_HOOK_Language_Get sub_823EFC68
#define GAME_HOOK_Language_SoundPak sub_825F5B48
#define GAME_HOOK_Movie_Open sub_825A71D0

// Ultrawide (games/swapforce/ultrawide.cpp, docs/runtime/gpu/ultrawide.md "Swap Force"): the scene's render-target pass computes
// its size from the display's (sub_8258EBE8: display 1280x720 times the pass's scales 0.8125 and 0.861 = 1040x620; its
// children, half, quarter and the reductions, read their parent's); the world camera's set-up (sub_82B992A8) reads the
// display's aspect from its viewport object (+8: 16:9). The full-screen composite and the HUD are drawn by cameras that
// read the same aspect and need 16:9, so only the world camera gets the window's. Giants' Ultrawide_* hooks are not in this
// engine version (shape similarity under 0.2); these were found with a store watch (docs/porting/debugging.md).
#define GAME_HOOK_Ultrawide_ScenePass sub_8258EBE8
#define GAME_HOOK_Ultrawide_WorldCamera sub_82B992A8
// The final composite (the scene into the frame) is an indexed quad here (6 indices), not Giants' 4 vertices.
#define GAME_ULTRAWIDE_COMPOSITE_INDEXED

#include <cstdint>
#include <string_view>

namespace game
{
    // D3D: XDK 2.0.21250 (D3D9). The device layout is Giants' (XDK 21076): every offset the runtime reads was checked in
    // Swap Force's own setters and draws (games/swapforce/docs/boot.md, "Direct3D").
    constexpr uint32_t DeviceVertexDeclaration = 0x2FD0; // SetVertexDeclaration sub_82F51E68: stw r4,0x2FD0(r3)
    constexpr bool DrawIndexedBaseVertex = true;         // (device, primitive, base vertex, start, count), as SSA
    constexpr bool ExpandTiledTargets = true;
    constexpr bool ResolveHasLevel = true;               // the public Resolve has SSA's (.., level, slice, color) again
    constexpr uint32_t DeviceStateShift = 0x80;
    constexpr uint32_t LanguageIndex = 0;
    constexpr uint32_t LanguageCodes = 0;
    constexpr uint32_t NglCurrentScene = 0;
    constexpr uint32_t Engine1CriticalSection = 0x831106E4; // Xma1_LockAll enters it first
    constexpr uint32_t DeviceGlobal = 0;
    constexpr uint32_t UltrawideDisplayAspect = 0;
    constexpr uint32_t UltrawideSceneWidth = 1040, UltrawideSceneHeight = 620;
    constexpr bool UltrawideSceneKeepsArea = true;
    constexpr uint32_t UltrawideCompositeVertices = 6;
    constexpr int UltrawideCompositeSlot = 0;
    constexpr bool UltrawideCompositeVpos = false;
    constexpr std::string_view FirstZone = "tutorial\\tutorial_a\\tutorial_a"; // a new game's first zone (SSA_LEVEL)
    // The engine's name table (sub_8294EA00 turns a name handle into its string, sub_82954BA0 returns the table, sub_82954080
    // clears it): a u32 generation, 32768 u32 string offsets, at +0x20004 the u32 end of the strings, at +0x20008 the
    // strings, packed and NUL-terminated. A new game's first zone is the table's last string when it is looked up, so
    // SSA_LEVEL can write a longer zone name there and move the end (engines/alchemy/zone.cpp).
    constexpr uint32_t NameTable = 0x831379A8;
    // The game mode (SSA_LEVEL in a survival arena, engines/alchemy/zone.cpp). The game object is a function-local
    // static (sub_829B05A8 returns it, built before the title); it holds the mode at +1184 (sub_82989648 reads it;
    // SetGameMode sub_829A8928 stores it and tells the mode's systems) and the next zone's mode at +1188 (sub_82989658
    // stores it). Entering a zone (sub_829C6220) keeps the mode at +1180 and calls SetGameMode with the next one.
    // Logged from the menus [V]: Story Mode 1, Arena Modes > Solo Survival 64 (sub_82BDE3E8, from the menu item).
    // A survival arena's objective (sub_82A621F8, the component's start) builds its rules object at +28 only in modes 4,
    // 32, 64, 128 and 256; in Story Mode it stays null and the arena faults at its first wave (sub_82A5D168 reads it).
    constexpr uint32_t GameInstance = 0x83209B18;
    constexpr uint32_t GameModeOffset = 1184, NextGameModeOffset = 1188, GameModeSoloSurvival = 64;
    constexpr std::string_view SurvivalZones = "multiplayer\\survival\\"; // the survival arenas' zone names
    // The save tasks ("SL_LOAD_SLOTS", "SL_SAVE_OPTIONS", ...: sub_829CC7F0 names the thread by task): the storage object
    // has its busy byte at +185 and its igThread at +188 (that thread's "running" byte at +16, set before the thread is
    // resumed and cleared by the thread's trampoline sub_823F3B70 after the worker returns); every worker sets the done
    // byte as its last store (sub_829CAA90 ...).
    constexpr uint32_t SaveTaskDone = 0x8320ABC1;
    constexpr uint32_t SaveTaskBusyOffset = 185, SaveTaskThreadOffset = 188, ThreadRunningOffset = 16;
}
