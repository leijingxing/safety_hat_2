package com.test.libncnn;

import android.content.res.AssetManager;
import android.util.Log;
import android.view.Surface;

public class VestMod extends BaseAi {
    @Override
    public boolean loadModel(AssetManager mgr, int modelid, int cpugpu){
        return vestLoadModel(mgr, modelid, cpugpu);
    }

    @Override
    public boolean onRecvImage(long matptr, long pts) {
        return onImage(matptr);
    }

    @SuppressWarnings("unchecked")
    public void onValue(VestMod obj, String[] value) {
        if(obj!=null && obj.mListener!=null){
            // 强制转换 mListener 类型为 BaseListener<String[]>
            BaseListener<String[], ?> listener = (BaseListener<String[],?>) obj.mListener;
            listener.onValue(value);
        }
    }

    /**
     * A native method that is implemented by the 'libncnn' native library,
     * which is packaged with this application.
     */
    public native boolean vestLoadModel(AssetManager mgr, int modelid, int cpugpu);
    public native boolean setOutputWindow(Surface surface);
    public native boolean onImage(long rgbaPtr);
}
