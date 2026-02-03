/**************************************************************************************************
 * @date  : 2024/9/14 15:41:26
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "ppocr.h"


PPOCR::PPOCR(AAssetManager *mgr, Param &param) {
    this->use_ocr_cls = param.use_ocr_cls;
    ppocr_det = std::make_unique<PPOCR_Det>(mgr, param.ppocr_det_param);
    if (use_ocr_cls) {
        ppocr_cls = std::make_unique<PPOCR_Cls>(mgr, param.ppocr_cls_param);
    }
    ppocr_rec = std::make_unique<PPOCR_Rec>(mgr, param.ppocr_rec_param);
}


PPOCR::PPOCR(AAssetManager *mgr, std::string ocr_det_paramPath, std::string ocr_det_binPath,
             std::string ocr_cls_paramPath, std::string ocr_cls_binPath,
             std::string ocr_rec_paramPath, std::string ocr_rec_binPath) {
    this->use_ocr_cls = false;
    ppocr_det = std::make_unique<PPOCR_Det>(mgr, ocr_det_paramPath, ocr_det_binPath);
    if (use_ocr_cls) {
        ppocr_cls = std::make_unique<PPOCR_Cls>(mgr, ocr_cls_paramPath, ocr_cls_binPath);
    }
    ppocr_rec = std::make_unique<PPOCR_Rec>(mgr, ocr_rec_paramPath, ocr_rec_binPath);
}


PPOCR::~PPOCR() = default;


void PPOCR::det_infer(cv::Mat &img, std::vector<OcrObj> &ocr_objs) {
    ncnn::Mat outputs;
    ppocr_det->infer(img, outputs);
    std::vector<ImgBox> text_regions;
    ppocr_det->postprocess(img, outputs, text_regions);
    for (int i = 0; i < text_regions.size(); i++) {
        OcrObj ocr_obj;
        ocr_obj.textRegion = text_regions[i].img;
        //cv::imwrite("../output/det_result_" + std::to_string(i) + ".jpg", text_regions[i].img);
        ocr_obj.boxVertex = text_regions[i].imgPoint;
        ocr_objs.push_back(ocr_obj);
    }
}


void PPOCR::cls_infer(std::vector<OcrObj> &ocr_objs) {
    for (auto &ocr_obj: ocr_objs) {
        // 根据宽高旋转
        int class_name = 0;
        cv::Mat img = ocr_obj.textRegion.clone();
        if (img.rows > img.cols) {
            cv::rotate(img, img, cv::ROTATE_90_CLOCKWISE);
            class_name = 90;
        }

        ncnn::Mat outputs;
        ppocr_cls->infer(img, outputs);
        ppocr_cls->postprocess(outputs, class_name);
        ocr_obj.angle = class_name;
        // std::cout << "class_name=" << class_name << std::endl;
        // cv::imwrite("./ppocr_cls_result.jpg", ocr_obj.textRegion);

        if (class_name == 90) {
            cv::rotate(ocr_obj.textRegion, ocr_obj.textRegion, cv::ROTATE_90_CLOCKWISE);
            std::vector<cv::Point> boxVertex_;
            boxVertex_.emplace_back(ocr_obj.boxVertex[3]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[0]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[1]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[2]);
            ocr_obj.boxVertex = boxVertex_;
        }
        else if (class_name == 180) {
            cv::rotate(ocr_obj.textRegion, ocr_obj.textRegion, cv::ROTATE_180);
            std::vector<cv::Point> boxVertex_;
            boxVertex_.emplace_back(ocr_obj.boxVertex[2]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[3]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[0]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[1]);
            ocr_obj.boxVertex = boxVertex_;
        }
        else if (class_name == 270) {
            cv::rotate(ocr_obj.textRegion, ocr_obj.textRegion, cv::ROTATE_90_COUNTERCLOCKWISE);
            std::vector<cv::Point> boxVertex_;
            boxVertex_.emplace_back(ocr_obj.boxVertex[1]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[2]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[3]);
            boxVertex_.emplace_back(ocr_obj.boxVertex[0]);
            ocr_obj.boxVertex = boxVertex_;
        }
    }
}


void PPOCR::cls_infer(cv::Mat &img, int &angle) {
    ncnn::Mat outputs;
    ppocr_cls->infer(img, outputs);
    ppocr_cls->postprocess(outputs, angle);
}


void PPOCR::rec_infer(std::vector<OcrObj> &ocr_objs) {
    for (auto &ocr_obj: ocr_objs) {
        ncnn::Mat outputs;
        ppocr_rec->infer(ocr_obj.textRegion, outputs);
        ppocr_rec->postprocess(outputs, ocr_obj.text);
        //std::cout << "text = " << ocr_obj.text << std::endl;
        LOGD("text=%s", ocr_obj.text.c_str());
    }
}


void PPOCR::rec_infer(cv::Mat &img, std::string &text) {
    ncnn::Mat outputs;
    ppocr_rec->infer(img, outputs);
    ppocr_rec->postprocess(outputs, text);
}


void PPOCR::infer(cv::Mat &img, std::vector<OcrObj> &ocr_objs) {
    det_infer(img, ocr_objs);
    if (use_ocr_cls) {
        cls_infer(ocr_objs);
    }
    rec_infer(ocr_objs);
}
