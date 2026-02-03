package com.lei.safety_hat_2.domain.usecase

import com.lei.safety_hat_2.domain.repository.StreamRepository

class ConnectStreamUseCase(
    private val repository: StreamRepository
) {
    suspend fun connect() = repository.connect()
    suspend fun disconnect() = repository.disconnect()
}
