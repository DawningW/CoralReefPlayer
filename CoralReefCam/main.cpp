#include <stdio.h>
#include "SDL.h"
#include "readerwriterqueue.h"
#include "StreamPuller.h"

#define WIDTH 1280
#define HEIGHT 720
#define SDL_REFRESH_EVENT (SDL_USEREVENT + 1)

moodycamel::ReaderWriterQueue<AVFrame*> queue;
StreamPuller streamPuller;

#undef main
extern "C"
int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* screen = SDL_CreateWindow("CoralReefCamCpp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    if (!screen)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(screen, -1, 0);
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

    streamPuller.start("rtsp://172.6.2.20/main", Transport::UDP, [](AVFrame* frame) {
        queue.enqueue(frame);
        SDL_Event event = {};
        event.type = SDL_REFRESH_EVENT;
        SDL_PushEvent(&event);
    });

    SDL_Event event;
    for (;;)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_REFRESH_EVENT)
        {
            AVFrame* frame;
            while (queue.try_dequeue(frame))
            {
                SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0],
                    frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                av_frame_free(&frame);
            }
        }
        else if (event.type == SDL_QUIT)
        {
            break;
        }
    }

    streamPuller.stop();
    AVFrame* frame;
    while (queue.try_dequeue(frame))
        av_frame_free(&frame);
    SDL_Quit();
    
    return 0;
}
