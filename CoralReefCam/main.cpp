#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <emscripten/websocket.h>
#include <emscripten/posix_socket.h>
#ifndef __EMSCRIPTEN_PTHREADS__
extern "C"
{
#include "emft-pthread.h"
}
#include "emscripten_browser_clipboard.h"
#endif
#endif
#include "SDL.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "implot.h"
#if ENABLE_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#endif
#include "coralreefplayer.h"
#include "optparse.h"
#include "Fonts.h"

#define IDLE_FPS 25
#define SDL_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SDL_REPLAY_EVENT (SDL_USEREVENT + 2)

std::string url;
Option option;
crp_handle player;
bool playing;
uint64_t pts;
SDL_TimerID timer;
std::vector<SystemFont> system_fonts;
std::vector<ImFont*> system_fonts_loaded;
float ui_font_size = 16.0f;
std::string ui_font_file;
int ui_font_index = 0;

#ifdef __EMSCRIPTEN__
static std::function<void()> EmscriptenMainLoopFunc;
static void EmscriptenMainLoop() { EmscriptenMainLoopFunc(); }
#endif

SDL_PixelFormatEnum GetPixelFormat(Format format)
{
    switch (format)
    {
        case CRP_YUV420P:
            return SDL_PIXELFORMAT_IYUV;
        case CRP_NV12:
            return SDL_PIXELFORMAT_NV12;
        case CRP_NV21:
            return SDL_PIXELFORMAT_NV21;
        case CRP_RGB24:
            return SDL_PIXELFORMAT_RGB24;
        case CRP_BGR24:
            return SDL_PIXELFORMAT_BGR24;
        case CRP_ARGB32:
            return SDL_PIXELFORMAT_ARGB8888;
        case CRP_RGBA32:
            return SDL_PIXELFORMAT_RGBA8888;
        case CRP_ABGR32:
            return SDL_PIXELFORMAT_ABGR8888;
        case CRP_BGRA32:
            return SDL_PIXELFORMAT_BGRA8888;
    }
    return SDL_PIXELFORMAT_UNKNOWN;
}

SDL_AudioFormat GetAudioFormat(Format format)
{
    switch (format)
    {
        case CRP_U8:
            return AUDIO_U8;
        case CRP_S16:
            return AUDIO_S16SYS;
        case CRP_S32:
            return AUDIO_S32SYS;
        case CRP_F32:
            return AUDIO_F32SYS;
    }
    return AUDIO_S16SYS;
}

enum WindowLocation
{
    Center = -1,
    TopLeft = 0,
    TopRight = 1,
    BottomLeft = 2,
    BottomRight = 3
};

void SetNextWindowLoc(WindowLocation location, ImGuiCond cond)
{
    static const float MARGIN = 8.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (location == -1)
    {
        ImGui::SetNextWindowPos(viewport->GetCenter(), cond, ImVec2(0.5f, 0.5f));
    }
    else
    {
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - MARGIN) : (work_pos.x + MARGIN);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - MARGIN) : (work_pos.y + 24 + MARGIN);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, cond, window_pos_pivot);
    }
}

bool BeginOverlay(const char* name, WindowLocation location, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
{
    static const float ALPHA = 0.35f;

    SetNextWindowLoc(location, ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(ALPHA);

    ImGuiWindowFlags window_flags = flags | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    return ImGui::Begin(name, p_open, window_flags);
}

static std::string GetFontDisplayName(const SystemFont& font)
{
    // The current font may be unable to render a non-ASCII name yet, so also
    // show the file name, which always is ASCII.
    bool non_ascii = false;
    for (unsigned char c : font.name)
        if (c >= 0x80) { non_ascii = true; break; }
    std::string file = std::filesystem::path(font.file).filename().string();
    if (non_ascii && !file.empty())
        return font.name + " (" + file + ")";
    return font.name;
}

static ImFont* LoadSystemFont(size_t index)
{
    if (system_fonts_loaded[index])
        return system_fonts_loaded[index];

    const SystemFont& system_font = system_fonts[index];
    ImFontConfig config;
    config.FontNo = system_font.index;
    config.Flags |= ImFontFlags_NoLoadError;
    std::string name = GetFontDisplayName(system_font);
    size_t len = std::min(name.size(), sizeof(config.Name) - 1);
    while (len > 0 && ((unsigned char) name[len] & 0xC0) == 0x80)  // keep whole UTF-8 sequences
        len--;
    memcpy(config.Name, name.data(), len);
    config.Name[len] = '\0';
    system_fonts_loaded[index] = ImGui::GetIO().Fonts->AddFontFromFileTTF(system_font.file.c_str(), 16.0f, &config);
    return system_fonts_loaded[index];
}

static void SelectSystemFont(size_t index)
{
    ImFont* font = LoadSystemFont(index);
    if (font)
    {
        ImGui::GetIO().FontDefault = font;
        ui_font_file = system_fonts[index].file;
        ui_font_index = system_fonts[index].index;
        ImGui::MarkIniSettingsDirty();
    }
}

// Font/size settings persistence, stored in ImGui's own imgui.ini through a
// custom settings handler:
//
// [UI][Settings]
// FontSize=25
// FontFile=C:\Windows\Fonts\msyh.ttc
// FontIndex=0

static void UiSettingsWriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    buf->appendf("[%s][Settings]\n", handler->TypeName);
    buf->appendf("FontSize=%.0f\n", ui_font_size);
    if (!ui_font_file.empty())
    {
        buf->appendf("FontFile=%s\n", ui_font_file.c_str());
        buf->appendf("FontIndex=%d\n", ui_font_index);
    }
    buf->append("\n");
}

static void* UiSettingsReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
{
    return strcmp(name, "Settings") == 0 ? (void*) 1 : nullptr;
}

static void UiSettingsReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
{
    float size = 0;
    int index = 0;
    if (sscanf(line, "FontSize=%f", &size) == 1)
        ui_font_size = size < 10.0f ? 10.0f : (size > 40.0f ? 40.0f : size);
    else if (sscanf(line, "FontIndex=%d", &index) == 1)
        ui_font_index = index;
    else if (strncmp(line, "FontFile=", 9) == 0)
        ui_font_file = line + 9;
}

static void UiSettingsApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler)
{
    // Runs during the first NewFrame, before the font size is resolved and the
    // default font is picked, so both settings apply from the very first frame.
    ImGuiStyle& style = ImGui::GetStyle();
    style.FontSizeBase = ui_font_size;
    style._NextFrameFontSizeBase = ui_font_size;
    if (!ui_font_file.empty())
    {
        for (size_t i = 0; i < system_fonts.size(); i++)
        {
            if (system_fonts[i].file == ui_font_file && system_fonts[i].index == ui_font_index)
            {
                ImFont* font = LoadSystemFont(i);
                if (font)
                    ImGui::GetIO().FontDefault = font;
                break;
            }
        }
    }
}

static void RegisterUiSettings()
{
    ImGuiSettingsHandler handler;
    handler.TypeName = "UI";
    handler.TypeHash = ImHashStr("UI");
    handler.ReadOpenFn = UiSettingsReadOpen;
    handler.ReadLineFn = UiSettingsReadLine;
    handler.ApplyAllFn = UiSettingsApplyAll;
    handler.WriteAllFn = UiSettingsWriteAll;
    ImGui::AddSettingsHandler(&handler);
}

void play()
{
    if (playing)
        crp_stop(player);
    SDL_FlushEvent(SDL_REFRESH_EVENT);
    crp_play(player, url.c_str(), &option, [](int ev, void* data, void* userdata)
        {
            if (ev == CRP_EV_NEW_FRAME || ev == CRP_EV_NEW_AUDIO)
            {
                SDL_Event event = {};
                event.type = SDL_REFRESH_EVENT;
                event.user.data1 = (void*) (ev == CRP_EV_NEW_AUDIO);
                event.user.data2 = data;
                SDL_PushEvent(&event);
            }
            else if (ev == CRP_EV_ERROR)
            {
                printf("An error has occurred, will reconnect after 5 seconds\n");
                timer = SDL_AddTimer(5000, [](Uint32 interval, void* param)
                    {
                        SDL_Event event = {};
                        event.type = SDL_REPLAY_EVENT;
                        SDL_PushEvent(&event);
                        return 0U;
                    }, NULL);
            }
            else if (ev == CRP_EV_START)
            {
                playing = true;
            }
            else if (ev == CRP_EV_PLAYING)
            {
                printf("Playing stream\n");
            }
            else if (ev == CRP_EV_END)
            {
                printf("The stream has reached its end\n");
                SDL_Event event = {};
                event.type = SDL_QUIT;
                SDL_PushEvent(&event);
            }
            else if (ev == CRP_EV_STOP)
            {
                playing = false;
            }
            else if (ev == CRP_EV_VIDEO_EXTRADATA || ev == CRP_EV_AUDIO_EXTRADATA)
            {
                int size = ((const EventData*) data)->extra_data.size;
                printf("Received %s extradata, size: %d bytes\n", ev == CRP_EV_VIDEO_EXTRADATA ? "video" : "audio", size);
            }
        }, nullptr);
}

void loop(SDL_Window* window)
{
    static bool show_imgui_demo_window = false;
    static bool show_implot_demo_window = false;
    static bool show_fps_window = true;
    bool open_play_window = false;
    bool open_options_window = false;
    bool open_about_window = false;
    ImGuiIO& io = ImGui::GetIO();
    ImVec4 color;

    ImGui::NewFrame();

    color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    color.w = 0.35f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
    color = ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg];
    color.w = 0.0f;
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, color);
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Media"))
        {
            if (ImGui::MenuItem("Play", "Ctrl+O"))
                open_play_window = true;
            if (ImGui::MenuItem("Stop", NULL, false, playing))
            {
                if (timer != 0)
                {
                    SDL_RemoveTimer(timer);
                    timer = 0;
                }
                crp_stop(player);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDL_Event event = {};
                event.type = SDL_QUIT;
                SDL_PushEvent(&event);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Advance"))
        {
            if (ImGui::MenuItem("Options"))
                open_options_window = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::SeparatorText("Panels");
            ImGui::MenuItem("ImGui Demo", NULL, &show_imgui_demo_window);
            ImGui::MenuItem("ImPlot Demo", NULL, &show_implot_demo_window);
            ImGui::MenuItem("Frame Info", NULL, &show_fps_window);
            ImGui::Separator();
            bool isFullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
            if (ImGui::MenuItem("Fullscreen", "F11", isFullscreen))
            {
                SDL_SetWindowFullscreen(window, isFullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Help", "F1")) {}
            if (ImGui::MenuItem("About"))
                open_about_window = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleColor(2);

    if (open_play_window)
        ImGui::OpenPopup("Play");
    SetNextWindowLoc(Center, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Play", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Stream URL", &url);
        ImGui::Text("Transport: ");
        ImGui::SameLine();
        ImGui::RadioButton("UDP", (int*) &option.transport, CRP_UDP);
        ImGui::SameLine();
        ImGui::RadioButton("TCP", (int*) &option.transport, CRP_TCP);
        ImGui::BeginDisabled(url.empty());
        if (ImGui::Button("Play", ImVec2(120, 0)))
        {
            play();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (open_options_window)
        ImGui::OpenPopup("Options");
    bool options_window_opened = true;
    SetNextWindowLoc(Center, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Options", &options_window_opened, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::BeginTabBar("OptionsTabBar", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("UI"))
            {
                ImGuiIO& io = ImGui::GetIO();
                ImFont* font_embedded = io.Fonts->Fonts[0];
                ImFont* font_current = io.FontDefault ? io.FontDefault : font_embedded;
                if (ImGui::BeginCombo("Font", font_current == font_embedded ? "Default" : font_current->GetDebugName()))
                {
                    if (ImGui::Selectable("Default", font_current == font_embedded))
                    {
                        io.FontDefault = font_embedded;
                        ui_font_file.clear();
                        ui_font_index = 0;
                        ImGui::MarkIniSettingsDirty();
                    }
                    for (size_t i = 0; i < system_fonts.size(); i++)
                    {
                        std::string label = GetFontDisplayName(system_fonts[i]);
                        ImGui::PushID((int) i);
                        if (ImGui::Selectable(label.c_str(), font_current == system_fonts_loaded[i]))
                            SelectSystemFont(i);
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                // style.FontSizeBase is restored from the font stack by Begin()/End()
                // throughout the frame, so a plain write is reverted before it reaches
                // the next NewFrame. Mirror the upstream demo: on change, also queue
                // the value to be re-applied at the next NewFrame.
                if (ImGui::SliderFloat("Size", &ui_font_size, 10.0f, 40.0f, "%.0f px"))
                {
                    ImGuiStyle& style = ImGui::GetStyle();
                    style.FontSizeBase = ui_font_size;
                    style._NextFrameFontSizeBase = ui_font_size;
                    ImGui::MarkIniSettingsDirty();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndPopup();
    }

    if (show_imgui_demo_window)
        ImGui::ShowDemoWindow(&show_imgui_demo_window);

    if (show_implot_demo_window)
    {
        if (ImGui::Begin("Charts", &show_implot_demo_window))
        {
            if (ImPlot::BeginPlot("Example Plot"))
            {
                static double values[] = { 1., 3., 5., 3., 1. };
                ImPlot::PlotLine("Values", values, 5);
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
    }

    if (show_fps_window)
    {
        // ImGuiWindowFlags_NoMove
        if (BeginOverlay("Top-Left Overlay", TopLeft, &show_fps_window))
        {
            ImGui::Text("Frame Info");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("PTS: %lu", pts);
        }
        ImGui::End();
    }

    if (open_about_window)
        ImGui::OpenPopup("About");
    SetNextWindowLoc(Center, ImGuiCond_Appearing);
    if (ImGui::BeginPopup("About", ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("CoralReefCam");
        ImGui::Text("Powered by CoralReefPlayer %s", crp_version_str());
        ImGui::Text("Copyright (c) 2023-2025 OurEDA");
        ImGui::EndPopup();
    }

    ImGui::Render();
}

#if ENABLE_OPENCV
void process(cv::Mat &mat)
{
    cv::rectangle(mat, cv::Rect(100, 200, 100, 100), cv::Scalar(0, 0, 255), 2);
    cv::putText(mat, "Hello OpenCV!", cv::Point(100, 400), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
}
#endif

static void parse_args(int argc, char* argv[], bool& fullscreen)
{
    const struct optparse_long longopts[] =
    {
        {"transport", 't', OPTPARSE_REQUIRED},
        {"videoformat", 'v', OPTPARSE_REQUIRED},
        {"size", 's', OPTPARSE_REQUIRED},
        {"hwdevice", 'd', OPTPARSE_REQUIRED},
        {"audioformat", 'a', OPTPARSE_REQUIRED},
        {"samplerate", 'r', OPTPARSE_REQUIRED},
        {"channels", 'c', OPTPARSE_REQUIRED},
        {"fullscreen", 'f', OPTPARSE_NONE},
        {"version", 0, OPTPARSE_NONE},
        {"help", 'h', OPTPARSE_NONE},
        {0}
    };
    const struct {
        const char* name;
        Format format;
    } video_formats[] = {
        {"yuv420", CRP_YUV420P},
        {"nv12", CRP_NV12},
        {"nv21", CRP_NV21},
        {"rgb24", CRP_RGB24},
        {"bgr24", CRP_BGR24},
        {"argb32", CRP_ARGB32},
        {"rgba32", CRP_RGBA32},
        {"abgr32", CRP_ABGR32},
        {"bgra32", CRP_BGRA32},
    }, audio_formats[] = {
        {"u8", CRP_U8},
        {"s16", CRP_S16},
        {"s32", CRP_S32},
        {"f32", CRP_F32},
    };

    struct optparse parser;
    optparse_init(&parser, argv);

    int opt, index;
    while ((opt = optparse_long(&parser, longopts, &index)) != -1)
    {
        switch (opt)
        {
            case 't':
                option.transport = strcmp(parser.optarg, "tcp") == 0 ? CRP_TCP : CRP_UDP;
                break;
            case 'v':
                for (auto& format : video_formats)
                {
                    if (strcmp(parser.optarg, format.name) == 0)
                    {
                        option.video.format = format.format;
                        break;
                    }
                }
                break;
            case 's':
                sscanf(parser.optarg, "%dx%d", &option.video.width, &option.video.height);
                break;
            case 'd':
                strncpy(option.video.hw_device, parser.optarg, sizeof(option.video.hw_device));
                option.video.hw_device[sizeof(option.video.hw_device) - 1] = '\0';
                break;
            case 'a':
                option.enable_audio = strcmp(parser.optarg, "none") != 0;
                for (auto& format : audio_formats)
                {
                    if (strcmp(parser.optarg, format.name) == 0)
                    {
                        option.audio.format = format.format;
                        break;
                    }
                }
                break;
            case 'r':
                option.audio.sample_rate = atoi(parser.optarg);
                break;
            case 'c':
                option.audio.channels = atoi(parser.optarg);
                break;
            case 'f':
                fullscreen = true;
                break;
            case 'h':
                printf("Usage: %s [OPTION]... [URL]\n", argv[0]);
                printf("Options:\n");
                printf("  -t, --transport=TRANSPORT       The transport of the stream, either tcp or udp\n");
                printf("  -v, --videoformat=FORMAT        The video format of the video, can be:yuv420,nv12,nv21,rgb24,bgr24,argb32,rgba32,abgr32,bgra32\n");
                printf("  -s, --size=WIDTHxHEIGHT         The width and height of the video\n");
                printf("  -d, --hwdevice=TYPE[:DEVICE]    The hardware device for decoding, can be:qsv,cuvid,cuda,dxva2,d3d11va,vaapi,vdpau,v4l2m2m,drm,vulkan\n");
                printf("                                  Use 'ffmpeg -decoders' and 'ffmpeg -hwaccels' to see all supported devices\n");
                printf("                                  See https://trac.ffmpeg.org/wiki/HWAccelIntro for more information\n");
                printf("                                  Available devices for embedded platforms:\n");
                printf("                                    Android: mediacodec\n");
                printf("                                    macOS and iOS: videotoolbox\n");
                printf("                                    Raspberry Pi: mmal,v4lm2m,drm:/dev/dri/renderD128\n");
                printf("                                    Rockchip: rkmpp\n");
                printf("                                    Allwinner H6: drm:/dev/dri/renderD128 (with the ffmpeg patch from LibreELEC)\n");
                printf("  -a, --audioformat=FORMAT        The audio format of the audio, can be:none,u8,s16,s32,f32\n");
                printf("  -r, --samplerate=RATE           The sample rate of the audio\n");
                printf("  -c, --channels=CHANNELS         The channels of the audio\n");
                printf("  -f, --fullscreen                Start in fullscreen mode\n");
                printf("      --version                   Show version infomation and exit\n");
                printf("  -h, --help                      Show this help and exit\n");
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "%s: %s\n", argv[0], parser.errmsg);
                exit(EXIT_FAILURE);
            default:
                if (strcmp(longopts[index].longname, "version") == 0)
                {
                    printf("CoralReefCam %s\n", crp_version_str());
                    exit(EXIT_SUCCESS);
                }
        }
    }

    char* arg;
    if ((arg = optparse_arg(&parser)))
        url = arg;
}

#undef main
extern "C"
int main(int argc, char* argv[])
{
    option.transport = CRP_UDP;
    option.video.format = ENABLE_OPENCV ? CRP_BGR24 : CRP_YUV420P;
    option.video.width = 1280;
    option.video.height = 720;
    strcpy(option.video.hw_device, "");
    option.enable_audio = true;
    option.audio.format = CRP_S16;
    option.audio.sample_rate = 44100;
    option.audio.channels = 2;
    bool fullscreen = false;
    parse_args(argc, argv, fullscreen);
    int width = option.video.width;
    int height = option.video.height;
    if (width == 0 && height == 0)
    {
        width = 1920;
        height = 1080;
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBSOCKET_T bridgeSocket = emscripten_init_websocket_to_posix_socket_bridge("ws://localhost:80");
    uint16_t readyState = 0;
#endif

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER))
    {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
    if (fullscreen)
        window_flags = (SDL_WindowFlags) (window_flags | SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Window* window = SDL_CreateWindow("CoralReefCam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, window_flags);
    if (!window)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        printf("Could not create renderer: %s\n", SDL_GetError());
        return -1;
    }
    Uint32 format = GetPixelFormat((Format) option.video.format);
    SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture)
    {
        printf("Could not create texture: %s\n", SDL_GetError());
        return -1;
    }
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = option.audio.sample_rate;
    wanted_spec.format = GetAudioFormat((Format) option.audio.format);
    wanted_spec.channels = option.audio.channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = NULL;
    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, NULL, 0);
    if (device_id < 2)
    {
        printf("Could not open audio: %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudioDevice(device_id, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImFontConfig font_config;
    font_config.SizePixels = 16.0f;
    io.Fonts->AddFontDefault(&font_config);
    system_fonts = EnumerateSystemFonts();
    system_fonts_loaded.assign(system_fonts.size(), nullptr);
    RegisterUiSettings();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
#ifdef __EMSCRIPTEN__
    static std::string clipboardContent;
    static bool simulatedPaste = false;
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    pio.Platform_SetClipboardTextFn = [](ImGuiContext* ctx, const char* text)
    {
        clipboardContent = text;
        emscripten_browser_clipboard::copy(clipboardContent);
    };
    pio.Platform_GetClipboardTextFn = [](ImGuiContext* ctx)
    {
        return clipboardContent.c_str();
    };
    EM_ASM({
        // https://github.com/pthom/hello_imgui/issues/3
        window.addEventListener('keydown', function(event) {
            if (event.ctrlKey && event.key == 'v')    
                event.stopImmediatePropagation();
        }, true);
    });
    emscripten_browser_clipboard::paste([](std::string&& paste_data, void* callback_data)
    {
        clipboardContent = std::move(paste_data);
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true);
		ImGui::GetIO().AddKeyEvent(ImGuiKey_V, true);
		simulatedPaste = true;
    });
#endif

    player = crp_create();
    if (!url.empty())
        play();

    bool is_running = true;
    bool has_frame = false;
    Uint64 last_time = SDL_GetTicks64();
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EmscriptenMainLoopFunc = [&]() { do
#else
    while (is_running)
#endif
    {
#ifdef __EMSCRIPTEN__
        if (readyState == 0)
        {
            emscripten_websocket_get_ready_state(bridgeSocket, &readyState);
            if (readyState == 0) continue;
        }
#ifndef __EMSCRIPTEN_PTHREADS__
        emfiber_pthread_yield();
#endif
#endif

        bool force_update = false;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_REFRESH_EVENT)
            {
                if (!playing)
                    continue;

                bool is_audio = (bool) event.user.data1;
                Frame* frame = (Frame*) event.user.data2;
                if (!is_audio)
                {
                    if (format == SDL_PIXELFORMAT_IYUV)
                    {
                        SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->stride[0],
                            frame->data[1], frame->stride[1], frame->data[2], frame->stride[2]);
                    }
                    else if (format == SDL_PIXELFORMAT_NV12 || format == SDL_PIXELFORMAT_NV21)
                    {
                        SDL_UpdateNVTexture(texture, NULL, frame->data[0], frame->stride[0],
                            frame->data[1], frame->stride[1]);
                    }
                    else
                    {
#if ENABLE_OPENCV
                        cv::Mat mat(cv::Size(frame->width, frame->height), CV_8UC3);
                        mat.data = (uchar*)frame->data[0];
                        process(mat);
#endif
                        SDL_UpdateTexture(texture, NULL, frame->data[0], frame->stride[0]);
                    }
                    pts = frame->pts;
                    has_frame = true;
                    force_update = true;
                }
                else
                {
                    SDL_QueueAudio(device_id, frame->data[0], frame->stride[0]);
                }
            }
            else if (event.type == SDL_REPLAY_EVENT)
            {
                crp_replay(player);
            }
            else if (event.type == SDL_QUIT)
            {
                is_running = false;
                break;
            }
        }

        Uint64 current_time = SDL_GetTicks64();
        if (!force_update && current_time - last_time < 1000 / IDLE_FPS)
        {
            continue;
        }
        last_time = current_time;

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        loop(window);
#ifdef __EMSCRIPTEN__
        if (simulatedPaste)
        {
            simulatedPaste = false;
            ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false);
            ImGui::GetIO().AddKeyEvent(ImGuiKey_V, false);
        }
#endif
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_RenderClear(renderer);
        if (has_frame)
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
#ifdef __EMSCRIPTEN__
    while (0); };
    emscripten_set_main_loop(EmscriptenMainLoop, 0, true);
#endif

    crp_destroy(player);
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_Quit();

    return 0;
}
