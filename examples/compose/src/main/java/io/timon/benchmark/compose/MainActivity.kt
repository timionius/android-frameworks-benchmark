package io.timon.benchmark.compose

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
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

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        PixelSampler.onActivityResult(requestCode, resultCode, data)
    }
}

@Composable
fun SampleApp() {
    var targetScale by remember { mutableFloatStateOf(0.5f) }
    var targetColor by remember { mutableStateOf(Color.Red) }

    LaunchedEffect(Unit) {
        targetScale = 1.8f
        targetColor = Color.Blue
    }

    val scale by animateFloatAsState(
        targetValue = targetScale,
        animationSpec =
            tween(
                durationMillis = 1000,
                easing = LinearEasing,
            ),
    )

    val backgroundColor by animateColorAsState(
        targetValue = targetColor,
        animationSpec =
            tween(
                durationMillis = 1000,
                easing = LinearEasing,
            ),
        label = "color",
    )

    Box(
        modifier =
            Modifier
                .fillMaxSize()
                .background(backgroundColor),
        contentAlignment = Alignment.Center,
    ) {
        Box(
            modifier =
                Modifier
                    .size(180.dp)
                    .scale(scale)
                    .background(Color(0xFF00BFA5), CircleShape),
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center,
                modifier = Modifier.fillMaxSize(),
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
