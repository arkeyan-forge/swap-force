// Swap Force: the texts that name a pad button by word follow the keyboard (docs: games/swapforce/docs/art.md, "Button
// pictures"). Every other prompt draws its buttons from the fonts' "button" images, which glyphs::MemoryImage redraws;
// one prompt spells its button instead, the title's "press Start" (in ui\title_<lang>.igz and
// strings\xenonplatformstrings_<lang>.igx, English and French, upper case, START its last word). Every string reaches the
// game through its string table, String_Intern (r3: the table, r4: the text; returns the shared copy, found by content or
// copied in). On the keyboard style the hook interns the same text with that last START replaced by the bound key
// ("... ENTER"; "ENTR\u00c9E" and "ESPACE" in French). No game text is stored here. The text is chosen when the strings load
// (at start-up and with the title's screen): a device switch shows from the next load on.
#include "game_hooks.h"
#include "host/glyphs.h"
#include "memory.h"
#include "swapforce.h"
#include <cctype>
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
    constexpr size_t LongestPrompt = 40; // the title's prompt is 11 (English) or 17 (French) bytes

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

    // The key's name as the prompt shows it: upper case; the two usual keys in French for the French text.
    std::string KeyWord(const std::string& label, bool french)
    {
        if (french && label == "Enter")
            return "ENTR\xC3\x89" "E";
        if (french && label == "Space")
            return "ESPACE";
        std::string word = label;
        for (char& c : word)
            c = char(std::toupper(static_cast<unsigned char>(c)));
        return word;
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
}

#ifdef GAME_HOOK_String_Intern
PPC_FUNC_IMPL(GAME_IMP(String_Intern));
PPC_FUNC(GAME_HOOK(String_Intern))
{
    if (ctx.r4.u32 != 0 && glyphs::Current() == glyphs::Style::Keyboard)
    {
        const char* text = static_cast<const char*>(g_memory.Translate(ctx.r4.u32));
        std::string_view view(text, strnlen(text, LongestPrompt + 1));
        if (NamesStart(view))
        {
            std::string replaced = std::string(view.substr(0, view.size() - Word.size() + 1)) +
                KeyWord(glyphs::BoundKey(glyphs::Glyph::Start), swapforce::TextIsFrench());
            fprintf(stderr, "[glyphs] text \"%s\" shown as \"%s\" (keyboard)\n", std::string(view).c_str(), replaced.c_str());
            ctx.r4.u64 = GuestText(replaced);
        }
    }
    GAME_IMP(String_Intern)(ctx, base);
}
#endif
