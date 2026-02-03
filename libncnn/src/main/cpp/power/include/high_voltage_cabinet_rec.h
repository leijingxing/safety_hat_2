/**************************************************************************************************
 * @date  : 2024/9/14 14:28:58
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef HIGH_VOLTAGE_CABINET_RECOGNITION_H
#define HIGH_VOLTAGE_CABINET_RECOGNITION_H

#include "yolov8_detect.h"
#include "yolov8_pose.h"
#include "ppocr.h"
//#include <optional>


/**
* high_voltage_cabinet_recognition 高压柜识别
*/
class HVC_Recognition {
public:
    struct MeterObj {
        int class_id{0};
        std::string className{};
        float confidence{0.0};
        cv::Scalar color{};
        cv::Rect box{}; //x1、y1、width、height   耳标坐标。

        std::vector<std::vector<float> > kps;
        // std::vector<keyPoint> kps;
        std::vector<int> kps_indexs;
        int number = -1;  //刻度示数
        std::string meter_type{};

        // 默认构造函数
        MeterObj() = default;  // 默认构造函数允许无参数初始化

        // 从 PoseObj 初始化的构造函数
        MeterObj(const PoseObj &pose_obj) {
            this->class_id = pose_obj.class_id;
            this->className = pose_obj.className;
            this->confidence = pose_obj.confidence;
            this->color = pose_obj.color;
            this->box = pose_obj.box;
            this->kps = pose_obj.kps;
            this->kps_indexs = pose_obj.kps_indexs;
        }
    };

    //
    struct SwitchLamp {
        std::string lamp;   // 黄灯lamp1、红灯lamp2、不亮lamp3
        cv::Rect box;   // lamp区域的左上角
    };

    // 分合闸指示灯
    struct RunnningLamp {
        // 分闸(绿亮红熄)|合闸(绿熄红亮)
        std::string green_lamp;   // 绿灯状态:亮|熄
        std::string red_lamp;   // 红灯状态:亮|熄
        // 分闸(闸刀指示灯断开)|合闸(闸刀指示灯连接)
        SwitchLamp switch_lamp;
        // 开|合
        std::string _running_lamp;
    };

    struct high_voltage_cabinet_result {
        std::string box_number;  // 柜号及设备号
        std::string switch_x;   // 就地/远方开关   就地|远方/遥控/现场
        std::string warning_lamp;  // 综保警告指示灯---无报警|有报警
        std::string I;  // 电流

        std::string breaker_lamp;   // 断路小车指示灯：运行(lamp2)|实验(lamp1)|不亮(lamp3)
        RunnningLamp running_lamp;  // 分合闸指示灯1:分闸(绿亮红熄)|合闸(绿熄红亮)  分合闸指示灯2:分闸(闸刀指示灯断开)|合闸(闸刀指示灯连接)
        std::vector<SwitchLamp> switch_lamps;
        std::string grounding_lamp;  // 接地刀闸指示灯:分闸|合闸
    };

    struct Param {
        YOLOv8Detect::Param hvc_part_position_detector_param;
        YOLOv8Detect::Param hvc_warning_detector_param;
        YOLOv8Pose::Param hvc_meter_recognizer_param;
        PPOCR::Param ppocr_param;
    };

    HVC_Recognition(AAssetManager *mgr, Param &param);

    HVC_Recognition(AAssetManager *mgr);

    ~HVC_Recognition();

    void pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &dst);

    void get_number(std::vector<MeterObj> &objs);

    float processMeterReading(cv::Mat &meter, std::vector<MeterObj> &objs);

private:
    std::unique_ptr<YOLOv8Detect> hvc_part_position_detector{nullptr};
    std::unique_ptr<YOLOv8Detect> hvc_warning_detector{nullptr};
    std::unique_ptr<YOLOv8Pose> hvc_meter_recognizer{nullptr};
    std::unique_ptr<PPOCR> ppocr{nullptr};
};


#endif //HIGH_VOLTAGE_CABINET_RECOGNITION_H
