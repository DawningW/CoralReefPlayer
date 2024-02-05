package cn.oureda.coralreefplayer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

public class PullTest {
    private static final String URL = "rtsp://127.0.0.1/main.sdp";

    @BeforeAll
    static void setUp() {
        System.out.printf("CoralReefPlayer version: %s (%d)\n", CoralReefPlayer.versionStr(), CoralReefPlayer.versionCode());
    }

    @Test
    void test() {
        CoralReefPlayer player = new CoralReefPlayer();
        Boolean[] hasFrame = new Boolean[] {false};
        player.play(URL, CoralReefPlayer.TRANSPORT_UDP, 0, 0, CoralReefPlayer.FORMAT_RGB24,
                new CoralReefPlayer.Callback() {
                    @Override
                    public void onEvent(int event, long data) {
                        System.out.println("event: " + event + ", data: " + data);
                    }

                    @Override
                    public void onFrame(int width, int height, int format, byte[] data, long pts) {
                        System.out.println("frame: " + width + "x" + height + ", format: " + format + ", pts: " + pts);
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
