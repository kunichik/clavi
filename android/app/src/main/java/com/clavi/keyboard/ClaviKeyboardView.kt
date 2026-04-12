package com.clavi.keyboard

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import android.os.Handler
import android.os.Looper
import android.util.AttributeSet
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import androidx.core.content.ContextCompat

/**
 * Custom-drawn keyboard view.
 *
 * Layout (top → bottom):
 *   [Clipboard strip — scrollable chips, ~40dp]
 *   [Keyboard rows]
 */
class ClaviKeyboardView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : View(context, attrs) {

    interface OnKeyListener {
        fun onKey(key: Key)
        fun onSpaceLongPress()
    }

    interface OnStripListener {
        fun onClipTap(index: Int)
        fun onClipLongPress(index: Int)
        fun onStripClear()
        fun onDiacriticTap(variant: String)
        fun onFixTap(fix: TextFixEngine.Fix)
        fun onTranslationTap(suggestion: TranslationEngine.TranslationSuggestion)
        fun onTranslationDismiss()
        fun onPredictionTap(word: String)
        fun onShortcutTranslit()
        fun onShortcutSettings()
    }

    var listener: OnKeyListener? = null
    var stripListener: OnStripListener? = null
    var translitActive = false
    var currentLanguage = Language.UK
    var hapticEnabled = true

    // Clipboard strip data — set from ClaviIME
    var clipItems: List<String> = emptyList()
        set(value) { field = value; invalidate() }

    // Diacritics suggestion strip — overrides clipboard strip when non-empty
    var diacriticItems: List<String> = emptyList()
        set(value) { field = value; invalidate() }

    // Text fix suggestion — highest priority, overrides all other strips
    var fixSuggestion: TextFixEngine.Fix? = null
        set(value) { field = value; invalidate() }

    // Translation suggestion — second priority (after fix)
    var translationSuggestion: TranslationEngine.TranslationSuggestion? = null
        set(value) { field = value; invalidate() }

    // Word predictions — shown when no reactive strip is active
    var predictionItems: List<String> = emptyList()
        set(value) { field = value; invalidate() }

    // Strip priority: fix > translation > diacritics > predictions > clipboard > shortcuts
    private val stripShowsFix         get() = fixSuggestion != null
    private val stripShowsTranslation get() = !stripShowsFix && translationSuggestion != null
    private val stripShowsDiacritics  get() = !stripShowsFix && !stripShowsTranslation && diacriticItems.isNotEmpty()
    private val stripShowsPredictions get() = !stripShowsFix && !stripShowsTranslation && !stripShowsDiacritics && predictionItems.isNotEmpty()
    private val stripShowsShortcuts   get() = !stripShowsFix && !stripShowsTranslation && !stripShowsDiacritics && !stripShowsPredictions && clipItems.isEmpty()

    private var rows: List<Row> = emptyList()
    private val keyRects = mutableListOf<Pair<RectF, Key>>()

    // Strip chip rects: index → RectF
    private val chipRects = mutableListOf<RectF>()
    private var clearButtonRect = RectF()
    private var stripHeight = 0f
    private var pressedChipIndex = -1

    // Paints
    private val keyBgPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val keySpecialBgPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val keyTextPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val bgPaint = Paint()
    private val translitPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val stripBgPaint = Paint()
    private val chipBgPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val chipPressedPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val chipTextPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val clearBtnPaint = Paint(Paint.ANTI_ALIAS_FLAG)

    private val keyRadius = 8f
    private val keyPadding = 3f
    private val rowPadding = 4f
    private val chipRadius = 6f
    private val chipPaddingH = 12f  // dp
    private val chipPaddingV = 6f   // dp
    private val chipSpacing = 6f    // dp

    private var pressedKey: Key? = null

    // Long press detection for clip chips
    private val longPressHandler = Handler(Looper.getMainLooper())
    private var longPressPending = false

    // Long press detection for backspace key → repeat deletion
    private var backspaceRepeatRunnable: Runnable? = null

    // Long press detection for space key → emoji panel
    private var spaceLongPressPending = false
    private val spaceLongPressRunnable = Runnable {
        spaceLongPressPending = false
        pressedKey = null
        invalidate()
        performHapticFeedback(HapticFeedbackConstants.LONG_PRESS)
        listener?.onSpaceLongPress()
    }
    private var longPressChipIndex = -1
    private val longPressRunnable = Runnable {
        val idx = longPressChipIndex
        if (idx >= 0) {
            performHapticFeedback(HapticFeedbackConstants.LONG_PRESS)
            stripListener?.onClipLongPress(idx)
            pressedChipIndex = -1
            longPressPending = false
            invalidate()
        }
    }

    // Colors
    private val keyBgColor: Int
    private val keyBgPressedColor: Int
    private val keySpecialBgColor: Int
    private val keyTextColor: Int
    private val keyboardBgColor: Int
    private val translitActiveColor: Int
    private val langIndicatorColor: Int
    private val stripBgColor: Int
    private val chipBgColor: Int
    private val chipTextColor: Int

    init {
        keyBgColor       = ContextCompat.getColor(context, R.color.key_bg)
        keyBgPressedColor= ContextCompat.getColor(context, R.color.key_bg_pressed)
        keySpecialBgColor= ContextCompat.getColor(context, R.color.key_special_bg)
        keyTextColor     = ContextCompat.getColor(context, R.color.key_text)
        keyboardBgColor  = ContextCompat.getColor(context, R.color.keyboard_bg)
        translitActiveColor = ContextCompat.getColor(context, R.color.translit_active)
        langIndicatorColor  = ContextCompat.getColor(context, R.color.lang_indicator)
        stripBgColor     = ContextCompat.getColor(context, R.color.strip_bg)
        chipBgColor      = ContextCompat.getColor(context, R.color.strip_chip_bg)
        chipTextColor    = ContextCompat.getColor(context, R.color.strip_chip_text)

        bgPaint.color = keyboardBgColor
        stripBgPaint.color = stripBgColor
        keyBgPaint.color = keyBgColor
        keySpecialBgPaint.color = keySpecialBgColor

        keyTextPaint.color = keyTextColor
        keyTextPaint.textAlign = Paint.Align.CENTER
        keyTextPaint.typeface = Typeface.DEFAULT

        translitPaint.color = translitActiveColor
        translitPaint.textAlign = Paint.Align.CENTER
        translitPaint.typeface = Typeface.DEFAULT_BOLD

        chipBgPaint.color = chipBgColor
        chipPressedPaint.color = keyBgPressedColor

        chipTextPaint.color = chipTextColor
        chipTextPaint.textAlign = Paint.Align.LEFT

        clearBtnPaint.color = Color.argb(180, 255, 100, 100)
        clearBtnPaint.textAlign = Paint.Align.CENTER
    }

    fun setLayout(rows: List<Row>) {
        this.rows = rows
        requestLayout()
        invalidate()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val w = MeasureSpec.getSize(widthMeasureSpec)
        val density = resources.displayMetrics.density
        val rowCount = rows.size.coerceAtLeast(4)
        val keyHeight = (w * 0.13f).toInt()
        val rp = rowPadding * density
        val keysH = (keyHeight * rowCount + rp * (rowCount + 1)).toInt()

        // Strip height: always 40dp so the input field never jumps
        stripHeight = 40f * density

        setMeasuredDimension(w, keysH + stripHeight.toInt())
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawRect(0f, 0f, width.toFloat(), height.toFloat(), bgPaint)

        val density = resources.displayMetrics.density

        // ── Strip (fix > translation > diacritics > predictions > clipboard > shortcuts) ──
        when {
            stripShowsFix          -> drawFixStrip(canvas, density)
            stripShowsTranslation  -> drawTranslationStrip(canvas, density)
            stripShowsDiacritics   -> drawDiacriticsStrip(canvas, density)
            stripShowsPredictions  -> drawPredictionsStrip(canvas, density)
            clipItems.isNotEmpty() -> drawStrip(canvas, density)
            else                   -> drawShortcutStrip(canvas, density)
        }

        // ── Keyboard rows ──
        drawKeys(canvas, density, stripHeight)
    }

    private fun drawStrip(canvas: Canvas, density: Float) {
        chipRects.clear()

        val sh = stripHeight
        canvas.drawRect(0f, 0f, width.toFloat(), sh, stripBgPaint)

        val chipH = sh - chipPaddingV * density * 2
        val chipTop = chipPaddingV * density
        val chipBottom = chipTop + chipH
        val chipR = chipRadius * density
        val textSize = 13f * density
        chipTextPaint.textSize = textSize
        clearBtnPaint.textSize = textSize

        var x = chipPaddingH * density

        // Clipboard icon chip (non-tappable label)
        val iconPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(120, 255, 255, 255)
            this.textSize = textSize
            textAlign = Paint.Align.LEFT
        }
        canvas.drawText("\uD83D\uDCCB", x, chipBottom - chipPaddingV * density * 0.5f, iconPaint)
        x += 28f * density

        // Clip chips
        clipItems.forEachIndexed { i, clip ->
            val label = if (clip.length > 30) clip.take(29) + "\u2026" else clip.replace('\n', ' ')
            val textW = chipTextPaint.measureText(label)
            val chipW = textW + chipPaddingH * density * 2
            if (x + chipW > width - 36f * density) return@forEachIndexed  // stop if no room

            val rect = RectF(x, chipTop, x + chipW, chipBottom)
            chipRects.add(rect)

            val bgP = if (pressedChipIndex == i) chipPressedPaint else chipBgPaint
            canvas.drawRoundRect(rect, chipR, chipR, bgP)
            canvas.drawText(label, x + chipPaddingH * density, chipBottom - chipPaddingV * density * 0.8f, chipTextPaint)

            x += chipW + chipSpacing * density
        }

        // Clear button (✕) — right side
        val clearW = 32f * density
        val clearRect = RectF(width - clearW - 4f * density, chipTop, width - 4f * density, chipBottom)
        clearButtonRect = clearRect
        canvas.drawRoundRect(clearRect, chipR, chipR, chipBgPaint)
        canvas.drawText("\u00D7", clearRect.centerX(), chipBottom - chipPaddingV * density * 0.8f, clearBtnPaint)
    }

    private fun drawFixStrip(canvas: Canvas, density: Float) {
        chipRects.clear()
        clearButtonRect = RectF()
        val fix = fixSuggestion ?: return

        val sh = stripHeight
        // Green tint background — "suggestion available"
        canvas.drawRect(0f, 0f, width.toFloat(), sh, Paint().apply { color = Color.argb(255, 22, 48, 35) })

        val chipH = sh - chipPaddingV * density * 2
        val chipTop = chipPaddingV * density
        val chipBottom = chipTop + chipH
        val chipR = chipRadius * density
        chipTextPaint.textSize = 13f * density
        chipTextPaint.textAlign = Paint.Align.LEFT

        var x = 8f * density

        // Label
        val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(160, 255, 255, 255)
            textSize = 11f * density
            textAlign = Paint.Align.LEFT
        }
        canvas.drawText("✦ fix:", x, chipBottom - chipPaddingV * density * 0.5f, labelPaint)
        x += labelPaint.measureText("✦ fix:") + 8f * density

        // Show corrected text preview (truncated)
        val preview = fix.fixed.trimEnd().takeLast(40).let { if (fix.fixed.length > 40) "…$it" else it }
        val previewW = chipTextPaint.measureText(preview)
        val chipW = previewW + chipPaddingH * density * 2

        val rect = RectF(x, chipTop, x + chipW, chipBottom)
        chipRects.add(rect)

        val bgColor = if (pressedChipIndex == 0) keyBgPressedColor
                      else Color.argb(220, 39, 174, 96)  // green
        canvas.drawRoundRect(rect, chipR, chipR, Paint(Paint.ANTI_ALIAS_FLAG).apply { color = bgColor })
        chipTextPaint.color = Color.WHITE
        canvas.drawText(preview, rect.left + chipPaddingH * density, chipBottom - chipPaddingV * density * 0.8f, chipTextPaint)
        chipTextPaint.color = chipTextColor   // reset

        x += chipW + chipSpacing * density

        // Dismiss (✕) chip
        val dismissW = 36f * density
        val dismissRect = RectF(x, chipTop, x + dismissW, chipBottom)
        chipRects.add(dismissRect)  // index 1 = dismiss
        canvas.drawRoundRect(dismissRect, chipR, chipR, Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(120, 120, 120, 120)
        })
        clearBtnPaint.textSize = 14f * density
        canvas.drawText("✕", dismissRect.centerX(), chipBottom - chipPaddingV * density * 0.8f, clearBtnPaint)

        // Reason hint
        val hintPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(100, 255, 255, 255)
            textSize = 10f * density
            textAlign = Paint.Align.RIGHT
        }
        canvas.drawText(fix.description, width - 8f * density, chipBottom - chipPaddingV * density * 0.5f, hintPaint)
    }

    private fun drawTranslationStrip(canvas: Canvas, density: Float) {
        chipRects.clear()
        clearButtonRect = RectF()
        val suggestion = translationSuggestion ?: return

        val sh = stripHeight
        // Deep blue background — "translation available"
        canvas.drawRect(0f, 0f, width.toFloat(), sh,
            Paint().apply { color = Color.argb(255, 13, 71, 161) })

        val chipH = sh - chipPaddingV * density * 2
        val chipTop = chipPaddingV * density
        val chipBottom = chipTop + chipH
        val chipR = chipRadius * density
        chipTextPaint.textSize = 13f * density
        chipTextPaint.textAlign = Paint.Align.LEFT

        var x = 8f * density

        // Globe icon + source label
        val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(160, 255, 255, 255)
            textSize = 11f * density
            textAlign = Paint.Align.LEFT
        }
        val srcLabel = "\uD83C\uDF10 ${suggestion.sourceLang}→${suggestion.targetLang}:"
        canvas.drawText(srcLabel, x, chipBottom - chipPaddingV * density * 0.5f, labelPaint)
        x += labelPaint.measureText(srcLabel) + 8f * density

        // Translation chip (main tap target)
        val preview = suggestion.translated.let {
            if (it.length > 35) it.take(34) + "\u2026" else it
        }
        val previewW = chipTextPaint.measureText(preview)
        val chipW = previewW + chipPaddingH * density * 2

        val rect = RectF(x, chipTop, x + chipW, chipBottom)
        chipRects.add(rect)  // index 0 = tap to apply

        val bgColor = if (pressedChipIndex == 0) keyBgPressedColor
                      else Color.argb(220, 25, 118, 210)  // blue chip
        canvas.drawRoundRect(rect, chipR, chipR, Paint(Paint.ANTI_ALIAS_FLAG).apply { color = bgColor })
        chipTextPaint.color = Color.WHITE
        canvas.drawText(preview, rect.left + chipPaddingH * density,
            chipBottom - chipPaddingV * density * 0.8f, chipTextPaint)
        chipTextPaint.color = chipTextColor

        // Small original text hint below the translated text
        val origPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(120, 255, 255, 255)
            textSize = 9f * density
            textAlign = Paint.Align.LEFT
        }
        val origPreview = suggestion.original.let {
            if (it.length > 30) it.take(29) + "\u2026" else it
        }
        canvas.drawText(origPreview, rect.left + chipPaddingH * density,
            chipBottom - 1f * density, origPaint)

        x += chipW + chipSpacing * density

        // Dismiss (✕) chip
        val dismissW = 36f * density
        val dismissRect = RectF(x, chipTop, x + dismissW, chipBottom)
        chipRects.add(dismissRect)  // index 1 = dismiss
        canvas.drawRoundRect(dismissRect, chipR, chipR, Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(120, 120, 120, 120)
        })
        clearBtnPaint.textSize = 14f * density
        canvas.drawText("✕", dismissRect.centerX(), chipBottom - chipPaddingV * density * 0.8f, clearBtnPaint)
    }

    private fun drawDiacriticsStrip(canvas: Canvas, density: Float) {
        chipRects.clear()
        clearButtonRect = RectF()  // no clear button in diacritics mode

        val sh = stripHeight
        // Slightly different background to signal "suggestion mode"
        val diacBgPaint = Paint().apply { color = Color.argb(255, 30, 50, 60) }
        canvas.drawRect(0f, 0f, width.toFloat(), sh, diacBgPaint)

        val chipH = sh - chipPaddingV * density * 2
        val chipTop = chipPaddingV * density
        val chipBottom = chipTop + chipH
        val chipR = chipRadius * density
        val textSize = 18f * density   // larger text for diacritics — letters, not truncated text
        chipTextPaint.textSize = textSize

        // Label on the left
        val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(140, 255, 255, 255)
            this.textSize = 11f * density
            textAlign = Paint.Align.LEFT
        }
        var x = 8f * density
        canvas.drawText("díacr:", x, chipBottom - chipPaddingV * density * 0.5f, labelPaint)
        x += labelPaint.measureText("díacr:") + 8f * density

        // Diacritic variant chips
        diacriticItems.forEachIndexed { i, variant ->
            val textW = chipTextPaint.measureText(variant)
            val chipW = (textW + chipPaddingH * density * 2).coerceAtLeast(40f * density)
            if (x + chipW > width.toFloat()) return@forEachIndexed

            val rect = RectF(x, chipTop, x + chipW, chipBottom)
            chipRects.add(rect)

            // Highlight first item (most common) and last (base letter, no diacritic)
            val isFirst = i == 0
            val isBase = i == diacriticItems.lastIndex
            val bgColor = when {
                isFirst -> Color.argb(200, 31, 120, 160)   // teal — most common
                isBase  -> Color.argb(100, 80, 80, 80)     // grey — base (no diacritic)
                else    -> chipBgColor
            }
            chipBgPaint.color = bgColor
            if (pressedChipIndex == i) chipBgPaint.color = keyBgPressedColor

            canvas.drawRoundRect(rect, chipR, chipR, chipBgPaint)
            chipTextPaint.textAlign = Paint.Align.CENTER
            canvas.drawText(variant, rect.centerX(), chipBottom - chipPaddingV * density * 0.8f, chipTextPaint)
            chipTextPaint.textAlign = Paint.Align.LEFT

            x += chipW + chipSpacing * density
        }

        // Hint on right side
        val hintPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.argb(100, 255, 255, 255)
            this.textSize = 10f * density
            textAlign = Paint.Align.RIGHT
        }
        canvas.drawText("tap to insert", width - 8f * density, chipBottom - chipPaddingV * density * 0.5f, hintPaint)

        // Reset chipBgPaint color
        chipBgPaint.color = chipBgColor
    }

    private fun drawPredictionsStrip(canvas: Canvas, density: Float) {
        chipRects.clear()
        clearButtonRect = RectF()

        val sh = stripHeight
        // Neutral dark bg — predictions are passive, not urgent
        canvas.drawRect(0f, 0f, width.toFloat(), sh,
            Paint().apply { color = Color.argb(255, 28, 35, 42) })

        val chipH = sh - chipPaddingV * density * 2
        val chipTop = chipPaddingV * density
        val chipBottom = chipTop + chipH
        val chipR = chipRadius * density
        val textSize = 14f * density
        chipTextPaint.textSize = textSize
        chipTextPaint.textAlign = Paint.Align.CENTER

        // Dividers between chips — iOS-style
        val divW = 1f * density
        val divPaintColor = Color.argb(60, 255, 255, 255)

        // Spread up to 3 chips evenly across the full strip width
        val items = predictionItems.take(3)
        val colW = width.toFloat() / items.size

        items.forEachIndexed { i, word ->
            val chipLeft = colW * i
            val chipRight = colW * (i + 1)

            val rect = RectF(chipLeft, chipTop, chipRight, chipBottom)
            chipRects.add(rect)

            // Pressed highlight
            if (pressedChipIndex == i) {
                canvas.drawRoundRect(
                    RectF(chipLeft + 4f * density, chipTop, chipRight - 4f * density, chipBottom),
                    chipR, chipR,
                    Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.argb(60, 255, 255, 255) }
                )
            }

            // Word text
            chipTextPaint.color = Color.WHITE
            canvas.drawText(word, chipLeft + colW / 2f, chipBottom - chipPaddingV * density * 0.8f, chipTextPaint)

            // Divider (skip last)
            if (i < items.size - 1) {
                canvas.drawRect(
                    chipRight - divW / 2f, chipTop + 6f * density,
                    chipRight + divW / 2f, chipBottom - 6f * density,
                    Paint().apply { color = divPaintColor }
                )
            }
        }

        chipTextPaint.color = chipTextColor
        chipTextPaint.textAlign = Paint.Align.LEFT
    }

    private fun drawKeys(canvas: Canvas, density: Float, topOffset: Float) {
        keyRects.clear()

        val kp = keyPadding * density
        val kr = keyRadius * density
        val rp = rowPadding * density
        keyTextPaint.textSize = 20f * density
        translitPaint.textSize = 18f * density

        val rowCount = rows.size.coerceAtLeast(1)
        val keysHeight = height - topOffset
        val totalVertPadding = rp * (rowCount + 1)
        val keyHeight = (keysHeight - totalVertPadding) / rowCount

        var y = topOffset + rp

        for (row in rows) {
            val totalWeight = row.keys.sumOf { it.widthMultiplier.toDouble() }.toFloat()
            val availableWidth = width - kp * 2 * row.keys.size
            val unitWidth = availableWidth / totalWeight
            var x = 0f

            for (key in row.keys) {
                val keyWidth = unitWidth * key.widthMultiplier + kp * 2
                val rect = RectF(x + kp, y, x + keyWidth - kp, y + keyHeight)
                keyRects.add(Pair(rect, key))

                val paint = when {
                    key == pressedKey -> {
                        keyBgPaint.color = keyBgPressedColor; keyBgPaint
                    }
                    key.code == KeyboardLayout.KEYCODE_TRANSLIT && translitActive ->
                        Paint(Paint.ANTI_ALIAS_FLAG).apply { color = translitActiveColor; alpha = 80 }
                    key.code == KeyboardLayout.KEYCODE_LANG_SWITCH ->
                        Paint(Paint.ANTI_ALIAS_FLAG).apply { color = langIndicatorColor; alpha = 60 }
                    key.isSpecial -> keySpecialBgPaint
                    else -> { keyBgPaint.color = keyBgColor; keyBgPaint }
                }

                canvas.drawRoundRect(rect, kr, kr, paint)

                val textY = rect.centerY() + keyTextPaint.textSize / 3f
                when {
                    key.code == KeyboardLayout.KEYCODE_TRANSLIT -> {
                        val tp = if (translitActive) translitPaint else keyTextPaint
                        canvas.drawText("Tr", rect.centerX(), textY, tp)
                    }
                    key.code == KeyboardLayout.KEYCODE_LANG_SWITCH -> {
                        val langPaint = Paint(keyTextPaint).apply {
                            color = langIndicatorColor
                            typeface = Typeface.DEFAULT_BOLD
                        }
                        canvas.drawText(key.label, rect.centerX(), textY, langPaint)
                    }
                    else -> canvas.drawText(key.label, rect.centerX(), textY, keyTextPaint)
                }

                x += keyWidth
            }
            y += keyHeight + rp
        }

        keyBgPaint.color = keyBgColor
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        val x = event.x
        val y = event.y

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                // Check strip first
                if (y < stripHeight) {
                    if (!stripShowsDiacritics && !stripShowsFix && !stripShowsTranslation &&
                        !stripShowsPredictions && clipItems.isNotEmpty() && clearButtonRect.contains(x, y)) return true
                    val idx = chipRects.indexOfFirst { it.contains(x, y) }
                    if (idx >= 0) {
                        pressedChipIndex = idx
                        longPressChipIndex = idx
                        longPressPending = true
                        if (!stripShowsDiacritics && !stripShowsFix && !stripShowsTranslation &&
                            !stripShowsPredictions && !stripShowsShortcuts) {
                            longPressHandler.postDelayed(longPressRunnable, 500)
                        }
                        invalidate()
                        if (hapticEnabled) performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP)
                        return true
                    }
                    return true
                }
                // Keyboard area
                val hit = keyRects.find { it.first.contains(x, y) }
                if (hit != null) {
                    pressedKey = hit.second
                    invalidate()
                    if (hapticEnabled) performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP)
                    when (hit.second.code) {
                        KeyboardLayout.KEYCODE_SPACE -> {
                            spaceLongPressPending = true
                            longPressHandler.postDelayed(spaceLongPressRunnable, 600)
                        }
                        KeyboardLayout.KEYCODE_BACKSPACE -> {
                            val key = hit.second
                            val runnable = object : Runnable {
                                override fun run() {
                                    listener?.onKey(key)
                                    longPressHandler.postDelayed(this, 50)
                                }
                            }
                            backspaceRepeatRunnable = runnable
                            longPressHandler.postDelayed(runnable, 400)
                        }
                    }
                }
                return true
            }

            MotionEvent.ACTION_UP -> {
                if (pressedChipIndex >= 0) {
                    longPressHandler.removeCallbacks(longPressRunnable)
                    if (longPressPending) {
                        when {
                            stripShowsFix -> {
                                if (pressedChipIndex == 0) {
                                    fixSuggestion?.let { stripListener?.onFixTap(it) }
                                } else {
                                    fixSuggestion = null  // dismiss
                                }
                            }
                            stripShowsTranslation -> {
                                if (pressedChipIndex == 0) {
                                    translationSuggestion?.let { stripListener?.onTranslationTap(it) }
                                } else {
                                    stripListener?.onTranslationDismiss()
                                }
                            }
                            stripShowsDiacritics -> {
                                val variant = diacriticItems.getOrNull(pressedChipIndex)
                                if (variant != null) stripListener?.onDiacriticTap(variant)
                            }
                            stripShowsPredictions -> {
                                val word = predictionItems.getOrNull(pressedChipIndex)
                                if (word != null) stripListener?.onPredictionTap(word)
                            }
                            stripShowsShortcuts -> when (pressedChipIndex) {
                                0 -> stripListener?.onShortcutTranslit()
                                1 -> stripListener?.onShortcutSettings()
                            }
                            else -> stripListener?.onClipTap(pressedChipIndex)
                        }
                    }
                    pressedChipIndex = -1
                    longPressPending = false
                    invalidate()
                    return true
                }
                if (clearButtonRect.contains(x, y) && y < stripHeight) {
                    stripListener?.onStripClear()
                    return true
                }
                backspaceRepeatRunnable?.let { longPressHandler.removeCallbacks(it) }
                backspaceRepeatRunnable = null
                longPressHandler.removeCallbacks(spaceLongPressRunnable)
                spaceLongPressPending = false
                // pressedKey is null when spaceLongPressRunnable already fired; non-null on regular tap
                pressedKey?.let { listener?.onKey(it) }
                pressedKey = null
                invalidate()
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                longPressHandler.removeCallbacks(longPressRunnable)
                longPressHandler.removeCallbacks(spaceLongPressRunnable)
                backspaceRepeatRunnable?.let { longPressHandler.removeCallbacks(it) }
                backspaceRepeatRunnable = null
                spaceLongPressPending = false
                pressedKey = null
                pressedChipIndex = -1
                longPressPending = false
                invalidate()
                return true
            }
        }
        return false
    }

    /** Idle strip: shown when there are no suggestions or clipboard items.
     *  Displays shortcut buttons: Translit toggle + Settings. */
    private fun drawShortcutStrip(canvas: Canvas, density: Float) {
        chipRects.clear()
        clearButtonRect = RectF()

        val sh = stripHeight
        canvas.drawRect(0f, 0f, width.toFloat(), sh, stripBgPaint)

        val chipH = sh - chipPaddingV * density * 2
        val chipTop = chipPaddingV * density
        val chipBottom = chipTop + chipH
        val chipR = chipRadius * density

        chipTextPaint.textSize = 13f * density
        chipTextPaint.textAlign = Paint.Align.CENTER

        // Shortcut buttons definition: label, active-tint
        val shortcuts = listOf(
            if (translitActive) "Tr ●" else "Tr",  // translit with active dot
            "⚙",
        )

        var x = chipPaddingH * density
        shortcuts.forEachIndexed { i, label ->
            val textW = chipTextPaint.measureText(label)
            val chipW = (textW + chipPaddingH * density * 2).coerceAtLeast(44f * density)
            val rect = RectF(x, chipTop, x + chipW, chipBottom)
            chipRects.add(rect)

            val isActive = i == 0 && translitActive
            val bgColor = if (pressedChipIndex == i) keyBgPressedColor
                          else if (isActive) Color.argb(200, 30, 80, 160)
                          else chipBgColor
            canvas.drawRoundRect(rect, chipR, chipR,
                Paint(Paint.ANTI_ALIAS_FLAG).apply { color = bgColor })

            chipTextPaint.color = Color.WHITE
            canvas.drawText(label, rect.centerX(), chipBottom - chipPaddingV * density * 0.8f, chipTextPaint)
            chipTextPaint.color = chipTextColor

            x += chipW + chipSpacing * density
        }
    }
}
