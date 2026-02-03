package com.lei.safety_hat_2.domain.repository

import kotlinx.coroutines.flow.Flow

interface StreamRepository {
    val isStreaming: Flow<Boolean>
    suspend fun connect()
    suspend fun disconnect()
}
