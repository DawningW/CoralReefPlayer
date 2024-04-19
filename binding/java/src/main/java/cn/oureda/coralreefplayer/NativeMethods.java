package cn.oureda.coralreefplayer;

class NativeMethods {
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

    static native long crp_create();
    static native void crp_destroy(long handle);
    static native void crp_auth(long handle, String username, String password, boolean is_md5);
    static native void crp_play(long handle, String url, Object option, Object callback);
    static native void crp_replay(long handle);
    static native void crp_stop(long handle);
    static native int crp_version_code();
    static native String crp_version_str();

    private static boolean isAndroid() {
        try {
            Class.forName("android.os.Build");
            return true;
        } catch (ClassNotFoundException e) {
            return false;
        }
    }
}
