/**************************************************************************************************
 * @date  : 2024/9/18 15:24:57
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef DEVICE_STATUS_REC_H
#define DEVICE_STATUS_REC_H

#include "yolov8_detect.h"
#include "ppocr.h"


class DeviceStatusRec {
public:
    struct Param {
        YOLOv8Detect::Param device_status_position_detector_param;
        PPOCR::Param ppocr_param;
    };

    DeviceStatusRec(AAssetManager *mgr, Param &param);

    DeviceStatusRec(AAssetManager *mgr);

    ~DeviceStatusRec();

    void pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &device_status);

private:
    std::unique_ptr<YOLOv8Detect> yolov8_detector{nullptr};
    std::unique_ptr<PPOCR> ppocr{nullptr};
};


#endif //DEVICE_STATUS_REC_H
