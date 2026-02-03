package com.lei.safety_hat_2.data.rtmp

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.util.Log
import com.example.librtmps.NativeLib
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.launch
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicBoolean

class RtmpStreamer(
    private val url: String,
    private val width: Int,
    private val height: Int,
    private val fps: Int,
    private val bitrate: Int
) {
    private val tag = "RtmpStreamer"
    private val rtmpLib = NativeLib()
    private val running = AtomicBoolean(false)
    private var codec: MediaCodec? = null
    private var colorFormat: Int = MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible
    private var sendJob: Job? = null
    private val scope = CoroutineScope(Dispatchers.Default)
    private val frameChannel = Channel<VideoFrame>(capacity = 2, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    data class VideoFrame(val nv21: ByteArray, val timestampNs: Long)

    fun start(): Boolean {
        if (running.get()) return true
        if (!rtmpLib.initMem()) {
            Log.e(tag, "rtmp init mem failed")
            return false
        }
        if (!rtmpLib.open(url, width, height, fps)) {
            Log.e(tag, "rtmp open failed")
            return false
        }
        codec = createCodec()
        if (codec == null) {
            rtmpLib.close()
            return false
        }
        running.set(true)
        sendJob = scope.launch {
            runEncodeLoop()
        }
        return true
    }

    fun stop() {
        running.set(false)
        sendJob?.cancel()
        sendJob = null
        frameChannel.tryReceive().getOrNull()
        codec?.stop()
        codec?.release()
        codec = null
        rtmpLib.close()
    }

    fun offerFrame(nv21: ByteArray, timestampNs: Long) {
        if (!running.get()) return
        frameChannel.trySend(VideoFrame(nv21, timestampNs))
    }

    private fun createCodec(): MediaCodec? {
        return try {
            val codec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC)
            colorFormat = chooseColorFormat(codec)
            val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height).apply {
                setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat)
                setInteger(MediaFormat.KEY_BIT_RATE, bitrate)
                setInteger(MediaFormat.KEY_FRAME_RATE, fps)
                setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1)
            }
            codec.apply {
                configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
                start()
            }
        } catch (e: Exception) {
            Log.e(tag, "createCodec error: ${e.message}")
            null
        }
    }

    private suspend fun runEncodeLoop() {
        val bufferInfo = MediaCodec.BufferInfo()
        while (running.get()) {
            val frame = frameChannel.receive()
            val codec = codec ?: continue
            val inputIndex = codec.dequeueInputBuffer(10_000)
            if (inputIndex >= 0) {
                val inputBuffer = codec.getInputBuffer(inputIndex)
                if (inputBuffer != null) {
                    inputBuffer.clear()
                    val yuv = when (colorFormat) {
                        MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar -> nv21ToNv12(frame.nv21)
                        MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar -> nv21ToI420(frame.nv21)
                        else -> frame.nv21
                    }
                    if (yuv.size > inputBuffer.remaining()) {
                        Log.w(tag, "drop frame: yuv=${yuv.size} > buffer=${inputBuffer.remaining()}")
                        codec.queueInputBuffer(inputIndex, 0, 0, frame.timestampNs / 1_000, 0)
                        continue
                    }
                    inputBuffer.put(yuv)
                    codec.queueInputBuffer(
                        inputIndex,
                        0,
                        inputBuffer.position(),
                        frame.timestampNs / 1_000,
                        0
                    )
                }
            }
            var outputIndex = codec.dequeueOutputBuffer(bufferInfo, 0)
            while (outputIndex >= 0) {
                val outputBuffer = codec.getOutputBuffer(outputIndex)
                if (outputBuffer != null && bufferInfo.size > 0) {
                    outputBuffer.position(bufferInfo.offset)
                    outputBuffer.limit(bufferInfo.offset + bufferInfo.size)
                    val timestampMs = bufferInfo.presentationTimeUs / 1000
                    rtmpLib.writeVideo(outputBuffer, timestampMs)
                }
                codec.releaseOutputBuffer(outputIndex, false)
                outputIndex = codec.dequeueOutputBuffer(bufferInfo, 0)
            }
        }
    }

    private fun nv21ToNv12(nv21: ByteArray): ByteArray {
        // NV21: YYYY VU VU -> NV12: YYYY UV UV
        val out = nv21.clone()
        var i = width * height
        while (i + 1 < out.size) {
            val v = out[i]
            out[i] = out[i + 1]
            out[i + 1] = v
            i += 2
        }
        return out
    }

    private fun nv21ToI420(nv21: ByteArray): ByteArray {
        val frameSize = width * height
        val out = ByteArray(nv21.size)
        // Y
        System.arraycopy(nv21, 0, out, 0, frameSize)
        // U/V
        var uvIndex = frameSize
        var uIndex = frameSize
        var vIndex = frameSize + frameSize / 4
        while (uvIndex + 1 < nv21.size) {
            out[vIndex++] = nv21[uvIndex]     // V
            out[uIndex++] = nv21[uvIndex + 1] // U
            uvIndex += 2
        }
        return out
    }

    private fun chooseColorFormat(codec: MediaCodec): Int {
        return try {
            val caps = codec.codecInfo.getCapabilitiesForType(MediaFormat.MIMETYPE_VIDEO_AVC)
            val formats = caps.colorFormats.toSet()
            when {
                formats.contains(MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar) ->
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar
                formats.contains(MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar) ->
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar
                else -> MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible
            }
        } catch (_: Exception) {
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible
        }
    }
}
