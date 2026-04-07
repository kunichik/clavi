package com.clavi.keyboard

import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import android.view.inputmethod.InputMethodManager
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.setPadding

/**
 * Simple settings / onboarding activity.
 * Guides the user to enable Clavi as an input method.
 */
class SettingsActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val dp = resources.displayMetrics.density
        val pad = (24 * dp).toInt()

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad)
        }

        val title = TextView(this).apply {
            text = getString(R.string.app_name)
            textSize = 32f
            setTextColor(0xFF4FC3F7.toInt())
            setPadding(0, 0, 0, (16 * dp).toInt())
        }

        val instructions = TextView(this).apply {
            text = getString(R.string.enable_instructions)
            textSize = 16f
            setTextColor(0xFFFFFFFF.toInt())
            setPadding(0, 0, 0, (24 * dp).toInt())
        }

        val enableButton = Button(this).apply {
            text = "Enable Clavi Keyboard"
            setOnClickListener {
                startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
            }
        }

        val switchButton = Button(this).apply {
            text = "Switch to Clavi"
            setOnClickListener {
                val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
                imm.showInputMethodPicker()
            }
        }

        val statusText = TextView(this).apply {
            textSize = 14f
            setTextColor(0xFFB0BEC5.toInt())
            setPadding(0, (24 * dp).toInt(), 0, 0)
            text = if (isClaviEnabled()) {
                "Clavi is enabled"
            } else {
                "Clavi is not yet enabled as an input method"
            }
        }

        layout.addView(title)
        layout.addView(instructions)
        layout.addView(enableButton)
        layout.addView(switchButton)
        layout.addView(statusText)

        layout.setBackgroundColor(0xFF263238.toInt())
        setContentView(layout)
    }

    private fun isClaviEnabled(): Boolean {
        val enabledMethods = Settings.Secure.getString(
            contentResolver,
            Settings.Secure.ENABLED_INPUT_METHODS
        ) ?: return false
        return enabledMethods.contains(packageName)
    }
}
