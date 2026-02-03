package com.example.librtmps

import android.graphics.Matrix
import android.util.Log
import android.view.TextureView
import java.nio.ByteBuffer
import kotlin.math.max

class MediaUtils {

    companion object {
        fun logCurrentMethod(): String {
            val stackTrace = Thread.currentThread().stackTrace
            // 堆栈中的 [0] 是 getThreadStackTrace 方法
            // 堆栈中的 [1] 是 getStackTrace 方法,
            // 堆栈中的 [2] 是当前方法 logCurrentMethod
            // 堆栈中的 [3] 是调用 前一个方法
            val currentMethod = stackTrace[3]
            return currentMethod.methodName
        }

        fun fitCenterTextureView(textureView: TextureView, imageWidth: Int, imageHeight: Int) {
            val aw = textureView.width
            val ah = textureView.height
            val bw = imageWidth
            val bh = imageHeight
            val scaleImg = max(aw/bw, ah/bh)
            val imgW = scaleImg*bw
            val imgH = scaleImg*bh

            val scaleX = imgW.toFloat() / aw.toFloat()
            val scaleY = imgH.toFloat() / ah.toFloat()

            var dx = 0f
            var dy = 0f
            if (scaleX > 0) {
                dx = (aw - imgW).toFloat() / 2.0f
            }
            if (scaleY > 0) {
                dy = (ah - imgH).toFloat() / 2.0f
            }
            // 创建矩阵进行缩放和平移
            val matrix = Matrix()
            matrix.setScale(scaleX, scaleY) // 1920()1200
            matrix.postTranslate(dx, dy)

            // 应用矩阵变换
            textureView.setTransform(matrix)
        }


        fun logByteBuffer(buffer: ByteBuffer, logstr:String) {
            val length = minOf(buffer.remaining(), 20)
            val hexString = StringBuilder()

            for (i in 0 until length) {
                val byte = buffer.get()
                hexString.append(String.format("0x%02X ", byte))
            }

            // 移除最后一个多余的空格
            if (hexString.isNotEmpty()) {
                hexString.setLength(hexString.length - 1)
            }

            val capacity = buffer.capacity() // 总大小
            val remaining = buffer.remaining() // post到有效数据尾的大小
            val postion = buffer.position()
            // 使用 Log.v 输出
            Log.v("ByteBufferData", "$logstr >>> capacity:$capacity post:$postion remaining:$remaining hex:$hexString")
        }
    }

}