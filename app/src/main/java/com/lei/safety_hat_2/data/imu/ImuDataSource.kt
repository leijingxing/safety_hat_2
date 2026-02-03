package com.lei.safety_hat_2.data.imu

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import com.lei.safety_hat_2.core.model.ImuSample
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

class ImuDataSource(context: Context) : SensorEventListener {
    private val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val accel = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
    private val gyro = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE)

    private val _sample = MutableStateFlow<ImuSample?>(null)
    val sample: StateFlow<ImuSample?> = _sample

    private var latestAccel = FloatArray(3)
    private var latestGyro = FloatArray(3)
    private var lastAccelTs = 0L
    private var lastGyroTs = 0L

    fun start() {
        // 高频采样用于 VIO/SLAM，采样率可根据设备性能再调
        accel?.let { sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_GAME) }
        gyro?.let { sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_GAME) }
    }

    fun stop() {
        sensorManager.unregisterListener(this)
    }

    override fun onSensorChanged(event: SensorEvent) {
        when (event.sensor.type) {
            Sensor.TYPE_ACCELEROMETER -> {
                latestAccel = event.values.clone()
                lastAccelTs = event.timestamp
            }
            Sensor.TYPE_GYROSCOPE -> {
                latestGyro = event.values.clone()
                lastGyroTs = event.timestamp
            }
        }
        // 用最新的 accel + gyro 合成一条样本，方便 UI/日志展示
        val ts = maxOf(lastAccelTs, lastGyroTs)
        if (ts > 0L) {
            _sample.value = ImuSample(
                timestampNs = ts,
                ax = latestAccel[0],
                ay = latestAccel[1],
                az = latestAccel[2],
                gx = latestGyro[0],
                gy = latestGyro[1],
                gz = latestGyro[2]
            )
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) = Unit
}
