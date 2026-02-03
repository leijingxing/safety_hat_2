package com.lei.safety_hat_2.domain.usecase

import com.lei.safety_hat_2.domain.repository.AiRepository

class StartAiDetectionUseCase(
    private val repository: AiRepository
) {
    suspend fun start() = repository.start()
    suspend fun stop() = repository.stop()
}
