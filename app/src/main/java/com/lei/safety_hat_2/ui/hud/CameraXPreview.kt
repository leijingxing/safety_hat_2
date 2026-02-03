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
    onFrame: (Mat, Long) -> Unit
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
                handleImageProxy(imageProxy, onFrame)
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
    onFrame: (Mat, Long) -> Unit
) {
    val mat = imageProxyToRgba(imageProxy)
    if (mat != null) {
        try {
            onFrame(mat, imageProxy.imageInfo.timestamp)
        } finally {
            mat.release()
        }
    }
    imageProxy.close()
}

private fun imageProxyToRgba(imageProxy: ImageProxy): Mat? {
    val image = imageProxy.image ?: return null
    val width = imageProxy.width
    val height = imageProxy.height

    val yBuffer = image.planes[0].buffer
    val uBuffer = image.planes[1].buffer
    val vBuffer = image.planes[2].buffer

    val ySize = yBuffer.remaining()
    val uSize = uBuffer.remaining()
    val vSize = vBuffer.remaining()

    val nv21 = ByteArray(ySize + uSize + vSize)
    yBuffer.get(nv21, 0, ySize)
    vBuffer.get(nv21, ySize, vSize)
    uBuffer.get(nv21, ySize + vSize, uSize)

    yBuffer.rewind()
    uBuffer.rewind()
    vBuffer.rewind()

    val yuvMat = Mat(height + height / 2, width, CvType.CV_8UC1)
    yuvMat.put(0, 0, nv21)
    val rgbaMat = Mat()
    Imgproc.cvtColor(yuvMat, rgbaMat, Imgproc.COLOR_YUV2RGBA_NV21)
    yuvMat.release()

    val rotation = imageProxy.imageInfo.rotationDegrees
    if (rotation == 0) return rgbaMat

    val rotated = Mat()
    when (rotation) {
        90 -> Core.rotate(rgbaMat, rotated, Core.ROTATE_90_CLOCKWISE)
        180 -> Core.rotate(rgbaMat, rotated, Core.ROTATE_180)
        270 -> Core.rotate(rgbaMat, rotated, Core.ROTATE_90_COUNTERCLOCKWISE)
        else -> {
            rgbaMat.release()
            return null
        }
    }
    rgbaMat.release()
    return rotated
}
