package com.lei.safety_hat_2.core.model

data class AiEvent(
    val type: String,
    val message: String,
    val boxes: List<BoundingBox>
)
