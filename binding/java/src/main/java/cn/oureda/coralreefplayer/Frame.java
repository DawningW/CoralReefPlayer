package cn.oureda.coralreefplayer;

import java.nio.ByteBuffer;

public class Frame {
    public int width;
    public int height;
    public int sampleRate;
    public int channels;
    public int format;
    public ByteBuffer[] data;
    public int[] stride;
    public long pts;
}
