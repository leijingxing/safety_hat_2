#include <jni.h>
#include <string>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>
#include <opencv2/imgproc.hpp>
#include "benchmark.h"

#include "yolov8.h"
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "test_debug", __VA_ARGS__);
#define printe(...) __android_log_print(ANDROID_LOG_ERROR, "test_debug", __VA_ARGS__);


static YoloV8* g_yolo = 0;
static ncnn::Mutex lock;
static ANativeWindow* win = 0;
static ANativeWindow* encoderWin = 0;
std::mutex surfaceMtx; // 创建一个互斥锁

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

void set_window_safe(ANativeWindow* _win)
{
    ANativeWindow* old_win = win;
    if (_win) {
        ANativeWindow_acquire(_win);
    }
    win = _win;
    if (old_win) {
        ANativeWindow_release(old_win);
    }

//    if (win)
//    {
//        ANativeWindow_release(win);
//    }
//
//    win = _win;
//    ANativeWindow_acquire(win);
}

void set_encoder_safe(ANativeWindow* _win)
{
    if (encoderWin)
    {
        ANativeWindow_release(encoderWin);
        encoderWin = nullptr;
    }

    encoderWin = _win;
    ANativeWindow_acquire(encoderWin);
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_SafeMod_loadModeljni(JNIEnv* env, jobject thiz, jobject assetManager, jint modelid, jint cpugpu)
{
    printf("Java_com_test_libncnn_SafeMod_loadModeljni \n");

    if (modelid < 0 || modelid > 6 || cpugpu < 0 || cpugpu > 1)
    {
        return JNI_FALSE;
    }

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "loadModel %p", mgr);

    const char* modeltypes[] =
            {
                    "n",
                    "s",
            };

    const int target_sizes[] =
            {
                    320,
                    320,
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
            if (!g_yolo){
                g_yolo = new YoloV8;
                printf("g_yolo->load \n");
            }
            g_yolo->load(mgr, modeltype, target_size, mean_vals[(int)modelid], norm_vals[(int)modelid], use_gpu);

        }
    }

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_SafeMod_setOutputWindow(JNIEnv* env, jobject thiz, jobject surface)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

    set_window_safe(win);

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_SafeMod_setEncoderSurface(JNIEnv* env, jobject thiz, jobject surface)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setEncoderSurface %p", win);

    set_encoder_safe(win);

    return JNI_TRUE;
}


JNIEXPORT void JNICALL
Java_com_test_libncnn_SafeMod_removeEncSurface(JNIEnv* env, jobject thiz)
{
    surfaceMtx.lock();
    if (encoderWin)
    {
        ANativeWindow_release(encoderWin);
        encoderWin = nullptr;
    }
    surfaceMtx.unlock();
}
// 自定义条件函数，返回 true 表示不满足条件，需要删除
bool condition(const Object& obj) {
    return (obj.label != 0 && obj.label != 1 && obj.label != 2 && obj.label != 3 && obj.label != 5); // 例如，删除 value 小于 10 的对象
}
// 自定义条件函数，返回 true 表示不满足条件，需要删除
bool nonlyPerson(const Object& obj) {
    return (obj.label != 0 && obj.label != 1); // 例如，删除 value 小于 10 的对象
}

//static cv::Mat convertRGBToNV21(const cv::Mat& rgbImage) {
//    // Step 1: 将RGB转换为YUV420p
//    cv::Mat yuv420p;
//    cv::cvtColor(rgbImage, yuv420p, cv::COLOR_RGB2YUV_I420);  // YUV420p格式
//
//    int width = rgbImage.cols;
//    int height = rgbImage.rows;
//
//    // NV21格式的输出矩阵
//    std::vector<uint8_t> nv21Data(width * height * 3 / 2);  // NV21格式是1.5倍的像素大小
//
//    // Step 2: 拆分YUV420p中的Y、U、V平面
//    int ySize = width * height;
//    int uvSize = ySize / 4;  // U和V平面大小为Y平面的1/4
//
//    uint8_t* yPlane = yuv420p.data;
//    uint8_t* uPlane = yuv420p.data + ySize;
//    uint8_t* vPlane = yuv420p.data + ySize + uvSize;
//
//    // Step 3: 复制Y平面到NV21的前ySize字节
//    memcpy(nv21Data.data(), yPlane, ySize);
//
//    // Step 4: 交错存储V和U平面
//    uint8_t* uvPlane = nv21Data.data() + ySize;
//    for (int i = 0; i < uvSize; ++i) {
//        uvPlane[2 * i] = vPlane[i];   // 先存V
//        uvPlane[2 * i + 1] = uPlane[i]; // 后存U
//    }
//
//    // 返回NV21格式的矩阵（1通道，1.5倍的行大小）
//    return cv::Mat(height + height / 2, width, CV_8UC1, nv21Data.data());
//}

JNIEXPORT jboolean JNICALL
Java_com_test_libncnn_SafeMod_onImage(JNIEnv* env, jobject thiz, jlong rgbaPtr)
{
    static jobject java_cls_ncnn = nullptr;
    static jmethodID java_method_arrayValue = nullptr;
    java_cls_ncnn = thiz;
    jclass cls = env->GetObjectClass(thiz);
    java_method_arrayValue = env->GetMethodID(cls, "arrayValue",
                                                "(Lcom/test/libncnn/SafeMod;[Lcom/test/libncnn/SafeMod$SafeObj;)V");


    cv::Mat &rgba = *(cv::Mat *) rgbaPtr;
    cv::Mat rgbMat;
    cv::cvtColor(rgba, rgbMat, cv::COLOR_RGBA2RGB);

    int render_w = 0;
    int render_h = 0;
    int roi_w = 0;
    int roi_h = 0;
    int render_rotate_type = 0;

    if (g_yolo) {
        std::vector<Object> objects;
        std::vector<Object> persionObjects;
        g_yolo->detect(rgbMat, objects);

        for (size_t i = 0; i < objects.size(); i++)
        {
            persionObjects.push_back(objects[i]);
        }
        // 去除正常的值
        objects.erase(
            std::remove_if(objects.begin(), objects.end(), condition),
            objects.end()
        );
        // 去除正常的值
        persionObjects.erase(
                std::remove_if(persionObjects.begin(), persionObjects.end(), nonlyPerson),
                persionObjects.end()
        );
        if(objects.size()>0 && java_cls_ncnn != nullptr && java_method_arrayValue!= nullptr ) {
            jclass safeObjClass = env->FindClass("com/test/libncnn/SafeMod$SafeObj");
            jmethodID safeObjInit = env->GetMethodID(safeObjClass, "<init>", "(ILjava/lang/String;FLorg/opencv/core/Rect;)V");
            if (safeObjClass == nullptr) {
                // 处理错误：找不到类
                printe("Error:找不到类 com/test/libncnn/SafeMod$SafeObj")
            }
            if (safeObjInit == nullptr) {
                // 处理错误：找不到构造器方法
                printe("Error:找不到构造器方法 com/test/libncnn/SafeMod$SafeObj")
            }
            jobjectArray safeObjArray = env->NewObjectArray(objects.size(), safeObjClass, nullptr);

            // 创建Java的Rect对象（示例参数）
            jclass rectClass = env->FindClass("org/opencv/core/Rect");
            jmethodID rectInit = env->GetMethodID(rectClass, "<init>", "(IIII)V");

            for (size_t i = 0; i < objects.size(); i++)
            {
                jobject rect = env->NewObject(rectClass, rectInit, (jint)objects[i].rect.x, (jint)objects[i].rect.y, (jint)objects[i].rect.width, (jint)objects[i].rect.height);

                jstring jstr = env->NewStringUTF(class_names[objects[i].label]);
                // 创建SafeObj实例
                jobject safeObj = env->NewObject(safeObjClass, safeObjInit, (jint)objects[i].label,jstr, (jfloat)objects[i].prob, rect);
                if (safeObj == nullptr) continue; // 错误处理
                env->SetObjectArrayElement(safeObjArray, i, safeObj);
                // 释放局部引用
                env->DeleteLocalRef(rect);
                env->DeleteLocalRef(jstr);
                env->DeleteLocalRef(safeObj);
            }
            env->CallVoidMethod(java_cls_ncnn, java_method_arrayValue, java_cls_ncnn, safeObjArray); // 新值
        }

        if(!persionObjects.empty()){
            g_yolo->draw(rgbMat, persionObjects, [&](int a){});
        }

        // 图像绘制
        cv::Mat rgb = rgbMat;
        if(win!= nullptr){
            draw_fps(rgb);
            roi_w = render_w = rgb.cols;
            roi_h = render_h = rgb.rows;
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

            ANativeWindow_unlockAndPost(win);
        }

        surfaceMtx.lock();
        if(encoderWin){
            roi_w = render_w = rgb.cols;
            roi_h = render_h = rgb.rows;
            cv::Mat rgb_render(render_h, render_w, CV_8UC3);
            ncnn::kanna_rotate_c3(rgb.data, roi_w, roi_h, rgb_render.data, render_w, render_h, render_rotate_type);

            ANativeWindow_setBuffersGeometry(encoderWin, render_w, render_h, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
            ANativeWindow_Buffer buf;
            ANativeWindow_lock(encoderWin, &buf, NULL);

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

            ANativeWindow_unlockAndPost(encoderWin);
        }
        surfaceMtx.unlock();
    }else{
        draw_unsupported(rgbMat);
    }

    return JNI_TRUE;
}

}