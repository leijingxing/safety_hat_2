package com.lei.safety_hat_2.domain.repository

import com.lei.safety_hat_2.core.model.AiEvent
import kotlinx.coroutines.flow.Flow

interface AiRepository {
    val events: Flow<AiEvent>
    suspend fun start()
    suspend fun stop()
}
