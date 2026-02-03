/**************************************************************************************************
 * @date  : 2024/9/18 15:24:30
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef STOP_SEND_CARD_REC_H
#define STOP_SEND_CARD_REC_H

#include "yolov8_detect.h"
#include "ppocr.h"


class StopSendCardRec {
public:
    struct Param {
        AAssetManager *mgr;
        YOLOv8Detect::Param stop_send_card_position_detector_param;
        PPOCR::Param ppocr_param;
    };

    StopSendCardRec(AAssetManager *mgr);
    StopSendCardRec(AAssetManager *mgr, std::string paramPath, std::string binPath, int modelWidth, int modelHeight, bool useVulkan, std::vector<std::string> classes);

    StopSendCardRec(AAssetManager *mgr, Param &param);

    ~StopSendCardRec();

    void pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &dst);

private:
    AAssetManager *mgr;
    std::unique_ptr<YOLOv8Detect> stop_send_card_position_detector{nullptr};
    std::unique_ptr<PPOCR> ppocr{nullptr};
};

#endif //STOP_SEND_CARD_REC_H
