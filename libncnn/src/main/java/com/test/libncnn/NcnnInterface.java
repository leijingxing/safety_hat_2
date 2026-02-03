package com.test.libncnn;

import android.content.res.AssetManager;
import android.view.Surface;

public interface NcnnInterface {
    public boolean onRecvImage(long matptr);

    public boolean loadModel(AssetManager assets, int currentModel, int currentCpugpu);

    public boolean setOutputWindow(Surface surface);
    public void setValueCallback(ValueListener listener);
    public interface ValueListener{
        void onValue(String value);
//        void onSure();
        void onCancel();
    };
}