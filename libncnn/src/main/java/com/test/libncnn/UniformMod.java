package com.test.libncnn;

import android.content.res.AssetManager;
import android.util.Log;
import android.view.Surface;

public class UniformMod extends BaseAi {
    @Override
    public boolean loadModel(AssetManager mgr, int modelid, int cpugpu){
        return uniformLoadModel(mgr, modelid, cpugpu);
    }

    public boolean onRecvImage(long matptr, long pts) {
        return onImage(matptr);
    }

    @SuppressWarnings("unchecked")
    public void onValue(UniformMod obj, String[] value) {
        if(obj!=null && obj.mListener!=null){
            // 强制转换 mListener 类型为 BaseListener<String[]>
            BaseListener<String[], Object> listener = (BaseListener<String[],Object>) obj.mListener;
            listener.onValue(value);
        }
    }

    /**
     * A native method that is implemented by the 'libncnn' native library,
     * which is packaged with this application.
     */
    public native boolean uniformLoadModel(AssetManager mgr, int modelid, int cpugpu);
    public native boolean setOutputWindow(Surface surface);
    public native boolean onImage(long rgbaPtr);
}
