package com.lei.safety_hat_2.core.stream

import kotlinx.coroutines.flow.Flow

interface ImageStreamer {
    val frames: Flow<ImageFrame>
}
