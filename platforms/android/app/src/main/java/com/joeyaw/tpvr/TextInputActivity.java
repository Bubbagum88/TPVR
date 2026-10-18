package com.joeyaw.tpvr;

import android.app.Activity;
import android.os.Bundle;
import android.text.InputFilter;
import android.text.InputType;
import android.view.Gravity;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

/**
 * Plain 2D text entry window for the VR build. While the game runs as an immersive OpenXR app
 * the Android keyboard can't appear over it, but a 2D activity shows as a Quest panel with the
 * system keyboard. DuskActivity.requestTextInput() opens this from a menu text field; the result
 * goes back through DuskActivity.deliverTextInput() and the native side picks it up by polling.
 */
public class TextInputActivity extends Activity {
    public static final String EXTRA_TEXT = "text";
    public static final String EXTRA_NUMBER = "number";
    public static final String EXTRA_MAX_LENGTH = "maxLength";

    private boolean delivered = false;
    private EditText editText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle("Enter text");

        editText = new EditText(this);
        editText.setSingleLine(true);
        editText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        if (getIntent().getBooleanExtra(EXTRA_NUMBER, false)) {
            editText.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED |
                                  InputType.TYPE_NUMBER_FLAG_DECIMAL);
        } else {
            editText.setInputType(InputType.TYPE_CLASS_TEXT);
        }
        int maxLength = getIntent().getIntExtra(EXTRA_MAX_LENGTH, -1);
        if (maxLength > 0) {
            editText.setFilters(new InputFilter[] {new InputFilter.LengthFilter(maxLength)});
        }
        String text = getIntent().getStringExtra(EXTRA_TEXT);
        if (text != null) {
            editText.setText(text);
            editText.selectAll();
        }
        editText.setOnEditorActionListener((view, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                finishWith(true);
                return true;
            }
            return false;
        });

        Button ok = new Button(this);
        ok.setText("OK");
        ok.setOnClickListener(view -> finishWith(true));
        Button cancel = new Button(this);
        cancel.setText("Cancel");
        cancel.setOnClickListener(view -> finishWith(false));

        LinearLayout buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.HORIZONTAL);
        buttons.setGravity(Gravity.END);
        buttons.addView(cancel);
        buttons.addView(ok);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = (int) (24 * getResources().getDisplayMetrics().density);
        root.setPadding(pad, pad, pad, pad);
        root.addView(editText, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        root.addView(buttons, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        setContentView(root);

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        editText.requestFocus();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            editText.post(() -> {
                InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
                if (imm != null) {
                    imm.showSoftInput(editText, InputMethodManager.SHOW_IMPLICIT);
                }
            });
        }
    }

    private void finishWith(boolean accepted) {
        if (!delivered) {
            delivered = true;
            DuskActivity.deliverTextInput(accepted ? editText.getText().toString() : null);
        }
        finish();
    }

    @Override
    protected void onDestroy() {
        // Closed some other way (back button, panel closed): treat as cancel.
        if (!delivered) {
            delivered = true;
            DuskActivity.deliverTextInput(null);
        }
        super.onDestroy();
    }
}
