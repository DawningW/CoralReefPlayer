#include <string>
#include <stdio.h>
#include "SDL.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "implot.h"
#if ENABLE_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#endif
#include "coralreefplayer.h"

#define WIDTH 1280
#define HEIGHT 720
#define SDL_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SDL_REPLAY_EVENT (SDL_USEREVENT + 2)

std::string url;
Transport transport = CRP_UDP;
crp_handle player;
bool playing;
bool has_frame;
uint64_t pts;

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

void play()
{
    if (playing)
        crp_stop(player);
    SDL_FlushEvent(SDL_REFRESH_EVENT);
    crp_play(player, url.c_str(), transport, WIDTH, HEIGHT, ENABLE_OPENCV ? CRP_BGR24 : CRP_YUV420P,
        [](int ev, void* data, void* userdata)
        {
            if (ev == CRP_EV_NEW_FRAME)
            {
                SDL_Event event = {};
                event.type = SDL_REFRESH_EVENT;
                event.user.data1 = data;
                SDL_PushEvent(&event);
            }
            else if (ev == CRP_EV_ERROR)
            {
                printf("An error has occurred, will reconnect after 5 seconds\n");
                SDL_AddTimer(5000, [](Uint32 interval, void* param)
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
        }, nullptr);
}

void loop()
{
    static bool show_imgui_demo_window = false;
    static bool show_implot_demo_window = false;
    static bool show_fps_window = true;
    bool open_play_window = false;
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
                crp_stop(player);
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
            if (ImGui::MenuItem("Options")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::SeparatorText("Panels");
            ImGui::MenuItem("ImGui Demo", NULL, &show_imgui_demo_window);
            ImGui::MenuItem("ImPlot Demo", NULL, &show_implot_demo_window);
            ImGui::MenuItem("Frame Info", NULL, &show_fps_window);
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
        ImGui::RadioButton("UDP", (int*) &transport, CRP_UDP);
        ImGui::SameLine();
        ImGui::RadioButton("TCP", (int*) &transport, CRP_TCP);
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
        ImGui::Text("Copyright (c) 2023-2024 OurEDA");
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

#undef main
extern "C"
int main(int argc, char* argv[])
{
    if (argc > 1)
        url = argv[1];
    if (argc > 2)
        transport = strcmp(argv[2], "tcp") == 0 ? CRP_TCP : CRP_UDP;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER))
    {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
    SDL_Window* window = SDL_CreateWindow("CoralReefCam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, window_flags);
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
    Uint32 format = ENABLE_OPENCV ? SDL_PIXELFORMAT_BGR24 : SDL_PIXELFORMAT_IYUV;
    SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture)
    {
        printf("Could not create texture: %s\n", SDL_GetError());
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImFontConfig font_config;
    font_config.SizePixels = 16.0f;
    io.Fonts->AddFontDefault(&font_config);
    // io.Fonts->AddFontFromFileTTF("./unifont-15.0.06.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    player = crp_create();
    if (!url.empty())
        play();

    SDL_Event event;
    while (true)
    {
        SDL_WaitEvent(&event);
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_REFRESH_EVENT)
        {
            if (!playing)
                continue;

            Frame* frame = (Frame*) event.user.data1;
#if ENABLE_OPENCV
            cv::Mat mat(cv::Size(frame->width, frame->height), CV_8UC3);
            mat.data = (uchar*) frame->data[0];
            process(mat);
            SDL_UpdateTexture(texture, NULL, frame->data[0], frame->linesize[0]);
#else
            SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0],
                frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
#endif
            pts = frame->pts;
            has_frame = true;
        }
        else if (event.type == SDL_REPLAY_EVENT)
        {
            crp_replay(player);
            continue;
        }
        else if (event.type == SDL_QUIT)
        {
            break;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        loop();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_RenderClear(renderer);
        if (has_frame)
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    crp_destroy(player);
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_Quit();
    
    return 0;
}
