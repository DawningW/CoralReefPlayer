#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#endif

#include "version.h"
#include "coralreefplayer.h"
#include "StreamPuller.h"

crp_handle crp_create()
{
    return new StreamPuller();
}

void crp_destroy(crp_handle handle)
{
    StreamPuller* player = (StreamPuller*) handle;
    delete player;
}

void crp_auth(crp_handle handle, const char* username, const char* password, bool is_md5)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->authenticate(username, password, is_md5);
}

void crp_play(crp_handle handle, const char* url, int transport, Option* option, crp_callback callback, void* user_data)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->start(url, (Transport) transport, option, callback, user_data);
}

void crp_replay(crp_handle handle)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->restart();
}

void crp_stop(crp_handle handle)
{
    StreamPuller* player = (StreamPuller*) handle;
    player->stop();
}

int crp_version_code()
{
    return CRP_VER_MAJOR * 1000 + CRP_VER_MINOR;
}

const char* crp_version_str()
{
    return CRP_VER_STR;
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
