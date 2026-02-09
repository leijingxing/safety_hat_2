package com.lei.safety_hat_2.usb

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.PixelFormat
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.media.Image
import android.media.ImageReader
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.os.Handler
import android.os.HandlerThread
import com.herohan.uvcapp.CameraHelper
import com.herohan.uvcapp.ICameraHelper
import com.serenegiant.usb.Size
import com.serenegiant.usb.UVCCamera
import com.serenegiant.usb.UVCParam
import java.util.concurrent.atomic.AtomicBoolean

class UsbCameraController(
    private val onFrameRgba: (Image) -> Unit,
    private val onPreviewError: (String) -> Unit = {}
) : SurfaceHolder.Callback {
    private val tag = "UsbCameraController"
    private val usbAction = "com.lei.safety_hat_2.USB_PERMISSION"

    private var context: Context? = null
    private var surfaceView: SurfaceView? = null
    private var cameraHelper: ICameraHelper? = null
    private var imageReader: ImageReader? = null
    private var usbManager: UsbManager? = null
    private var usbDevice: UsbDevice? = null
    // opened 只表示“控制器已进入打开流程”，避免重复初始化。
    private val opened = AtomicBoolean(false)
    private var frameThread: HandlerThread? = null
    private var frameHandler: Handler? = null

    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(ctx: Context, intent: Intent) {
            when (intent.action) {
                usbAction -> {
                    val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                    if (granted) {
                        // 权限回调后重新扫描设备，避免持有过期设备引用。
                        findAndRequest()
                    } else {
                        onPreviewError("USB permission denied")
                    }
                }
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                    val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                    if (device != null && isTargetDevice(device)) {
                        usbDevice = device
                        requestPermission(device)
                    }
                }
                UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                    val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                    if (device != null && usbDevice?.deviceId == device.deviceId) {
                        stop()
                    }
                }
            }
        }
    }

    fun start(context: Context, surfaceView: SurfaceView) {
        this.context = context
        this.surfaceView = surfaceView
        // SurfaceView 负责实时预览显示，算法帧统一走 ImageReader 输出。
        surfaceView.holder.addCallback(this)

        usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
        registerReceiver(context)
        findAndRequest()
    }

    fun stop() {
        if (!opened.get()) {
            releaseReader()
            unregisterReceiver()
            return
        }
        opened.set(false)
        // 先移除输出 Surface，再停预览/关相机，避免底层回调继续写入已销毁对象。
        cameraHelper?.removeSurface(imageReader?.surface)
        val holderSurface = surfaceView?.holder?.surface
        if (holderSurface != null) {
            cameraHelper?.removeSurface(holderSurface)
        }
        cameraHelper?.stopPreview()
        if (cameraHelper?.isCameraOpened == true) {
            cameraHelper?.closeCamera()
        }
        cameraHelper?.setStateCallback(null)
        cameraHelper?.release()
        cameraHelper = null
        releaseReader()
        stopFrameThread()
        unregisterReceiver()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        val helper = cameraHelper ?: return
        helper.addSurface(holder.surface, false)
        // 若相机已打开而 Surface 刚创建，需要补一次 startPreview 才会出图。
        if (opened.get() && helper.isCameraOpened) {
            helper.startPreview()
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // no-op
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        cameraHelper?.removeSurface(holder.surface)
    }

    private fun registerReceiver(context: Context) {
        val filter = IntentFilter().apply {
            addAction(usbAction)
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        context.registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED)
    }

    private fun unregisterReceiver() {
        val ctx = context ?: return
        try {
            ctx.unregisterReceiver(receiver)
        } catch (_: Exception) {
        }
    }

    private fun findAndRequest() {
        val manager = usbManager ?: return
        val devices = manager.deviceList.values
        val target = devices.firstOrNull { isTargetDevice(it) }
        if (target == null) {
            onPreviewError("USB camera not found")
            return
        }
        usbDevice = target
        if (manager.hasPermission(target)) {
            initCamera()
        } else {
            requestPermission(target)
        }
    }

    private fun requestPermission(device: UsbDevice) {
        val ctx = context ?: return
        val intent = Intent(usbAction)
        val pendingIntent = PendingIntent.getBroadcast(
            ctx,
            0,
            intent,
            PendingIntent.FLAG_IMMUTABLE
        )
        usbManager?.requestPermission(device, pendingIntent)
    }

    private fun initCamera() {
        val ctx = context ?: return
        val device = usbDevice ?: return
        if (opened.get()) return

        // 在选设备前先准备帧读取和状态回调，保证开流后第一帧就能接住。
        ensureReader()
        if (cameraHelper == null) {
            cameraHelper = CameraHelper()
            cameraHelper?.setStateCallback(stateCallback)
        }
        cameraHelper?.selectDevice(device)
        opened.set(true)
        Log.i(tag, "USB camera selected: ${device.vendorId}:${device.productId}")
    }

    private fun ensureReader() {
        if (imageReader != null) return
        ensureFrameThread()
        imageReader = ImageReader.newInstance(
            UsbCameraConfig.WIDTH,
            UsbCameraConfig.HEIGHT,
            PixelFormat.RGBA_8888,
            2
        ).also { reader ->
            // 使用 acquireLatestImage 丢弃堆积旧帧，优先保证实时性。
            reader.setOnImageAvailableListener({ r ->
                val image = r.acquireLatestImage() ?: return@setOnImageAvailableListener
                try {
                    onFrameRgba(image)
                } finally {
                    image.close()
                }
            }, frameHandler)
        }
    }

    private fun releaseReader() {
        imageReader?.close()
        imageReader = null
    }

    private fun ensureFrameThread() {
        if (frameThread != null) return
        // 图像回调放到独立线程，避免阻塞主线程和 UVC 内部线程。
        frameThread = HandlerThread("UsbCamFrameThread").also { thread ->
            thread.start()
            frameHandler = Handler(thread.looper)
        }
    }

    private fun stopFrameThread() {
        frameThread?.quitSafely()
        frameThread = null
        frameHandler = null
    }

    private fun isTargetDevice(device: UsbDevice): Boolean {
        return device.vendorId == UsbCameraConfig.USB_VID &&
            device.productId == UsbCameraConfig.USB_PID
    }

    private val stateCallback = object : ICameraHelper.StateCallback {
        override fun onAttach(device: UsbDevice?) = Unit

        override fun onDeviceOpen(device: UsbDevice?, isFirstOpen: Boolean) {
            val helper = cameraHelper ?: return
            val target = usbDevice ?: return
            if (device == null || device.deviceId != target.deviceId) return
            val param = UVCParam().apply {
                // 该 quirk 用于兼容部分设备带宽协商问题，减少黑屏概率。
                quirks = UVCCamera.UVC_QUIRK_FIX_BANDWIDTH
            }
            helper.openCamera(param)
        }

        override fun onCameraOpen(device: UsbDevice?) {
            val helper = cameraHelper ?: return
            val target = usbDevice ?: return
            if (device == null || device.deviceId != target.deviceId) return

            // 优先选择目标分辨率 + MJPEG，失败时回退到设备首个可用分辨率。
            val supported = helper.supportedSizeList
            val picked = supported?.firstOrNull {
                it.width == UsbCameraConfig.WIDTH &&
                    it.height == UsbCameraConfig.HEIGHT &&
                    it.type == UVCCamera.UVC_VS_FRAME_MJPEG
            }
            val sizeToUse: Size? = picked ?: supported?.firstOrNull()
            if (sizeToUse == null) {
                onPreviewError("No supported UVC size")
                return
            }

            helper.previewSize = sizeToUse
            imageReader?.surface?.let { helper.addSurface(it, false) }
            surfaceView?.holder?.surface?.let { helper.addSurface(it, false) }
            helper.startPreview()
        }

        override fun onCameraClose(device: UsbDevice?) {
            onPreviewError("USB camera closed")
            stop()
        }

        override fun onDeviceClose(device: UsbDevice?) {
            onPreviewError("USB device closed")
            stop()
        }

        override fun onDetach(device: UsbDevice?) {
            onPreviewError("USB camera detached")
            stop()
        }

        override fun onCancel(device: UsbDevice?) {
            onPreviewError("USB camera canceled")
            stop()
        }
    }
}
