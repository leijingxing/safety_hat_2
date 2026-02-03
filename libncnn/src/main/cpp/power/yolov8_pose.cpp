/**************************************************************************************************
 * @date  : 2024/9/3 11:30:11
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

#include "yolov8_pose.h"
#include <android/log.h>
#define LOG_TAG "JNI_Example"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)


void YOLOv8Pose::readYaml(const std::string &cfg) {
    // 加载参数
    cv::FileStorage fs(cfg, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        //std::cout << "could not find the parameters config file:" << cfg << std::endl;
        LOGD("could not find the parameters config file");
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


void YOLOv8Pose::loadModel(const char *param, const char *bin) {
    model = new ncnn::Net(); // TODO
    model->opt.use_vulkan_compute = use_vulkan;
    // model->opt.num_threads = 4;
    // model->opt.use_winograd_convolution = true;  // enabled by default
    // model->opt.use_int8_inference = true;  // enabled by default
    model->load_param(param);
    model->load_model(bin);
}

YOLOv8Pose::YOLOv8Pose(const std::string &cfg) {
    YOLOv8Pose::readYaml(cfg);
    YOLOv8Pose::loadModel(paramPath.data(), binPath.data());
}


YOLOv8Pose::YOLOv8Pose(AAssetManager *mgr, std::string param, std::string bin) {
    model = new ncnn::Net(); // TODO
    model->opt.use_vulkan_compute = use_vulkan;
    // model->opt.num_threads = 4;
    // model->opt.use_winograd_convolution = true;  // enabled by default
    // model->opt.use_int8_inference = true;  // enabled by default
    model->load_param(mgr, param.c_str());
    model->load_model(mgr, bin.c_str());
}


YOLOv8Pose::YOLOv8Pose(AAssetManager *mgr, Param &param) {
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


YOLOv8Pose::~YOLOv8Pose() {
    delete model;
}

void pose_padding_resize(cv::Mat &img, const int &model_H, const int &model_W, float &scale_h, float &scale_w,
                    int &padding_top, int &padding_bottom, int &padding_left, int &padding_right) {
    float scale = std::max((float) img.cols / model_W, (float) img.rows / model_H);
    scale_w = scale;
    scale_h = scale;
    int nw = int(img.cols / scale);
    int nh = int(img.rows / scale);
    // cv::resize(img, img, cv::Size(nw, nh), cv::INTER_CUBIC);
    cv::resize(img, img, cv::Size(nw, nh), cv::INTER_LINEAR);
    if (nh != model_H || nw != model_W) {
        padding_top = (int) ((model_H - img.rows) / 2);
        padding_bottom = (int) (model_H - img.rows - padding_top);
        padding_left = (int) ((model_W - img.cols) / 2);
        padding_right = (int) (model_W - img.cols - padding_left);
        cv::copyMakeBorder(img, img, padding_top, padding_bottom, padding_left, padding_right, cv::BORDER_CONSTANT,
                           cv::Scalar(114, 114, 114));
    }
}

void YOLOv8Pose::infer(cv::Mat &original_img, ncnn::Mat &outputs) {
    // preprocess
    // cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    // float scale_h = (float) img.rows / (float) model_height;
    // float scale_w = (float) img.cols / (float) model_width;
    // int padding_top = 0;
    // int padding_bottom = 0;
    // int padding_left = 0;
    // int padding_right = 0;

    cv::Mat img = original_img.clone();
    scale_h = (float) img.rows / (float) model_height;
    scale_w = (float) img.cols / (float) model_width;

    if (img.rows != model_height || img.cols != model_width) {
        pose_padding_resize(img, model_height, model_width, scale_h, scale_w,
                       padding_top, padding_bottom, padding_left, padding_right);
        // cv::Mat resized_img = crnn_narrow_48_pad(img, model_height, model_width);
        // cv::imwrite("./yolo_pose_resized.png", img); // for debug
    }
    ncnn::Mat input = ncnn::Mat::from_pixels(img.data, ncnn::Mat::PIXEL_BGR2RGB, model_width, model_height);

    // 归一化
    input.substract_mean_normalize(mean_vals, norm_vals);

    // std::vector<const char *> input_names = model->input_names();
    // std::vector<const char *> output_names = model->output_names();

    // ncnn前向计算
    ncnn::Extractor extractor = model->create_extractor();

    //设置推理的轻量模式
    extractor.set_light_mode(true);

    // 输入数据
    extractor.input(input_node.c_str(), input); // images是模型的输入节点名称

    // 进行推理
    extractor.extract(output_node.c_str(), outputs);
}

void YOLOv8Pose::postprocess(ncnn::Mat &outputs, std::vector<PoseObj> &results) {
    // std::cout << outputs.w << " " << outputs.h << std::endl;

    int num_channels = outputs.h;
    int num_anchors = outputs.w;

    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    std::vector<std::vector<std::vector<float> > > kpss;

    for (int j = 0; j < num_anchors; j++) {
        std::vector<float> scores;
        for (int i = 4; i < 4 + classes.size(); i++) {
            float score = outputs.row(i)[j];
            scores.emplace_back(score);
        }
        // int class_id = int(argmax(scores.begin(), scores.end()));
        int class_id = [&scores]()-> int { return std::distance(scores.begin(), std::max_element(scores.begin(), scores.end())); }();

        float score = scores[class_id];
        if (score > boxThresh) {
            float cx = outputs.row(0)[j];
            float cy = outputs.row(1)[j];
            float w = outputs.row(2)[j];
            float h = outputs.row(3)[j];
            if (w < 0 || h < 0) {
                continue;
            }

            float x1 = cx - 0.5 * w - padding_left;
            float y1 = cy - 0.5 * h - padding_top;
            x1 = x1 > 0 ? x1 : 0;
            y1 = y1 > 0 ? y1 : 0;
            w = w < model_width - x1 ? w : model_width - x1;
            h = h < model_height - y1 ? h : model_height - y1;

            int x = int(x1 * scale_w);
            int y = int(y1 * scale_h);
            int box_width = int(w * scale_w);
            int box_height = int(h * scale_h);

            // key points
            std::vector<std::vector<float> > kps;
            for (int m = 0; m < numPoint; m++) {
                float kps_x = outputs.row(4 + classes.size() + m * 3 + 0)[j];
                float kps_y = outputs.row(4 + classes.size() + m * 3 + 1)[j];
                float kps_conf = outputs.row(4 + classes.size() + m * 3 + 2)[j];
                kps_x = (kps_x - padding_left) * scale_w;
                kps_y = (kps_y - padding_top) * scale_h;
                kps.push_back({kps_x, kps_y, kps_conf});
            }

            boxes.emplace_back(x, y, box_width, box_height);
            class_ids.emplace_back(class_id);
            confidences.emplace_back(score);
            kpss.emplace_back(kps);
        }
    }

    std::vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, boxThresh, nmsThresh, nms_result);
    for (int i = 0; i < nms_result.size(); ++i) {
        int idx = nms_result[i];

        PoseObj result;
        result.class_id = class_ids[idx];
        result.confidence = confidences[idx];

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 255);
        result.color = cv::Scalar(dis(gen), dis(gen), dis(gen));

        result.className = classes[result.class_id];
        result.box = boxes[idx];

        result.kps_indexs.push_back(idx);
        for (int j = 0; j < numPoint; j++) {
            if (kpss.at(idx).at(j).at(2) > kps_thres) {
                result.kps.push_back(kpss.at(idx).at(j));
            }
        }

        results.emplace_back(result);
    }
}

void YOLOv8Pose::draw_results(cv::Mat &img, std::vector<PoseObj> &detect_results) {
    for (const auto &detection: detect_results) {
        cv::Rect box = detection.box;
        cv::Scalar color = detection.color;
        std::vector<std::vector<float> > kps = detection.kps;

        // draw obj box
        cv::rectangle(img, box.tl(), box.br(), color, 2);

        // draw text background
        std::string classString = detection.className + ' ' + std::to_string(detection.confidence).substr(0, 4);
        double font_scale = 0.5;
        cv::Size textSize = cv::getTextSize(classString, cv::FONT_HERSHEY_DUPLEX, font_scale, 2, nullptr);
        cv::Rect textBox(box.x, box.y - textSize.height, textSize.width, textSize.height);
        cv::rectangle(img, textBox, color, cv::FILLED);

        cv::putText(img, classString, cv::Point(box.x, box.y),
                    cv::FONT_HERSHEY_DUPLEX, font_scale, cv::Scalar(0, 0, 0));

        for (int i = 0; i < kps.size(); i++) {
            auto center = cv::Point(kps.at(i).at(0), kps.at(i).at(1));
            cv::circle(img, center, 5, cv::Scalar(0, 255, 255), -1);
        }
    }
}
