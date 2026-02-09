package com.lei.safety_hat_2.ui.hud

import android.media.Image
import android.view.SurfaceView
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.viewinterop.AndroidView
import com.lei.safety_hat_2.usb.UsbCameraController
import org.opencv.core.CvType
import org.opencv.core.Mat
import org.opencv.imgproc.Imgproc

@Composable
fun UsbCameraPreview(
    modifier: Modifier = Modifier,
    onFrame: (Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit,
    onPreviewError: (String) -> Unit = {}
) {
    val context = LocalContext.current
    val controller = remember {
        UsbCameraController(
            onFrameRgba = { image ->
                handleUsbImage(image, onFrame, onFrameNv21)
            },
            onPreviewError = onPreviewError
        )
    }
    val surfaceView = remember { SurfaceView(context) }

    DisposableEffect(Unit) {
        controller.start(context, surfaceView)
        onDispose {
            controller.stop()
        }
    }

    AndroidView(
        modifier = modifier,
        factory = { surfaceView }
    )
}

private fun handleUsbImage(
    image: Image,
    onFrame: (Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit
) {
    val width = image.width
    val height = image.height
    val buffer = image.planes[0].buffer

    // Wrap RGBA buffer as Mat for AI pipeline (no copy).
    val rgbaMat = Mat(height, width, CvType.CV_8UC4, buffer)
    try {
        onFrame(rgbaMat, image.timestamp)
        val nv21 = rgbaToNv21(rgbaMat, width, height)
        onFrameNv21(nv21, width, height, image.timestamp)
    } finally {
        rgbaMat.release()
    }
}

private fun rgbaToNv21(rgbaMat: Mat, width: Int, height: Int): ByteArray {
    // Convert RGBA -> I420, then pack to NV21 (Y + VU).
    val i420 = Mat()
    Imgproc.cvtColor(rgbaMat, i420, Imgproc.COLOR_RGBA2YUV_I420)
    val frameSize = width * height
    val i420Bytes = ByteArray(frameSize * 3 / 2)
    i420.get(0, 0, i420Bytes)
    i420.release()

    val nv21 = ByteArray(i420Bytes.size)
    System.arraycopy(i420Bytes, 0, nv21, 0, frameSize)

    var uIndex = frameSize
    var vIndex = frameSize + frameSize / 4
    var uvIndex = frameSize
    val qSize = frameSize / 4
    for (i in 0 until qSize) {
        nv21[uvIndex++] = i420Bytes[vIndex++] // V
        nv21[uvIndex++] = i420Bytes[uIndex++] // U
    }
    return nv21
}
