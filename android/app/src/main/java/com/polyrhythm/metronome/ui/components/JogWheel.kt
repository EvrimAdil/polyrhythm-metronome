package com.polyrhythm.metronome.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.*

@Composable
fun JogWheel(
    currentBpm: Double,
    onBpmChanged: (Double) -> Unit,
    onTapTempo: () -> Unit,
    modifier: Modifier = Modifier
) {
    var rotationAngle by remember { mutableStateOf(0f) }
    var previousTouchAngle by remember { mutableStateOf<Float?>(null) }
    var angleAccumulator by remember { mutableStateOf(0f) }
    val radPerBpm = 4.5f * (PI.toFloat() / 180f) // 4.5 degrees per 1 BPM

    Column(
        modifier = modifier.fillMaxWidth(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // Rotary Jog Wheel Container
        Box(
            modifier = Modifier
                .size(240.dp)
                .pointerInput(Unit) {
                    detectDragGestures(
                        onDragStart = { offset ->
                            val center = Offset(size.width / 2f, size.height / 2f)
                            previousTouchAngle = atan2(offset.y - center.y, offset.x - center.x)
                            angleAccumulator = 0f
                        },
                        onDragEnd = {
                            previousTouchAngle = null
                            angleAccumulator = 0f
                        },
                        onDragCancel = {
                            previousTouchAngle = null
                            angleAccumulator = 0f
                        },
                        onDrag = { change, _ ->
                            change.consume()
                            val center = Offset(size.width / 2f, size.height / 2f)
                            val currentAngle = atan2(change.position.y - center.y, change.position.x - center.x)
                            previousTouchAngle?.let { prev ->
                                var delta = currentAngle - prev
                                // Angle wrap-around handling (-PI to +PI)
                                if (delta > PI) delta -= (2 * PI).toFloat()
                                if (delta < -PI) delta += (2 * PI).toFloat()

                                rotationAngle += delta
                                angleAccumulator += delta

                                // Hassasiyet Ölçekleme: Her 4.5 derecede 1 BPM
                                val bpmSteps = (angleAccumulator / radPerBpm).toInt()
                                if (bpmSteps != 0) {
                                    angleAccumulator -= bpmSteps * radPerBpm
                                    val newBpm = (currentBpm + bpmSteps).coerceIn(20.0, 400.0)
                                    onBpmChanged(round(newBpm))
                                }
                            }
                            previousTouchAngle = currentAngle
                        }
                    )
                },
            contentAlignment = Alignment.Center
        ) {
            // Dairesel çentikli kadran görseli
            Canvas(modifier = Modifier.fillMaxSize()) {
                val center = Offset(size.width / 2f, size.height / 2f)
                val outerRadius = size.minDimension / 2f - 12.dp.toPx()
                val innerRadius = outerRadius - 16.dp.toPx()

                // Arka plan dış halka
                drawCircle(
                    brush = Brush.radialGradient(
                        colors = listOf(Color(0xFF242730), Color(0xFF14161D)),
                        center = center,
                        radius = outerRadius
                    ),
                    radius = outerRadius,
                    center = center
                )

                // Kadran çentikleri (Notches)
                val totalNotches = 60
                for (i in 0 until totalNotches) {
                    val angle = (i.toFloat() / totalNotches) * 2f * PI.toFloat() + rotationAngle
                    val isMajor = (i % 5 == 0)
                    val r1 = outerRadius - (if (isMajor) 14.dp.toPx() else 8.dp.toPx())
                    val r2 = outerRadius - 2.dp.toPx()

                    val start = Offset(center.x + r1 * cos(angle), center.y + r1 * sin(angle))
                    val end = Offset(center.x + r2 * cos(angle), center.y + r2 * sin(angle))

                    drawLine(
                        color = if (isMajor) Color(0xFF00E5FF) else Color.White.copy(alpha = 0.25f),
                        start = start,
                        end = end,
                        strokeWidth = if (isMajor) 2.5.dp.toPx() else 1.5.dp.toPx(),
                        cap = StrokeCap.Round
                    )
                }

                // Dış mavi neon halka
                drawCircle(
                    color = Color(0xFF00E5FF).copy(alpha = 0.4f),
                    radius = outerRadius,
                    center = center,
                    style = Stroke(width = 2.dp.toPx())
                )
            }

            // Merkez BPM Göstergesi
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center,
                modifier = Modifier
                    .size(130.dp)
                    .clip(CircleShape)
                    .background(Color(0xFF0D0E12))
            ) {
                Text(
                    text = String.format("%.0f", currentBpm),
                    fontSize = 38.sp,
                    fontWeight = FontWeight.Bold,
                    color = Color.White
                )
                Text(
                    text = "BPM",
                    fontSize = 12.sp,
                    fontWeight = FontWeight.SemiBold,
                    color = Color(0xFF00E5FF)
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Tap Tempo ve Hassas Ayar Butonları (+/-)
        Row(
            modifier = Modifier.fillMaxWidth(0.85f),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            FilledTonalButton(
                onClick = { onBpmChanged((currentBpm - 1.0).coerceAtLeast(20.0)) },
                colors = ButtonDefaults.filledTonalButtonColors(containerColor = Color(0xFF20232B))
            ) {
                Text("-1", color = Color.White, fontWeight = FontWeight.Bold)
            }

            Button(
                onClick = onTapTempo,
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF00E5FF)),
                modifier = Modifier.height(44.dp)
            ) {
                Text("TAP TEMPO", color = Color.Black, fontWeight = FontWeight.ExtraBold)
            }

            FilledTonalButton(
                onClick = { onBpmChanged((currentBpm + 1.0).coerceAtMost(400.0)) },
                colors = ButtonDefaults.filledTonalButtonColors(containerColor = Color(0xFF20232B))
            ) {
                Text("+1", color = Color.White, fontWeight = FontWeight.Bold)
            }
        }
    }
}
