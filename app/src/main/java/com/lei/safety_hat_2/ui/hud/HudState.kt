package com.lei.safety_hat_2.ui.hud

import com.lei.safety_hat_2.core.model.BoundingBox
import com.lei.safety_hat_2.core.model.SystemStatus

data class HudState(
    val systemStatus: SystemStatus = SystemStatus(
        batteryPercent = 86,
        temperatureC = 42.0f,
        cpuLoad = 38,
        isStreaming = false,
        isCameraOnline = true
    ),
    val alertMessage: String = "系统待命",
    val alertId: Long = 0L,
    val boxes: List<BoundingBox> = emptyList(),
    val fps: Int = 30,
    val violations: List<String> = emptyList(),
    val capabilities: List<String> = emptyList(),
    val violationCounts: Map<String, Int> = emptyMap(),
    val imu: com.lei.safety_hat_2.core.model.ImuSample? = null
)
