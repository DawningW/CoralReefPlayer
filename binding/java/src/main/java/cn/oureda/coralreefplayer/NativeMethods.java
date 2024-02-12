package cn.oureda.coralreefplayer;

public class NativeMethods {
    static {
        if (isAndroid()) {
            System.loadLibrary("crp_native");
        } else {
            try {
                Class.forName("cn.oureda.coralreefplayer.LibraryLoader");
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
            }
        }
    }

    public static native long crp_create();
    public static native void crp_destroy(long handle);
    public static native void crp_auth(long handle, String username, String password, boolean is_md5);
    public static native void crp_play(long handle, String url, int transport, int width, int height, int format, Object callback);
    public static native void crp_replay(long handle);
    public static native void crp_stop(long handle);
    public static native int crp_version_code();
    public static native String crp_version_str();

    private static boolean isAndroid() {
        try {
            Class.forName("android.os.Build");
            return true;
        } catch (ClassNotFoundException e) {
            return false;
        }
    }
}
