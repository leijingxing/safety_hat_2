/**************************************************************************************************
 * @date  : 2024/9/18 14:18:19
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "low_voltage_cabinet_rec.h"


/**
* 将ocr识别的结果结构化2---低压柜屏幕识别结果结构化。
*/
std::vector<std::pair<std::string, std::string> > structure_ocrResult_2(const std::vector<std::string> &input) {
    std::vector<std::pair<std::string, std::string> > result; // 结果向量
    size_t pos;
    std::string key;
    std::string value;
    if (input.size() == 4) {
        if (input[0].find("Ia") != std::string::npos &&
            input[1].find("Ib") != std::string::npos &&
            input[2].find("Ic") != std::string::npos) {
            pos = input[0].find("Ia");
            if (pos != std::string::npos) {
                key = "Ia";
                pos = input[0].find("= ");
                if (pos != std::string::npos) {
                    value = input[0].substr(pos + 2);
                }
                else {
                    pos = input[0].find("=");
                    if (pos != std::string::npos) {
                        value = input[0].substr(pos + 1);
                    }
                    else {
                        pos = input[0].find(" ");
                        if (pos != std::string::npos) {
                            value = input[0].substr(pos + 1);
                        }
                    }
                }
                result.push_back(std::make_pair(key, value));
            }

            pos = input[1].find("Ib");
            if (pos != std::string::npos) {
                key = "Ib";
                pos = input[1].find("= ");
                if (pos != std::string::npos) {
                    value = input[1].substr(pos + 2);
                }
                else {
                    pos = input[1].find("=");
                    if (pos != std::string::npos) {
                        value = input[1].substr(pos + 1);
                    }
                    else {
                        pos = input[1].find(" ");
                        if (pos != std::string::npos) {
                            value = input[1].substr(pos + 1);
                        }
                    }
                }
                result.push_back(std::make_pair(key, value));
            }

            pos = input[2].find("Ic");
            if (pos != std::string::npos) {
                key = "Ic";
                pos = input[2].find("= ");
                if (pos != std::string::npos) {
                    value = input[2].substr(pos + 2);
                }
                else {
                    pos = input[2].find("=");
                    if (pos != std::string::npos) {
                        value = input[2].substr(pos + 1);
                    }
                    else {
                        pos = input[2].find(" ");
                        if (pos != std::string::npos) {
                            value = input[2].substr(pos + 1);
                        }
                    }
                }
                result.push_back(std::make_pair(key, value));
            }
        }
    }

    return result; // 返回转换后的结果向量
}


LVC_Recognition::LVC_Recognition(AAssetManager *mgr, Param &param) {
    lvc_part_position_detector = std::make_unique<YOLOv8Detect>(mgr, param.lvc_part_position_detector_param);
    lvc_drawer_position_detector = std::make_unique<YOLOv8Detect>(mgr, param.lvc_drawer_position_detector_param);
    ppocr = std::make_unique<PPOCR>(mgr, param.ppocr_param);
}


//LVC_Recognition::LVC_Recognition(AAssetManager *mgr) {
//    std::string lvc_part_position_detector_paramPath = "weights/low_voltage_cabinet_part_position.ncnn.param";
//    std::string lvc_part_position_detector_binPath = "weights/low_voltage_cabinet_part_position.ncnn.bin";
//    lvc_part_position_detector = std::make_unique<YOLOv8Detect>(mgr, lvc_part_position_detector_paramPath, lvc_part_position_detector_binPath);
//    std::string lvc_drawer_position_detector_paramPath = "weights/low_voltage_cabinet_drawer_position.ncnn.param";
//    std::string lvc_drawer_position_detector_binPath = "weights/low_voltage_cabinet_drawer_position.ncnn.bin";
//    lvc_drawer_position_detector = std::make_unique<YOLOv8Detect>(mgr, lvc_drawer_position_detector_paramPath, lvc_drawer_position_detector_binPath);
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


LVC_Recognition::~LVC_Recognition() {}


void LVC_Recognition::pipeline(cv::Mat &original_image, std::unordered_map<std::string, std::string> &dst) {
    low_voltage_cabinet_result result;

    cv::Mat original_image_copy = original_image.clone();

    std::vector<DetectObj> part_results;
    try {
        ncnn::Mat part_outputs;
        lvc_part_position_detector->infer(original_image_copy, part_outputs);
        lvc_part_position_detector->postprocess(part_outputs, part_results);
        //lvc_part_position_detector->draw_results(original_image, part_results);
        //cv::imwrite("./lvc_result.png", original_image);
    }
    catch (const std::exception &e) {
        //std::cerr << "lvc part_position_detector module Caught an exception: " << e.what() << std::endl;
        LOGE("lvc part_position_detector module Caught an exception");
    }


    for (const auto &part_obj: part_results) {
        // 柜号OCR
        if (part_obj.class_id == 0) {
            cv::Mat box_number_img = original_image_copy(part_obj.box).clone();
            //cv::imwrite("./lvc_box_number_img.jpg", box_number_img);
            //OCR逻辑。
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
                LOGE("lvc OCR Module Caught an exception");
            }
            //std::cout << "box_number = " << result.box_number << std::endl;
            LOGD("box_number=%s", result.box_number.c_str());
        }
            // 开关状态
        else if (part_obj.class_id == 1) {
            result.switch_x = "on";
        }
        else if (part_obj.class_id == 2) {
            result.switch_x = "off";
        }
            // 指示灯
        else if (part_obj.class_id == 3) {
            result.green_x = "亮";
        }
        else if (part_obj.class_id == 4) {
            result.green_x = "熄";
        }
        else if (part_obj.class_id == 5) {
            result.red_x = "亮";
        }
        else if (part_obj.class_id == 6) {
            result.red_x = "熄";
        }
        else if (part_obj.class_id == 7) {  //屏幕
            std::vector<std::pair<std::string, std::string> > structured_ocrResults;

            cv::Mat screen_img = original_image_copy(part_obj.box).clone();
            //cv::imwrite("/sdcard/Download/screen_img.jpg", screen_img);
            try {
                std::vector<OcrObj> ocr_objs;
                ppocr->infer(screen_img, ocr_objs);
                //
                std::vector<std::string> r;
                for (auto ocr_obj: ocr_objs) {
                    if (ocr_obj.text.empty()) { continue; }
                    r.push_back(ocr_obj.text);
                }
                try {
                    structured_ocrResults = structure_ocrResult_2(r);
                }
                catch (const std::exception &e) {
                    //std::cerr << "structure_ocrResult Module Caught an exception: " << e.what() << std::endl;
                    LOGE("lvc structure_ocrResult Module Caught an exception");
                }
            }
            catch (const std::exception &e) {
                //std::cerr << "OCR Module Caught an exception: " << e.what() << std::endl;
                LOGE("lvc OCR Module Caught an exception");
            }
            for (const auto &structured_ocrResult: structured_ocrResults) {
                //std::cout << "屏幕ocr结果---" << structured_ocrResult.first << ":"
                //          << structured_ocrResult.second << std::endl;
                LOGD("屏幕ocr结果---%s:%s", structured_ocrResult.first.c_str(), structured_ocrResult.second.c_str());
            }

            for (auto &structured_ocrResult: structured_ocrResults) {
                if (structured_ocrResult.second == "0 00A" || structured_ocrResult.second == "0 000A" ||
                    structured_ocrResult.second == "0.000A" || structured_ocrResult.second == "0. 00A") {
                    structured_ocrResult.second = "0.00A";
                }

                if (structured_ocrResult.first == "Ia") {
                    result.structured_ocr_result.Ia = structured_ocrResult.second;
                }
                else if (structured_ocrResult.first == "Ib") {
                    result.structured_ocr_result.Ib = structured_ocrResult.second;
                }
                else if (structured_ocrResult.first == "Ic") {
                    result.structured_ocr_result.Ic = structured_ocrResult.second;
                }
            }
        }
        else if (part_obj.class_id == 8) {
            result.yellow_x = "亮";
        }
        else if (part_obj.class_id == 9) {
            result.yellow_x = "熄";
        }
        else if (part_obj.class_id == 10) {
            result.cabinet_position = "运行";
        }
        else if (part_obj.class_id == 11) {
            result.cabinet_position = "试验";
        }
        else if (part_obj.class_id == 12) {
            result.cabinet_position = "抽出";
        }
    }
    dst["柜号及设备号"] = result.box_number;
    dst["开关位置"] = result.switch_x;
    dst["绿灯"] = result.green_x;
    dst["红灯"] = result.red_x;
    dst["黄灯"] = result.yellow_x;
    dst["Ia"] = result.structured_ocr_result.Ia;
    dst["Ib"] = result.structured_ocr_result.Ib;
    dst["Ic"] = result.structured_ocr_result.Ic;
    dst["抽屉位置"] = result.cabinet_position;

}
