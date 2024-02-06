using System;
using System.Threading;
using Xunit;

namespace CoralReefPlayer.Test {
    public class PullTest {
        private static readonly string URL = "rtsp://127.0.0.1/h264.sdp";

        public PullTest()
        {
            Console.WriteLine("CoralReefPlayer version: {0} ({1})", Player.VersionStr(), Player.VersionCode());
        }

        [Fact]
        public void Test()
        {
            Player player = new Player();
            bool hasFrame = false;
            player.Play(URL, Transport.UDP, 0, 0, Format.RGB24, new ActionCallback(
                (ev, data) =>
                {
                    Console.WriteLine("event: {0}, data: {1}", ev, data);
                },
                (frame) =>
                {
                    Console.WriteLine("frame: {0}x{1}, format: {2}, PTS: {3}", frame.Width, frame.Height, frame.Format, frame.PTS);
                    hasFrame = true;
                }
            ));
            long start = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
            while (!hasFrame)
            {
                Thread.Sleep(1000);
                if (DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() - start > 10000)
                {
                    break;
                }
            }
            player.Release();
            Assert.True(hasFrame);
        }
    }
}
