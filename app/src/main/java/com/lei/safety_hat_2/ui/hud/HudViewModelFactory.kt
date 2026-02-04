package com.lei.safety_hat_2.ui.hud

import android.content.res.AssetManager
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import android.content.Context
import com.lei.safety_hat_2.data.repository.NcnnCascadeAiRepository
import com.lei.safety_hat_2.data.repository.RtmpRepository

class HudViewModelFactory(
    private val assets: AssetManager,
    private val context: Context,
    private val useGpu: Boolean = false,
    private val useDemoFrames: Boolean = false,
    private val rtmpBaseUrl: String,
    private val rtmpStreamId: String
) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(HudViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return HudViewModel(
                aiRepository = NcnnCascadeAiRepository(
                    assets = assets,
                    useGpu = useGpu
                ),
                rtmpRepository = RtmpRepository(
                    baseUrl = rtmpBaseUrl,
                    streamId = rtmpStreamId
                ),
                useDemoFrames = useDemoFrames
            ) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class: ${modelClass.name}")
    }
}
