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
        val bitrate = width * height * 2
        val newStreamer = RtmpStreamer(
            url = url,
            width = width,
            height = height,
            fps = fps,
            bitrate = bitrate,
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
        streamer?.offerFrame(nv21, timestampNs)
    }

    fun isRunning(): Boolean = running.get()
}

