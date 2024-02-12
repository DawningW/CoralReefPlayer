package cn.oureda.crpdemo;

import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.widget.Toast;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import java.nio.ByteBuffer;

import cn.oureda.coralreefplayer.CoralReefPlayer;
import cn.oureda.crpdemo.databinding.ActivityPlayerBinding;

public class PlayerActivity extends AppCompatActivity implements CoralReefPlayer.Callback {
    private final static String TAG = "PlayerActivity";
    private ActivityPlayerBinding binding;
    private CoralReefPlayer player;
    private boolean played;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityPlayerBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        // Delayed removal of status and navigation bar
        if (Build.VERSION.SDK_INT >= 30) {
            binding.getRoot().getWindowInsetsController().hide(
                    WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
        } else {
            // Note that some of these constants are new as of API 16 (Jelly Bean)
            // and API 19 (KitKat). It is safe to use them, as they are inlined
            // at compile-time and do nothing on earlier devices.
            binding.getRoot().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }

        player = new CoralReefPlayer();
    }

    @Override
    protected void onStart() {
        super.onStart();
        Intent intent = getIntent();
        String url = intent.getStringExtra("url");
        int transport = intent.getBooleanExtra("is_tcp", false) ? 0 : 1;
        player.play(url, transport, 0, 0, CoralReefPlayer.FORMAT_RGBA32, this);
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
        player = null;
    }

    @Override
    public void onEvent(int event, long data) {
        runOnUiThread(() -> {
            Log.i(TAG, "event: " + event);
            if (event == CoralReefPlayer.EVENT_ERROR) {
                toast("错误!");
            } else if (event == CoralReefPlayer.EVENT_START) {
                played = false;
                toast("开始拉流");
            } else if (event == CoralReefPlayer.EVENT_PLAYING) {
                toast("正在播放");
            } else if (event == CoralReefPlayer.EVENT_END) {
                toast("视频流已到达末尾");
            } else if (event == CoralReefPlayer.EVENT_STOP) {
                toast("停止拉流");
            }
        });
    }

    @Override
    public void onFrame(int width, int height, int format, byte[] data, long pts) {
        runOnUiThread(() -> {
            if (!played) {
                int viewWidth = binding.getRoot().getWidth();
                int viewHeight = binding.getRoot().getHeight();
                float ratio = (float) width / height;
                if ((float) viewWidth / viewHeight < ratio) {
                    // 以宽度为基准
                    binding.getRoot().getLayoutParams().width = ViewGroup.LayoutParams.MATCH_PARENT;
                    binding.getRoot().getLayoutParams().height = (int) (viewWidth / ratio + 0.5f);
                } else {
                    // 以高度为基准
                    binding.getRoot().getLayoutParams().height = ViewGroup.LayoutParams.MATCH_PARENT;
                    binding.getRoot().getLayoutParams().width = (int) (viewHeight * ratio + 0.5f);
                }
                played = true;
            }

            Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            bitmap.copyPixelsFromBuffer(ByteBuffer.wrap(data));
            binding.imagePlayer.setImageBitmap(bitmap);
        });
    }

    private void toast(String message) {
        Toast.makeText(this, message, message.length() >= 10 ?
                Toast.LENGTH_LONG : Toast.LENGTH_SHORT).show();
    }
}
