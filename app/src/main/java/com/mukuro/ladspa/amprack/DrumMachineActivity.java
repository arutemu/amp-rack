package com.mukuro.ladspa.amprack;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import android.view.ViewGroup;

import com.mukuro.ladspa.amprack.databinding.ActivityDrumMachineBinding;

public class DrumMachineActivity extends AppCompatActivity {
    DrumMachine drumMachine ;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        drumMachine = new DrumMachine(getApplicationContext());
        drumMachine.setMainActivity(this);
        drumMachine.create();
        setTitle("Drummer");
        ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        addContentView(drumMachine, layoutParams);


    }

}