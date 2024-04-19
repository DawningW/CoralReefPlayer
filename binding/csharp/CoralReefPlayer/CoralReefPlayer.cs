using System;
using System.IO;
using System.Runtime.InteropServices;
using static CoralReefPlayer.NativeMethods;

namespace CoralReefPlayer
{
    public enum Transport
    {
        UDP = 0,
        TCP = 1,
    }

    public enum Format
    {
        YUV420P = 0,
        NV12 = 1,
        NV21 = 2,
        RGB24 = 3,
        BGR24 = 4,
        ARGB32 = 5,
        RGBA32 = 6,
        ABGR32 = 7,
        BGRA32 = 8,

        U8 = 0,
        S16 = 1,
        S32 = 2,
        F32 = 3,
    }

    public enum Event
    {
        NEW_FRAME = 0,
        ERROR = 1,
        START = 2,
        PLAYING = 3,
        END = 4,
        STOP = 5,
        NEW_AUDIO = 6,
    }

    public struct Option
    {
        public Transport Transport;
        public int Width;
        public int Height;
        public Format VideoFormat;
        public string HWDevice;
        public bool EnableAudio;
        public int SampleRate;
        public int Channels;
        public Format AudioFormat;
        public long Timeout;
    }

    public struct Frame
    {
        public int Width;
        public int Height;
        public int SampleRate;
        public int Channels;
        public Format Format;
        public UnmanagedMemoryStream[] Data;
        public int[] LineSize;
        public ulong PTS;
    }

    public interface ICallback
    {
        void OnEvent(Event ev, long data);
        void OnFrame(bool isAudio, Frame frame);
    }

    public class ActionCallback : ICallback
    {
        private Action<Event, long> EventListener;
        private Action<bool, Frame> FrameListener;

        public ActionCallback(Action<Event, long> eventListener, Action<bool, Frame> frameListener)
        {
            EventListener = eventListener;
            FrameListener = frameListener;
        }

        public void OnEvent(Event ev, long data)
        {
            EventListener?.Invoke(ev, data);
        }

        public void OnFrame(bool isAudio, Frame frame)
        {
            FrameListener?.Invoke(isAudio, frame);
        }
    }

    public class Player
    {
        private IntPtr handle;
        private crp_callback cs_callback;

        public Player()
        {
            handle = crp_create();
        }

        public void Release()
        {
            crp_destroy(handle);
            handle = IntPtr.Zero;
        }

        public void Auth(string username, string password, bool isMD5)
        {
            crp_auth(handle, username, password, isMD5);
        }

        public void Play(string url, Option option, ICallback callback)
        {
            COption cOption = new COption
            {
                transport = (int)option.Transport,
                video_width = option.Width,
                video_height = option.Height,
                video_format = (int)option.VideoFormat,
                hw_device = option.HWDevice,
                enable_audio = option.EnableAudio,
                audio_sample_rate = option.SampleRate,
                audio_channels = option.Channels,
                audio_format = (int)option.AudioFormat,
                timeout = option.Timeout,
            };
            cs_callback = (ev, data, userData) =>
            {
                Event ev2 = (Event)ev;
                if (ev2 == Event.NEW_FRAME)
                {
                    CFrame cFrame = Marshal.PtrToStructure<CFrame>(data);
                    Frame frame = new Frame
                    {
                        Width = cFrame.width,
                        Height = cFrame.height,
                        Format = (Format)cFrame.format,
                        Data = new UnmanagedMemoryStream[4],
                        LineSize = new int[4],
                        PTS = cFrame.pts,
                    };
                    unsafe
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            byte* pData = (byte*)cFrame.data[i].ToPointer();
                            int length = cFrame.linesize[i] * cFrame.height;
                            if (i > 0) length /= 2;
                            if (pData != null)
                                frame.Data[i] = new UnmanagedMemoryStream(pData, length);
                            frame.LineSize[i] = cFrame.linesize[i];
                        }
                    }
                    callback.OnFrame(false, frame);
                }
                else if (ev2 == Event.NEW_AUDIO)
                {
                    CFrame cFrame = Marshal.PtrToStructure<CFrame>(data);
                    Frame frame = new Frame
                    {
                        SampleRate = cFrame.width,
                        Channels = cFrame.height,
                        Format = (Format)cFrame.format,
                        Data = new UnmanagedMemoryStream[1],
                        LineSize = new int[1],
                        PTS = cFrame.pts,
                    };
                    unsafe
                    {
                        byte* pData = (byte*)cFrame.data[0].ToPointer();
                        int length = cFrame.linesize[0];
                        if (pData != null)
                            frame.Data[0] = new UnmanagedMemoryStream(pData, length);
                        frame.LineSize[0] = cFrame.linesize[0];
                    }
                    callback.OnFrame(true, frame);
                }
                else
                {
                    callback.OnEvent(ev2, data.ToInt64());
                }
            };
            crp_play(handle, url, ref cOption, cs_callback, IntPtr.Zero);
        }

        public void Replay()
        {
            crp_replay(handle);
        }

        public void Stop()
        {
            crp_stop(handle);
        }

        public static int VersionCode()
        {
            return crp_version_code();
        }

        public static string VersionStr()
        {
            IntPtr pStr = crp_version_str();
            return Marshal.PtrToStringAnsi(pStr);
        }
    }
}
