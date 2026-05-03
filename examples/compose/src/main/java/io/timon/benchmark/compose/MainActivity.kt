package io.timon.benchmark.compose

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.timon.android.pixelsampler.PixelSampler

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Start sampler immediately (no callback needed anymore)
        PixelSampler.start(this)

        setContent {
            SampleApp()
        }
    }

    override fun onDestroy() {
        PixelSampler.release()
        super.onDestroy()
    }
}

@Composable
fun SampleApp() {
    // 1-second basic animation that starts RIGHT AFTER LAUNCH
    var targetScale by remember { mutableStateOf(0.5f) }

    LaunchedEffect(Unit) {
        targetScale = 1.8f   // triggers animation instantly
    }

    val scale by animateFloatAsState(
        targetValue = targetScale,
        animationSpec = tween(
            durationMillis = 1000,      // exactly 1 second
            easing = LinearEasing
        )
    )

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF1A1A1A)),
        contentAlignment = Alignment.Center
    ) {
        Box(
            modifier = Modifier
                .size(180.dp)
                .scale(scale)
                .background(Color(0xFF00BFA5), CircleShape)
        ) {
            Text(
                text = "ANIMATING",
                color = Color.White,
                fontSize = 18.sp,
                modifier = Modifier.align(Alignment.Center)
            )
        }
    }
}