package com.lei.safety_hat_2.ui.hud

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.lei.safety_hat_2.core.model.SystemStatus
import com.lei.safety_hat_2.data.repository.FakeAiRepository
import com.lei.safety_hat_2.data.repository.FakeCameraRepository
import com.lei.safety_hat_2.data.repository.FakeStreamRepository
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

class HudViewModel(
    private val aiRepository: AiRepository = FakeAiRepository(),
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

    init {
        viewModelScope.launch {
            startAiDetectionUseCase.start()
            connectStreamUseCase.connect()
            cameraRepository.start()
        }
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
                _state.value = current.copy(
                    alertMessage = "检测: ${event.message}",
                    boxes = if (event.boxes.isNotEmpty()) event.boxes else current.boxes
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
}
