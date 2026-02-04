#include <jni.h>
#include <string>
#include "rtmp.h"
#include <mutex>

#include "flvmuxer/xiecc_rtmp.h"

#include <android/log.h>

#define LIBRTMP_LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "librtmp", __VA_ARGS__))
#define LIBRTMP_LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO , "librtmp", __VA_ARGS__))
#define LIBRTMP_LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN , "librtmp", __VA_ARGS__))
#define LIBRTMP_LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "librtmp", __VA_ARGS__))

class RtmpPublish{
public:
    RtmpPublish(){

    }
    ~RtmpPublish(){

    }
    int open(const char *url, uint32_t video_width, uint32_t video_height,
             uint32_t frame_rate, int video_codec){
        if (rtmpPtr && *rtmpPtr) {
            return 0;
        }else{
            int result = rtmp_open_for_write(url, video_width, video_height, frame_rate, video_codec, rtmpPtr);
            return result;
        }
    }

    void close(){
        if (rtmpPtr && *rtmpPtr) {
            rtmp_close(*rtmpPtr);
            *rtmpPtr = NULL;
        }
    }

    int writeVideo(uint8_t *data, int total, uint64_t dts_us, int key, uint32_t abs_ts){
        int result = -1;
        if (rtmpPtr && *rtmpPtr) {
            result = rtmp_sender_write_video_frame(*rtmpPtr, data, total, dts_us, key, abs_ts);
        }
        return result;
    }

    bool isConnect(){
        bool result = false;
//        LIBRTMP_LOGD("rtmp_is_connected rtmp %p\n",rtmpPtr);
//        LIBRTMP_LOGD("rtmp_is_connected *rtmp %p\n", &(*rtmpPtr));
        if (rtmpPtr && *rtmpPtr) {
            result = rtmp_is_connected(*rtmpPtr);
//            LIBRTMP_LOGD("Status: is initialized result:%d",result);
        } else {
            LIBRTMP_LOGD("Status: is not initialized");
        }
        return result;
    }

private:
    RTMP* rtmp = nullptr;
    RTMP **rtmpPtr = &rtmp;
//    std::mutex mutex;
};

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_librtmps_NativeLib_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_librtmps_NativeLib_nativeAlloc(JNIEnv* env, jobject thiz) {
    RtmpPublish *publish = new RtmpPublish();
    return (jlong)publish;
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_librtmps_NativeLib_nativeOpen(JNIEnv* env, jobject thiz,
                jlong rtmpPointer, jstring url, jint w, jint h, jint fps, jint videoCodec) {
    RtmpPublish *rtmp = (RtmpPublish *) rtmpPointer;
    if (rtmp != NULL && !rtmp->isConnect()) {
        const char *nativeString = NULL;
        nativeString = env->GetStringUTFChars(url, 0);
        if (nativeString == NULL) {
            return;
        }
        LIBRTMP_LOGD("nativeOpen url:%s\n", nativeString);
        rtmp->open(nativeString, w, h, fps, videoCodec);
        env->ReleaseStringUTFChars(url, nativeString);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_librtmps_NativeLib_nativeClose(JNIEnv* env, jobject thiz, jlong rtmpPointer) {
    RtmpPublish *rtmp = (RtmpPublish *) rtmpPointer;
    if (rtmp != NULL) {
        rtmp->close();
//        delete(rtmp);
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_librtmps_NativeLib_nativeIsConnected(JNIEnv* env, jobject thiz, jlong rtmpPointer) {
    RtmpPublish *rtmp = (RtmpPublish *) rtmpPointer;
    if (rtmp == NULL) {
        return false;
    }
    bool ret = rtmp->isConnect();
    LIBRTMP_LOGD("nativeIsConnected isConnect:%s\n", (ret)?"true":"false");
    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_librtmps_NativeLib_nativeWriteVideo(
        JNIEnv* env, jobject thiz,
        jlong rtmpPointer, jobject buffer, jint size, jlong timestamp) {

    RtmpPublish *rtmp = (RtmpPublish *) rtmpPointer;
    if (rtmp == NULL) {
        return -1;
    }
    // 获取 Direct ByteBuffer 的地址
    void* bufferAddress = env->GetDirectBufferAddress(buffer);
    if(bufferAddress == nullptr)
        return -1;
    uint8_t* directBuffer = static_cast<uint8_t*>(bufferAddress);

    size_t bufferSize = env->GetDirectBufferCapacity(buffer);
//    LIBRTMP_LOGD("nativeWriteVideo bufferSize:%d size:%d\n", bufferSize, size);

    if(false){ // 调试
        char out_str[100];  // 假设前10个数据足够存在100字节内
        size_t offset = 0;

        for (size_t i = 0; i < size && i < 10; ++i) {
            offset += sprintf(out_str + offset, "0x%02X ", directBuffer[i]);
        }
//        LIBRTMP_LOGD("nativeWriteVideo directBuffer:%s",out_str);
    }

    if(bufferSize >= size){
        return rtmp->writeVideo(directBuffer, size, timestamp, 0, 0);
    }else{
        return -1;
    }
}
