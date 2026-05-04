package io.timon.benchmark.compose

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.animateColorAsState
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

        PixelSampler.start(this)

        setContent {
            SampleApp()
        }
    }

    override fun onDestroy() {
        PixelSampler.stop()
        super.onDestroy()
    }
}

@Composable
fun SampleApp() {
    // 1-second basic animation that starts RIGHT AFTER LAUNCH
    var targetScale by remember { mutableStateOf(0.5f) }
    var targetColor by remember { mutableStateOf(Color.Red) }

    LaunchedEffect(Unit) {
        targetScale = 1.8f   // triggers animation instantly
        targetColor = Color.Blue  // color animation
    }

    val scale by animateFloatAsState(
        targetValue = targetScale,
        animationSpec = tween(
            durationMillis = 1000,      // exactly 1 second
            easing = LinearEasing
        )
    )

    // Color animation
    val backgroundColor by animateColorAsState(
        targetValue = targetColor,
        animationSpec = tween(
            durationMillis = 1000,
            easing = LinearEasing
        ),
        label = "color"
    )

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(backgroundColor),
        contentAlignment = Alignment.Center
    ) {
        Box(
            modifier = Modifier
                .size(180.dp)
                .scale(scale)
                .background(Color(0xFF00BFA5), CircleShape)
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center,
                modifier = Modifier.fillMaxSize()
            ) {
                Text(
                    text = "ANIMATING",
                    color = Color.White,
                    fontSize = 18.sp,
                )
            }
        }
    }
}