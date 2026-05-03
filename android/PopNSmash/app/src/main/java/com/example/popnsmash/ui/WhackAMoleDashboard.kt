package com.example.popnsmash.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.popnsmash.ui.theme.DarkBackground
import com.example.popnsmash.ui.theme.HitGreen
import com.example.popnsmash.ui.theme.MissRed
import com.example.popnsmash.ui.theme.NeonCyan

@Composable
fun MoleHole(index: Int, isActive: Boolean) {
    Box(
        modifier = Modifier
            .padding(8.dp)
            .aspectRatio(1f)
            .clip(RoundedCornerShape(12.dp))
            .background(if (isActive) NeonCyan.copy(alpha = 0.2f) else Color.White.copy(alpha = 0.05f))
            .border(
                width = 2.dp,
                color = if (isActive) NeonCyan else Color.Gray.copy(alpha = 0.3f),
                shape = RoundedCornerShape(12.dp)
            ),
        contentAlignment = Alignment.Center
    ) {
        if (isActive) {
            Text("!", color = NeonCyan, fontSize = 24.sp, fontWeight = FontWeight.Bold)
        } else {
            Text(index.toString(), color = Color.Gray, fontSize = 12.sp)
        }
    }
}

@Composable
fun MissTracker(misses: Int) {
    Row(
        modifier = Modifier.padding(vertical = 8.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        repeat(5) { index ->
            Box(
                modifier = Modifier
                    .size(20.dp)
                    .clip(CircleShape)
                    .background(if (index < misses) MissRed else Color.Gray.copy(alpha = 0.3f))
                    .border(1.dp, if (index < misses) MissRed else Color.Gray, CircleShape),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    "X",
                    color = if (index < misses) Color.White else Color.Transparent,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.Bold
                )
            }
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
    Box(modifier = Modifier.fillMaxSize()) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(DarkBackground)
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // Header: Score, Level & Connection
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.Top
            ) {
                Column {
                    Text("SCORE", color = Color.Gray, fontSize = 12.sp)
                    Text("${state.score}", color = Color.White, fontSize = 42.sp, fontWeight = FontWeight.Black)
                    Text("LEVEL ${state.level}", color = NeonCyan, fontSize = 14.sp, fontWeight = FontWeight.Bold)
                }

                val connectionColor = if (state.isConnected) HitGreen else MissRed
                Text(
                    text = if (state.isConnected) "ONLINE" else "OFFLINE",
                    color = connectionColor,
                    modifier = Modifier
                        .border(1.dp, connectionColor, RoundedCornerShape(4.dp))
                        .padding(horizontal = 8.dp, vertical = 4.dp)
                )
            }

            Spacer(modifier = Modifier.height(16.dp))

            MissTracker(misses = state.misses)

            Spacer(modifier = Modifier.height(16.dp))

            // Grid
            LazyVerticalGrid(
                columns = GridCells.Fixed(2),
                modifier = Modifier.weight(1f)
            ) {
                items(6) { index ->
                    MoleHole(index = index, isActive = state.activeMoleIndex == index)
                }
            }

            // Status Message
            Text(
                text = state.statusMessage.uppercase(),
                color = Color.Cyan.copy(alpha = 0.7f),
                fontSize = 14.sp,
                fontWeight = FontWeight.Medium,
                modifier = Modifier.padding(vertical = 16.dp)
            )

            // Controls
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Button(
                    onClick = onStart,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = HitGreen),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text("START", color = DarkBackground, fontWeight = FontWeight.Bold)
                }
                Button(
                    onClick = onStop,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = MissRed),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text("STOP", color = Color.White, fontWeight = FontWeight.Bold)
                }
                Button(
                    onClick = onReset,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF2196F3)),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text("RESET", color = Color.White, fontWeight = FontWeight.Bold)
                }
            }
        }

        // Game Over Overlay
        if (state.isGameOver) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(Color.Black.copy(alpha = 0.85f)),
                contentAlignment = Alignment.Center
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        "GAME OVER",
                        color = MissRed,
                        fontSize = 48.sp,
                        fontWeight = FontWeight.Black
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        "FINAL SCORE: ${state.score}",
                        color = Color.White,
                        fontSize = 24.sp
                    )
                    Spacer(modifier = Modifier.height(32.dp))
                    Button(
                        onClick = onReset,
                        colors = ButtonDefaults.buttonColors(containerColor = NeonCyan),
                        shape = RoundedCornerShape(12.dp),
                        modifier = Modifier.height(56.dp).width(200.dp)
                    ) {
                        Text("RESTART", color = DarkBackground, fontSize = 18.sp, fontWeight = FontWeight.Bold)
                    }
                }
            }
        }
    }
}
