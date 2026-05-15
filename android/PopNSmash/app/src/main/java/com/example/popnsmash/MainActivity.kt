package com.example.popnsmash

import android.content.Context
import android.hardware.usb.UsbManager
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.lifecycleScope
import com.example.popnsmash.ui.WhackAMoleDashboard
import com.example.popnsmash.ui.WhackGameState
import com.example.popnsmash.ui.theme.PopNSmashTheme
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

import androidx.activity.viewModels
import com.example.popnsmash.ui.WhackViewModel

class MainActivity : ComponentActivity() {
    private var usbSerialPort: UsbSerialPort? = null
    private val viewModel: WhackViewModel by viewModels()
    private var isListening = false
    private val lineBuffer = StringBuilder()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        enableEdgeToEdge()
        
        viewModel.setSendCommandCallback { command ->
            writeToUsb(command)
        }

        setContent {
            PopNSmashTheme {
                WhackAMoleDashboard(
                    state = viewModel.state,
                    onStart = { viewModel.startGame() },
                    onStop = { viewModel.stopGame() },
                    onReset = { viewModel.resetGame() }
                )
            }
        }

        connectUsb()
    }

    private fun writeToUsb(command: String) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                usbSerialPort?.write((command + "\n").toByteArray(), 1000)
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    viewModel.updateConnectionStatus(false)
                }
            }
        }
    }

    private fun connectUsb() {
        val manager = getSystemService(Context.USB_SERVICE) as UsbManager
        val availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager)

        if (availableDrivers.isEmpty()) {
            viewModel.handleSerialData("LOG:No Arduino detected")
            return
        }

        val driver = availableDrivers[0]
        val connection = manager.openDevice(driver.device)
        if (connection == null) {
            viewModel.handleSerialData("LOG:USB Permission Denied")
            return
        }

        val port = driver.ports[0]
        try {
            port.open(connection)
            port.setParameters(9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)
            usbSerialPort = port
            viewModel.updateConnectionStatus(true)
            viewModel.handleSerialData("LOG:Connected! Ready to Smash!")
            startSerialListener()
        } catch (e: Exception) {
            viewModel.handleSerialData("LOG:Connection failed: ${e.message}")
        }
    }

    private fun startSerialListener() {
        if (isListening) return
        isListening = true
        lifecycleScope.launch(Dispatchers.IO) {
            val port = usbSerialPort ?: return@launch
            val buffer = ByteArray(1024)
            while (isListening && port.isOpen) {
                try {
                    val len = port.read(buffer, 1000)
                    if (len > 0) {
                        val received = String(buffer, 0, len)
                        withContext(Dispatchers.Main) {
                            receiveData(received)
                        }
                    }
                } catch (e: Exception) {
                    isListening = false
                    withContext(Dispatchers.Main) {
                        viewModel.updateConnectionStatus(false)
                        viewModel.handleSerialData("LOG:Connection Lost")
                    }
                    break
                }
            }
        }
    }

    private fun receiveData(data: String) {
        lineBuffer.append(data)
        var newlineIndex = lineBuffer.indexOf("\n")
        while (newlineIndex != -1) {
            val line = lineBuffer.substring(0, newlineIndex).trim()
            lineBuffer.delete(0, newlineIndex + 1)
            if (line.isNotEmpty()) {
                viewModel.handleSerialData(line)
            }
            newlineIndex = lineBuffer.indexOf("\n")
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        isListening = false
        try {
            usbSerialPort?.close()
        } catch (e: Exception) {
            // Ignore
        }
    }
}
