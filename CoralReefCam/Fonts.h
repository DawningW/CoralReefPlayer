#pragma once

#include <string>
#include <vector>

struct SystemFont
{
    std::string name;   // UTF-8 display name, e.g. "Microsoft YaHei" or "Noto Sans CJK SC Regular"
    std::string file;   // UTF-8 path to the font file
    int index;          // Face index within a .ttc collection, 0 for plain .ttf/.otf files
};

// Enumerate fonts installed on the system. The list is sorted by name and
// deduplicated. Returns an empty list when enumeration is unavailable
// (e.g. Emscripten).
std::vector<SystemFont> EnumerateSystemFonts();
