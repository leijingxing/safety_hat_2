package com.lei.safety_hat_2.ui.hud

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.lei.safety_hat_2.core.model.SystemStatus
import com.lei.safety_hat_2.data.repository.FakeAiRepository
import com.lei.safety_hat_2.data.repository.FakeCameraRepository
import com.lei.safety_hat_2.data.repository.FakeStreamRepository
import com.lei.safety_hat_2.data.repository.RtmpRepository
import com.lei.safety_hat_2.domain.repository.AiRepository
import com.lei.safety_hat_2.domain.usecase.ConnectStreamUseCase
import com.lei.safety_hat_2.domain.usecase.StartAiDetectionUseCase
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import kotlin.random.Random
import org.opencv.core.CvType
import org.opencv.core.Mat
import java.util.ArrayDeque

class HudViewModel(
    private val aiRepository: AiRepository = FakeAiRepository(),
    private val rtmpRepository: RtmpRepository? = null,
    private val useDemoFrames: Boolean = false,
    private val demoWidth: Int = 1280,
    private val demoHeight: Int = 720
) : ViewModel() {
    private val cameraRepository = FakeCameraRepository()
    private val streamRepository = FakeStreamRepository()

    private val startAiDetectionUseCase = StartAiDetectionUseCase(aiRepository)
    private val connectStreamUseCase = ConnectStreamUseCase(streamRepository)

    private val _state = MutableStateFlow(HudState())
    val state: StateFlow<HudState> = _state.asStateFlow()
    private val violationHistory: ArrayDeque<Pair<Long, String>> = ArrayDeque()

    init {
        // 原生模型加载可能耗时，必须放到后台线程，避免主线程卡死/ANR
        viewModelScope.launch(Dispatchers.Default) {
            startAiDetectionUseCase.start()
            connectStreamUseCase.connect()
            cameraRepository.start()
        }
        _state.value = _state.value.copy(
            capabilities = listOf("安全帽", "抽烟", "打电话", "工装", "反光衣")
        )
        observeAiEvents()
        simulateSystemStatus()
        if (useDemoFrames) {
            startDemoFrameLoop()
        }
    }

    private fun observeAiEvents() {
        viewModelScope.launch(Dispatchers.Default) {
            aiRepository.events.collectLatest { event ->
                val current = _state.value
                // 每次识别事件都会刷新告警文本并触发 UI 高亮提示
                val newViolations = extractViolations(event.message)
                val mergedViolations = if (newViolations.isEmpty()) {
                    current.violations
                } else {
                    (newViolations + current.violations).take(8)
                }
                updateViolationStats(newViolations)
                _state.value = current.copy(
                    alertMessage = "检测: ${event.message}",
                    alertId = current.alertId + 1,
                    boxes = if (event.boxes.isNotEmpty()) event.boxes else current.boxes,
                    violations = mergedViolations,
                    violationCounts = computeViolationCounts()
                )
            }
        }
    }

    private fun simulateSystemStatus() {
        viewModelScope.launch(Dispatchers.Default) {
            var battery = 86
            var temp = 41.5f
            while (true) {
                battery = (battery - 1).coerceAtLeast(12)
                temp = (temp + Random.nextFloat() * 0.6f - 0.2f).coerceIn(35f, 70f)
                val cpu = (30 + Random.nextInt(0, 55)).coerceAtMost(95)
                _state.value = _state.value.copy(
                    systemStatus = SystemStatus(
                        batteryPercent = battery,
                        temperatureC = temp,
                        cpuLoad = cpu,
                        isStreaming = true,
                        isCameraOnline = true
                    ),
                    fps = 24 + Random.nextInt(0, 12)
                )
                delay(2000)
            }
        }
    }

    private fun startDemoFrameLoop() {
        viewModelScope.launch(Dispatchers.Default) {
            val mat = Mat(demoHeight, demoWidth, CvType.CV_8UC4)
            while (true) {
                aiRepository.submitFrame(mat, System.currentTimeMillis())
                delay(500)
            }
        }
    }

    fun submitFrame(rgbaMat: Mat, pts: Long) {
        aiRepository.submitFrame(rgbaMat, pts)
    }

    fun submitRtmpFrame(nv21: ByteArray, width: Int, height: Int, timestampNs: Long) {
        if (_state.value.frameWidth == 0 || _state.value.frameHeight == 0) {
            _state.value = _state.value.copy(frameWidth = width, frameHeight = height)
        }
        rtmpRepository?.offerFrame(nv21, width, height, timestampNs)
    }

    fun toggleStreaming(width: Int, height: Int) {
        val repo = rtmpRepository ?: return
        if (repo.isRunning()) {
            repo.stop()
            _state.value = _state.value.copy(isStreaming = false)
        } else {
            repo.start(width, height)
            _state.value = _state.value.copy(isStreaming = true)
        }
    }

    private fun extractViolations(message: String): List<String> {
        if (message.isBlank()) return emptyList()
        val tokens = message.split(",").map { it.trim() }.filter { it.isNotBlank() }
        val violations = mutableListOf<String>()
        tokens.forEach { token ->
            when (token) {
                "抽烟" -> violations.add("抽烟")
                "未戴安全帽" -> violations.add("未戴安全帽")
                "非工装" -> violations.add("未穿工装")
                "未穿反光衣" -> violations.add("未穿反光衣")
                "未系安全绳" -> violations.add("未系安全绳")
                "打电话" -> violations.add("打电话")
            }
        }
        return violations
    }

    private fun updateViolationStats(newViolations: List<String>) {
        val now = System.currentTimeMillis()
        newViolations.forEach { violationHistory.addLast(now to it) }
        val cutoff = now - 60_000
        while (violationHistory.isNotEmpty() && violationHistory.first().first < cutoff) {
            violationHistory.removeFirst()
        }
    }

    private fun computeViolationCounts(): Map<String, Int> {
        val map = linkedMapOf(
            "抽烟" to 0,
            "未戴安全帽" to 0,
            "未穿工装" to 0,
            "未穿反光衣" to 0,
            "未系安全绳" to 0,
            "打电话" to 0
        )
        violationHistory.forEach { (_, type) ->
            if (map.containsKey(type)) {
                map[type] = (map[type] ?: 0) + 1
            }
        }
        return map
    }

    override fun onCleared() {
        super.onCleared()
        rtmpRepository?.stop()
    }
}
