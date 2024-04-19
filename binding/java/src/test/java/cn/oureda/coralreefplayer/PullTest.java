package cn.oureda.coralreefplayer;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class PullTest {
    private static final String URL = "rtsp://127.0.0.1:8554/";

    @BeforeAll
    static void setUp() {
        System.out.printf("CoralReefPlayer version: %s (%d)\n", CoralReefPlayer.versionStr(), CoralReefPlayer.versionCode());
    }

    @Test
    void test() {
        CoralReefPlayer player = new CoralReefPlayer();
        Boolean[] hasFrame = new Boolean[] {false};
        Option option = new Option();
        option.transport = CoralReefPlayer.TRANSPORT_UDP;
        option.videoFormat = CoralReefPlayer.FORMAT_YUV420P;
        player.play(URL, option, new CoralReefPlayer.Callback() {
            @Override
            public void onEvent(int event, long data) {
                System.out.println("event: " + event + ", data: " + data);
            }

            @Override
            public void onFrame(boolean isAudio, Frame frame) {
                if (!isAudio)
                    System.out.printf("frame: %dx%d, format: %d, pts: %d\n", frame.width, frame.height, frame.format, frame.pts);
                else
                    System.out.printf("audio: %dx%d, format: %d, pts: %d\n", frame.sampleRate, frame.channels, frame.format, frame.pts);
                hasFrame[0] = true;
            }
        });
        long start = System.currentTimeMillis();
        while (!hasFrame[0]) {
            try {
                Thread.sleep(1000);
                if (System.currentTimeMillis() - start > 10000) {
                    break;
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        player.release();
        assertTrue(hasFrame[0]);
    }
}
