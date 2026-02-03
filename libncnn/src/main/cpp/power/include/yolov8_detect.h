/**************************************************************************************************
 * @date  : 2024/9/2 11:22:44
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#ifndef YOLOV8_DETECT_H
#define YOLOV8_DETECT_H

#include <vector>
#include "opencv2/opencv.hpp"
#include "net.h"
#include <random>

#include <android/log.h>  // 提供 __android_log_print() 函数
#define LOG_TAG "infer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct DetectObj {
    int class_id{0};
    std::string className{};
    float confidence{0.0};
    cv::Scalar color{};
    cv::Rect box{}; //x1、y1、width、height   耳标坐标。

    std::vector<cv::Mat> textRegion{}; // 文字区域截图
    std::vector<std::vector<cv::Point> > boxVertex{}; // 文字区域坐标
    std::vector<std::string> text{}; //文字区域识别结果
};


class YOLOv8Detect {
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


    //void readYaml(const std::string &cfg);
    //
    //void loadModel(const char *param, const char *bin);
    //
    //explicit YOLOv8Detect(const std::string &cfg);

    //YOLOv8Detect(AAssetManager *mgr,
    //             std::string paramPath, std::string binPath,
    //             std::string inputNode, std::string outputNode,
    //             int modelWidth, int modelHeight,
    //             bool useVulkan, std::vector<std::string> classes);

    YOLOv8Detect(AAssetManager *mgr, Param &param);

    ~YOLOv8Detect();

    void infer(cv::Mat &img, ncnn::Mat &outputs);

    void pretty_print(const ncnn::Mat &m);

    void postprocess(ncnn::Mat &outputs, std::vector<DetectObj> &results);

    void draw_results(cv::Mat &img, std::vector<DetectObj> &detect_results);


    ncnn::Net *model{nullptr};
    std::string paramPath;
    std::string binPath;
    std::string input_node = {"in0"};
    std::string output_node = {"out0"};
    int model_height{640};
    int model_width{640};
    bool use_vulkan{false};
    const float mean_vals[3] = {0.f, 0.f, 0.f};
    const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f};
    std::vector<std::string> classes{};

    float boxThresh = {0.5f};
    float nmsThresh = {0.25f};

    float scale_h{0};
    float scale_w{0};
    int padding_top{0};
    int padding_bottom{0};
    int padding_left{0};
    int padding_right{0};
};


#endif //YOLOV8_DETECT_H
