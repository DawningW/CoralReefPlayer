#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#endif

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

void crp_auth(crp_handle handle, const char* username, const char* password, bool is_md5)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->authenticate(username, password, is_md5);
}

void crp_play(crp_handle handle, const char* url, int transport,
    int width, int height, int format, crp_callback callback)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->start(url, (Transport) transport, width, height, (Format) format, callback);
}

#ifdef WIN32
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
#endif
