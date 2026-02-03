package com.lei.safety_hat_2.data.repository

import android.content.res.AssetManager
import com.lei.safety_hat_2.data.ai.NcnnSafeDetector
import com.lei.safety_hat_2.domain.repository.AiRepository
import kotlinx.coroutines.flow.Flow
import org.opencv.core.Mat

class NcnnAiRepository(
    assets: AssetManager,
    modelId: Int = 0,
    useGpu: Boolean = false
) : AiRepository {
    private val detector = NcnnSafeDetector(assets, modelId, useGpu)

    override val events: Flow<com.lei.safety_hat_2.core.model.AiEvent> = detector.events

    override suspend fun start() {
        detector.start()
    }

    override suspend fun stop() {
        detector.stop()
    }

    override fun submitFrame(rgbaMat: Mat, pts: Long) {
        detector.submitFrame(rgbaMat, pts)
    }
}
