package com.test.libncnn;

import android.content.res.AssetManager;
import android.view.Surface;

import java.util.Timer;

public class NcnnFinger implements NcnnInterface {
    private static final String TAG = NcnnFinger.class.getSimpleName();
    private ValueListener listener;
    private Timer timer = null;
    public NcnnFinger(){
    }

    @Override
    public void setValueCallback(ValueListener listener) {
        this.listener = listener;
    }

    @Override
    public boolean onRecvImage(long matptr) {
        return onImage(matptr);
    }

    public void newValue(NcnnFinger obj, String label, float prob) {
        timer = new Timer();

        if(obj != null && obj.listener != null){
//            if(label.equals("fist")){
//                obj.listener.onSure();
//            }else{
                obj.listener.onValue(label);
//            }
        }
    }

    public void outValue(NcnnFinger obj) {
        obj.listener.onCancel();
    }

    // Used to load the 'yolo_ai' library on application startup.
    static {
        System.loadLibrary("yolo_ai");
    }

    /**
     * A native method that is implemented by the 'libncnn' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
    public native boolean setOutputWindow(Surface surface);
    private native boolean onImage(long rgbaPtr);
}
