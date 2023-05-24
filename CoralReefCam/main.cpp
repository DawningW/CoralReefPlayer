#include <stdio.h>
#include "SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"
#include "coralreefplayer.h"

#define WIDTH 1280
#define HEIGHT 720
#define SDL_REFRESH_EVENT (SDL_USEREVENT + 1)

crp_handle player;
bool has_frame;

void loop()
{
    static bool show_demo_window = true;

    ImGui::NewFrame();
    ImGui::ShowDemoWindow(&show_demo_window);
    ImGui::Render();
}

#undef main
extern "C"
int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER))
    {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
    SDL_Window* window = SDL_CreateWindow("CoralReefCamCpp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, window_flags);
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
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture)
    {
        printf("Could not create texture: %s\n", SDL_GetError());
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    const char* url = "rtsp://172.6.2.20/main";
    if (argc > 1) {
        url = argv[1];
    }
    player = crp_create();
    crp_play(player, url, Transport::UDP, WIDTH, HEIGHT, Format::YUV420P, [](int eventCode, void *data) {
        if (eventCode == 0) {
            SDL_Event event = {};
            event.type = SDL_REFRESH_EVENT;
            event.user.data1 = data;
            SDL_PushEvent(&event);
        }
    });

    SDL_Event event;
    while (true)
    {
        SDL_WaitEvent(&event);
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_REFRESH_EVENT)
        {
            Frame* frame = (Frame*) event.user.data1;
            SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0],
                frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
            //SDL_UpdateTexture(texture, NULL, frame->data[0], frame->linesize[0]);
            has_frame = true;
        }
        else if (event.type == SDL_QUIT)
        {
            break;
        }

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        loop();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_RenderClear(renderer);
        if (has_frame)
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    crp_destroy(player);
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_Quit();
    
    return 0;
}
