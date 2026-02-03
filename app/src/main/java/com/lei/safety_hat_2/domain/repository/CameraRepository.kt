package com.lei.safety_hat_2.domain.repository

import kotlinx.coroutines.flow.Flow

interface CameraRepository {
    val isOnline: Flow<Boolean>
    suspend fun start()
    suspend fun stop()
}
