package com.lei.safety_hat_2.data.stream

import com.lei.safety_hat_2.core.stream.ImageFrame
import com.lei.safety_hat_2.core.stream.ImageStreamer
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlin.random.Random

class FakeImageStreamer : ImageStreamer {
    override val frames: Flow<ImageFrame> = flow {
        while (true) {
            emit(
                ImageFrame(
                    width = 1280,
                    height = 720,
                    timestampMs = System.currentTimeMillis()
                )
            )
            delay(33 + Random.nextLong(0, 16))
        }
    }
}
