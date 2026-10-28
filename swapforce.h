#pragma once

#include <string_view>

// Swap Force's own code, shared by the files of games/swapforce/.
namespace swapforce
{
    // Voices in another language than the text (language.cpp): "en" or "fr", the two the disc has.
    bool SetVoiceLanguage(std::string_view code);
    // Whether the text is French, as the title's last Language_Get answered (language.cpp; English until it asks).
    bool TextIsFrench();
}
