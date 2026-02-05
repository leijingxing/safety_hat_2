package com.lei.safety_hat_2

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.compose.ui.Modifier
import com.lei.safety_hat_2.ui.theme.Safety_hat_2Theme
import com.lei.safety_hat_2.ui.hud.HudScreen
import com.lei.safety_hat_2.ui.hud.HudViewModelFactory
import androidx.lifecycle.viewmodel.compose.viewModel

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        // Hide status/navigation bars for full-screen HUD.
        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
        setContent {
            Safety_hat_2Theme {
                val streamId = "123"
                val vm = viewModel<com.lei.safety_hat_2.ui.hud.HudViewModel>(
                    factory = HudViewModelFactory(
                        assets = assets,
                        context = applicationContext,
                        useGpu = false,
                        useDemoFrames = false,
                        rtmpBaseUrl = "rtmp://111.9.22.231:50168/live",
                        rtmpStreamId = "test123"
                    )
                )
                HudScreen(viewModel = vm)
            }
        }
    }
}
