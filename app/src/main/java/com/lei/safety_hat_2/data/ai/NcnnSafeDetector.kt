package com.lei.safety_hat_2.data.ai

import android.content.res.AssetManager
import com.lei.safety_hat_2.core.model.AiEvent
import com.lei.safety_hat_2.core.model.BoundingBox
import com.test.libncnn.BaseAi
import com.test.libncnn.SafeMod
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import org.opencv.core.Mat

class NcnnSafeDetector(
    private val assets: AssetManager,
    private val modelId: Int = 0,
    private val useGpu: Boolean = false
) {
    private val safeMod = SafeMod()
    private val _events = MutableSharedFlow<AiEvent>(replay = 0, extraBufferCapacity = 2)
    val events = _events.asSharedFlow()

    private val listener = object : BaseAi.BaseListener<Array<String>, Array<SafeMod.SafeObj>> {
        override fun onValue(value: Array<String>) = Unit

        override fun onValue(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
            val width = img.width().toFloat().coerceAtLeast(1f)
            val height = img.height().toFloat().coerceAtLeast(1f)
            val boxes = array.map { obj ->
                val rect = obj.rect
                val left = (rect.x / width).coerceIn(0f, 1f)
                val top = (rect.y / height).coerceIn(0f, 1f)
                val right = ((rect.x + rect.width) / width).coerceIn(0f, 1f)
                val bottom = ((rect.y + rect.height) / height).coerceIn(0f, 1f)
                BoundingBox(
                    left = left,
                    top = top,
                    right = right,
                    bottom = bottom,
                    label = obj.lableString,
                    confidence = obj.prob
                )
            }
            val message = boxes.joinToString(", ") { it.label }
            _events.tryEmit(
                AiEvent(
                    type = "safe",
                    message = message.ifBlank { "safe" },
                    boxes = boxes
                )
            )
        }
    }

    fun start() {
        safeMod.setCallback(listener)
        safeMod.loadModel(assets, modelId, if (useGpu) 1 else 0)
    }

    fun stop() {
        safeMod.setCallback(null)
    }

    fun submitFrame(rgbaMat: Mat, pts: Long) {
        safeMod.onRecvImage(rgbaMat, rgbaMat.nativeObj, pts)
    }
}
