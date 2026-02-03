package com.lei.safety_hat_2.data.repository

import com.lei.safety_hat_2.core.model.AiEvent
import com.lei.safety_hat_2.core.model.BoundingBox
import com.lei.safety_hat_2.domain.repository.AiRepository
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlin.random.Random
import org.opencv.core.Mat

class FakeAiRepository : AiRepository {
    private val scope = CoroutineScope(Dispatchers.Default)
    private val _events = MutableSharedFlow<AiEvent>(replay = 0, extraBufferCapacity = 1)
    override val events: Flow<AiEvent> = _events.asSharedFlow()

    private var job: Job? = null

    override suspend fun start() {
        if (job != null) return
        job = scope.launch {
            while (isActive) {
                val boxes = listOf(
                    BoundingBox(
                        left = Random.nextFloat() * 0.4f,
                        top = Random.nextFloat() * 0.4f,
                        right = 0.6f + Random.nextFloat() * 0.3f,
                        bottom = 0.6f + Random.nextFloat() * 0.3f,
                        label = listOf("helmet", "smoke", "phone").random(),
                        confidence = 0.6f + Random.nextFloat() * 0.35f
                    )
                )
                _events.tryEmit(
                    AiEvent(
                        type = "demo",
                        message = "AI event",
                        boxes = boxes
                    )
                )
                delay(700)
            }
        }
    }

    override suspend fun stop() {
        job?.cancel()
        job = null
    }

    override fun submitFrame(rgbaMat: Mat, pts: Long) {
        // no-op for fake repository
    }
}
