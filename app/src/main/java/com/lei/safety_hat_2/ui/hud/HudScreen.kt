package com.lei.safety_hat_2.ui.hud

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
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
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel

@Composable
fun HudScreen(viewModel: HudViewModel = viewModel()) {
    val state by viewModel.state.collectAsState()
    val bg = Brush.verticalGradient(
        listOf(Color(0xFF0B1F2A), Color(0xFF162B33), Color(0xFF1C1D1F))
    )

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(bg)
            .padding(16.dp)
    ) {
        Column(
            modifier = Modifier.fillMaxSize(),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            HeaderRow(
                title = "Safety Hat HUD",
                alert = state.alertMessage
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                PreviewPanel(
                    modifier = Modifier.weight(1.4f),
                    boxes = state.boxes
                )
                StatusPanel(
                    modifier = Modifier.weight(1f),
                    state = state
                )
            }
            FootPanel(
                fps = state.fps
            )
        }
    }
}

@Composable
private fun HeaderRow(title: String, alert: String) {
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
            Text(
                text = alert,
                style = MaterialTheme.typography.bodyMedium,
                color = Color(0xFF9FD6E8)
            )
        }
        StatusPill(label = "LIVE")
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
    boxes: List<com.lei.safety_hat_2.core.model.BoundingBox>
) {
    Card(
        modifier = modifier.height(420.dp),
        shape = RoundedCornerShape(18.dp),
        colors = CardDefaults.cardColors(containerColor = Color(0xFF0E141A))
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(
                        Brush.radialGradient(
                            listOf(Color(0xFF20313B), Color(0xFF0A0F12))
                        )
                    )
            )
            Text(
                text = "Camera Preview",
                color = Color(0xFF7FB3C6),
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .padding(16.dp)
            )
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
        HudTile(
            title = "设备状态",
            value = if (state.systemStatus.isCameraOnline) "摄像头在线" else "摄像头离线",
            accent = Color(0xFF53D3C7)
        )
        HudTile(
            title = "推流",
            value = if (state.systemStatus.isStreaming) "RTMP 连接中" else "未连接",
            accent = Color(0xFFFAA307)
        )
        HudTile(
            title = "电量",
            value = "${state.systemStatus.batteryPercent}%",
            accent = Color(0xFF52B788)
        )
        HudTile(
            title = "温度",
            value = "${state.systemStatus.temperatureC.toInt()}°C",
            accent = Color(0xFFE56B6F)
        )
        HudTile(
            title = "CPU 负载",
            value = "${state.systemStatus.cpuLoad}%",
            accent = Color(0xFF4EA8DE)
        )
    }
}

@Composable
private fun HudTile(title: String, value: String, accent: Color) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(containerColor = Color(0xFF111820))
    ) {
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
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(14.dp))
            .background(Color(0xFF0E151C))
            .padding(horizontal = 16.dp, vertical = 10.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = "Pipeline: Camera → AI → Stream",
            color = Color(0xFF7A9AA7)
        )
        Text(
            text = "FPS: $fps",
            color = Color(0xFF9AD0D3),
            fontWeight = FontWeight.Medium
        )
    }
}
