/**************************************************************************************************
 * @date  : 2024/9/14 11:35:54
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "high_voltage_cabinet_rec.h"


HVC_Recognition::HVC_Recognition(AAssetManager *mgr, Param &param) {
    hvc_part_position_detector = std::make_unique<YOLOv8Detect>(mgr, param.hvc_part_position_detector_param);
    hvc_warning_detector = std::make_unique<YOLOv8Detect>(mgr, param.hvc_warning_detector_param);
    hvc_meter_recognizer = std::make_unique<YOLOv8Pose>(mgr, param.hvc_meter_recognizer_param);
    ppocr = std::make_unique<PPOCR>(mgr, param.ppocr_param);
}


//HVC_Recognition::HVC_Recognition(AAssetManager *mgr) {
//    std::string hvc_part_position_detector_paramPath = "weights/high_voltage_cabinet_part_position.ncnn.param";
//    std::string hvc_part_position_detector_binParam = "weights/high_voltage_cabinet_part_position.ncnn.bin";
//    hvc_part_position_detector = std::make_unique<YOLOv8Detect>(mgr, hvc_part_position_detector_paramPath, hvc_part_position_detector_binParam);
//    std::string hvc_warning_detector_paramPath = "weights/high_voltage_cabinet_warning_detect.ncnn.param";
//    std::string hvc_warning_detector_binParam = "weights/high_voltage_cabinet_warning_detect.ncnn.bin";
//    hvc_warning_detector = std::make_unique<YOLOv8Detect>(mgr, hvc_warning_detector_paramPath, hvc_warning_detector_binParam);
//    std::string hvc_meter_recognizer_paramPath = "weights/high_voltage_cabinet_meter_recognition.ncnn.param";
//    std::string hvc_meter_recognizer_binParam = "weights/high_voltage_cabinet_meter_recognition.ncnn.bin";
//    hvc_meter_recognizer = std::make_unique<YOLOv8Pose>(mgr, hvc_meter_recognizer_paramPath, hvc_meter_recognizer_binParam);
//    std::string ocr_det_paramPath = "weights/ocr_det.ncnn.param";
//    std::string ocr_det_binPath = "weights/ocr_det.ncnn.bin";
//    std::string ocr_cls_paramPath = "weights/ocr_cls.ncnn.param";
//    std::string ocr_cls_binPath = "weights/ocr_cls.ncnn.bin";
//    std::string ocr_rec_paramPath = "weights/ocr_rec.ncnn.param";
//    std::string ocr_rec_binPath = "weights/ocr_rec.ncnn.bin";
//    ppocr = std::make_unique<PPOCR>(mgr, ocr_det_paramPath, ocr_det_binPath, ocr_cls_paramPath, ocr_cls_binPath, ocr_rec_paramPath, ocr_rec_binPath);
//}


HVC_Recognition::~HVC_Recognition() {}


void HVC_Recognition::pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &dst) {
    high_voltage_cabinet_result result;
    cv::Mat original_image_copy = original_image.clone();

    ncnn::Mat part_outputs;
    hvc_part_position_detector->infer(original_image_copy, part_outputs);
    std::vector<DetectObj> part_results;
    hvc_part_position_detector->postprocess(part_outputs, part_results);
    //hvc_part_position_detector->draw_results(original_image, part_results);
    //cv::imwrite("./hvc_result.png", original_image);

    for (const auto &part_result: part_results) {
        if (part_result.class_id == 0) {
            cv::Mat zongbao_img = original_image_copy(part_result.box).clone();
            cv::Mat zongbao_img_copy = zongbao_img.clone();
            ncnn::Mat warning_outputs;
            hvc_warning_detector->infer(original_image_copy, warning_outputs);
            std::vector<DetectObj> warning_objs;
            hvc_warning_detector->postprocess(warning_outputs, warning_objs);
            //hvc_warning_detector->draw_results(zongbao_img, warning_objs);
            //cv::imwrite("./zongbao_img_result.png", zongbao_img);
            result.warning_lamp = "无报警";
            for (const auto &warning_obj: warning_objs) {
                if (warning_obj.class_id == 2) {
                    result.warning_lamp = "有报警";
                    break;
                }
            }
        }
        else if (part_result.class_id == 1) {
            cv::Mat meter = original_image_copy(part_result.box).clone();
            //cv::imwrite("./meter.jpg", meter);
            cv::Mat meter_copy = meter.clone();
            try {
                float pointer_scale_value;  // 电压表/电流表的指针读数
                ncnn::Mat metet_outputs;
                hvc_meter_recognizer->infer(meter_copy, metet_outputs);
                std::vector<PoseObj> _meter_objs;
                hvc_meter_recognizer->postprocess(metet_outputs, _meter_objs);
                //hvc_meter_recognizer->draw_results(meter, _meter_objs);
                //cv::imwrite("./yolo_pose_result.png", meter);
                //转换类型
                std::vector<MeterObj> meter_objs;
                for (const auto &_meter_obj: _meter_objs) {
                    MeterObj meter_obj(_meter_obj);  // 使用 MeterObj 构造函数转换
                    meter_objs.push_back(meter_obj);
                }
                get_number(meter_objs);
                pointer_scale_value = processMeterReading(meter, meter_objs);  // 计算度数
                if (pointer_scale_value != -10000) {
                    //std::cout << "pointer scale value: " << static_cast<int>(pointer_scale_value) << std::endl;
                    LOGD("pointer scale value = %d", static_cast<int>(pointer_scale_value));
                    result.I = std::to_string(static_cast<int>(pointer_scale_value));
                }
            }
            catch (const std::exception &e) {
                //std::cerr << "meter_recognizer module Caught an exception: " << e.what() << std::endl;
                LOGE("meter_recognizer module Caught an exception");
            }
        }
        else if (part_result.class_id == 2) {
            cv::Mat box_number_img = original_image_copy(part_result.box).clone();
            //cv::imwrite("./box_number_img.jpg", box_number_img);
            try {
                std::vector<OcrObj> ocr_objs;
                ppocr->infer(box_number_img, ocr_objs);
                // 拼接柜号
                for (int i = 0; i < ocr_objs.size(); i++) {
                    auto ocr_obj = ocr_objs[i];
                    if (ocr_obj.text.empty()) { continue; }
                    if (i < ocr_objs.size() - 1) { result.box_number += ocr_obj.text + " "; }
                    else { result.box_number += ocr_obj.text; }
                }
            }
            catch (const std::exception &e) {
                //std::cerr << "OCR Module Caught an exception: " << e.what() << std::endl;
                LOGE("meter_recognizer OCR Module Caught an exception");
            }
        }
        else if (part_result.class_id == 3) {
            result.switch_x = "就地";
        }
        else if (part_result.class_id == 4) {
            result.switch_x = "远方";
        }
        else if (part_result.class_id == 5) {
            result.running_lamp.green_lamp = "熄";
        }
        else if (part_result.class_id == 6) {
            result.running_lamp.green_lamp = "亮";
        }
        else if (part_result.class_id == 7) {
            result.running_lamp.red_lamp = "熄";
        }
        else if (part_result.class_id == 8) {
            result.running_lamp.red_lamp = "亮";
        }
        else if (part_result.class_id == 9) {
            SwitchLamp _switch_lamp;
            _switch_lamp.lamp = "黄";
            _switch_lamp.box = part_result.box;
            result.switch_lamps.push_back(_switch_lamp);
        }
        else if (part_result.class_id == 10) {
            SwitchLamp _switch_lamp;
            _switch_lamp.lamp = "红";
            _switch_lamp.box = part_result.box;
            result.switch_lamps.push_back(_switch_lamp);
        }
        else if (part_result.class_id == 11) {
            SwitchLamp _switch_lamp;
            _switch_lamp.lamp = "熄";
            _switch_lamp.box = part_result.box;
            result.switch_lamps.push_back(_switch_lamp);
        }
    }

    std::sort(result.switch_lamps.begin(), result.switch_lamps.end(), [](const SwitchLamp &a, const SwitchLamp &b) { return a.box.y < b.box.y; });
    if (result.switch_lamps.size() == 4) {
        // 断路小车指示灯
        if (result.switch_lamps.at(0).lamp == "黄") {
            result.breaker_lamp = "实验";
        }
        else if (result.switch_lamps.at(0).lamp == "红") {
            result.breaker_lamp = "运行";
        }
        else if (result.switch_lamps.at(0).lamp == "熄") {
            result.breaker_lamp = "异常";
        }
        // 分合闸指示灯
        if (result.switch_lamps.at(1).lamp == "黄" || (result.running_lamp.green_lamp == "亮" && result.running_lamp.red_lamp == "熄")) {
            result.running_lamp._running_lamp = "分闸";
        }
        else if (result.switch_lamps.at(1).lamp == "红" || (result.running_lamp.green_lamp == "熄" && result.running_lamp.red_lamp == "亮")) {
            result.running_lamp._running_lamp = "合闸";
        }
        else if (result.switch_lamps.at(1).lamp == "熄") {
            result.running_lamp._running_lamp = "异常";
        }
        // 接地刀闸指示灯
        if (result.switch_lamps.at(3).lamp == "黄") {
            result.grounding_lamp = "分闸";
        }
        else if (result.switch_lamps.at(3).lamp == "红") {
            result.grounding_lamp = "合闸";
        }
        else if (result.switch_lamps.at(3).lamp == "熄") {
            result.grounding_lamp = "异常";
        }
    }
    else if (result.switch_lamps.size() == 3) {
        int dis = std::abs(result.switch_lamps.front().box.x - result.switch_lamps.back().box.x);
        if (dis < result.switch_lamps.front().box.width) {
            // 断路小车指示灯
            if (result.switch_lamps.at(0).lamp == "黄") {
                result.breaker_lamp = "实验";
            }
            else if (result.switch_lamps.at(0).lamp == "红") {
                result.breaker_lamp = "运行";
            }
            else if (result.switch_lamps.at(0).lamp == "熄") {
                result.breaker_lamp = "异常";
            }
            // 分合闸指示灯
            if (result.switch_lamps.at(1).lamp == "黄" || (result.running_lamp.green_lamp == "亮" && result.running_lamp.red_lamp == "熄")) {
                result.running_lamp._running_lamp = "分闸";
            }
            else if (result.switch_lamps.at(1).lamp == "红" || (result.running_lamp.green_lamp == "熄" && result.running_lamp.red_lamp == "亮")) {
                result.running_lamp._running_lamp = "合闸";
            }
            else if (result.switch_lamps.at(1).lamp == "熄") {
                result.running_lamp._running_lamp = "异常";
            }
        }
        else {
            // 断路小车指示灯
            if (result.switch_lamps.at(0).lamp == "黄") {
                result.breaker_lamp = "实验";
            }
            else if (result.switch_lamps.at(0).lamp == "红") {
                result.breaker_lamp = "运行";
            }
            else if (result.switch_lamps.at(0).lamp == "熄") {
                result.breaker_lamp = "异常";
            }
            // 分合闸指示灯
            if (result.switch_lamps.at(1).lamp == "黄" || (result.running_lamp.green_lamp == "亮" && result.running_lamp.red_lamp == "熄")) {
                result.running_lamp._running_lamp = "分闸";
            }
            else if (result.switch_lamps.at(1).lamp == "红" || (result.running_lamp.green_lamp == "熄" && result.running_lamp.red_lamp == "亮")) {
                result.running_lamp._running_lamp = "合闸";
            }
            else if (result.switch_lamps.at(1).lamp == "熄") {
                result.running_lamp._running_lamp = "异常";
            }
            // 接地刀闸指示灯
            if (result.switch_lamps.at(2).lamp == "黄") {
                result.grounding_lamp = "分闸";
            }
            else if (result.switch_lamps.at(2).lamp == "红") {
                result.grounding_lamp = "合闸";
            }
            else if (result.switch_lamps.at(2).lamp == "熄") {
                result.grounding_lamp = "异常";
            }
        }
    }
    else if (result.switch_lamps.size() == 2) {
        // 断路小车指示灯
        if (result.switch_lamps.at(0).lamp == "黄") {
            result.breaker_lamp = "实验";
        }
        else if (result.switch_lamps.at(0).lamp == "红") {
            result.breaker_lamp = "运行";
        }
        else if (result.switch_lamps.at(0).lamp == "熄") {
            result.breaker_lamp = "异常";
        }
        // 分合闸指示灯
        if (result.switch_lamps.at(1).lamp == "黄" || (result.running_lamp.green_lamp == "亮" && result.running_lamp.red_lamp == "熄")) {
            result.running_lamp._running_lamp = "分闸";
        }
        else if (result.switch_lamps.at(1).lamp == "红" || (result.running_lamp.green_lamp == "熄" && result.running_lamp.red_lamp == "亮")) {
            result.running_lamp._running_lamp = "合闸";
        }
        else if (result.switch_lamps.at(1).lamp == "熄") {
            result.running_lamp._running_lamp = "异常";
        }
    }

    dst["柜号及设备号"] = result.box_number;
    dst["就地/远方开关"] = result.switch_x;
    dst["断路小车"] = result.breaker_lamp;
    dst["分合闸"] = result.running_lamp._running_lamp;
    dst["接地刀闸"] = result.grounding_lamp;
    dst["综保警告"] = result.warning_lamp;
    dst["电流表读数"] = result.I;
}


void HVC_Recognition::get_number(std::vector<MeterObj> &objs) {
    // 寻找单位以及表的类型
    float unit{1.0};
    std::string _meter_type;
    for (auto &obj: objs) {
        if (obj.class_id == 51) {
            _meter_type = "A";
            break;
        }
        else if (obj.class_id == 52) {
            _meter_type = "V";
            break;
        }
        else if (obj.class_id == 53) {
            _meter_type = "KA";
            unit = 1000.0;
            break;
        }
        else if (obj.class_id == 54) {
            _meter_type = "KV";
            unit = 1000.0;
            break;
        }
    }
    if (_meter_type == "A" || _meter_type == "KA") {
        for (auto &obj: objs) {
            if (obj.class_id <= 50) {
                obj.number = std::stof(hvc_meter_recognizer->classes[obj.class_id]) * unit;
            }
        }
    }
    else { objs.clear(); }  // 不计算电压表的读数。
}


/**
* 计算pointer tail坐标
* @param a1x 左侧刻度x
* @param a1y 左侧刻度y
* @param a2x 右侧刻度x
* @param a2y 右侧刻度y
* @param c1x 指针头x
* @param c1y 指针头y
* @param c2x 指针尾x
* @param c2y 指针尾y
* @return 指针尾坐标
*/
std::pair<float, float> get_pointertail(float a1x, float a1y, float a2x, float a2y, float c1x, float c1y, float c2x, float c2y) {
    // 定义线段A的中点
    float midpoint_x = (a1x + a2x) / 2;
    float midpoint_y = (a1y + a2y) / 2;

    // 计算直线B的斜率和截距
    float slope_B = 0, intercept_B = 0; // 初始化斜率和截距
    if (std::abs(a2y - a1y) < 1e-6) { // 如果线段A是水平线
        slope_B = INFINITY; // 斜率不存在，表示为正无穷
        intercept_B = midpoint_x; // 截距为中点的x坐标
    }
    else if (std::abs(a2x - a1x) < 1e-6) { // 如果线段A是垂直线
        slope_B = 0; // 斜率为0
        intercept_B = midpoint_y; // 截距为中点的y坐标
    }
    else { // 否则，计算斜率和截距
        slope_B = (a2x - a1x) / (a1y - a2y); // 直线B与线段A的斜率的负倒数
        intercept_B = midpoint_y - slope_B * midpoint_x;
    }

    // 计算直线C的斜率和截距
    float slope_C = 0, intercept_C = 0; // 初始化斜率和截距
    if (std::abs(c2y - c1y) < 1e-6) { // 如果直线C是水平线
        slope_C = 0; // 斜率为0
        intercept_C = c1y; // 截距为起始点的y坐标
    }
    else if (std::abs(c2x - c1x) < 1e-6) { // 如果直线C是垂直线
        slope_C = INFINITY; // 斜率不存在，表示为正无穷
        intercept_C = c1x; // 截距为起始点的x坐标
    }
    else { // 否则，计算斜率和截距
        slope_C = (c2y - c1y) / (c2x - c1x);
        intercept_C = c1y - slope_C * c1x;
    }

    float intersection_x = -10000., intersection_y = -10000.;
    // 判断两条直线是否平行
    if (std::abs(slope_B - slope_C) < 1e-6) {
        //std::cout << "The two lines are parallel, no intersection exists." << std::endl;
        LOGD("The two lines are parallel, no intersection exists.");
    }
    else if (slope_C == INFINITY && slope_B != INFINITY) {
        intersection_x = intercept_C; // 交点的x坐标为直线C的截距
        intersection_y = slope_B * intersection_x + intercept_B; // 直线B的方程求得y坐标
    }
    else if (slope_C != INFINITY && slope_B == INFINITY) {
        intersection_x = intercept_B;
        intersection_y = slope_C * intersection_x + intercept_C;
    }
    else if (slope_C == 0 && slope_B != 0) {
        intersection_y = intercept_C;
        intersection_x = (intersection_y - intercept_B) / slope_B;
    }
    else if (slope_B == 0 && slope_C != 0) {
        intersection_y = intercept_B;
        intersection_x = (intersection_y - intercept_C) / slope_C;
    }
    else {
        // 计算直线B和直线C的交点
        intersection_x = (intercept_C - intercept_B) / (slope_B - slope_C);
        intersection_y = slope_B * intersection_x + intercept_B;
    }

    if (intersection_x < 0) {
        return std::make_pair(-10000., -10000.);
    }
    // // 输出交点坐标
    // std::cout << "Intersection point coordinates: (" << intersection_x << ", " << intersection_y << ")" << std::endl;

    return std::make_pair(intersection_x, intersection_y);
}


/**
* 计算角度
* @param A 点A坐标
* @param C 点C坐标
* @param B 点B坐标
* @return 角度
*/
float calculate_angle(std::vector<float> A, std::vector<float> C, std::vector<float> B) {
    // 创建向量ba和bc
    std::vector<float> ba = {A[0] - B[0], A[1] - B[1]};
    std::vector<float> bc = {C[0] - B[0], C[1] - B[1]};

    // 计算向量的点积
    float dot_product = ba[0] * bc[0] + ba[1] * bc[1];

    // 计算向量的模
    float norm_a = sqrt(ba[0] * ba[0] + ba[1] * ba[1]);
    float norm_c = sqrt(bc[0] * bc[0] + bc[1] * bc[1]);

    // 使用点积公式求出角度
    float angle = acos(dot_product / (norm_a * norm_c));

    // 将弧度转换为度
    angle = angle * 180 / M_PI;

    return angle;
}


/**
* 处理图像中的刻度点和指针，并计算尺度值
* @param res 图片
* @param objs 检测的目标
* @param device_type 设备类别
* @return 读数
*/
float HVC_Recognition::processMeterReading(cv::Mat &res, std::vector<MeterObj> &objs) {
    float pointer_scale_value = -10000;  // 初始化读数
    std::vector<std::vector<float> > dots;  // 存储所有的刻度点目标
    float pointer_x = -1, pointer_y = -1, pointertail_x = -1, pointertail_y = -1;  // 指针头、尾的坐标

    std::vector<MeterObj> objs_t;  // 临时存储目标
    for (auto &obj: objs) {
        if (obj.kps.empty()) { continue; }
        if (obj.class_id == 55 && obj.kps.size() >= 2) { // 检查标签是否为指针框，且头尾都检测到。
            if ((obj.kps[0][0] < obj.kps[1][0]) || ((obj.kps[0][0] >= obj.kps[1][0]) && (obj.kps[0][1] < obj.kps[1][1]))) {
                pointer_x = obj.kps[0][0];
                pointer_y = obj.kps[0][1];
                pointertail_x = obj.kps[1][0];
                pointertail_y = obj.kps[1][1];
            }
            else {
                pointer_x = obj.kps[1][0];
                pointer_y = obj.kps[1][1];
                pointertail_x = obj.kps[0][0];
                pointertail_y = obj.kps[0][1];
            }
        }
        else if (obj.class_id <= 50) { // 检查标签是否为数值框，且检测到至少一个关键点。
            // dots.push_back({obj.kps[0][0], obj.kps[0][1], class_values[obj.class_id]});  // 多检时，取其中一个。
            objs_t.push_back(obj);
        }
    }
    if (objs_t.size() < 2) {  // 电流表/电压表至少得有最小值和最大值两个刻度
        return pointer_scale_value;
    }
    if (pointer_x != -1 && pointertail_x != -1) {
        // 保持水平
        std::vector<MeterObj> objs_t1(objs_t);
        // std::vector<MeterObj> objs_t2(objs_t);
        // std::vector<MeterObj> objs_t3(objs_t);
        std::sort(objs_t1.begin(), objs_t1.end(), [&](auto &a, auto &b) { return a.number < b.number; });
        // std::sort(objs_t2.begin(), objs_t2.end(), [&](auto &a, auto &b) { return (a.rect.x + a.rect.width / 2) <= (b.rect.x + b.rect.width / 2); });
        // std::sort(objs_t3.begin(), objs_t3.end(), [&](auto &a, auto &b) { return (a.rect.y + a.rect.height / 2) > (b.rect.y + b.rect.height / 2); });
        // for (int i = 0; i < objs_t.size(); i++) {
        //     if (objs_t1.at(i).number != objs_t2.at(i).number || objs_t1.at(i).number != objs_t3.at(i).number || objs_t2.at(i).number != objs_t3.at(i).number) {
        //         // cv::putText(res, "Please keep level", cv::Point((int) (res.cols / 2), (int) (res.rows / 2)), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, {0, 255, 255}, 2);
        //         // 绘制中文文本
        //         cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
        //         ft2->loadFontData("../lib/font/SourceHanSerifSC-Regular.ttf", 0);
        //         int fontHeight = 30;
        //         ft2->putText(res, "请保持水平", cv::Point((int) (res.cols / 2), (int) (res.rows / 2)), fontHeight, cv::Scalar{0, 0, 200}, -1, cv::LINE_8, false);
        //         return std::nullopt;
        //     }
        // }

        // 找出离指针头最近的两个刻度点
        MeterObj left_dot, right_dot;
        int branch = 0;  // 用于后续计算读数走不同的分支，即计算方法不一样。
        if (pointer_y > objs_t1.front().kps.at(0).at(1)) {  // 指针头在最小刻度的下侧
            left_dot = objs_t1[0];
            right_dot = objs_t1[1];
            branch = -1;
        }
        else if (pointer_y == objs_t1.front().kps.at(0).at(1)) {  // 指针头刚好在最小刻度
            pointer_scale_value = objs_t1.front().number;
            return pointer_scale_value;
        }
        else if (pointer_x > objs_t1.back().kps.at(0).at(0)) {  // 指针头在最大刻度的右侧
            left_dot = objs_t1.back();
            right_dot = objs_t1.at(objs_t1.size() - 2);
            branch = 1;
        }
        else if (pointer_x == objs_t1.back().kps.at(0).at(0)) {  // 指针头刚好在最大刻度
            pointer_scale_value = objs_t1.back().number;
            return pointer_scale_value;
        }
        else {  // 指针头在最小与最大刻度之间
            // 寻找具体在哪两个刻度之间
            for (int i = 0; i < objs_t1.size(); i++) {
                if (objs_t1.at(i + 1).kps.at(0).at(1) <= pointer_y && pointer_y < objs_t1.at(i).kps.at(0).at(1)) {
                    left_dot = objs_t1.at(i);
                    right_dot = objs_t1.at(i + 1);
                    break;
                }
            }
        }

        std::pair<float, float> intersection;  // 存储通过计算得到的指针尾坐标。
        int mark = 0;  // 标记，是否找到。
        for (const auto &obj: objs_t1) {
            // 向右找
            if (obj.number > right_dot.number) {
                intersection = get_pointertail(left_dot.kps.at(0).at(0), left_dot.kps.at(0).at(1), obj.kps.at(0).at(0), obj.kps.at(0).at(1),
                                               pointer_x, pointer_y, pointertail_x, pointertail_y);
                mark = 1;
                break;
            }
        }
        if (mark == 0) {
            for (const auto &obj: objs_t1) {
                // 向左找
                if (obj.number < left_dot.number) {
                    intersection = get_pointertail(obj.kps.at(0).at(0), obj.kps.at(0).at(1), right_dot.kps.at(0).at(0), right_dot.kps.at(0).at(1),
                                                   pointer_x, pointer_y, pointertail_x, pointertail_y);
                    mark = 1;
                    break;
                }
            }
        }

        if (mark && intersection.first != -10000.) {
            pointertail_x = intersection.first;
            pointertail_y = intersection.second;
        }

        // cv::circle(res, cv::Point{(int) pointer_x, (int) pointer_y}, 4, cv::Scalar(0, 255, 255), -1);
        // cv::circle(res, cv::Point{(int) pointertail_x, (int) pointertail_y}, 4, cv::Scalar(0, 255, 255), -1);
        // // sprintf(text2, "%s", "head");
        // // cv::putText(res, text2, cv::Point((int) pointer_x, (int) pointer_y), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, {0, 100, 155}, 1);
        // sprintf(text2, "%s", "tail");
        // cv::putText(res, text2, cv::Point((int) pointertail_x, (int) pointertail_y), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, {0, 50, 150}, 2);
        // cv::imwrite("../output/draw.jpg", res);

        // 计算角度和尺度值
        float angle1 = calculate_angle(left_dot.kps.at(0), right_dot.kps.at(0), {pointertail_x, pointertail_y});
        if (branch == -1) {
            float angle2 = calculate_angle(left_dot.kps.at(0), {pointer_x, pointer_y}, {pointertail_x, pointertail_y});
            pointer_scale_value = left_dot.number - (angle2 / angle1) * (right_dot.number - left_dot.number);
        }
        else if (branch == 1) {
            float angle2 = calculate_angle(right_dot.kps.at(0), {pointer_x, pointer_y}, {pointertail_x, pointertail_y});
            pointer_scale_value = right_dot.number + (angle2 / angle1) * (right_dot.number - left_dot.number);
        }
        else {
            float angle2 = calculate_angle(left_dot.kps.at(0), {pointer_x, pointer_y}, {pointertail_x, pointertail_y});
            float scale = angle2 / angle1;
            //std::cout << angle2 << ", " << scale << std::endl;
            if (scale < 0.03) {
                pointer_scale_value = left_dot.number;
            }
            else {
                pointer_scale_value = left_dot.number + (angle2 / angle1) * (right_dot.number - left_dot.number);
            }
        }
        // std::cout << "pointer_scale_value=" << pointer_scale_value << std::endl;
    }
    return pointer_scale_value;
}
