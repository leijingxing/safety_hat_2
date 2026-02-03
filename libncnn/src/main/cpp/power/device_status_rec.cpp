/**************************************************************************************************
 * @date  : 2024/9/18 15:57:31
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "device_status_rec.h"


/**
* 将ocr识别的结果结构化4。
*/
std::unordered_map<std::string, std::string> structure_ocrResult_5(std::vector<std::string> &input) {
    auto size_ = input.size();
    std::unordered_map<std::string, std::string> result;
    result["dev_encodeid"] = "";
    result["dev_id"] = "";

    if (size_ == 2) {
        result["dev_encodeid"] = input.at(0);
        result["dev_id"] = input.at(1);
    }

    return result;
}


DeviceStatusRec::DeviceStatusRec(AAssetManager *mgr, Param &param) {
    yolov8_detector = std::make_unique<YOLOv8Detect>(mgr, param.device_status_position_detector_param);
    ppocr = std::make_unique<PPOCR>(mgr, param.ppocr_param);
}


//DeviceStatusRec::DeviceStatusRec(AAssetManager *mgr) {
//    std::string yolov8_detector_paramPath = "weights/high_voltage_cabinet_part_position.ncnn.param";
//    std::string yolov8_detector_binPath = "weights/high_voltage_cabinet_part_position.ncnn.bin";
//    yolov8_detector = std::make_unique<YOLOv8Detect>(mgr, yolov8_detector_paramPath, yolov8_detector_binPath);
//
//    std::string ocr_det_paramPath = "weights/ocr_det.ncnn.param";
//    std::string ocr_det_binPath = "weights/ocr_det.ncnn.bin";
//
//    std::string ocr_cls_paramPath = "weights/ocr_cls.ncnn.param";
//    std::string ocr_cls_binPath = "weights/ocr_cls.ncnn.bin";
//
//    std::string ocr_rec_paramPath = "weights/ocr_rec.ncnn.param";
//    std::string ocr_rec_binPath = "weights/ocr_rec.ncnn.bin";
//    ppocr = std::make_unique<PPOCR>(mgr, ocr_det_paramPath, ocr_det_binPath, ocr_cls_paramPath, ocr_cls_binPath, ocr_rec_paramPath, ocr_rec_binPath);
//}


DeviceStatusRec::~DeviceStatusRec() {}


void DeviceStatusRec::pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &device_status) {
    cv::Mat original_image_copy = original_image.clone();
    std::vector<DetectObj> objs;
    try {
        ncnn::Mat part_outputs;
        yolov8_detector->infer(original_image_copy, part_outputs);
        yolov8_detector->postprocess(part_outputs, objs);
        //yolov8_detector->draw_results(original_image, objs);
        //cv::imwrite("./device_status_result.jpg", original_image);
    }
    catch (const std::exception &e) {
        //std::cerr << "device_status detect module Caught an exception: " << e.what() << std::endl;
        LOGE("device_status yolov8_detector module Caught an exception");
    }

    for (auto obj: objs) {
        if (obj.class_id == 12) {
            device_status["dev_status"] = "送电";
        }
        else if (obj.class_id == 13) {
            device_status["dev_status"] = "停电";
        }
        else if (obj.class_id == 2) {
            cv::Mat box_number_img = original_image_copy(obj.box).clone();
            //cv::imwrite("./box_number_img.jpg", box_number_img);
            //OCR逻辑。
            std::unordered_map<std::string, std::string> box_info;
            try {
                std::vector<OcrObj> ocr_objs;
                ppocr->infer(box_number_img, ocr_objs);
                //
                std::vector<std::string> r;
                for (auto ocr_obj: ocr_objs) {
                    if (ocr_obj.text.empty()) { continue; }
                    r.push_back(ocr_obj.text);
                }
                try {
                    box_info = structure_ocrResult_5(r);
                }
                catch (const std::exception &e) {
                    //std::cerr << "structure_ocrResult Module Caught an exception: " << e.what() << std::endl;
                    LOGE("device_status structure_ocrResult Module Caught an exception");
                }
                device_status["dev_encodeid"] = box_info["dev_encodeid"];
                device_status["dev_id"] = box_info["dev_id"];
            }
            catch (const std::exception &e) {
                //std::cerr << "OCR Module Caught an exception: " << e.what() << std::endl;
                LOGE("device_status OCR Module Caught an exception");
            }
        }
    }
}
