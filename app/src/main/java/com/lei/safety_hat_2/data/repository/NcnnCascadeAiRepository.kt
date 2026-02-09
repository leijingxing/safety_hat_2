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
    private val minPersonConfidence = 0.5f
    private val minPersonAreaRatio = 0.02f
    // 级联入口只认可人员相关标签，避免把其他类别误当作下游裁剪目标。
    private val personLabels = setOf("person", "personup", "persondown")

    private val safeListener = object : BaseAi.BaseListener<Array<String>, Array<SafeMod.SafeObj>> {
        override fun onValue(value: Array<String>) = Unit

        override fun onValue(img: Mat, pts: Long, array: Array<SafeMod.SafeObj>) {
            // 先筛掉低置信度和过小目标，减少后续级联模型的无效推理。
            val validPersons = filterValidPersons(img, array)
            if (validPersons.isEmpty()) {
                return
            }
            val boxes = mapBoxes(img, array, validPersons.isNotEmpty())
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
            cascadeSmoke(img, pts, validPersons)
            cascadeCall(img, pts, validPersons)
            cascadeUniform(img, pts, validPersons)
            cascadeVest(img, pts, validPersons)
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
        // 级联链路的唯一入口：始终先走 SafeMod，再由回调触发下游模型。
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

    private fun mapBoxes(
        img: Mat,
        array: Array<SafeMod.SafeObj>,
        hasPerson: Boolean
    ): List<BoundingBox> {
        if (!hasPerson) return emptyList()
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
        }.filter { box ->
            // 仅对人员类目标做面积过滤，避免远距离/小目标导致级联误触发。
            if (box.confidence < minPersonConfidence) return@filter false
            val labelKey = box.label
            val areaRatio = (box.right - box.left) * (box.bottom - box.top)
            if (labelKey == mapLabel("person") ||
                labelKey == mapLabel("personup") ||
                labelKey == mapLabel("persondown")
            ) {
                areaRatio >= minPersonAreaRatio
            } else {
                true
            }
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
    private fun cascadeSmoke(img: Mat, pts: Long, validPersons: List<SafeMod.SafeObj>) {
        if (!shouldRun(smokeLastMs, pts, 200)) return
        cascadeByLabels(img, validPersons, listOf("personup", "person")) { cropped ->
            smokeMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    // 拨打电话识别同样复用安全帽/人体检测结果裁剪区域
    private fun cascadeCall(img: Mat, pts: Long, validPersons: List<SafeMod.SafeObj>) {
        if (!shouldRun(callLastMs, pts, 200)) return
        cascadeByLabels(img, validPersons, listOf("personup", "person")) { cropped ->
            callMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    // 工装识别使用 personup/persondown 区域裁剪，降低误检与计算量
    private fun cascadeUniform(img: Mat, pts: Long, validPersons: List<SafeMod.SafeObj>) {
        if (!shouldRun(uniformLastMs, pts, 300)) return
        cascadeByLabels(img, validPersons, listOf("personup", "persondown")) { cropped ->
            uniformMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    // 反光衣检测：使用人员上/下身区域裁剪，送入反光衣分类模型
    private fun cascadeVest(img: Mat, pts: Long, validPersons: List<SafeMod.SafeObj>) {
        if (!shouldRun(vestLastMs, pts, 300)) return
        cascadeByLabels(img, validPersons, listOf("personup", "persondown")) { cropped ->
            vestMod.onRecvImage(cropped.nativeObj, 0)
        }
    }

    private fun cascadeByLabels(
        img: Mat,
        array: List<SafeMod.SafeObj>,
        labels: List<String>,
        onCrop: (Mat) -> Unit
    ) {
        val width = img.width()
        val height = img.height()
        array.filter { labels.contains(it.lableString) }
            .forEach { obj ->
                // 对检测框做轻微扩张，给下游分类模型保留上下文信息。
                val rect = expandRect(obj.rect, width, height, 0.2f)
                val cropped = img.submat(rect)
                try {
                    onCrop(cropped)
                } finally {
                    cropped.release()
                }
            }
    }

    private fun filterValidPersons(
        img: Mat,
        array: Array<SafeMod.SafeObj>
    ): List<SafeMod.SafeObj> {
        val width = img.width().toFloat().coerceAtLeast(1f)
        val height = img.height().toFloat().coerceAtLeast(1f)
        return array.filter { obj ->
            if (!personLabels.contains(obj.lableString)) return@filter false
            if (obj.prob < minPersonConfidence) return@filter false
            val rect = obj.rect
            val areaRatio = (rect.width / width) * (rect.height / height)
            areaRatio >= minPersonAreaRatio
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
        // 使用时间戳做模型限频，避免每帧都跑完整级联导致 CPU/GPU 持续满载。
        val nowMs = pts / 1_000_000
        val prev = lastMs.get()
        if (nowMs - prev < intervalMs) return false
        lastMs.set(nowMs)
        return true
    }
}
