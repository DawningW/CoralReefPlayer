using System;
using System.Runtime.InteropServices;

namespace CoralReefPlayer
{
    static class NativeMethods
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct COption
        {
            public int transport;
            public int video_width;
            public int video_height;
            public int video_format;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string hw_device;
            public bool enable_audio;
            public int audio_sample_rate;
            public int audio_channels;
            public int audio_format;
            public long timeout;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct CFrame
        {
            public int width;
            public int height;
            public int format;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public IntPtr[] data;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public int[] stride;
            public ulong pts;
        }

        internal delegate void crp_callback(int ev, IntPtr data, IntPtr user_data);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_create")]
        internal static extern IntPtr crp_create();

        [DllImport("CoralReefPlayer", EntryPoint = "crp_destroy")]
        internal static extern void crp_destroy(IntPtr handle);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_auth")]
        internal static extern void crp_auth(IntPtr handle, string username, string password, bool is_md5);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_play")]
        internal static extern void crp_play(IntPtr handle, string url, ref COption option,
            crp_callback callback, IntPtr user_data);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_replay")]
        internal static extern void crp_replay(IntPtr handle);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_stop")]
        internal static extern void crp_stop(IntPtr handle);

        [DllImport("CoralReefPlayer", EntryPoint = "crp_version_code")]
        internal static extern int crp_version_code();

        [DllImport("CoralReefPlayer", EntryPoint = "crp_version_str")]
        internal static extern IntPtr crp_version_str();
    }
}
