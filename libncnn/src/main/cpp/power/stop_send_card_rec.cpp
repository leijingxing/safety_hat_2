/**************************************************************************************************
 * @date  : 2024/9/18 15:35:07
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "stop_send_card_rec.h"


/**
* 将ocr识别的结果结构化4---停送牌。
*/
std::unordered_map<std::string, std::string> structure_ocrResult_4(std::vector<std::string> &input) {
    std::unordered_map<std::string, std::string> result;
    result["dev_encodeid"] = "";
    result["dev_id"] = "";
    result["dev_status"] = "";

    auto size_ = input.size();
    if (size_ == 3 || size_ == 4) {
        std::string input_0 = input.at(0); // 0
        std::string input_1 = input.at(size_ - 1); // -1
        std::string input_2 = input.at(size_ - 2); // -2
        if (input_1 == "电" && (input_2 == "送" || input_2 == "停")) {
            std::string tt;
            for (int i = 0; i < size_ - 2; i++) {
                tt += input.at(i);
            }
            if (tt.find("AA") == std::string::npos && tt.find("AH") == std::string::npos) {
                result["dev_encodeid"] = "";
                result["dev_id"] = tt;
                result["dev_status"] = input_2 + input_1;
                return result;
            }
        }
    }
    if (size_ > 3) {
        std::string input_0 = input.at(0); // 0
        std::string input_1 = input.at(size_ - 1); // -1
        std::string input_2 = input.at(size_ - 2); // -2
        std::string input_3 = input.at(size_ - 3); // -3
        std::string input_4 = input.at(size_ - 4); // -3
        if (input_1 == "送电" || input_1 == "停电") {
            if (size_ == 5) {
                result["dev_encodeid"] = input[size_ - 3] + " " + input[size_ - 2];
                result["dev_id"] = input[0] + " " + input[1];
                result["dev_status"] = input[size_ - 1];
            }
            else if (size_ == 6) {
                result["dev_encodeid"] = input[size_ - 3] + " " + input[size_ - 2];
                result["dev_id"] = input[0] + " " + input[1] + " " + input[2];
                result["dev_status"] = input[size_ - 1];
            }
        }
        else if (input_1 == "电" && (input_2 == "送" || input_2 == "停")) {
            if (input_0.find("AA") != std::string::npos || input_0.find("AH") != std::string::npos) {
                result["dev_encodeid"] = input_0;
                result["dev_status"] = input_2 + input_1;
                for (int i = 1; i < input.size() - 2; i++) {
                    result["dev_id"] += input[i];
                }
            }
            else if (input_3.find("AA") != std::string::npos || input_3.find("AH") != std::string::npos) {
                result["dev_encodeid"] = input_3;
                result["dev_status"] = input_2 + input_1;
                for (int i = 0; i < input.size() - 3; i++) {
                    result["dev_id"] += input[i];
                }
            }
        }
        else if (input_1 == "电" && (input_3 == "送" || input_3 == "停")) {
            if (input_0.find("AA") != std::string::npos || input_0.find("AH") != std::string::npos) {
                result["dev_encodeid"] = input_0;
                result["dev_status"] = input_3 + input_1;
                for (int i = 1; i < input.size() - 3; i++) {
                    result["dev_id"] += input[i];
                }
            }
            else if (input_4.find("AA") != std::string::npos || input_4.find("AH") != std::string::npos) {
                result["dev_encodeid"] = input_4;
                result["dev_status"] = input_3 + input_1;
                for (int i = 0; i < input.size() - 4; i++) {
                    result["dev_id"] += input[i];
                }
            }
        }
    }

    return result;
}


StopSendCardRec::StopSendCardRec(AAssetManager *mgr, Param &param) {
    stop_send_card_position_detector = std::make_unique<YOLOv8Detect>(mgr, param.stop_send_card_position_detector_param);
    ppocr = std::make_unique<PPOCR>(mgr, param.ppocr_param);
}

//StopSendCardRec::StopSendCardRec(AAssetManager *mgr) {
//    std::string stop_send_card_position_detector_paramPath = "weights/stop_send_card_position.ncnn.param";
//    std::string stop_send_card_position_detector_binPath = "weights/stop_send_card_position.ncnn.bin";
//    stop_send_card_position_detector = std::make_unique<YOLOv8Detect>(mgr, stop_send_card_position_detector_paramPath, stop_send_card_position_detector_binPath);
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
//
//}


//StopSendCardRec::StopSendCardRec(AAssetManager *mgr,
//                           std::string paramPath, std::string binPath,
//                           int modelWidth, int modelHeight,
//                           bool useVulkan,
//                           std::vector<std::string> classes)
//        : paramPath(paramPath),
//          binPath(binPath),
//          model_height(modelHeight),
//          model_width(modelWidth),
//          use_vulkan(useVulkan),
//          classes(classes) {
//
//
//    model = new ncnn::Net(); // TODO
//    model->opt.use_vulkan_compute = use_vulkan;
//    model->load_param(mgr, paramPath.c_str());
//    model->load_model(mgr, binPath.c_str());
//}



StopSendCardRec::~StopSendCardRec() {}


void StopSendCardRec::pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &dst) {
    cv::Mat original_image_copy = original_image.clone();
    std::vector<DetectObj> objs;  // 存储停送牌。
    try {
        ncnn::Mat part_outputs;
        stop_send_card_position_detector->infer(original_image_copy, part_outputs);
        stop_send_card_position_detector->postprocess(part_outputs, objs);
        //stop_send_card_position_detector->draw_results(original_image, objs);
        //cv::imwrite("./stop_send_card_position.jpg", original_image);
    }
    catch (const std::exception &e) {
        //std::cerr << "part_position_detector module Caught an exception: " << e.what() << std::endl;
        LOGE("stop_send_card_position_detector module Caught an exception");
    }

    // std::vector<std::string> stop_send_card_info;
    // stop_send_card_info.emplace_back("");  // 电气位号
    // stop_send_card_info.emplace_back("");  // 设备位号
    // stop_send_card_info.emplace_back("");  // 设备状态


    for (const auto &obj: objs) {
        // 识别停送牌
        if (obj.class_id == 0) {
            cv::Mat stop_send_card_img = original_image_copy(obj.box).clone();
            //cv::imwrite("./stop_send_card_img.jpg", stop_send_card_img);
            //OCR逻辑。
            try {
                std::vector<OcrObj> ocr_objs;
                ppocr->infer(stop_send_card_img, ocr_objs);
                //
                std::vector<std::string> r;
                for (auto ocr_obj: ocr_objs) {
                    if (ocr_obj.text.empty()) { continue; }
                    r.push_back(ocr_obj.text);
                }
                try {
                    std::unordered_map<std::string, std::string> stop_send_card_info;
                    stop_send_card_info = structure_ocrResult_4(r);
                    dst["dev_encodeid"] = stop_send_card_info["dev_encodeid"];  // 电气位号
                    dst["dev_id"] = stop_send_card_info["dev_id"];  // 设备位号
                    dst["dev_status"] = stop_send_card_info["dev_status"];  // 设备状态
                }
                catch (const std::exception &e) {
                    //std::cerr << "structure_ocrResult Module Caught an exception: " << e.what() << std::endl;
                    LOGE("stop_send_card_position_detector structure_ocrResult Module Caught an exception");
                }
            }
            catch (const std::exception &e) {
                //std::cerr << "OCR Module Caught an exception: " << e.what() << std::endl;
                LOGE("stop_send_card_position_detector OCR Module Caught an exception");
            }
            // std::cout << "stop_send_card_info = " << stop_send_card_info[0] << std::endl;
        }
    }
}
