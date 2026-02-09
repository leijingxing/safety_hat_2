package com.lei.safety_hat_2.usb

object UsbCameraConfig {
    // 默认参数沿用旧项目，改动前请确认设备固件与模型输入分辨率是否匹配。
    const val WIDTH = 1920
    const val HEIGHT = 1080
    // USB 设备 VID/PID 用于精准筛选目标摄像头。
    const val USB_VID = 3034
    const val USB_PID = 12341
}
