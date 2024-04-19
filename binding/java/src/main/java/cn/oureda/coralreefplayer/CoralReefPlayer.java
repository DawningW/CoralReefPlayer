package cn.oureda.coralreefplayer;

public class CoralReefPlayer {
    public static final int TRANSPORT_UDP = 0;
    public static final int TRANSPORT_TCP = 1;

    public static final int FORMAT_YUV420P = 0;
    public static final int FORMAT_NV12 = 1;
    public static final int FORMAT_NV21 = 2;
    public static final int FORMAT_RGB24 = 3;
    public static final int FORMAT_BGR24 = 4;
    public static final int FORMAT_ARGB32 = 5;
    public static final int FORMAT_RGBA32 = 6;
    public static final int FORMAT_ABGR32 = 7;
    public static final int FORMAT_BGRA32 = 8;
    public static final int FORMAT_U8 = 0;
    public static final int FORMAT_S16 = 1;
    public static final int FORMAT_S32 = 2;
    public static final int FORMAT_F32 = 3;

    public static final int EVENT_NEW_FRAME = 0;
    public static final int EVENT_ERROR = 1;
    public static final int EVENT_START = 2;
    public static final int EVENT_PLAYING = 3;
    public static final int EVENT_END = 4;
    public static final int EVENT_STOP = 5;
    public static final int EVENT_NEW_AUDIO = 6;

    private long handle;

    public CoralReefPlayer() {
        handle = NativeMethods.crp_create();
    }

    public void release() {
        NativeMethods.crp_destroy(handle);
        handle = 0;
    }

    public void auth(String username, String password, boolean isMD5) {
        NativeMethods.crp_auth(handle, username, password, isMD5);
    }

    public void play(String url, Option option, Callback callback) {
        NativeMethods.crp_play(handle, url, option, callback);
    }

    public void replay() {
        NativeMethods.crp_replay(handle);
    }

    public void stop() {
        NativeMethods.crp_stop(handle);
    }

    public static int versionCode() {
        return NativeMethods.crp_version_code();
    }

    public static String versionStr() {
        return NativeMethods.crp_version_str();
    }

    public interface Callback {
        void onEvent(int event, long data);
        void onFrame(boolean isAudio, Frame frame);
    }
}
