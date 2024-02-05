package cn.oureda.coralreefplayer;

import cn.oureda.coralreefplayer.util.LibraryLoader;

import java.nio.file.Files;
import java.nio.file.Path;

public class NativeMethods {
    static {
        try {
            // TODO 需要配合自定义 ClassLoader.findLibrary() 方法使用
            // if (!LibraryLoader.isAndroid()) {
            //     Path tempDir = Files.createTempDirectory("CoralReefPlayer");
            //     tempDir.toFile().deleteOnExit();
            //     LibraryLoader.addPath(tempDir.toString());
            //     LibraryLoader.extractFromJar("/native", tempDir);
            // }
            // System.loadLibrary("crp_native");

            if (!LibraryLoader.isAndroid()) {
                if (LibraryLoader.isWindows()) {
                    LibraryLoader.load("avutil-56");
                    LibraryLoader.load("swscale-5");
                    LibraryLoader.load("swresample-3");
                    LibraryLoader.load("avcodec-58");
                    LibraryLoader.load("libcurl");
                }
                LibraryLoader.load("CoralReefPlayer");
                LibraryLoader.load("crp_native");
            } else {
                System.loadLibrary("crp_native");
            }
        } catch (Exception e) {
            e.printStackTrace();
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
}
