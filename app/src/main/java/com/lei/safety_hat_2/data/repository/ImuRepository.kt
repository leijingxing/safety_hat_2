package com.lei.safety_hat_2.data.repository

import android.content.Context
import com.lei.safety_hat_2.core.model.ImuSample
import com.lei.safety_hat_2.data.imu.ImuDataSource
import kotlinx.coroutines.flow.StateFlow

class ImuRepository(context: Context) {
    private val dataSource = ImuDataSource(context)
    val sample: StateFlow<ImuSample?> = dataSource.sample

    fun start() {
        dataSource.start()
    }

    fun stop() {
        dataSource.stop()
    }
}
