using System;
using System.Runtime.InteropServices;

namespace CoralReefPlayer
{
    static class NativeMethods
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct CFrame
        {
            public int width;
            public int height;
            [MarshalAs(UnmanagedType.I4)]
            public int format;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public IntPtr[] data;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public int[] linesize;
            public ulong pts;
        }

        internal delegate void crp_callback(int ev, IntPtr data, IntPtr user_data);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_create")]
        public static extern IntPtr crp_create();

        [DllImport("CoralReefPlayer", EntryPoint = "crp_destroy")]
        public static extern void crp_destroy(IntPtr handle);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_auth")]
        public static extern void crp_auth(IntPtr handle, string username, string password, bool is_md5);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_play")]
        public static extern void crp_play(IntPtr handle, string url, int transport,
            int width, int height, int format, crp_callback callback, IntPtr user_data);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_replay")]
        public static extern void crp_replay(IntPtr handle);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_stop")]
        public static extern void crp_stop(IntPtr handle);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_version_code")]
        public static extern int crp_version_code();

        [DllImport("CoralReefPlayer", EntryPoint = "crp_version_str")]
        public static extern IntPtr crp_version_str();
    }
}
