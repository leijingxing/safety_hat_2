/**************************************************************************************************
 * @date  : 2024/9/14 15:41:16
 * @author: tangmin
 * @note  : 
**************************************************************************************************/


#ifndef PPOCR_HPP
#define PPOCR_HPP

#include "ppocr_det.h"
#include "ppocr_cls.h"
#include "ppocr_rec.h"

struct OcrObj {
    cv::Mat textRegion;   // 文字区域截图
    std::vector<cv::Point> boxVertex;  // 文字区域坐标
    int angle{0};  // 文字区域截图角度
    std::string text;  //文字区域识别结果
};


class PPOCR {
public:
    struct Param {
        PPOCR_Det::Param ppocr_det_param;
        PPOCR_Cls::Param ppocr_cls_param;
        PPOCR_Rec::Param ppocr_rec_param;

        //std::string ppocr_det_cfg;
        //std::string ppocr_cls_cfg;
        //std::string ppocr_rec_cfg;
        bool use_ocr_cls{true};

        // /**
        //  * 以下几种场景会触发拷贝构造函数:
        //  * 1.通过已有对象初始化新对象
        //  * 2.对象按值传递给函数（即不是传引用）
        //  * 3.函数按值返回对象
        //  */
        // Param(const Param &param) {
        //     this->ppocr_det_cfg = param.ppocr_det_cfg;
        //     this->ppocr_cls_cfg = param.ppocr_cls_cfg;
        //     this->ppocr_rec_cfg = param.ppocr_rec_cfg;
        //     this->use_ocr_cls = param.use_ocr_cls;
        // }
    };

    PPOCR(AAssetManager *mgr, Param &param);

    PPOCR(AAssetManager *mgr,
          std::string ocr_det_paramPath, std::string ocr_det_binPath,
          std::string ocr_cls_paramPath, std::string ocr_cls_binPath,
          std::string ocr_rec_paramPath, std::string ocr_rec_binPath);

    ~PPOCR();

    /**
    * 文字区域检测
    * @param model 文字区域检测模型
    * @param img 图片
    * @param ocr_results  存储结果
    * @return
    */
    void det_infer(cv::Mat &img, std::vector<OcrObj> &ocr_objs);

    /**
    * 文字方向分类
    * @param model 文字方向分类模型
    * @param result 存储结果
    * @return
    */
    void cls_infer(std::vector<OcrObj> &ocr_objs);

    /**
     * 文字方向分类
     * @param img 输入需要分辨方向的图片
     * @param angle 返回图片的方向
     */
    void cls_infer(cv::Mat &img, int &angle);

    /**
    * 文字识别
    * @param model 文字识别模型
    * @param result 存储结果
    * @return
    */
    void rec_infer(std::vector<OcrObj> &ocr_objs);

    /**
     * 文字识别
     * @param img 输入需要识别文字的图片
     * @param text 返回图片的文字
     */
    void rec_infer(cv::Mat &img, std::string &text);

    /**
    *
    * @param img 源图片
    * @param ocr_objs string的引用。
    */
    void infer(cv::Mat &img, std::vector<OcrObj> &ocr_objs);

private:
    bool use_ocr_cls{true};
    std::unique_ptr<PPOCR_Det> ppocr_det{nullptr};
    std::unique_ptr<PPOCR_Cls> ppocr_cls{nullptr};
    std::unique_ptr<PPOCR_Rec> ppocr_rec{nullptr};
};


#endif //PPOCR_HPP
