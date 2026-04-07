package com.clavi.keyboard

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import android.os.VibrationEffect
import android.os.Vibrator
import android.util.AttributeSet
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import androidx.core.content.ContextCompat

/**
 * Custom-drawn keyboard view. Renders rows of keys on a Canvas and
 * dispatches touch events to the listener.
 */
class ClaviKeyboardView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : View(context, attrs) {

    interface OnKeyListener {
        fun onKey(key: Key)
    }

    var listener: OnKeyListener? = null
    var translitActive = false
    var currentLanguage = Language.UK

    private var rows: List<Row> = emptyList()
    private val keyRects = mutableListOf<Pair<RectF, Key>>()

    private val keyBgPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val keySpecialBgPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val keyTextPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val keySubTextPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val bgPaint = Paint()
    private val translitPaint = Paint(Paint.ANTI_ALIAS_FLAG)

    private val keyRadius = 8f
    private val keyPadding = 3f
    private val rowPadding = 4f

    private var pressedKey: Key? = null
    private var pressedRect: RectF? = null

    private val keyBgColor: Int
    private val keyBgPressedColor: Int
    private val keySpecialBgColor: Int
    private val keyTextColor: Int
    private val keyboardBgColor: Int
    private val translitActiveColor: Int
    private val langIndicatorColor: Int

    init {
        keyBgColor = ContextCompat.getColor(context, R.color.key_bg)
        keyBgPressedColor = ContextCompat.getColor(context, R.color.key_bg_pressed)
        keySpecialBgColor = ContextCompat.getColor(context, R.color.key_special_bg)
        keyTextColor = ContextCompat.getColor(context, R.color.key_text)
        keyboardBgColor = ContextCompat.getColor(context, R.color.keyboard_bg)
        translitActiveColor = ContextCompat.getColor(context, R.color.translit_active)
        langIndicatorColor = ContextCompat.getColor(context, R.color.lang_indicator)

        bgPaint.color = keyboardBgColor
        keyBgPaint.color = keyBgColor
        keySpecialBgPaint.color = keySpecialBgColor

        keyTextPaint.color = keyTextColor
        keyTextPaint.textAlign = Paint.Align.CENTER
        keyTextPaint.typeface = Typeface.DEFAULT
        keyTextPaint.textSize = 48f

        keySubTextPaint.color = Color.argb(150, 255, 255, 255)
        keySubTextPaint.textAlign = Paint.Align.CENTER
        keySubTextPaint.textSize = 28f

        translitPaint.color = translitActiveColor
        translitPaint.textAlign = Paint.Align.CENTER
        translitPaint.textSize = 36f
        translitPaint.typeface = Typeface.DEFAULT_BOLD
    }

    fun setLayout(rows: List<Row>) {
        this.rows = rows
        requestLayout()
        invalidate()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val w = MeasureSpec.getSize(widthMeasureSpec)
        val rowCount = rows.size.coerceAtLeast(4)
        val keyHeight = (w * 0.13f).toInt()
        val h = (keyHeight * rowCount + rowPadding * (rowCount + 1)).toInt()
        setMeasuredDimension(w, h)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawRect(0f, 0f, width.toFloat(), height.toFloat(), bgPaint)

        keyRects.clear()
        val density = resources.displayMetrics.density
        val kp = keyPadding * density
        val kr = keyRadius * density
        val rp = rowPadding * density
        keyTextPaint.textSize = 20f * density
        keySubTextPaint.textSize = 14f * density
        translitPaint.textSize = 18f * density

        val rowCount = rows.size.coerceAtLeast(1)
        val totalVertPadding = rp * (rowCount + 1)
        val keyHeight = (height - totalVertPadding) / rowCount

        var y = rp

        for (row in rows) {
            val totalWeight = row.keys.sumOf { it.widthMultiplier.toDouble() }.toFloat()
            val availableWidth = width - kp * 2 * row.keys.size
            val unitWidth = availableWidth / totalWeight
            var x = 0f

            for (key in row.keys) {
                val keyWidth = unitWidth * key.widthMultiplier + kp * 2
                val rect = RectF(x + kp, y, x + keyWidth - kp, y + keyHeight)
                keyRects.add(Pair(rect, key))

                // Pick paint
                val paint = when {
                    key == pressedKey -> {
                        keyBgPaint.color = keyBgPressedColor
                        keyBgPaint
                    }
                    key.code == KeyboardLayout.KEYCODE_TRANSLIT && translitActive -> {
                        translitPaint.color = translitActiveColor
                        Paint(Paint.ANTI_ALIAS_FLAG).apply { color = translitActiveColor; alpha = 80 }
                    }
                    key.code == KeyboardLayout.KEYCODE_LANG_SWITCH -> {
                        Paint(Paint.ANTI_ALIAS_FLAG).apply { color = langIndicatorColor; alpha = 60 }
                    }
                    key.isSpecial -> keySpecialBgPaint
                    else -> {
                        keyBgPaint.color = keyBgColor
                        keyBgPaint
                    }
                }

                canvas.drawRoundRect(rect, kr, kr, paint)

                // Text
                val textY = rect.centerY() + keyTextPaint.textSize / 3f
                if (key.code == KeyboardLayout.KEYCODE_TRANSLIT) {
                    val tp = if (translitActive) translitPaint else keyTextPaint
                    canvas.drawText("Tr", rect.centerX(), textY, tp)
                } else if (key.code == KeyboardLayout.KEYCODE_LANG_SWITCH) {
                    val langPaint = Paint(keyTextPaint).apply {
                        color = langIndicatorColor
                        typeface = Typeface.DEFAULT_BOLD
                    }
                    canvas.drawText(key.label, rect.centerX(), textY, langPaint)
                } else {
                    canvas.drawText(key.label, rect.centerX(), textY, keyTextPaint)
                }

                x += keyWidth
            }

            y += keyHeight + rp
        }

        // Reset
        keyBgPaint.color = keyBgColor
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                val hit = findKey(event.x, event.y)
                if (hit != null) {
                    pressedKey = hit.second
                    pressedRect = hit.first
                    invalidate()
                    performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP)
                }
                return true
            }
            MotionEvent.ACTION_UP -> {
                pressedKey?.let { listener?.onKey(it) }
                pressedKey = null
                pressedRect = null
                invalidate()
                return true
            }
            MotionEvent.ACTION_CANCEL -> {
                pressedKey = null
                pressedRect = null
                invalidate()
                return true
            }
        }
        return false
    }

    private fun findKey(x: Float, y: Float): Pair<RectF, Key>? {
        return keyRects.find { it.first.contains(x, y) }
    }
}
