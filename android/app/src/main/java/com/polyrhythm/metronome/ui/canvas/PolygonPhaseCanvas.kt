package com.polyrhythm.metronome.ui.canvas

import androidx.compose.animation.core.*
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.sin

data class PolygonLayerVisual(
    val id: Int,
    val sides: Int,        // 3: Üçgen, 4: Kare, 5: Beşgen, vb.
    val phase: Float,      // 0.0f - 1.0f
    val color: Color,
    val baseRadius: Float
)

@Composable
fun PolygonPhaseCanvas(
    layers: List<PolygonLayerVisual>,
    masterPhase: Float,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier.fillMaxWidth().height(320.dp)) {
        val center = Offset(size.width / 2f, size.height / 2f)
        val maxAllowedRadius = (size.minDimension / 2f) * 0.85f

        // 1. Arka plan dairesel kılavuz çizgileri
        drawCircle(
            color = Color.White.copy(alpha = 0.05f),
            radius = maxAllowedRadius,
            center = center,
            style = Stroke(width = 1.5.dp.toPx())
        )

        // 2. Her ritim katmanı için dönen N-gen poligon çizimi
        layers.forEachIndexed { index, layer ->
            val radius = maxAllowedRadius * (0.45f + (index.toFloat() / (layers.size.coerceAtLeast(1) + 1)) * 0.55f)
            val numVertices = layer.sides.coerceAtLeast(2)
            val rotationAngle = layer.phase * 2f * PI.toFloat()

            drawRotatingPolygon(
                center = center,
                radius = radius,
                sides = numVertices,
                rotationRad = rotationAngle,
                color = layer.color,
                strokeWidth = 2.5.dp.toPx()
            )

            // Vuruş takipçisi (Orbital pulse beacon)
            val beaconAngle = rotationAngle - (PI / 2f).toFloat()
            val beaconPos = Offset(
                center.x + radius * cos(beaconAngle),
                center.y + radius * sin(beaconAngle)
            )

            drawCircle(
                color = layer.color,
                radius = 6.dp.toPx(),
                center = beaconPos
            )
            drawCircle(
                color = Color.White,
                radius = 3.dp.toPx(),
                center = beaconPos
            )
        }

        // 3. Faz Çakışma Noktaları (Master Downbeat Alignment Marker)
        val topCoincidencePoint = Offset(center.x, center.y - maxAllowedRadius)
        drawCircle(
            color = Color(0xFFFFD700).copy(alpha = 0.8f),
            radius = 5.dp.toPx(),
            center = topCoincidencePoint
        )
    }
}

private fun DrawScope.drawRotatingPolygon(
    center: Offset,
    radius: Float,
    sides: Int,
    rotationRad: Float,
    color: Color,
    strokeWidth: Float
) {
    if (sides < 3) {
        // 2 Vuruş için düz çizgi / elips
        drawLine(
            color = color.copy(alpha = 0.7f),
            start = Offset(center.x - radius, center.y),
            end = Offset(center.x + radius, center.y),
            strokeWidth = strokeWidth
        )
        return
    }

    val path = Path()
    val angleStep = (2f * PI / sides).toFloat()
    val initialOffset = - (PI / 2f).toFloat() + rotationRad

    for (i in 0 until sides) {
        val angle = initialOffset + i * angleStep
        val x = center.x + radius * cos(angle)
        val y = center.y + radius * sin(angle)

        if (i == 0) {
            path.moveTo(x, y)
        } else {
            path.lineTo(x, y)
        }
    }
    path.close()

    drawPath(
        path = path,
        color = color.copy(alpha = 0.75f),
        style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
    )

    // Köşe noktaları
    for (i in 0 until sides) {
        val angle = initialOffset + i * angleStep
        val vx = center.x + radius * cos(angle)
        val vy = center.y + radius * sin(angle)
        drawCircle(
            color = color,
            radius = 3.5.dp.toPx(),
            center = Offset(vx, vy)
        )
    }
}
