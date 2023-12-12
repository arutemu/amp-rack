package com.mukuro.ladspa.amprack;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import androidx.navigation.ui.AppBarConfiguration;

import com.mukuro.ladspa.amprack.databinding.ActivityFeaturedVideosBinding;

public class FeaturedVideos extends AppCompatActivity {

    private AppBarConfiguration appBarConfiguration;
    private ActivityFeaturedVideosBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityFeaturedVideosBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
    }
}