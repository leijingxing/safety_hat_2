package com.lei.safety_hat_2.data.repository

import com.lei.safety_hat_2.domain.repository.CameraRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow

class FakeCameraRepository : CameraRepository {
    // 仅用于本地调试：通过状态流模拟相机在线/离线。
    private val _isOnline = MutableStateFlow(true)
    override val isOnline: Flow<Boolean> = _isOnline.asStateFlow()

    override suspend fun start() {
        _isOnline.value = true
    }

    override suspend fun stop() {
        _isOnline.value = false
    }
}
