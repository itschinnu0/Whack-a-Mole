package com.example.popnsmash.ui

/**
 * Refined state for the ultra-minimal dashboard.
 */
data class WhackGameState(
    val score: Int = 0,
    val misses: Int = 0,
    val level: Int = 1,
    val activeMoleIndex: Int = -1,
    val isConnected: Boolean = false,
    val isGameOver: Boolean = false,
    val isCountingDown: Boolean = false,
    val statusMessage: String = "READY",
    val logs: List<String> = listOf("SYSTEM READY")
)
