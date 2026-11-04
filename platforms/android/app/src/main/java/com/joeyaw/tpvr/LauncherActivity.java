package com.joeyaw.tpvr;

import android.app.Activity;
import android.app.ActivityOptions;
import android.Manifest;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;

// The app's entry point on the Quest: a plain 2D panel (no VR category) for the
// setup the game's own prelaunch screen can't do on a headset. Started as a VR
// app, DuskActivity's window is never shown, so that screen is invisible, and
// the system disc picker breaks the hidden window (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR,
// 2026-11-04). Started from a 2D launcher it worked, so setup lives here: choose
// the disc image, grant file access, then start the game in VR.
public class LauncherActivity extends Activity {
    private static final String TAG = "TPVRLauncher";
    private static final String PREFS_NAME = "launcher";
    private static final String PREF_DISC_PATH = "discPath";
    private static final int PICK_DISC_REQUEST_CODE = 0x5450;
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 0x5451;

    private TextView discLabel;
    private TextView messageLabel;
    private Button storageButton;
    private Button playButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER_HORIZONTAL);
        int pad = (int) (24 * getResources().getDisplayMetrics().density);
        layout.setPadding(pad, pad, pad, pad);

        TextView title = new TextView(this);
        title.setText(R.string.app_name);
        title.setTextSize(28);
        title.setGravity(Gravity.CENTER);
        layout.addView(title);

        discLabel = new TextView(this);
        discLabel.setTextSize(16);
        discLabel.setPadding(0, pad, 0, pad / 2);
        layout.addView(discLabel);

        Button chooseButton = new Button(this);
        chooseButton.setText("Choose disc image");
        chooseButton.setOnClickListener(v -> openDiscPicker());
        layout.addView(chooseButton);

        storageButton = new Button(this);
        storageButton.setText("Allow file access");
        storageButton.setOnClickListener(v -> requestStorageAccess());
        layout.addView(storageButton);

        playButton = new Button(this);
        playButton.setText("Play in VR");
        playButton.setOnClickListener(v -> startGame());
        layout.addView(playButton);

        messageLabel = new TextView(this);
        messageLabel.setTextSize(14);
        messageLabel.setPadding(0, pad / 2, 0, 0);
        layout.addView(messageLabel);

        setContentView(layout);
    }

    @Override
    protected void onResume() {
        super.onResume();
        refresh();
    }

    private String savedDiscPath() {
        return getSharedPreferences(PREFS_NAME, MODE_PRIVATE).getString(PREF_DISC_PATH, "");
    }

    // Android 10 (the Quest 1) has no all-files access: reading a path in
    // shared storage needs READ_EXTERNAL_STORAGE, granted at run time, plus
    // requestLegacyExternalStorage in the manifest. Without it the file can
    // still be found (stat works) but not opened, which is why the game got
    // ValidationError=1 (IOError) for the disc the launcher saw (2026-11-04).
    private boolean hasStorageAccess() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED;
        }
        return Environment.isExternalStorageManager();
    }

    private void refresh() {
        String path = savedDiscPath();
        File disc = new File(path);
        boolean haveDisc = !path.isEmpty() && disc.isFile() && disc.canRead();
        boolean storage = hasStorageAccess();
        if (path.isEmpty()) {
            discLabel.setText("Disc image: none chosen");
        } else if (!storage) {
            discLabel.setText("Disc image: " + path);
        } else if (!haveDisc) {
            discLabel.setText("Disc image: " + path + " (not found)");
        } else {
            discLabel.setText("Disc image: " + path);
        }
        storageButton.setVisibility(storage ? android.view.View.GONE : android.view.View.VISIBLE);
        playButton.setEnabled(storage && haveDisc);
    }

    private void openDiscPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT)
            .addCategory(Intent.CATEGORY_OPENABLE)
            .setType("*/*");
        try {
            startActivityForResult(intent, PICK_DISC_REQUEST_CODE);
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "No document picker", e);
            messageLabel.setText("No file picker is available on this device.");
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != PICK_DISC_REQUEST_CODE) {
            super.onActivityResult(requestCode, resultCode, data);
            return;
        }
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        Uri uri = data.getData();
        String path = pathFromDocumentUri(uri);
        Log.i(TAG, "Picked " + uri + " -> " + path);
        if (path == null) {
            messageLabel.setText("Pick the disc image from the headset's own storage (Internal shared storage).");
            return;
        }
        getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit().putString(PREF_DISC_PATH, path).apply();
        messageLabel.setText("");
        refresh();
    }

    // The game reads the disc by file path: content:// reads of the disc were
    // wrong in October (hash mismatch, then garbage archive headers), while a
    // plain path to the same file works. So turn the picked document into the
    // file path behind it, for the two providers that expose one.
    private static String pathFromDocumentUri(Uri uri) {
        if (!"content".equals(uri.getScheme())) {
            return null;
        }
        String id;
        try {
            id = DocumentsContract.getDocumentId(uri);
        } catch (IllegalArgumentException e) {
            return null;
        }
        String authority = uri.getAuthority();
        if ("com.android.externalstorage.documents".equals(authority) && id.startsWith("primary:")) {
            return new File(Environment.getExternalStorageDirectory(), id.substring("primary:".length())).getPath();
        }
        if (id.startsWith("raw:")) {
            return id.substring("raw:".length());
        }
        return null;
    }

    private void requestStorageAccess() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            requestPermissions(new String[] {Manifest.permission.READ_EXTERNAL_STORAGE},
                STORAGE_PERMISSION_REQUEST_CODE);
            return;
        }
        try {
            startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
                .setData(Uri.parse("package:" + getPackageName())));
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "No all-files access settings screen", e);
            messageLabel.setText("Could not open the file access settings.");
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode != STORAGE_PERMISSION_REQUEST_CODE) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
            return;
        }
        if (!hasStorageAccess()) {
            messageLabel.setText("File access was not allowed, so the game can't read the disc.");
        }
        refresh();
    }

    private void startGame() {
        String path = savedDiscPath();
        // A 2D panel lives on its own virtual display. Started plainly from it,
        // the game would join the panel's task on that display and never get
        // the headset (found in the PrimedGun 2.0 port on the same Quest 1). A
        // new task on the main display, as `adb shell am start` does, makes it
        // the immersive app.
        Intent intent = new Intent(this, DuskActivity.class)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            .putExtra("dusk_argv", new String[] {"--dvd", path});
        ActivityOptions options = ActivityOptions.makeBasic().setLaunchDisplayId(Display.DEFAULT_DISPLAY);
        try {
            startActivity(intent, options.toBundle());
            Log.i(TAG, "Started the game with " + path);
            finish();
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "Cannot start the game activity", e);
            messageLabel.setText("Could not start the game.");
        }
    }
}
