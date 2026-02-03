/**************************************************************************************************
 * @date  : 2024/9/3 10:56:09
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef PPOCR_REC_H
#define PPOCR_REC_H

#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "net.h"

#include <android/log.h>  // 提供 __android_log_print() 函数
#define LOG_TAG "infer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class PPOCR_Rec {
public:
    struct Param {
        AAssetManager *mgr;
        std::string paramPath;
        std::string binPath;
        std::string input_node;
        std::string output_node;
        int model_height;
        int model_width;
        bool use_vulkan;
        std::vector<std::string> classes;
    };

    void readYaml(const std::string &cfg);

    void loadModel(const char *param, const char *bin);

    explicit PPOCR_Rec(const std::string &cfg);

    PPOCR_Rec(AAssetManager *mgr, Param &param);

    PPOCR_Rec(AAssetManager *mgr, std::string param, std::string bin);

    PPOCR_Rec(AAssetManager *mgr,
              std::string paramPath, std::string binPath,
              std::string inputNode, std::string outputNode,
              int modelWidth, int modelHeight,
              bool useVulkan, std::vector<std::string> classes);

    ~PPOCR_Rec();

    void pretty_print(const ncnn::Mat &m);

    void postprocess(ncnn::Mat &outputs, std::string &results);

    void infer(cv::Mat &img, ncnn::Mat &outputs);


    // private:
    ncnn::Net *model{nullptr};
    std::string paramPath = "model/rec_32x192-int8.param";
    std::string binPath = "model/rec_32x192-int8.bin";
    std::string input_node = "in0";
    std::string output_node = "out0";
    int model_height{32};
    int model_width{192};
    bool use_vulkan{false};
    const float mean_vals[3] = {0.f, 0.f, 0.f};
    const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f};
    std::vector<std::string> classes{};

    float scale_h{0};
    float scale_w{0};
    int padding_top{0};
    int padding_bottom{0};
    int padding_left{0};
    int padding_right{0};
};

#endif //PPOCR_REC_H
