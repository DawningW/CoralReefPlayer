package cn.oureda.coralreefplayer;

import android.graphics.SurfaceTexture;
import android.view.Surface;
import android.view.TextureView;

public class PlayerController {
    public final PlayerSurfaceTextureListener surfaceTextureListener;
    private final CoralReefPlayer player;

    public PlayerController() {
        this.surfaceTextureListener = new PlayerSurfaceTextureListener();
        this.player = new CoralReefPlayer();
    }

    public void setAuth(String username, String password, boolean isMD5) {
        this.player.auth(username, password, isMD5);
    }

    public void release() {
        this.player.release();
    }

    public void play(String url, Option option, CoralReefPlayer.Callback callback) {
        this.player.play(url, option, new CoralReefPlayer.Callback() {
            @Override
            public void onEvent(int event, Object data) {
                if (callback != null) {
                    callback.onEvent(event, data);
                }
            }

            @Override
            public void onFrame(boolean isAudio, Frame frame) {
                if (callback != null) {
                    callback.onFrame(isAudio, frame);
                }
                if (!isAudio) {
                    PlayerView.renderYUVOnSurface(PlayerController.this.surfaceTextureListener.surface, frame);
                }
            }
        });
    }

    public void replay() {
        this.player.replay();
    }

    public void stop() {
        this.player.stop();
    }

    public static class PlayerSurfaceTextureListener implements TextureView.SurfaceTextureListener {
        public Surface surface;

        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            this.surface = new Surface(surface);
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {}

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            if (this.surface != null) {
                this.surface.release();
                this.surface = null;
            }
            return true;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {}
    }
}
