package com.lei.safety_hat_2.core.model

data class SystemStatus(
    val batteryPercent: Int,
    val temperatureC: Float,
    val cpuLoad: Int,
    val isStreaming: Boolean,
    val isCameraOnline: Boolean
)
