package cn.oureda.coralreefplayer;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.TextureView;

public class PlayerView extends TextureView {
    private PlayerController controller;

    public PlayerView(Context context) {
        super(context);
    }

    public PlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setController(PlayerController controller) {
        this.controller = controller;
        this.setSurfaceTextureListener(this.controller.surfaceTextureListener);
    }

    public PlayerController getController() {
        return controller;
    }

    public static native void renderYUVOnSurface(Surface surface, Frame frame);
}
