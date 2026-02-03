package com.lei.safety_hat_2.data.repository

import com.lei.safety_hat_2.domain.repository.StreamRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow

class FakeStreamRepository : StreamRepository {
    private val _isStreaming = MutableStateFlow(false)
    override val isStreaming: Flow<Boolean> = _isStreaming.asStateFlow()

    override suspend fun connect() {
        _isStreaming.value = true
    }

    override suspend fun disconnect() {
        _isStreaming.value = false
    }
}
