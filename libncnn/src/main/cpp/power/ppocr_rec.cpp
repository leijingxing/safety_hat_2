/**************************************************************************************************
 * @date  : 2024/9/3 10:55:54
 * @author: tangmin
 * @note  : 
**************************************************************************************************/

# include "ppocr_rec.h"


//这个函数是官方提供的用于打印输出的tensor
void PPOCR_Rec::pretty_print(const ncnn::Mat &m) {
    for (int q = 0; q < m.c; q++) {
        const float *ptr = m.channel(q);
        for (int y = 0; y < m.h; y++) {
            for (int x = 0; x < m.w; x++) {
                printf("%f ", ptr[x]);
            }
            ptr += m.w;
            printf("\n");
        }
        printf("------------------------\n");
    }
}

void rec_padding_resize(cv::Mat &img, const int &model_H, const int &model_W, float &scale_h, float &scale_w,
                       int &padding_top, int &padding_bottom, int &padding_left, int &padding_right) {
    float scale = std::max((float) img.cols / model_W, (float) img.rows / model_H);
    scale_w = scale;
    scale_h = scale;
    int nw = int(img.cols / scale);
    int nh = int(img.rows / scale);
    // cv::resize(img, img, cv::Size(nw, nh), cv::INTER_CUBIC);
    cv::resize(img, img, cv::Size(nw, nh), cv::INTER_LINEAR);

    padding_top = (int) ((model_H - img.rows) / 2);
    padding_bottom = (int) (model_H - img.rows - padding_top);
    // padding_left = (int) ((model_W - img.cols) / 2);
    padding_left = 0;
    padding_right = (int) (model_W - img.cols - padding_left);
    cv::copyMakeBorder(img, img, padding_top, padding_bottom, padding_left, padding_right, cv::BORDER_CONSTANT,
                       cv::Scalar(114, 114, 114));
}


void PPOCR_Rec::readYaml(const std::string &cfg) {
    // 加载参数
    cv::FileStorage fs(cfg, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cout << "could not find the parameters config file:" << cfg << std::endl;
        LOGE("ocr rec could not find the parameters config file");
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

void PPOCR_Rec::loadModel(const char *param, const char *bin) {
    model = new ncnn::Net();
    model->opt.use_vulkan_compute = use_vulkan;
    // model->opt.num_threads = 4;
    // model->opt.use_winograd_convolution = true;  // enabled by default
    // model->opt.use_int8_inference = true;  // enabled by default
    model->load_param(param);
    model->load_model(bin);
}

PPOCR_Rec::PPOCR_Rec(const std::string &cfg) {
    PPOCR_Rec::readYaml(cfg);
    PPOCR_Rec::loadModel(paramPath.data(), binPath.data());
}


PPOCR_Rec::PPOCR_Rec(AAssetManager *mgr, std::string param, std::string bin) {
    model = new ncnn::Net(); // TODO
    model->opt.use_vulkan_compute = use_vulkan;
    // model->opt.num_threads = 4;
    // model->opt.use_winograd_convolution = true;  // enabled by default
    // model->opt.use_int8_inference = true;  // enabled by default
    model->load_param(mgr, param.c_str());
    model->load_model(mgr, bin.c_str());
}


PPOCR_Rec::PPOCR_Rec(AAssetManager *mgr,
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


PPOCR_Rec::PPOCR_Rec(AAssetManager *mgr, Param &param) {
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


PPOCR_Rec::~PPOCR_Rec() {
    delete model;
}


template<class ForwardIterator>
inline static size_t argmax(ForwardIterator first, ForwardIterator last) {
    return std::distance(first, std::max_element(first, last));
}

/**
 * 
 * @param outputs 
 * @param text 
 */
void PPOCR_Rec::postprocess(ncnn::Mat &outputs, std::string &text) {
    // std::cout << "outputs.h = " << outputs.h << std::endl;
    // std::cout << "outputs.w = " << outputs.w << std::endl;
    std::vector<std::string> text_vec;
    int class_id = 0;
    for (int i = 0; i < outputs.h; i++) {
        std::vector<float> scores;
        for (int j = 0; j < outputs.w; j++) {
            float score = outputs.row(i)[j];
            scores.emplace_back(score);
            // std::cout << score << " ";
        }
        // std::cout << std::endl;
        // std::cout << "scores size = " << scores.size() << std::endl;
        int class_id_t = int(argmax(scores.begin(), scores.end()));
        if (class_id_t == 0) {
            class_id = class_id_t;
            continue;
        }

        if (class_id != class_id_t) {
            text_vec.push_back(classes[class_id_t]);
        }
        class_id = class_id_t;
    }
    for (const auto &str: text_vec) {
        text += str;
    }
}

void PPOCR_Rec::infer(cv::Mat &img, ncnn::Mat &outputs) {
    scale_h = (float) img.rows / (float) model_height;
    scale_w = (float) img.cols / (float) model_width;

    if (img.rows != model_height || img.cols != model_width) {
        rec_padding_resize(img, model_height, model_width, scale_h, scale_w,
                          padding_top, padding_bottom, padding_left, padding_right);
        //cv::imwrite("./pp_rec_resize.png", img); // for debug
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
