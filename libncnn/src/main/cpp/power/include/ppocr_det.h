/**************************************************************************************************
 * @date  : 2024/9/2 18:02:38
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef PPOCR_DET_H
#define PPOCR_DET_H

#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "net.h"
#include "clipper.h"

#include <android/log.h>  // 提供 __android_log_print() 函数
#define LOG_TAG "infer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct TextBox {
    std::vector<cv::Point> boxPoint;
    float score;
};

struct ImgBox {
    cv::Mat img;
    std::vector<cv::Point> imgPoint;
};


class PPOCR_Det {
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

    explicit PPOCR_Det(const std::string &cfg);

    PPOCR_Det(AAssetManager *mgr, Param &param);

    explicit PPOCR_Det(AAssetManager *mgr, std::string param, std::string bin);

    PPOCR_Det(AAssetManager *mgr,
              std::string paramPath, std::string binPath,
              std::string inputNode, std::string outputNode,
              int modelWidth, int modelHeight,
              bool useVulkan, std::vector<std::string> classes);

    ~PPOCR_Det();

    void pretty_print(const ncnn::Mat &m);

    void postprocess(cv::Mat img, ncnn::Mat &outputs, std::vector<ImgBox> &text_region);

    void infer(cv::Mat &img, ncnn::Mat &outputs);

    // private:
    ncnn::Net *model{nullptr};
    std::string paramPath;
    std::string binPath;
    std::string input_node = "in0";
    std::string output_node = "out0";
    int model_height{0};
    int model_width{0};
    bool use_vulkan{false};
    const float mean_vals[3] = {118.0395, 118.0395, 118.0395};  // TODO:
    const float norm_vals[3] = {0.013177314, 0.013177314, 0.013177314};  // TODO:
    std::vector<std::string> classes{"text region"};

    float boxScoreThresh = 0.5f;
    float boxThresh = 0.3f;
    float unClipRatio = 1.8f;

    float scale_h{0};
    float scale_w{0};
    int padding_top{0};
    int padding_bottom{0};
    int padding_left{0};
    int padding_right{0};
};

#endif //PPOCR_DET_H
