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
            logs = listOf("SYSTEM REBOOTED", "INITIALIZING...", "READY")
        )
    }

    private fun addLog(message: String) {
        val currentLogs = state.logs.toMutableList()
        currentLogs.add(message.uppercase())
        if (currentLogs.size > 5) { // Keep a few more just in case
            currentLogs.removeAt(0)
        }
        state = state.copy(logs = currentLogs)
    }

    fun handleSerialData(data: String) {
        val trimmedData = data.trim()
        if (trimmedData.isEmpty()) return

        when {
            trimmedData.startsWith("P:") -> {
                val index = trimmedData.substringAfter("P:").toIntOrNull() ?: -1
                state = state.copy(activeMoleIndex = index)
//                if (index != -1) addLog("TARGET ACQUIRED: HOLE $index")
                
                // Note: 'Safety' timer would typically be handled by a LaunchedEffect 
                // in the Composable watching activeMoleIndex change, or here via viewModelScope.
            }
            trimmedData.startsWith("H:") -> {
                val scoreValue = trimmedData.substringAfter("H:").toIntOrNull() ?: (state.score + 1)
                state = state.copy(
                    score = scoreValue,
                    activeMoleIndex = -1
                )
                addLog("CRITICAL HIT! SCORE: $scoreValue")
            }
            trimmedData == "M" -> {
                val newMisses = state.misses + 1
                state = state.copy(
                    misses = newMisses,
                    isGameOver = newMisses >= 5,
                    activeMoleIndex = if (newMisses >= 5) -1 else state.activeMoleIndex
                )
                if (newMisses >= 5) {
                    addLog("SYSTEM FAILURE: GAME OVER")
                } else {
                    addLog("THREAT ESCAPED - MISS ($newMisses/5)")
                }
            }
            trimmedData.startsWith("LOG:") -> {
                val message = trimmedData.substringAfter("LOG:")
                addLog(message)
                
                if (message.contains("Level", ignoreCase = true)) {
                    val levelNum = message.filter { it.isDigit() }.toIntOrNull()
                    if (levelNum != null) {
                        state = state.copy(level = levelNum)
                    }
                }
            }
        }
    }
}
