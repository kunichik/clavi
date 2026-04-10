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
        const val PREF_DEFAULT_LANGUAGE = "default_language"
        const val PREF_HAPTIC = "haptic_feedback"
        const val PREF_TRANSLATION_SOURCE = "translation_source_lang"
        const val PREF_TRANSLATION_TARGET = "translation_target_lang"

        // Translation language options: display name → BCP 47 code (null = Off)
        val TRANSLATION_LANGUAGES = listOf(
            "Off"                    to null,
            "English (en)"           to "en",
            "Ukrainian (uk)"         to "uk",
            "German (de)"            to "de",
            "French (fr)"            to "fr",
            "Spanish (es)"           to "es",
            "Portuguese (pt)"        to "pt",
            "Norwegian (no)"         to "no",
            "Chinese (zh)"           to "zh",
            "Japanese (ja)"          to "ja",
        )

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
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).also { it.topMargin = (8 * dp).toInt() }
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
        layout.addView(saveBtn)

        // ── Section: Default Language ──
        layout.addView(sectionHeader("Default Language", dp))

        layout.addView(TextView(this).apply {
            text = "The keyboard language shown when the keyboard first opens."
            textSize = 14f
            setTextColor(0xFFB0BEC5.toInt())
            setPadding(0, 0, 0, (12 * dp).toInt())
        })

        val langOptions = listOf("Ukrainian (УК)" to Language.UK.name, "English (EN)" to Language.EN.name, "K'iche' (Q')" to Language.QUC.name)
        val savedDefaultLang = prefs.getString(PREF_DEFAULT_LANGUAGE, Language.UK.name)
        val langSpinner = Spinner(this).apply {
            adapter = ArrayAdapter(this@SettingsActivity, android.R.layout.simple_spinner_dropdown_item, langOptions.map { it.first })
            setSelection(langOptions.indexOfFirst { it.second == savedDefaultLang }.coerceAtLeast(0))
        }
        layout.addView(langSpinner)

        val langSaveBtn = Button(this).apply {
            text = "Save Language"
            setBackgroundColor(0xFF1565C0.toInt())
            setTextColor(0xFFFFFFFF.toInt())
            setOnClickListener {
                val lang = langOptions[langSpinner.selectedItemPosition].second
                prefs.edit().putString(PREF_DEFAULT_LANGUAGE, lang).apply()
                Toast.makeText(this@SettingsActivity, "Default: ${langOptions[langSpinner.selectedItemPosition].first}", Toast.LENGTH_SHORT).show()
            }
        }
        layout.addView(langSaveBtn)

        // ── Section: Translation ──
        layout.addView(sectionHeader("Translation", dp))

        layout.addView(TextView(this).apply {
            text = "Translate while you type. After each space, a blue strip shows the translation " +
                   "— tap to replace your text. First use downloads ~30MB model (WiFi only)."
            textSize = 14f
            setTextColor(0xFFB0BEC5.toInt())
            setPadding(0, 0, 0, (12 * dp).toInt())
        })

        val savedSrc = prefs.getString(PREF_TRANSLATION_SOURCE, null)
        val savedTgt = prefs.getString(PREF_TRANSLATION_TARGET, null)
        val srcIndex = TRANSLATION_LANGUAGES.indexOfFirst { it.second == savedSrc }.coerceAtLeast(0)
        val tgtIndex = TRANSLATION_LANGUAGES.indexOfFirst { it.second == savedTgt }.coerceAtLeast(1)

        layout.addView(TextView(this).apply {
            text = "Source language"
            textSize = 13f
            setTextColor(0xFFFFFFFF.toInt())
            setPadding(0, (8 * dp).toInt(), 0, 0)
        })
        val srcSpinner = Spinner(this).apply {
            adapter = ArrayAdapter(
                this@SettingsActivity,
                android.R.layout.simple_spinner_dropdown_item,
                TRANSLATION_LANGUAGES.map { it.first }
            )
            setSelection(srcIndex)
        }
        layout.addView(srcSpinner)

        layout.addView(TextView(this).apply {
            text = "Target language"
            textSize = 13f
            setTextColor(0xFFFFFFFF.toInt())
            setPadding(0, (8 * dp).toInt(), 0, 0)
        })
        val tgtSpinner = Spinner(this).apply {
            adapter = ArrayAdapter(
                this@SettingsActivity,
                android.R.layout.simple_spinner_dropdown_item,
                TRANSLATION_LANGUAGES.map { it.first }
            )
            setSelection(tgtIndex)
        }
        layout.addView(tgtSpinner)

        val transSaveBtn = Button(this).apply {
            text = "Save Translation Settings"
            setBackgroundColor(0xFF1565C0.toInt())
            setTextColor(0xFFFFFFFF.toInt())
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).also { it.topMargin = (8 * dp).toInt() }
            setOnClickListener {
                val src = TRANSLATION_LANGUAGES[srcSpinner.selectedItemPosition].second
                val tgt = TRANSLATION_LANGUAGES[tgtSpinner.selectedItemPosition].second
                prefs.edit().apply {
                    if (src == null) remove(PREF_TRANSLATION_SOURCE) else putString(PREF_TRANSLATION_SOURCE, src)
                    if (tgt == null) remove(PREF_TRANSLATION_TARGET) else putString(PREF_TRANSLATION_TARGET, tgt)
                }.apply()
                val msg = if (src == null) "Translation off"
                          else "Translation: $src → $tgt"
                Toast.makeText(this@SettingsActivity, msg, Toast.LENGTH_SHORT).show()
            }
        }
        layout.addView(transSaveBtn)

        // ── Section: Haptic Feedback ──
        layout.addView(sectionHeader("Haptic Feedback", dp))

        val hapticToggle = ToggleButton(this).apply {
            textOn = "Vibration: On"
            textOff = "Vibration: Off"
            isChecked = prefs.getBoolean(PREF_HAPTIC, true)
            setOnCheckedChangeListener { _, isChecked ->
                prefs.edit().putBoolean(PREF_HAPTIC, isChecked).apply()
            }
        }
        layout.addView(hapticToggle)

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
