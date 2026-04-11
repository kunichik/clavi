package com.clavi.keyboard

import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.View
import android.view.inputmethod.EditorInfo
import android.widget.*
import androidx.core.view.setPadding

/**
 * Emoji picker panel — overlays the keyboard when triggered by long-press on space.
 *
 * Layout (top → bottom):
 *   [Header: "Emoji" title + dismiss ×]
 *   [Recent strip — last 8 used emoji]
 *   [Search bar]
 *   [Scrollable emoji grid — 8 columns]
 */
class EmojiPanel(
    context: Context,
    private val prefs: android.content.SharedPreferences,
) : LinearLayout(context) {

    var onEmojiSelected: ((String) -> Unit)? = null
    var onDismiss: (() -> Unit)? = null

    private val recentRow = LinearLayout(context)
    private val gridContainer = LinearLayout(context)
    private val searchEdit = EditText(context)

    private val dp = context.resources.displayMetrics.density

    companion object {
        private const val PREF_RECENT = "emoji_recent"
        private const val COLS = 8
    }

    init {
        orientation = VERTICAL
        setBackgroundColor(Color.argb(255, 20, 26, 30))

        addView(buildHeader())
        addView(buildRecentStrip())
        addView(buildSearchBar())

        val scroll = ScrollView(context).apply {
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f)
        }
        gridContainer.orientation = VERTICAL
        gridContainer.layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT)
        scroll.addView(gridContainer)
        addView(scroll)

        EmojiData.load(context)
        rebuildGrid(EmojiData.all)
    }

    // MARK: - Header

    private fun buildHeader(): View {
        val row = LinearLayout(context).apply {
            orientation = HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding((10 * dp).toInt())
            setBackgroundColor(Color.argb(255, 28, 35, 42))
        }
        row.addView(TextView(context).apply {
            text = "Emoji"
            textSize = 14f
            setTextColor(Color.WHITE)
            typeface = Typeface.DEFAULT_BOLD
        }, LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))

        row.addView(TextView(context).apply {
            text = "✕"
            textSize = 18f
            setTextColor(Color.argb(200, 255, 100, 100))
            setPadding((8 * dp).toInt())
            isClickable = true
            isFocusable = true
            setOnClickListener { onDismiss?.invoke() }
        })
        return row
    }

    // MARK: - Recent strip

    private fun buildRecentStrip(): View {
        val scroll = HorizontalScrollView(context).apply {
            setBackgroundColor(Color.argb(255, 24, 30, 36))
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, (48 * dp).toInt())
        }
        recentRow.orientation = HORIZONTAL
        recentRow.gravity = Gravity.CENTER_VERTICAL
        recentRow.setPadding((6 * dp).toInt())
        scroll.addView(recentRow)
        refreshRecentStrip()
        return scroll
    }

    private fun refreshRecentStrip() {
        recentRow.removeAllViews()
        val recent = loadRecent()
        if (recent.isEmpty()) {
            recentRow.addView(TextView(context).apply {
                text = "No recent emoji"
                textSize = 11f
                setTextColor(Color.argb(100, 255, 255, 255))
                setPadding((8 * dp).toInt())
            })
        } else {
            recent.forEach { emoji ->
                recentRow.addView(emojiButton(emoji))
            }
        }
    }

    // MARK: - Search bar

    private fun buildSearchBar(): View {
        searchEdit.apply {
            hint = "Search emoji…"
            setHintTextColor(Color.argb(120, 255, 255, 255))
            setTextColor(Color.WHITE)
            textSize = 14f
            background = null
            setPadding((12 * dp).toInt(), (8 * dp).toInt(), (12 * dp).toInt(), (8 * dp).toInt())
            inputType = EditorInfo.TYPE_CLASS_TEXT
            imeOptions = EditorInfo.IME_ACTION_SEARCH
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT)
            setBackgroundColor(Color.argb(255, 34, 42, 50))
            addTextChangedListener(object : TextWatcher {
                override fun afterTextChanged(s: Editable?) {
                    val q = s?.toString() ?: ""
                    rebuildGrid(if (q.isBlank()) EmojiData.all else EmojiData.search(q))
                }
                override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
                override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
            })
        }
        return searchEdit
    }

    // MARK: - Grid

    private fun rebuildGrid(items: List<String>) {
        gridContainer.removeAllViews()
        items.chunked(COLS).forEach { rowItems ->
            val row = LinearLayout(context).apply {
                orientation = HORIZONTAL
                layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT)
            }
            rowItems.forEach { emoji -> row.addView(emojiButton(emoji), LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f)) }
            // Fill remaining cells for alignment
            repeat(COLS - rowItems.size) {
                row.addView(View(context), LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))
            }
            gridContainer.addView(row)
        }
    }

    // MARK: - Emoji button

    private fun emojiButton(emoji: String): TextView = TextView(context).apply {
        text = emoji
        textSize = 24f
        gravity = Gravity.CENTER
        setPadding((4 * dp).toInt())
        isClickable = true
        isFocusable = true
        setOnClickListener {
            onEmojiSelected?.invoke(emoji)
            addToRecent(emoji)
        }
    }

    // MARK: - Recent persistence

    private fun loadRecent(): List<String> {
        val raw = prefs.getString(PREF_RECENT, "") ?: ""
        return raw.split(",").filter { it.isNotBlank() }
    }

    private fun addToRecent(emoji: String) {
        val list = loadRecent().toMutableList()
        list.remove(emoji)
        list.add(0, emoji)
        prefs.edit().putString(PREF_RECENT, list.take(8).joinToString(",")).apply()
        refreshRecentStrip()
    }
}
