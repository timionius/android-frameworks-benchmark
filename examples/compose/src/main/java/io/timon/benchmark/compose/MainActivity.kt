package io.timon.benchmark.compose

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import io.timon.android.pixelsampler.PixelSampler  // Correct import

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            PixelSamplerDemo(this@MainActivity)
        }
    }
}

@Composable
fun PixelSamplerDemo(activity: MainActivity) {
    var renderTime by remember { mutableStateOf<Long?>(null) }
    var isMeasuring by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "PixelSampler Benchmark",
            style = MaterialTheme.typography.headlineMedium
        )

        Spacer(modifier = Modifier.height(48.dp))

        Button(
            onClick = {
                isMeasuring = true
                renderTime = null

                try {
                    PixelSampler.start(activity) { timeMs: Long ->
                        renderTime = timeMs
                        isMeasuring = false
                        Log.i("PixelSampler", "✅ Render completed in $timeMs ms")
                    }
                    Log.i("PixelSampler", "start() called successfully")
                } catch (e: Exception) {
                    Log.e("PixelSampler", "Crash in start()", e)
                    isMeasuring = false
                }
            },
            enabled = !isMeasuring
        ) {
            Text(if (isMeasuring) "Measuring..." else "Start Render Time Measurement")
        }

        Spacer(modifier = Modifier.height(32.dp))

        renderTime?.let {
            Text(
                text = "Render completed in ${it} ms",
                style = MaterialTheme.typography.headlineSmall,
                color = MaterialTheme.colorScheme.primary
            )
        }
    }
}