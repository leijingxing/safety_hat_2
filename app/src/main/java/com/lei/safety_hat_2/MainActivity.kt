package com.lei.safety_hat_2

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.ui.Modifier
import com.lei.safety_hat_2.ui.theme.Safety_hat_2Theme
import com.lei.safety_hat_2.ui.hud.HudScreen
import com.lei.safety_hat_2.ui.hud.HudViewModelFactory
import androidx.lifecycle.viewmodel.compose.viewModel

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            Safety_hat_2Theme {
                val vm = viewModel<com.lei.safety_hat_2.ui.hud.HudViewModel>(
                    factory = HudViewModelFactory(
                        assets = assets,
                        context = applicationContext,
                        useGpu = false,
                        useDemoFrames = false
                    )
                )
                HudScreen(viewModel = vm)
            }
        }
    }
}
