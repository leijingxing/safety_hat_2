// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>

#include "yolo_hivz_vest.h"

#include "ndkcamera.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

static int draw_unsupported(cv::Mat& rgb)
{
    const char text[] = "unsupported";

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

    int y = (rgb.rows - label_size.height) / 2;
    int x = (rgb.cols - label_size.width) / 2;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0));

    return 0;
}

static int draw_fps(cv::Mat& rgb)
{
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f)
        {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--)
        {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f)
        {
            return 0;
        }

        for (int i = 0; i < 10; i++)
        {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

static YoloVest* g_yolo = 0;
static ncnn::Mutex lock;

static ANativeWindow* mShowWin = 0;

extern "C" {
//
//JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
//{
//    printf("JNI_OnLoad")
//    return JNI_VERSION_1_4;
//}
//
//JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
//{
//    printf("JNI_OnUnload")
//
//    {
//        ncnn::MutexLockGuard g(lock);
//
//        delete g_yolo;
//        g_yolo = 0;
//    }
//}

void set_window_vest(ANativeWindow* _win)
{
    if (mShowWin)
    {
        ANativeWindow_release(mShowWin);
    }

    mShowWin = _win;
    ANativeWindow_acquire(mShowWin);
}

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_VestMod_vestLoadModel(JNIEnv* env, jobject thiz, jobject assetManager, jint modelid, jint cpugpu)
{
    if (modelid < 0 || modelid > 6 || cpugpu < 0 || cpugpu > 1)
    {
        return JNI_FALSE;
    }

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    printf("loadModel %p",mgr);

    const char* modeltypes[] =
            {
                    "n",
                    "s",
            };

    const int target_sizes[] =
            {
                    192,
                    192,
            };


    const float mean_vals[][3] =
            {
//        {103.53f, 116.28f, 123.675f},
//        {103.53f, 116.28f, 123.675f},
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f, 0.0f},
            };

    const float norm_vals[][3] =
            {
                    { 1 / 255.f, 1 / 255.f, 1 / 255.f },
                    { 1 / 255.f, 1 / 255.f, 1 / 255.f },
            };

    const char* modeltype = modeltypes[(int)modelid];
    int target_size = target_sizes[(int)modelid];
    bool use_gpu = (int)cpugpu == 1;

    // reload
    {
        ncnn::MutexLockGuard g(lock);

        if (use_gpu && ncnn::get_gpu_count() == 0)
        {
            // no gpu
            delete g_yolo;
            g_yolo = 0;
        }
        else
        {
            if (!g_yolo)
                g_yolo = new YoloVest;
            g_yolo->load(mgr, modeltype, target_size, mean_vals[(int)modelid], norm_vals[(int)modelid], use_gpu);
        }
    }

    return JNI_TRUE;
}

// public native boolean setOutputWindow(Surface surface);
JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_VestMod_setOutputWindow(JNIEnv* env, jobject thiz, jobject surface)
{
    ANativeWindow* showWin = ANativeWindow_fromSurface(env, surface);
    printf("setOutputWindow %p", showWin);
    set_window_vest(showWin);
    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_VestMod_onImage(JNIEnv* env, jobject thiz, jlong rgbaPtr)
{
    static jobject java_cls_ncnn = nullptr;
    static jmethodID java_method_onValue = nullptr;
    java_cls_ncnn = thiz;
    jclass cls = env->GetObjectClass(thiz);
    java_method_onValue = env->GetMethodID(cls, "onValue",
                                           "(Lcom/test/libncnn/VestMod;[Ljava/lang/String;)V");
    cv::Mat &rgba = *(cv::Mat *) rgbaPtr;
    cv::Mat rgbMat;
    cv::cvtColor(rgba, rgbMat, cv::COLOR_RGBA2RGB);

    int roi_w = 0;
    int roi_h = 0;
    int render_w = 0;
    int render_h = 0;
    int render_rotate_type = 0;
    if (g_yolo) {
        std::vector<YoloVest::Object> objects;
        g_yolo->detect(rgbMat, objects);
//        g_yolo->draw(rgbMat, objects);

        if(objects.size()>0 && java_cls_ncnn != nullptr && java_method_onValue!= nullptr ) {
            printf("size:%zu",objects.size());
            // 创建一个 Java 字符串数组
            jclass stringClass = env->FindClass("java/lang/String");
            jobjectArray stringArray = env->NewObjectArray(objects.size(), stringClass, nullptr);
            for (size_t i = 0; i < objects.size(); i++) {
                printf("label: %s %f", YoloVest::class_names[objects[i].label], objects[i].prob);
                jstring jstr = env->NewStringUTF(YoloVest::class_names[objects[i].label]);
                env->SetObjectArrayElement(stringArray, i, jstr);
                env->DeleteLocalRef(jstr); // 清理局部引用
            }
            env->CallVoidMethod(java_cls_ncnn, java_method_onValue, java_cls_ncnn,
                                stringArray); // 新值
        }

        cv::Mat rgb = rgbMat;
        if(mShowWin!= nullptr){
            draw_fps(rgb);
            roi_w = render_w = rgb.cols;
            roi_h = render_h = rgb.rows;
            // rotate to native window orientation
            cv::Mat rgb_render(render_h, render_w, CV_8UC3);
            ncnn::kanna_rotate_c3(rgb.data, roi_w, roi_h, rgb_render.data, render_w, render_h, render_rotate_type);

            ANativeWindow_setBuffersGeometry(mShowWin, render_w, render_h, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
            ANativeWindow_Buffer buf;
            ANativeWindow_lock(mShowWin, &buf, NULL);

            // scale to target size
            if (buf.format == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM || buf.format == AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM)
            {
                for (int y = 0; y < render_h; y++)
                {
                    const unsigned char* ptr = rgb.ptr<const unsigned char>(y);
                    unsigned char* outptr = (unsigned char*)buf.bits + buf.stride * 4 * y;

                    int x = 0;
                    for (; x + 7 < render_w; x += 8)
                    {
                        uint8x8x3_t _rgb = vld3_u8(ptr);
                        uint8x8x4_t _rgba;
                        _rgba.val[0] = _rgb.val[0];
                        _rgba.val[1] = _rgb.val[1];
                        _rgba.val[2] = _rgb.val[2];
                        _rgba.val[3] = vdup_n_u8(255);
                        vst4_u8(outptr, _rgba);

                        ptr += 24;
                        outptr += 32;
                    }
                }
            }

            ANativeWindow_unlockAndPost(mShowWin);
        }
    }else{
        draw_unsupported(rgbMat);
    }

    return JNI_TRUE;
}

}
