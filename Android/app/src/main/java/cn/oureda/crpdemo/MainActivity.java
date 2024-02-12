package cn.oureda.crpdemo;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import cn.oureda.coralreefplayer.CoralReefPlayer;
import cn.oureda.crpdemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {
    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.buttonPlay.setOnClickListener(view -> {
            Intent intent = new Intent(this, PlayerActivity.class);
            intent.putExtra("url", binding.textUrl.getText().toString());
            intent.putExtra("is_tcp", binding.radioTcp.isChecked());
            startActivity(intent);
        });

        binding.textVersion.setText(String.format("Version: %s (%d)",
                CoralReefPlayer.versionStr(), CoralReefPlayer.versionCode()));
    }
}
