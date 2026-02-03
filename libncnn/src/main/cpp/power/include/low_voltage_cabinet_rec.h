/**************************************************************************************************
 * @date  : 2024/9/18 14:18:34
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef LOW_VOLTAGE_CABINET_REC_H
#define LOW_VOLTAGE_CABINET_REC_H

#include "yolov8_detect.h"
#include "ppocr.h"

class LVC_Recognition {
public:
    struct structured_ocrResult {
        std::string Ia;
        std::string Ib;
        std::string Ic;
    };

    struct low_voltage_cabinet_result {
        std::string box_number;  // 柜号
        std::string switch_x;   // 开关状态:1 on  |  0 off
        std::string green_x;   // 绿灯状态:1 亮  |  0 熄
        std::string red_x;   // 红灯状态:1 亮  |  0 熄
        std::string yellow_x;   // 黄灯状态:1 亮  |  0 熄
        std::string cabinet_position;  // 抽屉位置:0抽出 | 1运行 | 2试验
        structured_ocrResult structured_ocr_result;  // 屏幕识别结果
    };

    struct Param {
        YOLOv8Detect::Param lvc_part_position_detector_param;
        YOLOv8Detect::Param lvc_drawer_position_detector_param;
        PPOCR::Param ppocr_param;
    };

    LVC_Recognition(AAssetManager *mgr, Param &param);

    LVC_Recognition(AAssetManager *mgr);

    ~LVC_Recognition();

    void pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &dst);

private:
    std::unique_ptr<YOLOv8Detect> lvc_part_position_detector{nullptr};
    std::unique_ptr<YOLOv8Detect> lvc_drawer_position_detector{nullptr};
    std::unique_ptr<PPOCR> ppocr{nullptr};
};

#endif //LOW_VOLTAGE_CABINET_REC_H
