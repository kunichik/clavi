package com.clavi.keyboard

import android.content.ClipboardManager
import android.content.Context

/**
 * Tracks clipboard history and notifies listeners on changes.
 * The IME context has elevated clipboard access (unlike regular apps on Android 10+).
 *
 * Stores up to MAX_ITEMS distinct entries, newest first.
 */
class ClipboardHistory(private val context: Context) {

    interface OnChangeListener {
        fun onClipboardChanged(clips: List<String>)
    }

    private val MAX_ITEMS = 20
    private val MAX_LABEL_LEN = 80  // truncate long clips in the strip

    private val clips = ArrayDeque<String>()
    var listener: OnChangeListener? = null

    private val clipboardManager: ClipboardManager by lazy {
        context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
    }

    private val primaryClipListener = ClipboardManager.OnPrimaryClipChangedListener {
        val text = clipboardManager.primaryClip
            ?.getItemAt(0)
            ?.coerceToText(context)
            ?.toString()
            ?.trim()
            ?: return@OnPrimaryClipChangedListener

        if (text.isBlank() || text.length > 2000) return@OnPrimaryClipChangedListener

        // Remove duplicate if already in history, then add to front
        clips.removeAll { it == text }
        clips.addFirst(text)
        if (clips.size > MAX_ITEMS) clips.removeLast()

        listener?.onClipboardChanged(getRecent())
    }

    fun startListening() {
        clipboardManager.addPrimaryClipChangedListener(primaryClipListener)
        // Seed with current clipboard content if available
        primaryClipListener.onPrimaryClipChanged()
    }

    fun stopListening() {
        clipboardManager.removePrimaryClipChangedListener(primaryClipListener)
    }

    /** Returns recent clips as display labels (truncated). Full text preserved internally. */
    fun getRecent(): List<String> = clips.toList()

    /** Returns the full text for a given display label index. */
    fun getFullText(index: Int): String? = clips.getOrNull(index)

    fun removeAt(index: Int) {
        if (index in clips.indices) {
            clips.removeAt(index)
            listener?.onClipboardChanged(getRecent())
        }
    }

    fun clear() {
        clips.clear()
        listener?.onClipboardChanged(emptyList())
    }

    /** Truncate for display in the strip chip. */
    fun displayLabel(text: String): String {
        val single = text.replace('\n', ' ').replace('\r', ' ')
        return if (single.length <= MAX_LABEL_LEN) single
               else single.take(MAX_LABEL_LEN - 1) + "\u2026"
    }
}
