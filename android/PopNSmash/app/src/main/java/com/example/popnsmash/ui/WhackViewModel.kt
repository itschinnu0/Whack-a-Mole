package com.example.popnsmash.ui

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel

class WhackViewModel : ViewModel() {
    var state by mutableStateOf(WhackGameState())
        private set

    private var onSendCommand: ((String) -> Unit)? = null

    fun setSendCommandCallback(callback: (String) -> Unit) {
        onSendCommand = callback
    }

    fun updateConnectionStatus(isConnected: Boolean) {
        state = state.copy(isConnected = isConnected)
    }

    fun startGame() {
        writeSerialData("S")
        resetLocalStats()
    }

    fun stopGame() {
        writeSerialData("X")
        state = state.copy(isGameOver = true)
    }

    fun resetGame() {
        writeSerialData("R")
        resetLocalStats()
    }

    fun writeSerialData(command: String) {
        onSendCommand?.invoke(command)
    }

    private fun resetLocalStats() {
        state = state.copy(
            score = 0,
            misses = 0,
            level = 1,
            activeMoleIndex = -1,
            isGameOver = false,
            hitsCount = 0,
            statusMessage = "Ready!"
        )
    }

    fun handleSerialData(data: String) {
        val trimmedData = data.trim()
        if (trimmedData.isEmpty()) return

        when {
            trimmedData.startsWith("P:") -> {
                val index = trimmedData.substringAfter("P:").toIntOrNull() ?: -1
                state = state.copy(activeMoleIndex = index)
            }
            trimmedData.startsWith("H:") -> {
                val scoreValue = trimmedData.substringAfter("H:").toIntOrNull() ?: state.score
                val newHitsCount = state.hitsCount + 1
                val newLevel = when {
                    newHitsCount < 5 -> 1
                    newHitsCount < 20 -> 2
                    else -> 3
                }
                state = state.copy(
                    score = scoreValue,
                    misses = 0,
                    hitsCount = newHitsCount,
                    level = newLevel,
                    statusMessage = "HIT!",
                    activeMoleIndex = -1
                )
            }
            trimmedData == "M" -> {
                val newMisses = state.misses + 1
                if (newMisses >= 5) {
                    state = state.copy(
                        misses = newMisses,
                        isGameOver = true,
                        statusMessage = "GAME OVER",
                        activeMoleIndex = -1
                    )
                } else {
                    state = state.copy(
                        misses = newMisses,
                        statusMessage = "MISS!"
                    )
                }
            }
            trimmedData.startsWith("LOG:") -> {
                val message = trimmedData.substringAfter("LOG:")
                state = state.copy(statusMessage = message)
                
                // Handle sub-messages to reset local state if needed
                if (message.contains("Game Started", ignoreCase = true) || 
                    message.contains("Score Reset", ignoreCase = true)) {
                    resetLocalStats()
                } else if (message.contains("Level", ignoreCase = true)) {
                    val levelNum = message.filter { it.isDigit() }.toIntOrNull()
                    if (levelNum != null) {
                        state = state.copy(level = levelNum)
                    }
                }
            }
        }
    }
}
