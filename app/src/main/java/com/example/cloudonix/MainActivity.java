package com.example.cloudonix;

import android.graphics.Color;
import android.os.Bundle;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import com.example.cloudonix.databinding.ActivityMainBinding;

import org.json.JSONObject;

import java.io.IOException;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("cloudonix");
    }

    private ActivityMainBinding binding;

    private final String BASE_URL = "https://s7om3fdgbt7lcvqdnxitjmtiim0uczux.lambda-url.us-east-2.on.aws/";

    public native String getIPAddress();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());

        binding.button.setOnClickListener(v -> fetchIPAddress());

        setContentView(binding.getRoot());
    }

    private void fetchIPAddress() {
        binding.progressBar.setVisibility(View.VISIBLE);
        binding.result.setVisibility(View.GONE);

        new Thread(() -> {
            String ipAddress = getIPAddress();
            if (ipAddress.equals("Error")) {
                runOnUiThread(this::showError);
            } else {
                sendIPAddress(ipAddress);
            }
        }).start();
    }

    private void sendIPAddress(String ipAddress) {
        OkHttpClient client = new OkHttpClient();
        MediaType JSON = MediaType.parse("application/json; charset=utf-8");
        RequestBody body = RequestBody.create(JSON, "{\"address\":\"" + ipAddress + "\"}");

        Request request = new Request
                .Builder()
                .url(BASE_URL)
                .post(body)
                .build();

        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                System.out.println("error" + e.getMessage());
                runOnUiThread(() -> showError());
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                if (response.isSuccessful()) {
                    String responseData = response.body().string();
                    try {
                        JSONObject jsonObject = new JSONObject(responseData);
                        boolean nat = jsonObject.getBoolean("nat");
                        runOnUiThread(() -> showResult(ipAddress, nat));
                    } catch (Exception e) {
                        runOnUiThread(() -> showError());
                    }
                } else {
                    runOnUiThread(() -> showError());
                }
            }
        });
    }

    private void showResult(String ipAddress, boolean nat) {
        binding.progressBar.setVisibility(View.GONE);
        binding.result.setVisibility(View.VISIBLE);
        binding.result.setText("IP Address: " + ipAddress + "\nNAT: " + nat);
        binding.result.setTextColor(nat ? Color.GREEN : Color.RED);
    }

    private void showError() {
        binding.progressBar.setVisibility(View.GONE);
        binding.result.setVisibility(View.VISIBLE);
        binding.result.setText("Error occurred");
        binding.result.setTextColor(Color.RED);
    }
}