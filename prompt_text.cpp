// Swap Force: the title's "press Start" shows a picture of the Start button, like every other prompt (docs:
// games/swapforce/docs/art.md, "Button pictures"). The game's in-text buttons are characters of the font's alternate
// list (the "button" image of proximanova-info / -body), which glyphs::MemoryImage redraws for the device in use; the
// title's prompt (proximanova-info, in ui\title_<lang>.igz and strings\xenonplatformstrings_<lang>.igx, English and
// French, upper case, START its last word) spells the button instead, and the font has no Start character.
// - String_Intern (the string table's intern, r3 the table, r4 the text; returns the shared copy): a short upper-case
//   text ending with the word START is interned with that word replaced by U+0001 (no game text uses a control
//   character but CR and LF).
// - Font_CharMetrics (r3 the font, r4 a u16 character code, r5 a byte set to 1 for an alternate character; returns the
//   character's igCharMetrics or 0): for U+0001 in the info font it answers a metrics block of ours, a copy of the A's
//   moved to an empty part of the button image (x 199-245, y 0-55 of 256x168), where the adapter's Start cell is drawn
//   in every style (games/swapforce/adapter.cpp, InfoButtons). Nothing is cached by the game: the next composed string
//   carries the picture of the style in use.
#include "game_hooks.h"
#include "memory.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <ppc_config.h>
#include <ppc_context.h>

namespace
{
    constexpr std::string_view Word = " START";
    constexpr size_t LongestPrompt = 40;  // the title's prompt is 11 (English) or 17 (French) bytes
    constexpr uint16_t StartCharacter = 1; // what stands for the Start button in the text

    // A short upper-case text (letters, spaces, UTF-8 accents) ending with the word START: a prompt naming the button.
    bool NamesStart(std::string_view text)
    {
        if (text.size() <= Word.size() || text.size() > LongestPrompt || !text.ends_with(Word))
            return false;
        for (char c : text)
            if (!(c == ' ' || (c >= 'A' && c <= 'Z') || static_cast<unsigned char>(c) >= 0x80))
                return false;
        return true;
    }

    // The guest copy of each replacement text (the intern copies it; a few bytes, made once and kept).
    uint32_t GuestText(const std::string& text)
    {
        static std::mutex mutex;
        static std::map<std::string, uint32_t> texts;
        std::lock_guard lock(mutex);
        auto [it, added] = texts.try_emplace(text, 0);
        if (added)
        {
            it->second = g_memory.SystemAlloc(uint32_t(text.size() + 1));
            std::memcpy(g_memory.Translate(it->second), text.c_str(), text.size() + 1);
        }
        return it->second;
    }

    // igBitmapFont: the alternate igCharMetricsList at +0x44 (count +0x08, pointer array +0x14, sorted by character);
    // igCharMetrics (0x30 bytes): character u16 +0x08, then step, offset, size, texture coordinates min (+0x20) and max
    // (+0x28) as fractions of the image. The info font's alternate list has 13 entries, the first three with character 0
    // (keyhole, bottle cap, "!"), the last the A (U+0298). Returns our Start metrics for that font, else 0.
    uint32_t StartMetrics(uint32_t font)
    {
        static std::mutex mutex;
        static std::map<uint32_t, uint32_t> made; // font -> our metrics
        uint32_t list = g_memory.Read<uint32_t>(font + 0x44);
        if (list == 0 || g_memory.Read<uint32_t>(list + 0x08) != 13)
            return 0;
        uint32_t entries = g_memory.Read<uint32_t>(list + 0x14);
        uint32_t first = g_memory.Read<uint32_t>(entries), a = g_memory.Read<uint32_t>(entries + 12 * 4);
        if (first == 0 || a == 0 || g_memory.Read<uint16_t>(first + 0x08) != 0 || g_memory.Read<uint16_t>(a + 0x08) != 0x298)
            return 0;
        std::lock_guard lock(mutex);
        uint32_t& metrics = made[font];
        if (metrics == 0)
        {
            metrics = g_memory.SystemAlloc(0x30);
            std::memcpy(g_memory.Translate(metrics), g_memory.Translate(a), 0x30);
            g_memory.Write<uint16_t>(metrics + 0x08, StartCharacter);
            float shift = 199.0f / 256.0f - g_memory.Read<float>(a + 0x20); // the A sits at x 0
            g_memory.Write<float>(metrics + 0x20, g_memory.Read<float>(a + 0x20) + shift);
            g_memory.Write<float>(metrics + 0x28, g_memory.Read<float>(a + 0x28) + shift);
            fprintf(stderr, "[glyphs] the info font's Start character made at %08X\n", metrics);
        }
        return metrics;
    }
}

#ifdef GAME_HOOK_String_Intern
PPC_FUNC_IMPL(GAME_IMP(String_Intern));
PPC_FUNC(GAME_HOOK(String_Intern))
{
    if (ctx.r4.u32 != 0)
    {
        const char* text = static_cast<const char*>(g_memory.Translate(ctx.r4.u32));
        std::string_view view(text, strnlen(text, LongestPrompt + 1));
        if (NamesStart(view))
        {
            std::string replaced = std::string(view.substr(0, view.size() - Word.size() + 1)) + char(StartCharacter);
            fprintf(stderr, "[glyphs] text \"%s\": START shown as the Start button's picture\n", std::string(view).c_str());
            ctx.r4.u64 = GuestText(replaced);
        }
    }
    GAME_IMP(String_Intern)(ctx, base);
}
#endif

#ifdef GAME_HOOK_Font_CharMetrics
PPC_FUNC_IMPL(GAME_IMP(Font_CharMetrics));
PPC_FUNC(GAME_HOOK(Font_CharMetrics))
{
    if (g_memory.Read<uint16_t>(ctx.r4.u32) == StartCharacter)
    {
        if (uint32_t metrics = StartMetrics(ctx.r3.u32))
        {
            g_memory.Write<uint8_t>(ctx.r5.u32, 1); // an alternate character: drawn from the button image
            ctx.r3.u64 = metrics;
            return;
        }
    }
    GAME_IMP(Font_CharMetrics)(ctx, base);
}
#endif
