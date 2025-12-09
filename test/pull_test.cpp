#include <unistd.h>
#include <iostream>
#include <string>
#include <print>
#include "coralreefplayer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define crp_sleep(s) emscripten_sleep(s * 1000)
#else
#define crp_sleep(s) sleep(s)
#endif

const char *url = "rtsp://127.0.0.1:8554/test";

int main()
{
    std::println("CoralReefPlayer version: {} ({})", crp_version_str(), crp_version_code());
    static bool hasFrame = false;
    crp_handle player = crp_create();
    Option option = {};
    option.transport = Transport::CRP_UDP;
    option.video.format = Format::CRP_YUV420P;
    crp_play(player, url, &option, [](int event, void* data, void* userData)
    {
        EventData *eventData = (EventData*) data;
        if (event == Event::CRP_EV_NEW_FRAME || event == Event::CRP_EV_NEW_AUDIO)
        {
            Frame& frame = eventData->frame;
            if (event == Event::CRP_EV_NEW_FRAME)
            {
                std::println("frame: {}x{}, format: {}, pts: {}", frame.width, frame.height, frame.format, frame.pts);
            }
            else
            {
                std::println("audio: {}x{}, format: {}, pts: {}", frame.sample_rate, frame.channels, frame.format, frame.pts);
            }
            hasFrame = true;
        }
        else if (event == Event::CRP_EV_VIDEO_EXTRADATA || event == Event::CRP_EV_AUDIO_EXTRADATA)
        {
            EventData::ExtraData &extraData = eventData->extra_data;
            std::string type = event == Event::CRP_EV_VIDEO_EXTRADATA ? "video" : "audio";
            std::println("received {} extra data, size: {}", type, extraData.size);
        }
        else if (event == CRP_EV_ERROR)
        {
            std::println("event: {}, data: {}", event, reinterpret_cast<uint64_t>(data));
        }
    }, nullptr);
    for (int i = 0; i < 10; i++)
    {
        crp_sleep(1);
        if (hasFrame) break;
    }
    if (!hasFrame)
    {
        std::println("no frame received");
        return 1;
    }
    return 0;
}
