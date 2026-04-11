package com.clavi.keyboard

import android.content.Context
import java.io.IOException

/**
 * Emoji catalogue — loads emoji.tsv from assets once, lazily.
 *
 * TSV format: emoji<TAB>keyword1 keyword2 keyword3
 * Empty line / line starting with '#' = skipped.
 *
 * Provides:
 *   - [all]         full list in file order
 *   - [search(q)]   filter by keyword prefix
 */
object EmojiData {

    data class Entry(val emoji: String, val keywords: List<String>)

    private var loaded = false
    private val entries = mutableListOf<Entry>()

    val all: List<String> get() { return entries.map { it.emoji } }

    fun load(context: Context) {
        if (loaded) return
        try {
            context.assets.open("emoji/emoji.tsv").bufferedReader().lineSequence()
                .filter { it.isNotBlank() && !it.startsWith('#') }
                .forEach { line ->
                    val tab = line.indexOf('\t')
                    if (tab < 0) return@forEach
                    val emoji = line.substring(0, tab).trim()
                    val kws = line.substring(tab + 1).trim().split(' ').filter { it.isNotBlank() }
                    if (emoji.isNotEmpty()) entries.add(Entry(emoji, kws))
                }
        } catch (_: IOException) { /* graceful degradation */ }
        loaded = true
    }

    fun search(query: String): List<String> {
        if (query.isBlank()) return all
        val q = query.trim().lowercase()
        return entries.filter { e ->
            e.keywords.any { kw -> kw.startsWith(q) }
        }.map { it.emoji }
    }
}
