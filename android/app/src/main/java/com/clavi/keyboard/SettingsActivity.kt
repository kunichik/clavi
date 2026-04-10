package com.clavi.keyboard

import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import android.view.inputmethod.InputMethodManager
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.setPadding

/**
 * Settings / onboarding activity.
 * - Guides user to enable Clavi as input method
 * - Lets user pick diacritics language (saved to SharedPreferences)
 */
class SettingsActivity : AppCompatActivity() {

    companion object {
        const val PREFS_NAME = "clavi_prefs"
        const val PREF_DIACRITICS_LOCALE = "diacritics_locale"

        // Displayed name → locale code (null = off)
        val DIACRITICS_OPTIONS = listOf(
            "Off (default)"         to null,
            "Portuguese (pt)"       to "pt",
            "German (de)"           to "de",
            "Norwegian (no)"        to "no",
            "French (fr)"           to "fr",
            "Spanish (es)"          to "es",
            "Swedish (sv)"          to "sv",
            "Finnish (fi)"          to "fi",
            "Polish (pl)"           to "pl",
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val dp = resources.displayMetrics.density
        val pad = (24 * dp).toInt()

        val scroll = ScrollView(this).apply {
            setBackgroundColor(0xFF263238.toInt())
        }

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad)
        }
        scroll.addView(layout)

        // ── Title ──
        layout.addView(TextView(this).apply {
            text = getString(R.string.app_name)
            textSize = 32f
            setTextColor(0xFF4FC3F7.toInt())
            setPadding(0, 0, 0, (8 * dp).toInt())
        })

        // ── Section: Enable keyboard ──
        layout.addView(sectionHeader("Setup", dp))

        layout.addView(TextView(this).apply {
            text = getString(R.string.enable_instructions)
            textSize = 16f
            setTextColor(0xFFFFFFFF.toInt())
            setPadding(0, 0, 0, (16 * dp).toInt())
        })

        layout.addView(Button(this).apply {
            text = "Enable Clavi Keyboard"
            setOnClickListener { startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS)) }
        })

        layout.addView(Button(this).apply {
            text = "Switch to Clavi"
            setOnClickListener {
                val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
                imm.showInputMethodPicker()
            }
        })

        val statusText = TextView(this).apply {
            textSize = 14f
            setTextColor(0xFFB0BEC5.toInt())
            setPadding(0, (8 * dp).toInt(), 0, (24 * dp).toInt())
            text = if (isClaviEnabled()) "✓ Clavi is enabled" else "✗ Clavi is not yet enabled"
        }
        layout.addView(statusText)

        // ── Section: Diacritics ──
        layout.addView(sectionHeader("Smart Diacritics", dp))

        layout.addView(TextView(this).apply {
            text = "Show diacritic variants (ã ü ø é…) in the strip above the keyboard. " +
                   "Pick your language — or Off if you don't need diacritics."
            textSize = 14f
            setTextColor(0xFFB0BEC5.toInt())
            setPadding(0, 0, 0, (12 * dp).toInt())
        })

        val prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
        val savedLocale = prefs.getString(PREF_DIACRITICS_LOCALE, null)
        val savedIndex = DIACRITICS_OPTIONS.indexOfFirst { it.second == savedLocale }.coerceAtLeast(0)

        val spinner = Spinner(this).apply {
            adapter = ArrayAdapter(
                this@SettingsActivity,
                android.R.layout.simple_spinner_dropdown_item,
                DIACRITICS_OPTIONS.map { it.first }
            )
            setSelection(savedIndex)
        }
        layout.addView(spinner)

        val saveBtn = Button(this).apply {
            text = "Save"
            setBackgroundColor(0xFF1565C0.toInt())
            setTextColor(0xFFFFFFFF.toInt())
            setPadding(0, (4 * dp).toInt(), 0, (4 * dp).toInt())
            setOnClickListener {
                val locale = DIACRITICS_OPTIONS[spinner.selectedItemPosition].second
                prefs.edit().apply {
                    if (locale == null) remove(PREF_DIACRITICS_LOCALE)
                    else putString(PREF_DIACRITICS_LOCALE, locale)
                }.apply()
                Toast.makeText(this@SettingsActivity,
                    if (locale == null) "Diacritics off" else "Diacritics: $locale",
                    Toast.LENGTH_SHORT).show()
            }
        }
        layout.addView(LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        ).let { params ->
            params.topMargin = (8 * dp).toInt()
            saveBtn.layoutParams = params
            saveBtn
        })

        setContentView(scroll)
    }

    private fun sectionHeader(title: String, dp: Float) = TextView(this).apply {
        text = title.uppercase()
        textSize = 11f
        letterSpacing = 0.1f
        setTextColor(0xFF4FC3F7.toInt())
        setPadding(0, (8 * dp).toInt(), 0, (4 * dp).toInt())
    }

    private fun isClaviEnabled(): Boolean {
        val enabledMethods = Settings.Secure.getString(
            contentResolver, Settings.Secure.ENABLED_INPUT_METHODS
        ) ?: return false
        return enabledMethods.contains(packageName)
    }
}
