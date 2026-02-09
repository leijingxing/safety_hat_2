package com.lei.safety_hat_2.ui.hud

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.TextButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import kotlinx.coroutines.delay
import androidx.compose.ui.platform.LocalConfiguration
import android.content.res.Configuration

@Composable
fun HudScreen(viewModel: HudViewModel = viewModel()) {
    val state by viewModel.state.collectAsState()
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
    Box(modifier = Modifier.fillMaxSize()) {
        FullScreenPreview(
            modifier = Modifier.fillMaxSize(),
            boxes = state.boxes,
            onFrame = viewModel::submitFrame,
            onFrameNv21 = viewModel::submitRtmpFrame
        )

        // Subtle sci-fi scrim for readability
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    Brush.verticalGradient(
                        listOf(
                            Color(0x99060A0E),
                            Color(0x33060A0E),
                            Color(0x99060A0E)
                        )
                    )
                )
        )

        if (isLandscape) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(14.dp)
            ) {
                Column(
                    modifier = Modifier.align(Alignment.TopStart),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    HeaderRow(
                        title = "智能安全帽",
                        alert = state.alertMessage,
                        alertId = state.alertId,
                        isStreaming = state.isStreaming,
                        onToggleStream = {
                            if (state.frameWidth > 0 && state.frameHeight > 0) {
                                viewModel.toggleStreaming(state.frameWidth, state.frameHeight)
                            }
                        }
                    )
                    CapabilityChips(
                        modifier = Modifier.padding(start = 6.dp),
                        items = state.capabilities
                    )
                }

                StatusPanel(
                    modifier = Modifier
                        .align(Alignment.CenterEnd)
                        .width(320.dp),
                    state = state
                )

                Box(
                    modifier = Modifier
                        .align(Alignment.BottomStart)
                        .width(360.dp)
                ) {
                    FootPanel(fps = state.fps)
                }
            }
        } else {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                HeaderRow(
                    title = "智能安全帽",
                    alert = state.alertMessage,
                    alertId = state.alertId,
                    isStreaming = state.isStreaming,
                    onToggleStream = {
                        if (state.frameWidth > 0 && state.frameHeight > 0) {
                            viewModel.toggleStreaming(state.frameWidth, state.frameHeight)
                        }
                    }
                )
                CapabilityChips(
                    modifier = Modifier.padding(start = 6.dp),
                    items = state.capabilities
                )

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Spacer(modifier = Modifier.weight(1.6f))
                    StatusPanel(
                        modifier = Modifier.weight(1f),
                        state = state
                    )
                }

                Spacer(modifier = Modifier.weight(1f))
                FootPanel(fps = state.fps)
            }
        }
    }
}

@Composable
private fun HeaderRow(
    title: String,
    alert: String,
    alertId: Long,
    isStreaming: Boolean,
    onToggleStream: () -> Unit
) {
    var visible by remember { mutableStateOf(false) }
    LaunchedEffect(alertId) {
        if (alertId > 0) {
            visible = true
            delay(2000)
            visible = false
        }
    }
    val alpha by animateFloatAsState(targetValue = if (visible) 1f else 0f, label = "alertAlpha")

    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column {
            Text(
                text = title,
                style = MaterialTheme.typography.headlineMedium.copy(
                    fontWeight = FontWeight.SemiBold,
                    letterSpacing = 0.5.sp
                ),
                color = Color(0xFFE8F1F4)
            )
            AlertBanner(
                text = alert,
                alpha = alpha
            )
        }
        Row(verticalAlignment = Alignment.CenterVertically) {
            StatusPill(label = "实时")
            Spacer(modifier = Modifier.width(10.dp))
            TextButton(onClick = onToggleStream) {
                Text(
                    text = if (isStreaming) "停止推流" else "开始推流",
                    color = if (isStreaming) Color(0xFFF97316) else Color(0xFF34D399),
                    fontWeight = FontWeight.SemiBold
                )
            }
        }
    }
}

@Composable
private fun AlertBanner(text: String, alpha: Float) {
    if (alpha <= 0.01f) return
    Row(
        modifier = Modifier
            .padding(top = 8.dp)
            .clip(RoundedCornerShape(12.dp))
            .background(
                Brush.horizontalGradient(
                    listOf(Color(0xFF7F1D1D), Color(0xFFEA580C), Color(0xFFF59E0B))
                )
            )
            .alpha(alpha)
            .padding(horizontal = 12.dp, vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .size(8.dp)
                .clip(CircleShape)
                .background(Color(0xFFFFEDD5))
        )
        Spacer(modifier = Modifier.width(8.dp))
        Text(
            text = text,
            color = Color(0xFFFFF7ED),
            fontWeight = FontWeight.SemiBold
        )
    }
}

@Composable
private fun StatusPill(label: String) {
    Row(
        modifier = Modifier
            .clip(CircleShape)
            .background(Color(0xFF1F5E59))
            .padding(horizontal = 12.dp, vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .size(8.dp)
                .clip(CircleShape)
                .background(Color(0xFF6EE7B7))
        )
        Spacer(modifier = Modifier.width(8.dp))
        Text(
            text = label,
            color = Color(0xFFCBF5E3),
            fontWeight = FontWeight.Medium
        )
    }
}

@Composable
private fun PreviewPanel(
    modifier: Modifier,
    boxes: List<com.lei.safety_hat_2.core.model.BoundingBox>,
    onFrame: (org.opencv.core.Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit
) {
    Card(
        modifier = modifier,
        shape = RoundedCornerShape(18.dp),
        colors = CardDefaults.cardColors(containerColor = Color(0xFF0E141A))
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            CameraPermissionGate {
                AdaptiveCameraPreview(
                    modifier = Modifier.fillMaxSize(),
                    onFrame = onFrame,
                    onFrameNv21 = onFrameNv21
                )
            }
            Canvas(modifier = Modifier.fillMaxSize()) {
                boxes.forEach { box ->
                    val left = box.left.coerceIn(0f, 1f) * size.width
                    val top = box.top.coerceIn(0f, 1f) * size.height
                    val right = box.right.coerceIn(0f, 1f) * size.width
                    val bottom = box.bottom.coerceIn(0f, 1f) * size.height
                    drawRect(
                        color = Color(0xFFE8A317),
                        topLeft = Offset(left, top),
                        size = Size(right - left, bottom - top),
                        style = Stroke(width = 3f)
                    )
                }
            }
        }
    }
}

@Composable
private fun StatusPanel(modifier: Modifier, state: HudState) {
    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        ViolationStatsCard(counts = state.violationCounts)
        ViolationList(
            title = "违规列表",
            items = state.violations
        )
    }
}

@Composable
private fun ViolationList(title: String, items: List<String>) {
    HudOverlayCard(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = title,
                color = Color(0xFFF59E0B),
                style = MaterialTheme.typography.bodyMedium,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(modifier = Modifier.height(10.dp))
            if (items.isEmpty()) {
                Text(
                    text = "暂无违规",
                    color = Color(0xFF7A9AA7)
                )
                return@Column
            }
            items.take(8).forEachIndexed { index, item ->
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Box(
                        modifier = Modifier
                            .size(6.dp)
                            .clip(CircleShape)
                            .background(Color(0xFFEF4444))
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text(
                        text = "${index + 1}. $item",
                        color = Color(0xFFFEE2E2),
                        fontWeight = FontWeight.Medium
                    )
                }
            }
        }
    }
}

@Composable
private fun ViolationStatsCard(counts: Map<String, Int>) {
    HudOverlayCard(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = "近 1 分钟违规统计",
                color = Color(0xFFF59E0B),
                style = MaterialTheme.typography.bodyMedium,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(modifier = Modifier.height(10.dp))
            if (counts.isEmpty()) {
                Text(
                    text = "暂无数据",
                    color = Color(0xFF7A9AA7)
                )
                return@Column
            }
            counts.forEach { (label, count) ->
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 4.dp),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = label,
                        color = Color(0xFFE5E7EB)
                    )
                    Text(
                        text = count.toString(),
                        color = if (count == 0) Color(0xFF7A9AA7) else Color(0xFFF97316),
                        fontWeight = FontWeight.SemiBold
                    )
                }
            }
        }
    }
}

@Composable
private fun CapabilityChips(modifier: Modifier, items: List<String>) {
    Row(
        modifier = modifier,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        items.forEach { item ->
            Box(
                modifier = Modifier
                    .clip(RoundedCornerShape(12.dp))
                    .background(Color(0xAA0F1D27))
                    .border(1.dp, Color(0x5538E8FF), RoundedCornerShape(12.dp))
                    .padding(horizontal = 10.dp, vertical = 4.dp)
            ) {
                Text(
                    text = item,
                    color = Color(0xFFE0F2FE),
                    fontSize = 12.sp
                )
            }
        }
    }
}

@Composable
private fun HudTile(title: String, value: String, accent: Color) {
    HudOverlayCard(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = title,
                color = Color(0xFF8BB7C9),
                style = MaterialTheme.typography.bodyMedium
            )
            Spacer(modifier = Modifier.height(6.dp))
            Text(
                text = value,
                color = accent,
                style = MaterialTheme.typography.titleLarge.copy(fontWeight = FontWeight.SemiBold)
            )
        }
    }
}

@Composable
private fun FootPanel(fps: Int) {
    HudOverlayCard(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp, vertical = 10.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "链路：相机 → AI → 推流",
                color = Color(0xFF7A9AA7)
            )
            Text(
                text = "FPS: $fps",
                color = Color(0xFF38E8FF),
                fontWeight = FontWeight.SemiBold
            )
        }
    }
}

@Composable
private fun FullScreenPreview(
    modifier: Modifier,
    boxes: List<com.lei.safety_hat_2.core.model.BoundingBox>,
    onFrame: (org.opencv.core.Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit
) {
    Box(modifier = modifier) {
        CameraPermissionGate {
            AdaptiveCameraPreview(
                modifier = Modifier.fillMaxSize(),
                onFrame = onFrame,
                onFrameNv21 = onFrameNv21
            )
        }
        Canvas(modifier = Modifier.fillMaxSize()) {
            // scanlines
            val step = 24f
            var y = 0f
            while (y < size.height) {
                drawLine(
                    color = Color(0x1100E5FF),
                    start = Offset(0f, y),
                    end = Offset(size.width, y),
                    strokeWidth = 1f
                )
                y += step
            }
            // bounding boxes
            boxes.forEach { box ->
                val left = box.left.coerceIn(0f, 1f) * size.width
                val top = box.top.coerceIn(0f, 1f) * size.height
                val right = box.right.coerceIn(0f, 1f) * size.width
                val bottom = box.bottom.coerceIn(0f, 1f) * size.height
                drawRect(
                    color = Color(0xFF38E8FF),
                    topLeft = Offset(left, top),
                    size = Size(right - left, bottom - top),
                    style = Stroke(width = 3f)
                )
                drawLine(
                    color = Color(0xFF38E8FF),
                    start = Offset(left, top),
                    end = Offset(left + 24f, top),
                    strokeWidth = 4f
                )
                drawLine(
                    color = Color(0xFF38E8FF),
                    start = Offset(right, bottom),
                    end = Offset(right - 24f, bottom),
                    strokeWidth = 4f
                )
            }
        }
    }
}

@Composable
private fun AdaptiveCameraPreview(
    modifier: Modifier = Modifier,
    onFrame: (org.opencv.core.Mat, Long) -> Unit,
    onFrameNv21: (ByteArray, Int, Int, Long) -> Unit
) {
    var fallbackToBuiltin by remember { mutableStateOf(false) }

    if (!fallbackToBuiltin) {
        UsbCameraPreview(
            modifier = modifier,
            onFrame = onFrame,
            onFrameNv21 = onFrameNv21,
            onPreviewError = {
                // 策略：USB 优先；USB 不可用或中断时自动回退到内置 CameraX。
                fallbackToBuiltin = true
            }
        )
    } else {
        CameraXPreview(
            modifier = modifier,
            onFrame = onFrame,
            onFrameNv21 = onFrameNv21
        )
    }
}

@Composable
private fun HudOverlayCard(
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit
) {
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(16.dp))
            .background(Color(0xA50B1218))
            .border(1.dp, Color(0x6638E8FF), RoundedCornerShape(16.dp))
    ) {
        content()
    }
}
