/**************************************************************************************************
 * @date  : 2024/9/2 18:01:47
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "ppocr_det.h"
// #include "clipper.h"

void PPOCR_Det::readYaml(const std::string &cfg) {
    // 加载参数
    cv::FileStorage fs(cfg, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        //std::cout << "could not find the parameters config file:" << cfg << std::endl;
        //std::cout << "could not find the parameters config file:" << cfg << std::endl;
        LOGE("could not find the parameters config file");
    }
    // 通过操作符>>重载实现读出
    fs["model"]["paramPath"] >> paramPath;
    fs["model"]["binPath"] >> binPath;
    fs["model"]["input_node"] >> input_node;
    fs["model"]["output_node"] >> output_node;
    // fs["model"]["model_height"] >> model_height;
    // fs["model"]["model_width"] >> model_width;
    // fs["model"]["use_vulkan"] >> use_vulkan;
    model_height = static_cast<int>(fs["model"]["model_height"]);
    model_width = static_cast<int>(fs["model"]["model_width"]);
    use_vulkan = static_cast<int>(fs["model"]["use_vulkan"]);
    for (int i = 0; i < fs["names"].size(); i++) {
        std::string name;
        // 使用整数 class_id 作为键，需要构造键名称
        // std::string class_key = cv::format("class_%d", i);
        fs["names"][i] >> name;
        classes.emplace_back(name);
    }

    fs.release();
}

void PPOCR_Det::loadModel(const char *param, const char *bin) {
    model = new ncnn::Net();
    model->opt.use_vulkan_compute = use_vulkan;
    // model->opt.num_threads = 4;
    // model->opt.use_winograd_convolution = true;  // enabled by default
    // model->opt.use_int8_inference = true;  // enabled by default
    model->load_param(param);
    model->load_model(bin);
}

PPOCR_Det::PPOCR_Det(const std::string &cfg) {
    PPOCR_Det::readYaml(cfg);
    PPOCR_Det::loadModel(paramPath.data(), binPath.data());
}


PPOCR_Det::PPOCR_Det(AAssetManager *mgr, std::string param, std::string bin) {
    model = new ncnn::Net(); // TODO
    model->opt.use_vulkan_compute = use_vulkan;
    // model->opt.num_threads = 4;
    // model->opt.use_winograd_convolution = true;  // enabled by default
    // model->opt.use_int8_inference = true;  // enabled by default
    model->load_param(mgr, param.c_str());
    model->load_model(mgr, bin.c_str());
}


PPOCR_Det::PPOCR_Det(AAssetManager *mgr,
                     std::string paramPath, std::string binPath,
                     std::string inputNode, std::string outputNode,
                     int modelWidth, int modelHeight,
                     bool useVulkan,
                     std::vector<std::string> classes)
        : paramPath(paramPath), binPath(binPath),
          input_node(inputNode), output_node(outputNode),
          model_height(modelHeight), model_width(modelWidth),
          use_vulkan(useVulkan), classes(classes) {

    model = new ncnn::Net(); // TODO
    model->opt.use_vulkan_compute = use_vulkan;
    model->load_param(mgr, paramPath.c_str());
    model->load_model(mgr, binPath.c_str());
}


PPOCR_Det::PPOCR_Det(AAssetManager *mgr, Param &param) {
    this->paramPath = param.paramPath;
    this->binPath = param.binPath;
    this->model_height = param.model_height;
    this->model_width = param.model_width;
    this->input_node = param.input_node;
    this->output_node = param.output_node;
    this->use_vulkan = param.use_vulkan;
    this->classes = param.classes;

    model = new ncnn::Net(); // TODO
    model->opt.use_vulkan_compute = use_vulkan;
    model->load_param(mgr, paramPath.c_str());
    model->load_model(mgr, binPath.c_str());
}


PPOCR_Det::~PPOCR_Det() {
    delete model;
}


void getImage(cv::Mat &src, std::vector<TextBox> &box, std::vector<ImgBox> &text_region) {
    cv::Mat src_copy = src.clone();

    for (int i = 0; i < box.size(); i++) {
        // int x[4] = {box[i].boxPoint[0].x, box[i].boxPoint[1].x, box[i].boxPoint[2].x, box[i].boxPoint[3].x};
        // int y[4] = {box[i].boxPoint[0].y, box[i].boxPoint[1].y, box[i].boxPoint[2].y, box[i].boxPoint[3].y};
        //
        // int min_x = (*std::min_element(x, x + 4));
        // int min_y = (*std::min_element(y, y + 4));
        // int max_x = (*std::max_element(x, x + 4));
        // int max_y = (*std::max_element(y, y + 4));

        // 透射变换目标框的宽高
        int img_w = int(sqrt(pow(box[i].boxPoint[0].x - box[i].boxPoint[1].x, 2) +
                             pow(box[i].boxPoint[0].y - box[i].boxPoint[1].y, 2)));
        int img_h = int(sqrt(pow(box[i].boxPoint[0].x - box[i].boxPoint[3].x, 2) +
                             pow(box[i].boxPoint[0].y - box[i].boxPoint[3].y, 2)));

        // int tw1 = int(sqrt(pow(box[i].boxPoint[0].x - box[i].boxPoint[1].x, 2) +
        //                    pow(box[i].boxPoint[0].y - box[i].boxPoint[1].y, 2)));
        // int tw2 = int(sqrt(pow(box[i].boxPoint[2].x - box[i].boxPoint[3].x, 2) +
        //                    pow(box[i].boxPoint[2].y - box[i].boxPoint[3].y, 2)));
        // int th1 = int(sqrt(pow(box[i].boxPoint[0].x - box[i].boxPoint[3].x, 2) +
        //                    pow(box[i].boxPoint[0].y - box[i].boxPoint[3].y, 2)));
        // int th2 = int(sqrt(pow(box[i].boxPoint[1].x - box[i].boxPoint[2].x, 2) +
        //                    pow(box[i].boxPoint[1].y - box[i].boxPoint[2].y, 2)));
        // int img_w = tw1;
        // int img_h = th1;

        // 预测框的4个顶点坐标。（顺时针）
        cv::Point2f src_points[] = {
                // +2、-2因为手动放大框
                // cv::Point2f(box[i].boxPoint[0].x - 2, box[i].boxPoint[0].y - 2),
                // cv::Point2f(box[i].boxPoint[1].x + 2, box[i].boxPoint[1].y - 2),
                // cv::Point2f(box[i].boxPoint[2].x + 2, box[i].boxPoint[2].y + 2),
                // cv::Point2f(box[i].boxPoint[3].x - 2, box[i].boxPoint[3].y + 2),
                cv::Point2f(box[i].boxPoint[0].x, box[i].boxPoint[0].y),
                cv::Point2f(box[i].boxPoint[1].x, box[i].boxPoint[1].y),
                cv::Point2f(box[i].boxPoint[2].x, box[i].boxPoint[2].y),
                cv::Point2f(box[i].boxPoint[3].x, box[i].boxPoint[3].y),
        };

        // 透射变换目标框的4个顶点坐标
        cv::Point2f dst_points[] = {
                cv::Point2f(0, 0),
                cv::Point2f(img_w, 0),
                cv::Point2f(img_w, img_h),
                cv::Point2f(0, img_h),
        };
        cv::Mat rot_img, img_warp;
        rot_img = cv::getPerspectiveTransform(src_points, dst_points);
        cv::warpPerspective(src_copy, img_warp, rot_img, cv::Size(img_w, img_h), cv::INTER_LINEAR);
        // cv::imwrite("./img_warp2.png", img_warp);
        text_region.emplace_back(ImgBox{img_warp, box[i].boxPoint});

        // cv::line(src, cv::Point(box[i].boxPoint[0].x, box[i].boxPoint[0].y), cv::Point(box[i].boxPoint[1].x, box[i].boxPoint[1].y), cv::Scalar(0, 0, 255), 1);
        // cv::line(src, cv::Point(box[i].boxPoint[1].x, box[i].boxPoint[1].y), cv::Point(box[i].boxPoint[2].x, box[i].boxPoint[2].y), cv::Scalar(0, 0, 255), 1);
        // cv::line(src, cv::Point(box[i].boxPoint[2].x, box[i].boxPoint[2].y), cv::Point(box[i].boxPoint[3].x, box[i].boxPoint[3].y), cv::Scalar(0, 0, 255), 1);
        // cv::line(src, cv::Point(box[i].boxPoint[3].x, box[i].boxPoint[3].y), cv::Point(box[i].boxPoint[0].x, box[i].boxPoint[0].y), cv::Scalar(0, 0, 255), 1);
        // cv::imwrite("./src.png", src);
    }
}


bool cvPointCompare(const cv::Point &a, const cv::Point &b) {
    return a.x < b.x;
}

std::vector<cv::Point> getMinBoxes(const std::vector<cv::Point> &inVec, float &minSideLen, float &allEdgeSize) {
    std::vector<cv::Point> minBoxVec;
    cv::RotatedRect textRect = cv::minAreaRect(inVec);
    cv::Mat boxPoints2f;
    cv::boxPoints(textRect, boxPoints2f);

    float *p1 = (float *) boxPoints2f.data;
    std::vector<cv::Point> tmpVec;
    for (int i = 0; i < 4; ++i, p1 += 2) {
        tmpVec.emplace_back(int(p1[0]), int(p1[1]));
    }

    std::sort(tmpVec.begin(), tmpVec.end(), cvPointCompare);

    int index1, index2, index3, index4;
    if (tmpVec[1].y > tmpVec[0].y) {
        index1 = 0;
        index4 = 1;
    } else {
        index1 = 1;
        index4 = 0;
    }

    if (tmpVec[3].y > tmpVec[2].y) {
        index2 = 2;
        index3 = 3;
    } else {
        index2 = 3;
        index3 = 2;
    }
    minBoxVec.push_back(tmpVec[index1]);
    minBoxVec.push_back(tmpVec[index2]);
    minBoxVec.push_back(tmpVec[index3]);
    minBoxVec.push_back(tmpVec[index4]);

    minSideLen = (std::min)(textRect.size.width, textRect.size.height);
    allEdgeSize = 2.f * (textRect.size.width + textRect.size.height);
    // allEdgeSize = textRect.size.width * textRect.size.height;  // 形状不变。

    return minBoxVec;
}

float boxScoreFast(const cv::Mat &inMat, const std::vector<cv::Point> &inBox) {
    std::vector<cv::Point> box = inBox;
    int width = inMat.cols;
    int height = inMat.rows;
    int maxX = -1, minX = 1000000, maxY = -1, minY = 1000000;
    for (int i = 0; i < box.size(); ++i) {
        if (maxX < box[i].x)
            maxX = box[i].x;
        if (minX > box[i].x)
            minX = box[i].x;
        if (maxY < box[i].y)
            maxY = box[i].y;
        if (minY > box[i].y)
            minY = box[i].y;
    }
    maxX = (std::min)((std::max)(maxX, 0), width - 1);
    minX = (std::max)((std::min)(minX, width - 1), 0);
    maxY = (std::min)((std::max)(maxY, 0), height - 1);
    minY = (std::max)((std::min)(minY, height - 1), 0);

    for (int i = 0; i < box.size(); ++i) {
        box[i].x = box[i].x - minX;
        box[i].y = box[i].y - minY;
    }

    std::vector<std::vector<cv::Point> > maskBox;
    maskBox.push_back(box);
    cv::Mat maskMat(maxY - minY + 1, maxX - minX + 1, CV_8UC1, cv::Scalar(0, 0, 0));
    cv::fillPoly(maskMat, maskBox, cv::Scalar(1, 1, 1), 1);

    return cv::mean(inMat(cv::Rect(cv::Point(minX, minY), cv::Point(maxX + 1, maxY + 1))).clone(),
                    maskMat).val[0];
}

// use clipper
std::vector<cv::Point> unClip(const std::vector<cv::Point> &inBox, float perimeter, float unClipRatio) {
    std::vector<cv::Point> outBox;
    ClipperLib::Path poly;

    // 将cv::Point的坐标添加到ClipperLib::Path对象中
    for (int i = 0; i < inBox.size(); ++i) {
        poly.push_back(ClipperLib::IntPoint(inBox[i].x, inBox[i].y));
    }

    double distance = unClipRatio * ClipperLib::Area(poly) / (double) perimeter;

    ClipperLib::ClipperOffset clipperOffset;
    clipperOffset.AddPath(poly, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
    ClipperLib::Paths polys;
    polys.push_back(poly);
    clipperOffset.Execute(polys, distance);

    std::vector<cv::Point> rsVec;
    for (int i = 0; i < polys.size(); ++i) {
        ClipperLib::Path tmpPoly = polys[i];
        for (int j = 0; j < tmpPoly.size(); ++j) {
            outBox.emplace_back(tmpPoly[j].X, tmpPoly[j].Y);
        }
    }
    return outBox;
}


std::vector<TextBox> findRsBoxes(const cv::Mat &src, const cv::Mat &fMapMat, const cv::Mat &norfMapMat,
                                 const float boxScoreThresh, const float unClipRatio, float s) {
    float minArea = 3;
    std::vector<TextBox> rsBoxes;
    // rsBoxes.clear();
    std::vector<std::vector<cv::Point> > contours;
    findContours(norfMapMat, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    for (int i = 0; i < contours.size(); ++i) {
        float minSideLen, perimeter;
        std::vector<cv::Point> minBox = getMinBoxes(contours[i], minSideLen, perimeter);
        if (minSideLen < minArea)
            continue;
        float score = boxScoreFast(fMapMat, contours[i]);
        if (score < boxScoreThresh)
            continue;

        //---use clipper start---
        std::vector<cv::Point> clipBox = unClip(minBox, perimeter, unClipRatio);
        std::vector<cv::Point> clipMinBox = getMinBoxes(clipBox, minSideLen, perimeter);
        //---use clipper end---

        if (minSideLen < minArea + 2)
            continue;

        //循环每个预测框的n个坐标点。除以s:将坐标还原到缩放前的原图上。
        for (int j = 0; j < clipMinBox.size(); ++j) {
            clipMinBox[j].x = (clipMinBox[j].x / s);
            clipMinBox[j].x = (std::min)((std::max)(clipMinBox[j].x, 0), src.cols);

            clipMinBox[j].y = (clipMinBox[j].y / s);
            clipMinBox[j].y = (std::min)((std::max)(clipMinBox[j].y, 0), src.rows);
        }

        rsBoxes.emplace_back(TextBox{clipMinBox, score});
    }
    reverse(rsBoxes.begin(), rsBoxes.end());
    return rsBoxes;
}


void PPOCR_Det::postprocess(cv::Mat img, ncnn::Mat &outputs, std::vector<ImgBox> &text_region)
/**
 * The post process for Differentiable Binarization (DB).
 * @param outputs
 * @param text
 */
{
    // for (int i = 0; i < outputs.h; i++)
    // {
    //     // std::vector<float> scores;
    //     for (int j = 0; j < outputs.w; j++)
    //     {
    //         float score = outputs.row(i)[j];
    //         score = score > 0.5 ? 255. : 0.;
    //         // std::cout << score << " ";
    //         outputs.row(i)[j] = score;
    //     }
    //     // std::cout << std::endl;
    // }

    cv::Mat fMapMat;
    fMapMat.create(outputs.h, outputs.w, CV_32FC1);
    fMapMat.data = outputs;
    // cv::imwrite("./mask.jpg", fMapMat);


    // std::vector<std::vector<cv::Point> > contours;
    // cv::findContours(mask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat norfMapMat;
    // norfMapMat = fMapMat > boxThresh;
    cv::compare(fMapMat, boxThresh, norfMapMat, cv::CMP_GT);
    // cv::imwrite("./norfMapMat.jpg", norfMapMat);

    std::vector<TextBox> box;
    box = findRsBoxes(img, fMapMat, norfMapMat, boxScoreThresh, unClipRatio, 1.0 / scale_h);
    // box = findRsBoxes(img, fMapMat, norfMapMat, boxScoreThresh, unClipRatio, 1.0);

    getImage(img, box, text_region);
}

void det_padding_resize(cv::Mat &img, const int &model_H, const int &model_W, float &scale_h, float &scale_w,
                       int &padding_top, int &padding_bottom, int &padding_left, int &padding_right) {
    float scale = std::max((float) img.cols / model_W, (float) img.rows / model_H);
    scale_w = scale;
    scale_h = scale;
    int nw = int(img.cols / scale);
    int nh = int(img.rows / scale);
    // cv::resize(img, img, cv::Size(nw, nh), cv::INTER_CUBIC);
    cv::resize(img, img, cv::Size(nw, nh), cv::INTER_LINEAR);

    // padding_top = (int) ((model_H - img.rows) / 2);
    padding_top = 0;
    padding_bottom = (int) (model_H - img.rows - padding_top);
    // padding_left = (int) ((model_W - img.cols) / 2);
    padding_left = 0;
    padding_right = (int) (model_W - img.cols - padding_left);
    cv::copyMakeBorder(img, img, padding_top, padding_bottom, padding_left, padding_right, cv::BORDER_CONSTANT,
            // cv::Scalar(114, 114, 114),
                       cv::Scalar(0, 0, 0)
    );
}

void PPOCR_Det::infer(cv::Mat &original_img, ncnn::Mat &outputs) {
    cv::Mat img = original_img.clone();
    // cv::Mat img = original_img;
    scale_h = (float) img.rows / (float) model_height;
    scale_w = (float) img.cols / (float) model_width;

    if (img.rows != model_height || img.cols != model_width) {
        det_padding_resize(img, model_height, model_width, scale_h, scale_w,
                          padding_top, padding_bottom, padding_left, padding_right);
        // cv::imwrite("./pp_det_result.png", img);  // for debug
    }
    ncnn::Mat input = ncnn::Mat::from_pixels(img.data, ncnn::Mat::PIXEL_BGR, model_width, model_height);

    // 归一化
    input.substract_mean_normalize(mean_vals, norm_vals);

    // ncnn前向计算
    ncnn::Extractor extractor = model->create_extractor();

    //设置推理的轻量模式
    extractor.set_light_mode(true);

    // 输入数据
    extractor.input(input_node.c_str(), input); // images是模型的输入节点名称

    // 进行推理
    extractor.extract(output_node.c_str(), outputs);
}
