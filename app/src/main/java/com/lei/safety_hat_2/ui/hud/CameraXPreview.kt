package com.lei.safety_hat_2.ui.hud

import android.content.Context
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import org.opencv.core.CvType
import org.opencv.core.Mat
import org.opencv.core.Core
import org.opencv.imgproc.Imgproc
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

@Composable
fun CameraXPreview(
    modifier: Modifier = Modifier,
    onFrame: (Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val analysisExecutor = remember { Executors.newSingleThreadExecutor() }
    val previewView = remember { PreviewView(context) }

    DisposableEffect(lifecycleOwner) {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
        val mainExecutor = ContextCompat.getMainExecutor(context)
        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()
            val preview = Preview.Builder().build().also {
                it.setSurfaceProvider(previewView.surfaceProvider)
            }
            val imageAnalysis = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()

            imageAnalysis.setAnalyzer(analysisExecutor) { imageProxy ->
                handleImageProxy(imageProxy, onFrame, onFrameNv21)
            }

            val selector = CameraSelector.DEFAULT_BACK_CAMERA
            try {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(lifecycleOwner, selector, preview, imageAnalysis)
            } catch (_: Exception) {
            }
        }, mainExecutor)

        onDispose {
            analysisExecutor.shutdown()
        }
    }

    AndroidView(
        modifier = modifier,
        factory = { previewView }
    )
}

private fun handleImageProxy(
    imageProxy: ImageProxy,
    onFrame: (Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit
) {
    val nv21 = imageProxyToNv21(imageProxy)
    if (nv21 != null) {
        onFrameNv21(nv21, imageProxy.width, imageProxy.height, imageProxy.imageInfo.timestamp)
    }
    val mat = nv21?.let { nv21ToRgbaMat(it, imageProxy.width, imageProxy.height, imageProxy.imageInfo.rotationDegrees) }
    if (mat != null) {
        try {
            onFrame(mat, imageProxy.imageInfo.timestamp)
        } finally {
            mat.release()
        }
    }
    imageProxy.close()
}

private fun imageProxyToNv21(imageProxy: ImageProxy): ByteArray? {
    val image = imageProxy.image ?: return null
    val width = imageProxy.width
    val height = imageProxy.height

    val yPlane = image.planes[0]
    val uPlane = image.planes[1]
    val vPlane = image.planes[2]

    val nv21 = ByteArray(width * height * 3 / 2)
    var offset = 0

    // Copy Y plane respecting rowStride
    val yBuffer = yPlane.buffer
    val yRowStride = yPlane.rowStride
    for (row in 0 until height) {
        val rowStart = row * yRowStride
        yBuffer.position(rowStart)
        yBuffer.get(nv21, offset, width)
        offset += width
    }

    // Interleave VU for NV21, respecting pixelStride and rowStride
    val uBuffer = uPlane.buffer
    val vBuffer = vPlane.buffer
    val uRowStride = uPlane.rowStride
    val vRowStride = vPlane.rowStride
    val uPixelStride = uPlane.pixelStride
    val vPixelStride = vPlane.pixelStride

    val chromaHeight = height / 2
    val chromaWidth = width / 2
    for (row in 0 until chromaHeight) {
        val uRowStart = row * uRowStride
        val vRowStart = row * vRowStride
        for (col in 0 until chromaWidth) {
            val uIndex = uRowStart + col * uPixelStride
            val vIndex = vRowStart + col * vPixelStride
            nv21[offset++] = vBuffer.get(vIndex)
            nv21[offset++] = uBuffer.get(uIndex)
        }
    }

    return nv21
}

private fun nv21ToRgbaMat(nv21: ByteArray, width: Int, height: Int, rotation: Int): Mat {
    val yuvMat = Mat(height + height / 2, width, CvType.CV_8UC1)
    yuvMat.put(0, 0, nv21)
    val rgbaMat = Mat()
    Imgproc.cvtColor(yuvMat, rgbaMat, Imgproc.COLOR_YUV2RGBA_NV21)
    yuvMat.release()

    if (rotation == 0) return rgbaMat

    val rotated = Mat()
    when (rotation) {
        90 -> Core.rotate(rgbaMat, rotated, Core.ROTATE_90_CLOCKWISE)
        180 -> Core.rotate(rgbaMat, rotated, Core.ROTATE_180)
        270 -> Core.rotate(rgbaMat, rotated, Core.ROTATE_90_COUNTERCLOCKWISE)
        else -> return rgbaMat
    }
    rgbaMat.release()
    return rotated
}
