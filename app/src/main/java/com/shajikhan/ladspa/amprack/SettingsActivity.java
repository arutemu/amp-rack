package com.shajikhan.ladspa.amprack;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.firebase.auth.FirebaseAuth;

import java.util.ArrayList;

public class SettingsActivity extends AppCompatActivity implements
        PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {

    private static final String TITLE_TAG = "Settings";
    public AudioManager audioManager ;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings_activity);
        if (savedInstanceState == null) {
            getSupportFragmentManager()
                    .beginTransaction()
                    .replace(R.id.settings, new HeaderFragment())
                    .commit();
        } else {
            setTitle(savedInstanceState.getCharSequence(TITLE_TAG));
        }
        getSupportFragmentManager().addOnBackStackChangedListener(
                new FragmentManager.OnBackStackChangedListener() {
                    @Override
                    public void onBackStackChanged() {
                        if (getSupportFragmentManager().getBackStackEntryCount() == 0) {
                            setTitle(R.string.title_activity_settings);
                        }
                    }
                });
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        // Save current activity title so we can set it again after a configuration change
        outState.putCharSequence(TITLE_TAG, getTitle());
    }

    @Override
    public boolean onSupportNavigateUp() {
        if (getSupportFragmentManager().popBackStackImmediate()) {
            return true;
        }
        return super.onSupportNavigateUp();
    }

    @Override
    public boolean onPreferenceStartFragment(PreferenceFragmentCompat caller, Preference pref) {
        // Instantiate the new Fragment
        final Bundle args = pref.getExtras();
        final Fragment fragment = getSupportFragmentManager().getFragmentFactory().instantiate(
                getClassLoader(),
                pref.getFragment());
        fragment.setArguments(args);
        fragment.setTargetFragment(caller, 0);
        // Replace the existing Fragment with the new Fragment
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.settings, fragment)
                .addToBackStack(null)
                .commit();
        setTitle(pref.getTitle());
        return true;
    }

    public static class HeaderFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.header_preferences, rootKey);
        }
    }

    public static class MessagesFragment extends PreferenceFragmentCompat {
        AudioDeviceInfo[] audioDevicesInput, audioDevicesOutput ;
        int defaultInputDevice = 0 ;
        int defaultOutputDevice = 0 ;

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.messages_preferences, rootKey);
            ListPreference listPreference = findPreference("input");
            ListPreference listPreferenceOutput = findPreference("output");

            ArrayList<CharSequence> entries = new ArrayList<>();
            ArrayList<CharSequence> entryValues = new ArrayList<>();
            SettingsActivity settingsActivity = (SettingsActivity) getActivity();
            AudioManager audioManager = settingsActivity.audioManager;
            if (audioManager == null) {
                MainActivity.alert("Cannot connect to Audio Manager", "Cannot connect to audio manager. Please restart the app");
                return;
            }

            audioDevicesInput = audioManager.getDevices (AudioManager.GET_DEVICES_INPUTS) ;
            audioDevicesOutput = audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS) ;

            entries.add("Default");
            entryValues.add("-1");

            for (int i = 0 ; i < audioDevicesInput.length ; i ++) {
                String name = MainActivity.typeToString(audioDevicesInput[i].getType());
                int deviceID = audioDevicesInput[i].getId();
//                name = (String) audioDevicesInput[i].getProductName();
                entries.add(name + " (" + (String) audioDevicesInput[i].getProductName() + ")");
//                entryValues.add(String.valueOf(audioDevicesInput[i]));
                entryValues.add(String.valueOf(deviceID));
                Log.d(TITLE_TAG, "onCreatePreferences: " + String.format ("%s: %s", deviceID, name));
            }

            listPreference.setEntries(entries.toArray(new CharSequence[entries.size()]));
            listPreference.setEntryValues(entryValues.toArray(new CharSequence[entryValues.size()]));

            listPreference.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
//                    AudioEngine.setRecordingDeviceId(new Integer(newValue.toString()));
                    return true;
                }
            });

            entries.clear();
            entryValues.clear();

            entries.add("Default");
            entryValues.add("-1");

            for (int i = 0 ; i < audioDevicesOutput.length ; i ++) {
                String name = MainActivity.typeToString(audioDevicesOutput[i].getType());
                int deviceID = audioDevicesOutput[i].getId();
//                name = (String) audioDevicesOutput[i].getProductName();
                entries.add(name + " (" + (String) audioDevicesOutput[i].getProductName() + ")");
                entryValues.add(String.valueOf(deviceID));
                Log.d(TITLE_TAG, "onCreatePreferences: " + String.format ("%s: %s", deviceID, name));
            }

            listPreferenceOutput.setEntries(entries.toArray(new CharSequence[entries.size()]));
            listPreferenceOutput.setEntryValues(entryValues.toArray(new CharSequence[entryValues.size()]));

            listPreferenceOutput.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    Log.d(getClass().getSimpleName(), "onPreferenceChange: [playbackDeviceID] " + newValue.toString());
//                    AudioEngine.setPlaybackDeviceId(Integer.parseInt(newValue.toString()));
                    return true;
                }
            });
        }
    }

    public static class SyncFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.sync_preferences, rootKey);
            if (MainActivity.proVersion == false) {
                ListPreference export = findPreference("export_format");
                export.setSummary("More export formats are available in the Pro Version");
                export.setShouldDisableView(true);
                export.setEnabled(false);
            }
        }
    }

    public static class ThemeFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.theme_settings, rootKey);
            Preference preference = findPreference("background_custom");
            preference.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    Intent intent_upload = new Intent();
                    intent_upload.setType("image/*");
                    intent_upload.setAction(Intent.ACTION_OPEN_DOCUMENT);
                    intent_upload.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                    intent_upload.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    getActivity().startActivityForResult(intent_upload,1);
                    return true;
                }
            });

            Preference customTheme = findPreference("theme_custom");
            if (customTheme == null) {
                MainActivity.toast("custom_theme == null");
            } else {
                customTheme.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference) {
                        Intent intent_upload = new Intent();
//                        intent_upload.setType("*/*");
                        intent_upload.setAction(Intent.ACTION_OPEN_DOCUMENT_TREE);
                        intent_upload.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                        intent_upload.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                        getActivity().startActivityForResult(intent_upload, 2);

                        return false;
                    }
                });
            }

            Preference themePreference1 = findPreference("theme") ;
            themePreference1.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getContext());
                    sharedPreferences.edit().remove("background").commit();
                    sharedPreferences.edit().putString("theme", newValue.toString()).commit();

                    return false;
                }
            });

            /*
            Preference colorScheme = findPreference("color_scheme");
            colorScheme.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    Log.d(getClass().getSimpleName(), "onPreferenceChange() called with: preference = [" + preference + "], newValue = [" + newValue + "]");
                    PreferenceManager.getDefaultSharedPreferences(getContext()).edit().putString("color_scheme", newValue.toString()).apply();
                    return false;
                }
            });

             */
        }
    }

    public static class ProfileFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.account_pref, rootKey);
            FirebaseAuth auth = FirebaseAuth.getInstance() ;
            Preference logOut = findPreference("log_out");
            Preference details = findPreference("delete_account");
            logOut.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    GoogleSignInOptions gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
                            .requestIdToken("983863263684-6ggjm8spjvvftm5noqtpl97v0le5laft.apps.googleusercontent.com")
                            .requestEmail()
                            .build();

                    GoogleSignIn.getClient(getContext(), gso).signOut();
                    auth.signOut();
                    MainActivity.alert("Logged out", "You have been logged out");
                    return false;
                }
            });
            Preference deleteAccountButton = findPreference("delete_account");
            if (auth == null || auth.getUid() == null) {
                deleteAccountButton.setVisible(false);
                logOut.setVisible(false);
                details.setVisible(false);
                return;
            }

            String email = auth.getCurrentUser().getEmail();
            Preference preference = findPreference("email");
            preference.setTitle(email);

            deleteAccountButton.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    new AlertDialog.Builder(getContext())
                            .setMessage("Are you sure you want to delete your account? This will delete your presets, favorites and all associated account data." +
                                    "\n\nThis cannot be undone. Are you sure you want to proceed?")
                            .setPositiveButton("Delete my account", new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    Intent myIntent = new Intent(getContext(), DeleteData.class);
                                    startActivity(myIntent);
                                }
                            })
                            .setNegativeButton("Cancel", null)
                            .show();

                    return false;
                }
            });
        }
    }

    public static class VersionFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.version_info, rootKey);
            findPreference("name_info").setTitle("AmpRack Pro Version");
            findPreference("build_number").setSummary(R.string.build_id);
            findPreference("build_number").setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    HashCommands commands = new HashCommands(getContext());
                    commands.show();
                    return false;
                }
            });
            findPreference("build_name").setSummary(R.string.app_version);
//            int plugins = AudioEngine.getTotalPlugins() ;
            int plugins = MainActivity.totalPlugins;
            findPreference("plugins").setSummary(String.valueOf(plugins));

            if (MainActivity.proVersion == false) {
                findPreference("encoders_supported").setSummary("WAV 16 Bit");
            }
        }
    }

    public static class TracksFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.tracks_settings, rootKey);
        }
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent imageReturnedIntent) {
        super.onActivityResult(requestCode, resultCode, imageReturnedIntent);
        Log.d(TITLE_TAG, "onActivityResult() called with: requestCode = [" + requestCode + "], resultCode = [" + resultCode + "], imageReturnedIntent = [" + imageReturnedIntent + "]");
        if (resultCode == RESULT_OK && requestCode == 1) {
            Uri selectedImage = imageReturnedIntent.getData();
            getContentResolver().takePersistableUriPermission(selectedImage, Intent.FLAG_GRANT_READ_URI_PERMISSION) ;
            Log.d(TITLE_TAG, "onActivityResult: " + selectedImage.getPath());

            SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
            sharedPreferences.edit().putString("background", selectedImage.toString()).commit();
            Log.d(TITLE_TAG, "onActivityResult: setting wallpaper: " + selectedImage.toString());
        }

        else if (resultCode == RESULT_OK && requestCode == 2) {
            Uri selectedImage = imageReturnedIntent.getData();
            getContentResolver().takePersistableUriPermission(selectedImage, Intent.FLAG_GRANT_READ_URI_PERMISSION) ;
            Log.d(TITLE_TAG, "onActivityResult: " + selectedImage.getPath());

            SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
            sharedPreferences.edit().putString("theme", selectedImage.toString()).commit();
            Log.d(TITLE_TAG, "onActivityResult: setting custom theme from folder: " + selectedImage.toString());
        }
    }
}