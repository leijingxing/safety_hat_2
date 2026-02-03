package com.test.libncnn;

import android.content.res.AssetManager;

import org.opencv.core.Mat;

import java.lang.reflect.Array;

public abstract class BaseAi {
    protected final String TAG;

    static {
        System.loadLibrary("yolo_ai");
    }

    BaseAi() {
        this.TAG = this.getClass().getSimpleName();
    }

    public interface BaseListener<T,Q>{
        void onValue(T value);
        void onValue(Mat img, Long pts, Q array);
    }
    protected BaseListener<?,?> mListener = null; // 使用通用类型

    public void setCallback(BaseListener<?,?> listener) {
        this.mListener = listener;
    }

    public abstract boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
    public abstract boolean onRecvImage(long matptr, long pts);
}
