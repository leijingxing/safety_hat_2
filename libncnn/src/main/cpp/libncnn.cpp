#include <jni.h>
#include <string>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>
#include <opencv2/imgproc.hpp>
#include <benchmark.h>

#include "yolo.h"
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "test_debug", __VA_ARGS__);


static Yolo* g_yolo = 0;
static ncnn::Mutex lock;
static ANativeWindow* win = 0;

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

void set_window(ANativeWindow* _win)
{
    if (win)
    {
        ANativeWindow_release(win);
    }

    win = _win;
    ANativeWindow_acquire(win);
}
extern "C" {

JNIEXPORT jstring JNICALL
Java_com_test_libncnn_NcnnFinger_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

// public native boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_NcnnFinger_loadModel(JNIEnv* env, jobject thiz, jobject assetManager, jint modelid, jint cpugpu)
{

    if (modelid < 0 || modelid > 6 || cpugpu < 0 || cpugpu > 1)
    {
        return JNI_FALSE;
    }

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "loadModel %p", mgr);

    const char* modeltypes[] =
            {
                    "best",
            };

    const int target_sizes[] =
            {
                    320,
            };

    const float norm_vals[][3] =
            {
                    {1/255.f, 1/255.f,1/255.f},
            };

    const char* modeltype = modeltypes[(int)modelid];
    __android_log_print(ANDROID_LOG_DEBUG, "康冠仔", "康冠仔加载模型 %s", modeltype);
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
                g_yolo = new Yolo;
            g_yolo->load(mgr, modeltype, target_size, norm_vals[(int)modelid], use_gpu);
        }
    }

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_NcnnFinger_setOutputWindow(JNIEnv* env, jobject thiz, jobject surface)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

    set_window(win);

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_NcnnFinger_onImage(JNIEnv* env, jobject thiz, jlong rgbaPtr)
{
    static jobject java_cls_ncnn = nullptr;
    static jmethodID java_method_updateValue = nullptr;
    static jmethodID java_method_outValue = nullptr;
    java_cls_ncnn = thiz;
    jclass cls = env->GetObjectClass(thiz);
    java_method_updateValue = env->GetMethodID(cls, "newValue",
                                               "(Lcom/test/libncnn/NcnnFinger;Ljava/lang/String;F)V");
    java_method_outValue = env->GetMethodID(cls, "outValue",
                                            "(Lcom/test/libncnn/NcnnFinger;)V");

    int render_w = 0;
    int render_h = 0;
    int roi_w = 0;
    int roi_h = 0;
    int render_rotate_type = 0;

    cv::Mat &rgba = *(cv::Mat *) rgbaPtr;
    cv::Mat rgb;
    cv::cvtColor(rgba, rgb, cv::COLOR_RGBA2RGB);

    if (g_yolo) {
        std::vector<Object> objects;
        g_yolo->detect(rgb, objects);
        printf("objects :%zu ", objects.size());

        if(objects.size()>0 && java_cls_ncnn != nullptr && java_method_updateValue!= nullptr ){
            g_yolo->value_count ++;
            jstring value = env->NewStringUTF(class_names[objects[0].label]);
            env->CallVoidMethod(java_cls_ncnn, java_method_updateValue,
                                java_cls_ncnn, value, objects[0].prob * 100);
            printf("objects :%d    %s  %f", objects.size(), class_names[objects[0].label], objects[0].prob * 100);

        }else{
            if(g_yolo->value_count > 0) {
                g_yolo->value_count = 0;
                env->CallVoidMethod(java_cls_ncnn, java_method_outValue, java_cls_ncnn);
            }
        }
        g_yolo->draw(rgb, objects);
    }else{
        draw_unsupported(rgb);
    }
    if(win!= nullptr){
        draw_fps(rgb);

        roi_w = render_w = rgb.cols;
        roi_h = render_h = rgb.rows;

//        printf("roi_w :%d    %d", roi_w, roi_h);

        // rotate to native window orientation
        cv::Mat rgb_render(render_h, render_w, CV_8UC3);
        ncnn::kanna_rotate_c3(rgb.data, roi_w, roi_h, rgb_render.data, render_w, render_h, render_rotate_type);

        ANativeWindow_setBuffersGeometry(win, render_w, render_h, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);

        ANativeWindow_Buffer buf;
        ANativeWindow_lock(win, &buf, NULL);

        // scale to target size
        if (buf.format == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM || buf.format == AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM)
        {
            for (int y = 0; y < render_h; y++)
            {
                const unsigned char* ptr = rgb.ptr<const unsigned char>(y);
    //            const unsigned char* ptr = rgb_render.ptr<const unsigned char>(y);
                unsigned char* outptr = (unsigned char*)buf.bits + buf.stride * 4 * y;

                int x = 0;
    //#if __ARM_NEON
                for (; x + 7 < render_w; x += 8)
                {uint8x8x3_t _rgb = vld3_u8(ptr);
                    uint8x8x4_t _rgba;
                    _rgba.val[0] = _rgb.val[0];
                    _rgba.val[1] = _rgb.val[1];
                    _rgba.val[2] = _rgb.val[2];
                    _rgba.val[3] = vdup_n_u8(255);
                    vst4_u8(outptr, _rgba);

                    ptr += 24;
                    outptr += 32;
                }
    //#endif // __ARM_NEON
    //            for (; x < render_w; x++)
    //            {
    //                outptr[0] = ptr[0];
    //                outptr[1] = ptr[1];
    //                outptr[2] = ptr[2];
    //                outptr[3] = 255;
    //
    //                ptr += 3;
    //                outptr += 4;
    //            }
            }
        }

        ANativeWindow_unlockAndPost(win);


    }
    return JNI_TRUE;
}

}