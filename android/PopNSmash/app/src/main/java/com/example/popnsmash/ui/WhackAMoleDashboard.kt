package com.example.popnsmash.ui

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.GenericShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.scale
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.popnsmash.ui.theme.HitGreen
import com.example.popnsmash.ui.theme.MissRed
import com.example.popnsmash.ui.theme.NeonCyan

/**
 * Custom Hexagon shape for the Level indicator.
 */
val HexagonShape = GenericShape { size, _ ->
    moveTo(size.width * 0.25f, 0f)
    lineTo(size.width * 0.75f, 0f)
    lineTo(size.width, size.height * 0.5f)
    lineTo(size.width * 0.75f, size.height)
    lineTo(size.width * 0.25f, size.height)
    lineTo(0f, size.height * 0.5f)
    close()
}

/**
 * Modifier extension to apply a neon glow effect using layered drawing.
 */
fun Modifier.neonGlow(
    color: Color,
    pulseAlpha: Float,
    radius: Dp = 12.dp
): Modifier = this.drawBehind {
    val pixelRadius = radius.toPx()
    // Inner Glow Layer
    drawCircle(
        color = color.copy(alpha = pulseAlpha * 0.4f),
        radius = size.minDimension / 2 + pixelRadius,
        center = center,
        style = Stroke(width = pixelRadius)
    )
    // Outer Halo Layer
    drawCircle(
        color = color.copy(alpha = pulseAlpha * 0.15f),
        radius = size.minDimension / 2 + pixelRadius * 2.5f,
        center = center,
        style = Stroke(width = pixelRadius * 1.2f)
    )
}

@Composable
fun TargetingReticle(index: Int, pulseAlpha: Float) {
    val infiniteTransition = rememberInfiniteTransition(label = "reticle_pulse")
    val scale by infiniteTransition.animateFloat(
        initialValue = 1f,
        targetValue = 1.15f,
        animationSpec = infiniteRepeatable(
            animation = tween(800, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse
        ), label = "scale"
    )

    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
        modifier = Modifier.fillMaxSize()
    ) {
        Box(
            modifier = Modifier
                .size(240.dp)
                .scale(scale)
                .neonGlow(NeonCyan, pulseAlpha, radius = 20.dp)
                .drawBehind {
                    val strokeWidth = 3.dp.toPx()
                    val color = NeonCyan.copy(alpha = pulseAlpha + 0.2f)
                    val pad = 10.dp.toPx()
                    val len = 50.dp.toPx()

                    // Corners
                    drawLine(color, Offset(pad, pad), Offset(pad + len, pad), strokeWidth)
                    drawLine(color, Offset(pad, pad), Offset(pad, pad + len), strokeWidth)
                    drawLine(color, Offset(size.width - pad, pad), Offset(size.width - pad - len, pad), strokeWidth)
                    drawLine(color, Offset(size.width - pad, pad), Offset(size.width - pad, pad + len), strokeWidth)
                    drawLine(color, Offset(pad, size.height - pad), Offset(pad + len, size.height - pad), strokeWidth)
                    drawLine(color, Offset(pad, size.height - pad), Offset(pad, size.height - pad - len), strokeWidth)
                    drawLine(color, Offset(size.width - pad, size.height - pad), Offset(size.width - pad - len, size.height - pad), strokeWidth)
                    drawLine(color, Offset(size.width - pad, size.height - pad), Offset(size.width - pad, size.height - pad - len), strokeWidth)

                    // Reticle Cross
                    val cX = size.width / 2
                    val cY = size.height / 2
                    val cLen = 15.dp.toPx()
                    drawLine(color, Offset(cX - cLen, cY), Offset(cX + cLen, cY), 1.dp.toPx())
                    drawLine(color, Offset(cX, cY - cLen), Offset(cX, cY + cLen), 1.dp.toPx())
                },
            contentAlignment = Alignment.Center
        ) {
            Box(
                modifier = Modifier
                    .size(160.dp)
                    .border(1.dp, NeonCyan.copy(alpha = pulseAlpha * 0.5f), CircleShape)
            )
        }

        Spacer(modifier = Modifier.height(32.dp))

        Text(
            text = "TARGET LOCKED: HOLE $index",
            color = NeonCyan,
            fontSize = 20.sp,
            fontWeight = FontWeight.Black,
            style = TextStyle(letterSpacing = 2.sp)
        )
    }
}

@Composable
fun CyberMissTracker(misses: Int) {
    Row(
        modifier = Modifier.padding(vertical = 16.dp),
        horizontalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        repeat(5) { index ->
            val isActive = index < misses
            Box(
                modifier = Modifier
                    .size(14.dp)
                    .clip(CircleShape)
                    .background(if (isActive) MissRed else Color(0xFF222222))
                    .border(1.dp, if (isActive) MissRed.copy(alpha = 0.5f) else Color.Transparent, CircleShape)
            )
        }
    }
}

@Composable
fun WhackAMoleDashboard(
    state: WhackGameState,
    onStart: () -> Unit,
    onStop: () -> Unit,
    onReset: () -> Unit
) {
    // Score Scale Animation
    val scoreScale = remember { Animatable(1f) }
    LaunchedEffect(state.score) {
        if (state.score > 0) {
            scoreScale.animateTo(
                targetValue = 1.15f,
                animationSpec = spring(
                    dampingRatio = Spring.DampingRatioMediumBouncy,
                    stiffness = Spring.StiffnessLow
                )
            )
            scoreScale.animateTo(1f)
        }
    }

    Scaffold(
        containerColor = Color.Black,
        topBar = {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .statusBarsPadding()
                    .padding(horizontal = 24.dp, vertical = 20.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                // Connection Badge
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier
                        .clip(RoundedCornerShape(100.dp))
                        .background(if (state.isConnected) Color(0xFF1B3320) else Color(0xFF331B1B))
                        .padding(horizontal = 12.dp, vertical = 6.dp)
                ) {
                    Box(
                        modifier = Modifier
                            .size(6.dp)
                            .clip(CircleShape)
                            .background(if (state.isConnected) Color(0xFF4CAF50) else Color(0xFFF44336))
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text(
                        text = if (state.isConnected) "ONLINE" else "OFFLINE",
                        color = if (state.isConnected) Color(0xFF4CAF50) else Color(0xFFF44336),
                        fontSize = 11.sp,
                        fontWeight = FontWeight.Bold,
                        letterSpacing = 1.sp
                    )
                }

                // Level Indicator
                Text(
                    text = "LVL ${state.level}",
                    color = Color(0xFFAAAAAA),
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Medium,
                    letterSpacing = 1.sp
                )
            }
        },
        bottomBar = {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .navigationBarsPadding()
                    .padding(24.dp)
                    .height(56.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                Button(
                    onClick = onStart,
                    modifier = Modifier.weight(1f).fillMaxHeight(),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF2E7D32)),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text("START", color = Color.White, fontWeight = FontWeight.Bold)
                }
                Button(
                    onClick = onStop,
                    modifier = Modifier.weight(1f).fillMaxHeight(),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFFC62828)),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text("STOP", color = Color.White, fontWeight = FontWeight.Bold)
                }
                Button(
                    onClick = onReset,
                    modifier = Modifier.weight(1f).fillMaxHeight(),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF424242)),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text("RESET", color = Color.White, fontWeight = FontWeight.Bold)
                }
            }
        }
    ) { paddingValues ->
        Box(modifier = Modifier.fillMaxSize()) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .padding(horizontal = 24.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                // Hero Stats
                Column(
                    modifier = Modifier.weight(1f),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    if (state.isCountingDown) {
                        Text(
                            text = state.statusMessage,
                            color = NeonCyan,
                            fontSize = 48.sp,
                            fontWeight = FontWeight.Black,
                            modifier = Modifier.neonGlow(NeonCyan, 1f)
                        )
                    } else {
                        Text(
                            text = "${state.score}",
                            color = Color.White,
                            fontSize = 100.sp,
                            fontWeight = FontWeight.Thin,
                            modifier = Modifier.scale(scoreScale.value)
                        )

                        Spacer(modifier = Modifier.height(8.dp))

                        Text(
                            text = if (state.activeMoleIndex == -1) {
                                if (state.isGameOver) "GAME OVER" else ""
                            } else "ACTIVE HOLE: ${state.activeMoleIndex}",
                            color = if (state.activeMoleIndex == -1) {
                                if (state.isGameOver) MissRed else Color.Gray
                            } else Color(0xFF00E5FF),
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Bold,
                            letterSpacing = 2.sp
                        )

                        Spacer(modifier = Modifier.height(32.dp))

                        // Miss Tracker (Lives Remaining)
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(
                                text = "LIVES: ${10 - state.misses}",
                                color = if (state.misses > 7) MissRed else Color.Gray,
                                fontSize = 12.sp,
                                fontWeight = FontWeight.Bold,
                                letterSpacing = 1.sp
                            )
                            Spacer(modifier = Modifier.height(12.dp))
                            Row(
                                horizontalArrangement = Arrangement.spacedBy(8.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                repeat(10) { index ->
                                    val isMissed = index < state.misses
                                    Box(
                                        modifier = Modifier
                                            .size(10.dp)
                                            .clip(CircleShape)
                                            .background(if (isMissed) Color(0xFFF44336) else Color(0xFF212121))
                                    )
                                }
                            }
                        }
                    }
                }
            }

            // Game Over Overlay
            if (state.isGameOver) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(Color.Black.copy(alpha = 0.9f)),
                    contentAlignment = Alignment.Center
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = "MISSION FAILED!",
                            color = MissRed,
                            fontSize = 40.sp,
                            fontWeight = FontWeight.Black,
                            modifier = Modifier.neonGlow(MissRed, 1f)
                        )
                        Spacer(modifier = Modifier.height(16.dp))
                        Text(
                            text = "FINAL SCORE: ${state.score}",
                            color = Color.White,
                            fontSize = 24.sp,
                            fontWeight = FontWeight.Light
                        )
                        Spacer(modifier = Modifier.height(48.dp))
                        Button(
                            onClick = onReset,
                            colors = ButtonDefaults.buttonColors(containerColor = MissRed),
                            shape = RoundedCornerShape(8.dp),
                            modifier = Modifier
                                .height(56.dp)
                                .width(220.dp)
                        ) {
                            Text("PLAY AGAIN!", color = Color.White, fontWeight = FontWeight.Bold)
                        }
                    }
                }
            }
        }
    }
}
