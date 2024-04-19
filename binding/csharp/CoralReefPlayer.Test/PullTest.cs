using System;
using System.Threading;
using Xunit;
using Xunit.Abstractions;

namespace CoralReefPlayer.Test {
    public class PullTest {
        private static readonly string URL = "rtsp://127.0.0.1:8554/";
        readonly ITestOutputHelper output;

        public PullTest(ITestOutputHelper output)
        {
            this.output = output;
            output.WriteLine("CoralReefPlayer version: {0} ({1})", Player.VersionStr(), Player.VersionCode());
        }

        [Fact]
        public void Test()
        {
            Player player = new Player();
            bool hasFrame = false;
            Option option = new Option
            {
                Transport = Transport.UDP,
                VideoFormat = Format.YUV420P,
            };
            player.Play(URL, option, new ActionCallback(
                (ev, data) =>
                {
                    output.WriteLine("event: {0}, data: {1}", ev, data);
                },
                (isAudio, frame) =>
                {
                    if (!isAudio)
                        output.WriteLine("frame: {0}x{1}, format: {2}, PTS: {3}", frame.Width, frame.Height, frame.Format, frame.PTS);
                    else
                        output.WriteLine("audio: {0}x{1}, format: {2}, PTS: {3}", frame.SampleRate, frame.Channels, frame.Format, frame.PTS);
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
