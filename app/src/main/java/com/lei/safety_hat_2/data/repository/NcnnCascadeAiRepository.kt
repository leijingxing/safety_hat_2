package com.lei.safety_hat_2.data.repository

import android.content.res.AssetManager
import com.lei.safety_hat_2.core.model.AiEvent
import com.lei.safety_hat_2.core.model.BoundingBox
import com.lei.safety_hat_2.domain.repository.AiRepository
import com.test.libncnn.BaseAi
import com.test.libncnn.CallMod
import com.test.libncnn.SafeMod
import com.test.libncnn.SmokeMod
import com.test.libncnn.UniformMod
import com.test.libncnn.VestMod
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import org.opencv.core.Mat
import org.opencv.core.Rect
import java.util.concurrent.atomic.AtomicLong
import kotlin.math.max
import kotlin.math.min
import android.util.Log

class NcnnCascadeAiRepository(
    private val assets: AssetManager,
    private val useGpu: Boolean = false
) : AiRepository {
    private val tag = "NcnnCascadeAi"
    private val _events = MutableSharedFlow<AiEvent>(replay = 0, extraBufferCapacity = 4)
    override val events: Flow<AiEvent> = _events.asSharedFlow()

    private val safeMod = SafeMod()
    private val smokeMod = SmokeMod()
    private val callMod = CallMod()
    private val uniformMod = UniformMod()
    private val vestMod = VestMod()

    private val smokeLastMs = AtomicLong(0)
    private val callLastMs = AtomicLong(0)
    private val uniformLastMs = AtomicLong(0)
    private val vestLastMs = AtomicLong(0)

    private val safeListener = object : BaseAi.BaseListener<Array<String>, Array<SafeMod.SafeObj>> {
        override fun onValue(value: Array<String>) = Unit

        override fun onValue(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
            val boxes = mapBoxes(img, array)
            val message = boxes.map { it.label }.distinct().joinToString(", ")
            _events.tryEmit(
                AiEvent(
                    type = "safe",
                    message = message.ifBlank { "safe" },
                    boxes = boxes
                )
            )
            if (message.isNotBlank()) {
                Log.i(tag, "safe=$message")
            }
            cascadeSmoke(img, pts, array)
            cascadeCall(img, pts, array)
            cascadeUniform(img, pts, array)
            cascadeVest(img, pts, array)
        }
    }

    private val smokeListener = object : BaseAi.BaseListener<Array<String>, Any> {
        override fun onValue(img: Mat, pts: Long, array: Any) = Unit
        override fun onValue(value: Array<String>) {
            emitSimple("smoke", value)
        }
    }

    private val callListener = object : BaseAi.BaseListener<Array<String>, Any> {
        override fun onValue(img: Mat, pts: Long, array: Any) = Unit
        override fun onValue(value: Array<String>) {
            emitSimple("call", value)
        }
    }

    private val uniformListener = object : BaseAi.BaseListener<Array<String>, Any> {
        override fun onValue(img: Mat, pts: Long, array: Any) = Unit
        override fun onValue(value: Array<String>) {
            emitSimple("uniform", value)
        }
    }

    private val vestListener = object : BaseAi.BaseListener<Array<String>, Any> {
        override fun onValue(img: Mat, pts: Long, array: Any) = Unit
        override fun onValue(value: Array<String>) {
            emitSimple("vest", value)
        }
    }

    override suspend fun start() {
        safeMod.setCallback(safeListener)
        smokeMod.setCallback(smokeListener)
        callMod.setCallback(callListener)
        uniformMod.setCallback(uniformListener)
        vestMod.setCallback(vestListener)

        safeMod.loadModel(assets, 0, if (useGpu) 1 else 0)
        smokeMod.loadModel(assets, 0, if (useGpu) 1 else 0)
        callMod.loadModel(assets, 0, if (useGpu) 1 else 0)
        uniformMod.loadModel(assets, 0, if (useGpu) 1 else 0)
        vestMod.loadModel(assets, 0, if (useGpu) 1 else 0)
    }

    override suspend fun stop() {
        safeMod.setCallback(null)
        smokeMod.setCallback(null)
        callMod.setCallback(null)
        uniformMod.setCallback(null)
        vestMod.setCallback(null)
    }

    override fun submitFrame(rgbaMat: Mat, pts: Long) {
        safeMod.onRecvImage(rgbaMat, rgbaMat.nativeObj, pts)
    }

    private fun emitSimple(type: String, value: Array<String>) {
        if (value.isEmpty()) return
        val mapped = value.joinToString(", ") { mapLabel(it) }
        Log.i(tag, "$type=$mapped")
        _events.tryEmit(
            AiEvent(
                type = type,
                message = mapped,
                boxes = emptyList()
            )
        )
    }

    private fun mapBoxes(img: Mat, array: Array<SafeMod.SafeObj>): List<BoundingBox> {
        val width = img.width().toFloat().coerceAtLeast(1f)
        val height = img.height().toFloat().coerceAtLeast(1f)
        return array.map { obj ->
            val rect = obj.rect
            BoundingBox(
                left = (rect.x / width).coerceIn(0f, 1f),
                top = (rect.y / height).coerceIn(0f, 1f),
                right = ((rect.x + rect.width) / width).coerceIn(0f, 1f),
                bottom = ((rect.y + rect.height) / height).coerceIn(0f, 1f),
                label = mapLabel(obj.lableString),
                confidence = obj.prob
            )
        }
    }

    private fun mapLabel(label: String): String {
        return when (label.lowercase()) {
            "nohelmat" -> "未戴安全帽"
            "helmat" -> "已戴安全帽"
            "helmet" -> "安全帽"
            "person" -> "人员"
            "personup" -> "人员上身"
            "persondown" -> "人员下身"
            "smoke", "smoking" -> "抽烟"
            "call", "calling", "phone" -> "打电话"
            "uniform" -> "工装"
            "vest", "refjacket", "reflective" -> "反光衣"
            "workclothes" -> "工作服"
            "work_clothes" -> "工装"
            "normal" -> "正常"
            "other_clothes", "otherclothes" -> "非工装"
            "no_reflective_jacket" -> "未穿反光衣"
            "reflective_jacket" -> "已穿反光衣"
            "unknow" -> "未知"
            "nolandyard" -> "未系安全绳"
            else -> label
        }
    }

    // 先通过安全帽/人体检测结果裁剪人脸/上半身区域，再触发抽烟识别，避免全图推理浪费算力
    private fun cascadeSmoke(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
        if (!shouldRun(smokeLastMs, pts, 200)) return
        cascadeByLabels(img, array, listOf("nohelmat", "helmat")) { cropped ->
            smokeMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    // 拨打电话识别同样复用安全帽/人体检测结果裁剪区域
    private fun cascadeCall(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
        if (!shouldRun(callLastMs, pts, 200)) return
        cascadeByLabels(img, array, listOf("nohelmat", "helmat")) { cropped ->
            callMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    // 工装识别使用 personup/persondown 区域裁剪，降低误检与计算量
    private fun cascadeUniform(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
        if (!shouldRun(uniformLastMs, pts, 300)) return
        cascadeByLabels(img, array, listOf("personup", "persondown")) { cropped ->
            uniformMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    // 反光衣检测：使用人员上/下身区域裁剪，送入反光衣分类模型
    private fun cascadeVest(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
        if (!shouldRun(vestLastMs, pts, 300)) return
        cascadeByLabels(img, array, listOf("personup", "persondown")) { cropped ->
            vestMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    private fun cascadeByLabels(
        img: Mat,
        array: Array<SafeMod.SafeObj>,
        labels: List<String>,
        onCrop: (Mat) -> Unit
    ) {
        val width = img.width()
        val height = img.height()
        array.filter { labels.contains(it.lableString) }
            .forEach { obj ->
                val rect = expandRect(obj.rect, width, height, 0.2f)
                val cropped = img.submat(rect)
                try {
                    onCrop(cropped)
                } finally {
                    cropped.release()
                }
            }
    }

    private fun expandRect(rect: Rect, imgW: Int, imgH: Int, ratio: Float): Rect {
        val leftPad = min(rect.x.toFloat(), rect.width * ratio).toInt()
        val topPad = min(rect.y.toFloat(), rect.height * ratio).toInt()
        val rightPad = min((imgW - rect.x - rect.width).toFloat(), rect.width * ratio).toInt()
        val bottomPad = min((imgH - rect.y - rect.height).toFloat(), rect.height * ratio).toInt()

        val x = max(0, rect.x - leftPad)
        val y = max(0, rect.y - topPad)
        val w = min(imgW - x, rect.width + leftPad + rightPad)
        val h = min(imgH - y, rect.height + topPad + bottomPad)
        return Rect(x, y, w, h)
    }

    private fun shouldRun(lastMs: AtomicLong, pts: Long, intervalMs: Long): Boolean {
        val nowMs = pts / 1_000_000
        val prev = lastMs.get()
        if (nowMs - prev < intervalMs) return false
        lastMs.set(nowMs)
        return true
    }
}
