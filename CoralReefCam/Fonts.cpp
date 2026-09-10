#include "Fonts.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

inline std::string ToLowerAscii(const std::string& text)
{
    std::string result = text;
    for (char& c : result)
        c = (char) tolower((unsigned char) c);
    return result;
}

inline bool IsSupportedFontExtension(const std::string& extension)
{
    std::string ext = ToLowerAscii(extension);
    return ext == ".ttf" || ext == ".ttc" || ext == ".otf";
}

inline void FinalizeFontList(std::vector<SystemFont>& fonts)
{
    // Drop duplicates pointing at the same face (the same font may be listed
    // by several sources).
    std::sort(fonts.begin(), fonts.end(), [](const SystemFont& a, const SystemFont& b)
    {
        if (a.file != b.file)
            return a.file < b.file;
        return a.index < b.index;
    });
    fonts.erase(std::unique(fonts.begin(), fonts.end(), [](const SystemFont& a, const SystemFont& b)
    {
        return a.index == b.index && a.file == b.file;
    }), fonts.end());

    std::sort(fonts.begin(), fonts.end(), [](const SystemFont& a, const SystemFont& b)
    {
        std::string la = ToLowerAscii(a.name);
        std::string lb = ToLowerAscii(b.name);
        if (la != lb)
            return la < lb;
        if (a.file != b.file)
            return a.file < b.file;
        return a.index < b.index;
    });
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

static std::string Utf16ToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};
    int size = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int) text.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int) text.size(), result.data(), size, nullptr, nullptr);
    return result;
}

static void EnumerateRegistryFonts(HKEY root, std::vector<SystemFont>& fonts)
{
    HKEY key;
    if (::RegOpenKeyExW(root, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &key) != ERROR_SUCCESS)
        return;

    wchar_t windows_dir[MAX_PATH];
    UINT windows_dir_size = ::GetWindowsDirectoryW(windows_dir, MAX_PATH);
    std::wstring fonts_path = windows_dir_size > 0 && windows_dir_size < MAX_PATH ? windows_dir : L"C:\\Windows";
    fonts_path += L"\\Fonts\\";

    wchar_t name[1024];
    wchar_t data[1024];
    for (DWORD i = 0;; i++)
    {
        DWORD name_size = (DWORD) std::size(name);
        DWORD data_size = sizeof(data);
        DWORD type = 0;
        LSTATUS status = ::RegEnumValueW(key, i, name, &name_size, nullptr, &type, (LPBYTE) data, &data_size);
        if (status != ERROR_SUCCESS)
            break;
        if ((type != REG_SZ && type != REG_EXPAND_SZ) || data_size < sizeof(wchar_t))
            continue;

        std::wstring file(data, data_size / sizeof(wchar_t));
        while (!file.empty() && file.back() == L'\0')
            file.pop_back();
        size_t dot = file.find_last_of(L'.');
        if (dot == std::wstring::npos || !IsSupportedFontExtension(Utf16ToUtf8(file.substr(dot))))
            continue;
        if (file.find(L':') == std::wstring::npos && file.front() != L'\\')
            file = fonts_path + file;

        // Value names look like "Microsoft YaHei & Microsoft YaHei UI (TrueType)".
        std::wstring display = name;
        size_t suffix = display.rfind(L" (");
        if (suffix != std::wstring::npos)
            display.resize(suffix);

        fonts.push_back({Utf16ToUtf8(display), Utf16ToUtf8(file), 0});
    }
    ::RegCloseKey(key);
}

std::vector<SystemFont> EnumerateSystemFonts()
{
    std::vector<SystemFont> fonts;
    EnumerateRegistryFonts(HKEY_LOCAL_MACHINE, fonts);
    EnumerateRegistryFonts(HKEY_CURRENT_USER, fonts);
    FinalizeFontList(fonts);
    return fonts;
}

#elif defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#include <filesystem>

static std::string CfToUtf8(CFStringRef text)
{
    if (!text)
        return {};
    CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(text), kCFStringEncodingUTF8) + 1;
    std::string result(size, '\0');
    if (!CFStringGetCString(text, result.data(), size, kCFStringEncodingUTF8))
        return {};
    result.resize(strlen(result.c_str()));
    return result;
}

std::vector<SystemFont> EnumerateSystemFonts()
{
    std::vector<SystemFont> fonts;
    std::vector<std::filesystem::path> dirs = {
        "/System/Library/Fonts",
        "/Library/Fonts",
    };
    if (const char* home = getenv("HOME"))
        dirs.push_back(std::filesystem::path(home) / "Library" / "Fonts");

    for (const auto& dir : dirs)
    {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
        {
            if (!IsSupportedFontExtension(entry.path().extension().string()))
                continue;
            std::string file = entry.path().string();

            // Enumerate every face in the file (a .ttc may hold several);
            // descriptors come back in face order.
            CFStringRef cf_path = CFStringCreateWithCString(nullptr, file.c_str(), kCFStringEncodingUTF8);
            CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, cf_path, kCFURLPOSIXPathStyle, false);
            CFArrayRef descriptors = CTFontManagerCreateFontDescriptorsFromURL(url);
            if (descriptors)
            {
                for (CFIndex j = 0; j < CFArrayGetCount(descriptors); j++)
                {
                    CTFontDescriptorRef desc = (CTFontDescriptorRef) CFArrayGetValueAtIndex(descriptors, j);
                    CFStringRef family = (CFStringRef) CTFontDescriptorCopyAttribute(desc, kCTFontFamilyNameAttribute);
                    CFStringRef style = (CFStringRef) CTFontDescriptorCopyAttribute(desc, kCTFontStyleNameAttribute);
                    std::string name = CfToUtf8(family);
                    std::string style_name = CfToUtf8(style);
                    if (!style_name.empty())
                    {
                        if (!name.empty())
                            name += ' ';
                        name += style_name;
                    }
                    fonts.push_back({name, file, (int) j});
                    if (style)
                        CFRelease(style);
                    if (family)
                        CFRelease(family);
                }
                CFRelease(descriptors);
            }
            if (url)
                CFRelease(url);
            if (cf_path)
                CFRelease(cf_path);
        }
    }
    FinalizeFontList(fonts);
    return fonts;
}

#elif defined(__linux__) || defined(__unix__)

#include <fontconfig/fontconfig.h>

std::vector<SystemFont> EnumerateSystemFonts()
{
    std::vector<SystemFont> fonts;
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config)
        return fonts;
    FcPattern* pattern = FcPatternCreate();
    FcObjectSet* object_set = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_INDEX, nullptr);
    FcFontSet* list = FcFontList(config, pattern, object_set);
    if (list)
    {
        for (int i = 0; i < list->nfont; i++)
        {
            FcPattern* font = list->fonts[i];
            FcChar8* family = nullptr;
            FcChar8* style = nullptr;
            FcChar8* file = nullptr;
            int index = 0;
            if (FcPatternGetString(font, FC_FAMILY, 0, &family) != FcResultMatch)
                continue;
            if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch)
                continue;
            FcPatternGetString(font, FC_STYLE, 0, &style);
            FcPatternGetInteger(font, FC_INDEX, 0, &index);

            std::string name = (const char*) family;
            std::string style_name = style ? (const char*) style : "";
            if (!style_name.empty() && style_name != "Regular")
            {
                if (!name.empty())
                    name += ' ';
                name += style_name;
            }
            fonts.push_back({name, (const char*) file, index});
        }
        FcFontSetDestroy(list);
    }
    if (object_set)
        FcObjectSetDestroy(object_set);
    if (pattern)
        FcPatternDestroy(pattern);
    FcConfigDestroy(config);
    FcFini();
    FinalizeFontList(fonts);
    return fonts;
}

#else

std::vector<SystemFont> EnumerateSystemFonts()
{
    return {};
}

#endif
