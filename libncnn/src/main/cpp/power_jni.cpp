#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>

#include "high_voltage_cabinet_rec.h"
#include "low_voltage_cabinet_rec.h"
#include "stop_send_card_rec.h"
#include "device_status_rec.h"

#include "ndkcamera.h"

#if __ARM_NEON

#include <arm_neon.h>

#endif // __ARM_NEON


#define LOG_TAG "infer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class Electrical_Cabinet_Recognition {
public:
    struct Param {
        StopSendCardRec::Param StopSendCardRecParam;
        DeviceStatusRec::Param DeviceStatusRecParam;
        HVC_Recognition::Param HVC_RecognitionParam;
        LVC_Recognition::Param LVC_RecognitionParam;
    };

    Electrical_Cabinet_Recognition(AAssetManager *mgr, Param &param) {
        this->mgr = mgr;
        stopsendcard_recognizer = new StopSendCardRec(mgr, param.StopSendCardRecParam);
        device_status_recognizer = new DeviceStatusRec(mgr, param.DeviceStatusRecParam);
        hvc_recognizer = new HVC_Recognition(mgr, param.HVC_RecognitionParam);
        lvc_recognizer = new LVC_Recognition(mgr, param.LVC_RecognitionParam);
    }

    ~Electrical_Cabinet_Recognition() {
        delete stopsendcard_recognizer;
        delete device_status_recognizer;
        delete hvc_recognizer;
        delete lvc_recognizer;
        //delete mgr;
    }

    void run(int operate_id, int step_id, cv::Mat &img, std::unordered_map<std::string, std::string> &dst) {
        if (operate_id == -1) {  // 停送牌识别
            stopsendcard_recognizer->pipeline(img, dst);
        } else if (operate_id == -2) {   // 设备状态识别
            device_status_recognizer->pipeline(img, dst);
        } else if (operate_id == 0) {  // 高压柜识别
            hvc_recognizer->pipeline(img, dst);
        } else if (operate_id == -3) {  // 低压柜识别
            lvc_recognizer->pipeline(img, dst);
        } else {
            LOGE("operate_id = %d is error.", operate_id);
        }
    }

private:
    //std::unique_ptr<StopSendCardRec> stopsendcard_recognizer;  // 停送牌识别器
    //std::unique_ptr<DeviceStatusRec> device_status_recognizer; // 设备状态识别器
    //std::unique_ptr<HVC_Recognition> hvc_recognizer;  // 高压柜识别器
    //std::unique_ptr<LVC_Recognition> lvc_recognizer;   // 低压柜识别器
    AAssetManager *mgr;
    StopSendCardRec *stopsendcard_recognizer = nullptr;
    DeviceStatusRec *device_status_recognizer = nullptr;
    HVC_Recognition *hvc_recognizer = nullptr;
    LVC_Recognition *lvc_recognizer = nullptr;
};


static Electrical_Cabinet_Recognition::Param ecr_param;
static Electrical_Cabinet_Recognition *ECR = 0;


// 辅助函数，将 jstring 转换为 std::string
std::string jstringToString(JNIEnv *env, jstring jStr) {
    const char *chars = env->GetStringUTFChars(jStr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jStr, chars);
    return result;
}


// C++ 接收 Java 传递的参数
extern "C" {

JNIEXPORT void JNICALL Java_com_test_libncnn_PowerLib_initParam(JNIEnv *env, jobject thiz,
                                                                jstring jParamType,
                                                                jstring jParamPath, jstring jBinPath,
                                                                jstring jinputNode, jstring joutputNode,
                                                                jint jmodelHeight, jint jmodelWidth,
                                                                jboolean juseVulkan, jobjectArray jNames) {
    // 将 jstring 转换为 std::string
    std::string paramType = jstringToString(env, jParamType);
    std::string paramPath = jstringToString(env, jParamPath);
    std::string binPath = jstringToString(env, jBinPath);
    std::string inputNode = jstringToString(env, jinputNode);
    std::string outputNode = jstringToString(env, joutputNode);
    // 将 jint 转换为 C++ int
    int modelHeight = static_cast<int>(jmodelHeight);  // 直接转换
    int modelWidth = static_cast<int>(jmodelWidth);    // 直接转换
    // 将 jboolean 转换为 C++ bool
    bool useVulkan = (juseVulkan == JNI_TRUE);  // 转换为布尔类型

    // 打印日志，显示接收到的模型相关参数
    //LOGD("Received paramPath: %s", paramPath.c_str());
    //LOGD("Received binPath: %s", binPath.c_str());
    //LOGD("Received Model inputNode: %s", inputNode.c_str());
    //LOGD("Received Model outputNode: %s", outputNode.c_str());
    //LOGD("Received Model Height: %d", modelHeight);
    //LOGD("Received Model Width: %d", modelWidth);
    //LOGD("Received Use Vulkan: %d", useVulkan);

    // 获取 names 数组的长度
    jsize namesLength = env->GetArrayLength(jNames);

    // 遍历 names 数组
    std::vector<std::string> classes;
    for (jsize i = 0; i < namesLength; ++i) {
        jstring jName = (jstring) env->GetObjectArrayElement(jNames, i);
        std::string name = jstringToString(env, jName);
        classes.push_back(name);
        // 清理局部引用
        env->DeleteLocalRef(jName);
    }

    if (paramType == "stop_send_card_position_detector_param") {
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.paramPath = paramPath;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.binPath = binPath;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.input_node = inputNode;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.output_node = outputNode;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.model_height = modelHeight;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.model_width = modelWidth;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.use_vulkan = useVulkan;
        ecr_param.StopSendCardRecParam.stop_send_card_position_detector_param.classes = classes;
    } else if (paramType == "ppocr_det_param") {
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.paramPath = paramPath;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.binPath = binPath;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.input_node = inputNode;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.output_node = outputNode;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.model_height = modelHeight;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.model_width = modelWidth;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.use_vulkan = useVulkan;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_det_param.classes = classes;

        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.paramPath = paramPath;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.binPath = binPath;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.input_node = inputNode;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.output_node = outputNode;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.model_height = modelHeight;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.model_width = modelWidth;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.use_vulkan = useVulkan;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_det_param.classes = classes;

        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.paramPath = paramPath;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.binPath = binPath;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.input_node = inputNode;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.output_node = outputNode;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.model_height = modelHeight;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.model_width = modelWidth;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.use_vulkan = useVulkan;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_det_param.classes = classes;

        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.paramPath = paramPath;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.binPath = binPath;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.input_node = inputNode;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.output_node = outputNode;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.model_height = modelHeight;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.model_width = modelWidth;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.use_vulkan = useVulkan;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_det_param.classes = classes;
    } else if (paramType == "ppocr_cls_param") {
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.paramPath = paramPath;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.binPath = binPath;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.input_node = inputNode;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.output_node = outputNode;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.model_height = modelHeight;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.model_width = modelWidth;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.use_vulkan = useVulkan;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_cls_param.classes = classes;
        ecr_param.StopSendCardRecParam.ppocr_param.use_ocr_cls = false;

        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.paramPath = paramPath;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.binPath = binPath;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.input_node = inputNode;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.output_node = outputNode;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.model_height = modelHeight;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.model_width = modelWidth;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.use_vulkan = useVulkan;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_cls_param.classes = classes;
        ecr_param.DeviceStatusRecParam.ppocr_param.use_ocr_cls = false;

        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.paramPath = paramPath;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.binPath = binPath;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.input_node = inputNode;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.output_node = outputNode;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.model_height = modelHeight;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.model_width = modelWidth;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.use_vulkan = useVulkan;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_cls_param.classes = classes;
        ecr_param.HVC_RecognitionParam.ppocr_param.use_ocr_cls = false;

        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.paramPath = paramPath;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.binPath = binPath;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.input_node = inputNode;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.output_node = outputNode;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.model_height = modelHeight;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.model_width = modelWidth;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.use_vulkan = useVulkan;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_cls_param.classes = classes;
        ecr_param.LVC_RecognitionParam.ppocr_param.use_ocr_cls = false;
    } else if (paramType == "ppocr_rec_param") {
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.paramPath = paramPath;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.binPath = binPath;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.input_node = inputNode;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.output_node = outputNode;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.model_height = modelHeight;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.model_width = modelWidth;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.use_vulkan = useVulkan;
        ecr_param.StopSendCardRecParam.ppocr_param.ppocr_rec_param.classes = classes;

        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.paramPath = paramPath;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.binPath = binPath;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.input_node = inputNode;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.output_node = outputNode;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.model_height = modelHeight;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.model_width = modelWidth;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.use_vulkan = useVulkan;
        ecr_param.DeviceStatusRecParam.ppocr_param.ppocr_rec_param.classes = classes;

        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.paramPath = paramPath;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.binPath = binPath;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.input_node = inputNode;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.output_node = outputNode;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.model_height = modelHeight;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.model_width = modelWidth;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.use_vulkan = useVulkan;
        ecr_param.HVC_RecognitionParam.ppocr_param.ppocr_rec_param.classes = classes;

        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.paramPath = paramPath;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.binPath = binPath;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.input_node = inputNode;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.output_node = outputNode;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.model_height = modelHeight;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.model_width = modelWidth;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.use_vulkan = useVulkan;
        ecr_param.LVC_RecognitionParam.ppocr_param.ppocr_rec_param.classes = classes;
    } else if (paramType == "hvc_part_position_detector_param") {
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.paramPath = paramPath;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.binPath = binPath;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.input_node = inputNode;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.output_node = outputNode;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.model_height = modelHeight;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.model_width = modelWidth;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.use_vulkan = useVulkan;
        ecr_param.DeviceStatusRecParam.device_status_position_detector_param.classes = classes;

        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.paramPath = paramPath;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.binPath = binPath;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.input_node = inputNode;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.output_node = outputNode;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.model_height = modelHeight;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.model_width = modelWidth;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.use_vulkan = useVulkan;
        ecr_param.HVC_RecognitionParam.hvc_part_position_detector_param.classes = classes;
    } else if (paramType == "hvc_warning_detector_param") {
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.paramPath = paramPath;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.binPath = binPath;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.input_node = inputNode;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.output_node = outputNode;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.model_height = modelHeight;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.model_width = modelWidth;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.use_vulkan = useVulkan;
        ecr_param.HVC_RecognitionParam.hvc_warning_detector_param.classes = classes;
    } else if (paramType == "hvc_meter_recognizer_param") {
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.paramPath = paramPath;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.binPath = binPath;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.input_node = inputNode;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.output_node = outputNode;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.model_height = modelHeight;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.model_width = modelWidth;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.use_vulkan = useVulkan;
        ecr_param.HVC_RecognitionParam.hvc_meter_recognizer_param.classes = classes;
    } else if (paramType == "lvc_part_position_detector_param") {
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.paramPath = paramPath;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.binPath = binPath;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.input_node = inputNode;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.output_node = outputNode;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.model_height = modelHeight;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.model_width = modelWidth;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.use_vulkan = useVulkan;
        ecr_param.LVC_RecognitionParam.lvc_part_position_detector_param.classes = classes;
    } else if (paramType == "lvc_drawer_position_detector_param") {
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.paramPath = paramPath;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.binPath = binPath;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.input_node = inputNode;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.output_node = outputNode;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.model_height = modelHeight;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.model_width = modelWidth;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.use_vulkan = useVulkan;
        ecr_param.LVC_RecognitionParam.lvc_drawer_position_detector_param.classes = classes;
    }
}

JNIEXPORT jobject JNICALL Java_com_test_libncnn_PowerLib_cppmain(JNIEnv *env, jobject /* this */,
                                                                 jobject assetManager,
                                                                 jlong rgbaPtr, jint oper, jint step) {
//    LOGD("infer--- cppmain");
    cv::Mat &rgba = *(cv::Mat *) rgbaPtr;
    cv::Mat rgb;
    cv::cvtColor(rgba, rgb, cv::COLOR_RGBA2BGR);

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    if (ECR == NULL) {
        ECR = new Electrical_Cabinet_Recognition(mgr, ecr_param);
    }
    std::unordered_map<std::string, std::string> dst;  // 识别结果
    //LOGD("infer--- rgb.cols= %d rgb.rows= %d  rgb.channels= %d", rgb.cols, rgb.rows, rgb.channels());
    try {
        ECR->run(oper, step, rgb, dst);
    }
    catch (const std::exception &e) {
        LOGE("Electrical_Cabinet_Recognition Module Caught an exception");
    }

    for (const auto r: dst) {
        LOGD("infer--- %s:%s", r.first.c_str(), r.second.c_str());
    }

    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "()V");
    jmethodID hashMapPut = env->GetMethodID(hashMapClass, "put",
                                            "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    jobject hashMap = env->NewObject(hashMapClass, hashMapInit);

    for (const auto &pair: dst) {
        jstring key = env->NewStringUTF(pair.first.c_str());
        jstring value = env->NewStringUTF(pair.second.c_str());
        env->CallObjectMethod(hashMap, hashMapPut, key, value);
        // 释放局部引用以防止内存泄漏
        env->DeleteLocalRef(key);
        env->DeleteLocalRef(value);
    }

    return hashMap;
}

}
