#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include "coralreefplayer.h"
#include "StreamPuller.h"

crp_handle crp_create()
{
    return new StreamPuller();
}

void crp_destroy(crp_handle handle)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->stop();
    delete player;
}

void crp_play(crp_handle handle, const char* url, Transport transport,
    int width, int height, Format format, crp_callback callback)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->start(url, transport, width, height, format, callback);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
