package com.test.libncnn;

import android.content.res.AssetManager;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.util.ArrayMap;
import android.util.Log;
import android.view.Surface;

import org.opencv.core.Mat;
import org.opencv.core.Rect;

import java.nio.ByteBuffer;
import java.util.HashMap;

public class SafeMod extends BaseAi {
    AssetManager mgr = null;
    private long pts = 0;
    private Mat img = null;
    private ArrayMap<Long,Long> map= new ArrayMap<Long,Long>();
//
//    public boolean onRecvImage(long matptr) {
//        return onImage(matptr);
//    }
    public boolean onRecvImage(Mat src, long matptr, long pts) {
        this.img = src;
        this.pts = pts;
        // 存放当前时间戳
        map.put(pts,System.currentTimeMillis());
        return onImage(matptr);
    }

    public void arrayValue(SafeMod obj, SafeObj[] array) {
        Log.v(TAG, "arrayValue len:"+array.length);
        if(obj!=null && obj.mListener!=null){
            // 强制转换 mListener 类型为 BaseListener<String[]>
            BaseListener<?, SafeObj[]> listener = (BaseListener<?,SafeObj[]>) obj.mListener;
            long start= map.remove(obj.pts);
           long time= System.currentTimeMillis()-start;
           long time1= System.currentTimeMillis()- obj.pts;
           long time2= start- obj.pts;
            Log.i(TAG, "帧时间："+obj.pts+"arrayValue: 耗时："+time +"出帧到计算完成："+time1+"  出帧到开始计算:"+time2);
            listener.onValue(obj.img, obj.pts, array);
        }
    }

    public void callFreeType(SafeMod obj) {
        Typeface font = Typeface.createFromAsset(obj.mgr, "SimSun.ttf");
        Paint paint = new Paint();
        paint.setTypeface(font);
    }

    // Used to load the 'libncnn' library on application startup.
    static {
//        System.loadLibrary("yolo_ai");
        System.loadLibrary("opencv_java3");
    }

    public boolean loadModel(AssetManager mgr, int modelid, int cpugpu){
        this.mgr = mgr;
        return  loadModeljni(mgr, modelid, cpugpu);
    }

    /**
     * A native method that is implemented by the 'libncnn' native library,
     * which is packaged with this application.
     */
    public native boolean loadModeljni(AssetManager mgr, int modelid, int cpugpu);
    public native boolean setOutputWindow(Surface surface);

    public native boolean setEncoderSurface(Surface inputSurface);
    public native void removeEncSurface();

    public native boolean onImage(long rgbaPtr);
    @Override
    public boolean onRecvImage(long matptr, long pts) {
        return false;
    }

    public static class SafeObj {
        private final org.opencv.core.Rect rect;
        private final int label;
        private final String labelStr;
        private final float prob;
        public SafeObj(int label, String labelStr, float prob, org.opencv.core.Rect rect) {
            this.label = label;
            this.prob = prob;
            this.rect = rect;
            this.labelStr = labelStr;
        }
        // 还可以添加getter和setter方法
        public float getLable() {
            return label;
        }
        public String getLableString() {
            return labelStr;
        }
        public float getProb() {
            return prob;
        }
        public org.opencv.core.Rect getRect() {
            return rect;
        }
    }
}

