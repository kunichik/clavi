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
    }

    interface OnStripListener {
        fun onClipTap(index: Int)
        fun onClipLongPress(index: Int)
        fun onStripClear()
    }

    var listener: OnKeyListener? = null
    var stripListener: OnStripListener? = null
    var translitActive = false
    var currentLanguage = Language.UK

    // Clipboard strip data — set from ClaviIME
    var clipItems: List<String> = emptyList()
        set(value) { field = value; invalidate() }

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

        // Strip height: 40dp when there are clips, 0 otherwise
        stripHeight = if (clipItems.isNotEmpty()) 40f * density else 0f

        setMeasuredDimension(w, keysH + stripHeight.toInt())
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawRect(0f, 0f, width.toFloat(), height.toFloat(), bgPaint)

        val density = resources.displayMetrics.density

        // ── Clipboard strip ──
        if (clipItems.isNotEmpty()) {
            drawStrip(canvas, density)
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
            textSize = textSize
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
                if (y < stripHeight && clipItems.isNotEmpty()) {
                    if (clearButtonRect.contains(x, y)) return true
                    val idx = chipRects.indexOfFirst { it.contains(x, y) }
                    if (idx >= 0) {
                        pressedChipIndex = idx
                        longPressChipIndex = idx
                        longPressPending = true
                        longPressHandler.postDelayed(longPressRunnable, 500)
                        invalidate()
                        performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP)
                        return true
                    }
                    return true
                }
                // Keyboard area
                val hit = keyRects.find { it.first.contains(x, y) }
                if (hit != null) {
                    pressedKey = hit.second
                    invalidate()
                    performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP)
                }
                return true
            }

            MotionEvent.ACTION_UP -> {
                if (pressedChipIndex >= 0) {
                    longPressHandler.removeCallbacks(longPressRunnable)
                    if (longPressPending) {
                        stripListener?.onClipTap(pressedChipIndex)
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
                pressedKey?.let { listener?.onKey(it) }
                pressedKey = null
                invalidate()
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                longPressHandler.removeCallbacks(longPressRunnable)
                pressedKey = null
                pressedChipIndex = -1
                longPressPending = false
                invalidate()
                return true
            }
        }
        return false
    }
}
