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
        RGB24 = 1,
        BGR24 = 2,
        ARGB32 = 3,
        RGBA32 = 4,
        ABGR32 = 5,
        BGRA32 = 6,
    }

    public enum Event
    {
        NEW_FRAME = 0,
        ERROR = 1,
        START = 2,
        PLAYING = 3,
        END = 4,
        STOP = 5,
    }

    public struct Frame
    {
        public int Width;
        public int Height;
        public Format Format;
        public UnmanagedMemoryStream Data;
        public int LineSize;
        public ulong PTS;
    }

    public interface ICallback
    {
        void OnEvent(Event ev, long data);
        void OnFrame(Frame frame);
    }

    public class ActionCallback : ICallback
    {
        private Action<Event, long> EventListener;
        private Action<Frame> FrameListener;

        public ActionCallback(Action<Event, long> eventListener, Action<Frame> frameListener)
        {
            EventListener = eventListener;
            FrameListener = frameListener;
        }

        public void OnEvent(Event ev, long data)
        {
            EventListener?.Invoke(ev, data);
        }

        public void OnFrame(Frame frame)
        {
            FrameListener?.Invoke(frame);
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

        public void Play(string url, Transport transport, int width, int height, Format format, ICallback callback)
        {
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
                        Data = null,
                        LineSize = cFrame.linesize[0],
                        PTS = cFrame.pts,
                    };
                    unsafe
                    {
                        byte* pData = (byte*)cFrame.data[0].ToPointer();
                        int length = cFrame.linesize[0] * cFrame.height;
                        frame.Data = new UnmanagedMemoryStream(pData, length);
                    }
                    callback.OnFrame(frame);
                }
                else
                {
                    callback.OnEvent(ev2, data.ToInt64());
                }
            };
            crp_play(handle, url, (int)transport, width, height, (int)format, cs_callback, IntPtr.Zero);
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
