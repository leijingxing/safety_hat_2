package com.lei.safety_hat_2.data.repository

import android.util.Log
import android.media.MediaFormat
import com.lei.safety_hat_2.data.rtmp.RtmpStreamer
import java.util.concurrent.atomic.AtomicBoolean

class RtmpRepository(
    private val baseUrl: String,
    private val streamId: String,
    private val fps: Int = 20
) {
    private val tag = "RtmpRepository"
    private val running = AtomicBoolean(false)
    private var streamer: RtmpStreamer? = null

    fun start(width: Int, height: Int) {
        if (running.get()) return
        val url = "${baseUrl}/${streamId}"
        // 经验值码率：像素总量 * 2，后续可按网络质量做自适应。
        val bitrate = width * height * 2
        val newStreamer = RtmpStreamer(
            url = url,
            width = width,
            height = height,
            fps = fps,
            bitrate = bitrate,
            // 优先尝试 HEVC，设备不支持时由 RtmpStreamer 内部回退 AVC。
            preferredMimeType = MediaFormat.MIMETYPE_VIDEO_HEVC
        )
        if (newStreamer.start()) {
            streamer = newStreamer
            running.set(true)
            Log.i(tag, "rtmp start url=$url")
        } else {
            Log.e(tag, "rtmp start failed url=$url")
        }
    }

    fun stop() {
        streamer?.stop()
        streamer = null
        running.set(false)
    }

    fun offerFrame(nv21: ByteArray, width: Int, height: Int, timestampNs: Long) {
        if (!running.get()) return
        // 宽高参数在 start 阶段已固化，这里仅转发帧与时间戳。
        streamer?.offerFrame(nv21, timestampNs)
    }

    fun isRunning(): Boolean = running.get()
}

