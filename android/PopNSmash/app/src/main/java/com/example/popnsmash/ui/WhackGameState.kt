package com.example.popnsmash.ui

data class WhackGameState(
    val score: Int = 0,
    val misses: Int = 0,
    val level: Int = 1,
    val activeMoleIndex: Int = -1,
    val isConnected: Boolean = false,
    val isGameOver: Boolean = false,
    val statusMessage: String = "Plug in Arduino to Start",
    val hitsCount: Int = 0
)