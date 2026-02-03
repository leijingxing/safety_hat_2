// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "yolov8.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cpu.h"

#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "test_debug", __VA_ARGS__);

static float fast_exp(float x)
{
    union {
        uint32_t i;
        float f;
    } v{};
    v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);
    return v.f;
}

static float sigmoid(float x)
{
    return 1.0f / (1.0f + fast_exp(-x));
}
static float intersection_area(const Object& a, const Object& b)
{
    cv::Rect_<float> inter = a.rect & b.rect;
    return inter.area();
}

static void qsort_descent_inplace(std::vector<Object>& faceobjects, int left, int right)
{
    int i = left;
    int j = right;
    float p = faceobjects[(left + right) / 2].prob;

    while (i <= j)
    {
        while (faceobjects[i].prob > p)
            i++;

        while (faceobjects[j].prob < p)
            j--;

        if (i <= j)
        {
            // swap
            std::swap(faceobjects[i], faceobjects[j]);

            i++;
            j--;
        }
    }

    //     #pragma omp parallel sections
    {
        //         #pragma omp section
        {
            if (left < j) qsort_descent_inplace(faceobjects, left, j);
        }
        //         #pragma omp section
        {
            if (i < right) qsort_descent_inplace(faceobjects, i, right);
        }
    }
}

static void qsort_descent_inplace(std::vector<Object>& faceobjects)
{
    if (faceobjects.empty())
        return;

    qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
}

static void nms_sorted_bboxes(const std::vector<Object>& faceobjects, std::vector<int>& picked, float nms_threshold)
{
    picked.clear();

    const int n = faceobjects.size();

    std::vector<float> areas(n);
    for (int i = 0; i < n; i++)
    {
        areas[i] = faceobjects[i].rect.width * faceobjects[i].rect.height;
    }

    for (int i = 0; i < n; i++)
    {
        const Object& a = faceobjects[i];

        int keep = 1;
        for (int j = 0; j < (int)picked.size(); j++)
        {
            const Object& b = faceobjects[picked[j]];

            // intersection over union
            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            // float IoU = inter_area / union_area
            if (inter_area / union_area > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}
static void generate_grids_and_stride(const int target_w, const int target_h, std::vector<int>& strides, std::vector<GridAndStride>& grid_strides)
{
    for (int i = 0; i < (int)strides.size(); i++)
    {
        int stride = strides[i];
        int num_grid_w = target_w / stride;
        int num_grid_h = target_h / stride;
        for (int g1 = 0; g1 < num_grid_h; g1++)
        {
            for (int g0 = 0; g0 < num_grid_w; g0++)
            {
                GridAndStride gs;
                gs.grid0 = g0;
                gs.grid1 = g1;
                gs.stride = stride;
                grid_strides.push_back(gs);
            }
        }
    }
}
static void generate_proposals(std::vector<GridAndStride> grid_strides, const ncnn::Mat& pred, float prob_threshold, std::vector<Object>& objects)
{
    const int num_points = grid_strides.size();
    const int num_class = 6;
    const int reg_max_1 = 16;

    for (int i = 0; i < num_points; i++)
    {
        const float* scores = pred.row(i) + 4 * reg_max_1;

        // find label with max score
        int label = -1;
        float score = -FLT_MAX;
        for (int k = 0; k < num_class; k++)
        {
            float confidence = scores[k];
            if (confidence > score)
            {
                label = k;
                score = confidence;
            }
        }
        float box_prob = sigmoid(score);
        if (box_prob >= prob_threshold)
        {
            ncnn::Mat bbox_pred(reg_max_1, 4, (void*)pred.row(i));
            {
                ncnn::Layer* softmax = ncnn::create_layer("Softmax");

                ncnn::ParamDict pd;
                pd.set(0, 1); // axis
                pd.set(1, 1);
                softmax->load_param(pd);

                ncnn::Option opt;
                opt.num_threads = 1;
                opt.use_packing_layout = false;

                softmax->create_pipeline(opt);

                softmax->forward_inplace(bbox_pred, opt);

                softmax->destroy_pipeline(opt);

                delete softmax;
            }

            float pred_ltrb[4];
            for (int k = 0; k < 4; k++)
            {
                float dis = 0.f;
                const float* dis_after_sm = bbox_pred.row(k);
                for (int l = 0; l < reg_max_1; l++)
                {
                    dis += l * dis_after_sm[l];
                }

                pred_ltrb[k] = dis * grid_strides[i].stride;
            }

            float pb_cx = (grid_strides[i].grid0 + 0.5f) * grid_strides[i].stride;
            float pb_cy = (grid_strides[i].grid1 + 0.5f) * grid_strides[i].stride;

            float x0 = pb_cx - pred_ltrb[0];
            float y0 = pb_cy - pred_ltrb[1];
            float x1 = pb_cx + pred_ltrb[2];
            float y1 = pb_cy + pred_ltrb[3];

            Object obj;
            obj.rect.x = x0;
            obj.rect.y = y0;
            obj.rect.width = x1 - x0;
            obj.rect.height = y1 - y0;
            obj.label = label;
            obj.prob = box_prob;

            objects.push_back(obj);
        }
    }
}

YoloV8::YoloV8()
{
    blob_pool_allocator.set_size_compare_ratio(0.f);
    workspace_pool_allocator.set_size_compare_ratio(0.f);

    sensor_manager = 0;
    sensor_event_queue = 0;
    accelerometer_sensor = 0;
    accelerometer_orientation = 0;
    // sensor
    sensor_manager = ASensorManager_getInstance();
    accelerometer_sensor = ASensorManager_getDefaultSensor(sensor_manager, ASENSOR_TYPE_ACCELEROMETER);
}
YoloV8::~YoloV8(){

    if (accelerometer_sensor)
    {
        ASensorEventQueue_disableSensor(sensor_event_queue, accelerometer_sensor);
        accelerometer_sensor = 0;
    }

    if (sensor_event_queue)
    {
        ASensorManager_destroyEventQueue(sensor_manager, sensor_event_queue);
        sensor_event_queue = 0;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ftLibrary);
}

bool YoloV8::LoadFontFromAssets(const char* fontFileName, FT_Library* ftLibrary, FT_Face* face) {
    AAsset* asset = AAssetManager_open(assetsMgr, fontFileName, AASSET_MODE_STREAMING);
    if (!asset) {
        printf("YoloV8::LoadFontFromAssets asset==null")
        return false;
    }

    off_t assetLength = AAsset_getLength(asset);
    unsigned char* fontBuffer = new unsigned char[assetLength];

    AAsset_read(asset, fontBuffer, assetLength);
    AAsset_close(asset);

    FT_Error error = FT_New_Memory_Face(*ftLibrary, fontBuffer, assetLength, 0, face);
    delete[] fontBuffer;

    if (error) {
        printf("YoloV8::LoadFontFromAssets fontBuffer error")
        return false;
    }

    return true;
}

void drawTextWithCustomFont(cv::Mat& image, const char* text, int x, int y, FT_Face face) {
    FT_Set_Pixel_Sizes(face, 0, 24);

    for (const char* p = text; *p; p++) {
        if (FT_Load_Char(face, *p, FT_LOAD_RENDER)) {
            printf("YoloV8::drawTextWithCustomFont ... Error loading character.");
            continue;
        }

        FT_GlyphSlot g = face->glyph;
        cv::Mat bitmap(g->bitmap.rows, g->bitmap.width, CV_8UC1, g->bitmap.buffer, g->bitmap.pitch);
        cv::Mat bitmap_converted;
        cv::cvtColor(bitmap, bitmap_converted, cv::COLOR_GRAY2BGR);

        cv::Point org(x + g->bitmap_left, y - g->bitmap_top);
        if (org.x < 0 || org.y < 0 || org.x + g->bitmap.width > image.cols || org.y + g->bitmap.rows > image.rows) {
            printf("YoloV8::drawTextWithCustomFont ... Error Text position is out of image bounds..");
            continue;
        }

        bitmap_converted.copyTo(image(cv::Rect(org.x, org.y, g->bitmap.width, g->bitmap.rows)));
        x += (g->advance.x >> 6);
    }
}

void drawTextWithCustomFont2(cv::Mat& image, const char* text, int x, int y, FT_Face face) {
//    FT_Set_Pixel_Sizes(face, 24, 24); // 设置字体大小

    for (const char* p = text; *p; p++) {
        if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot g = face->glyph;

        // 将FreeType的位图转换为cv::Mat
        cv::Mat bitmap(g->bitmap.rows, g->bitmap.width, CV_8UC1, g->bitmap.buffer, g->bitmap.pitch);
        cv::Mat bitmap_converted;
        cv::cvtColor(bitmap, bitmap_converted, cv::COLOR_GRAY2BGR); // 转换为三通道图像

        // 计算绘制的位置
        cv::Point org(x + g->bitmap_left, y - g->bitmap_top);

        // 创建一个和原图同样大小的全黑背景图
        cv::Mat background(image.rows, image.cols, CV_8UC3, cv::Scalar(0, 0, 0));

        // 将文字位图绘制到背景图的指定位置
        bitmap_converted.copyTo(background(cv::Rect(org.x, org.y, g->bitmap.width, g->bitmap.rows)));

        // 将背景图加到原图上
        cv::add(image, background, image);

        // 更新x坐标
        x += (g->advance.x >> 6);
    }
}


int YoloV8::load(AAssetManager* mgr, const char* modeltype, int _target_size, const float* _mean_vals, const float* _norm_vals, bool use_gpu)
{
    assetsMgr = mgr;


    if (FT_Init_FreeType(&ftLibrary)) {
        // Error handling
        printf("YoloV8::load ... Could not init FreeType library.");
    }

    if (!LoadFontFromAssets("SimSun.ttf", &ftLibrary, &face)) {
        // Error handling
        printf("YoloV8::load ... Failed to load font.");
    }


    yolo.clear();
    blob_pool_allocator.clear();
    workspace_pool_allocator.clear();

//    printf("YoloV8::load  ============== 1\n")
    ncnn::Mat in_pad;
    ncnn::set_cpu_powersave(2);
//    ncnn::set_omp_num_threads(2);
    ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());
//    printf("YoloV8::load  ============== 2\n")
//    ncnn::set_cpu_powersave(1);
//    printf("YoloV8::load  ============== 3\n")
//    ncnn::set_omp_num_threads(1);
    yolo.opt = ncnn::Option();

#if NCNN_VULKAN
    yolo.opt.use_vulkan_compute = use_gpu;
#endif

    yolo.opt.num_threads = ncnn::get_big_cpu_count();
    yolo.opt.blob_allocator = &blob_pool_allocator;
    yolo.opt.workspace_allocator = &workspace_pool_allocator;

    char parampath[256];
    char modelpath[256];
//    sprintf(parampath, "yolov8%s.param", modeltype);
//    sprintf(parampath, "yolov8%s.bin", modeltype);

    //    sprintf(parampath, "跌倒安全帽安全绳V0.6_240731_ncn%s.param", modeltype);
//    sprintf(modelpath, "跌倒安全帽安全绳V0.6_240731_ncn%s.bin", modeltype);
    sprintf(parampath, "safe_hat_detect_20250509_ncnn_s_320.param");
    sprintf(modelpath, "safe_hat_detect_20250509_ncnn_s_320.bin");
    printf("222 load param file: %s\n", parampath);
    printf("222 load model file: %s\n", modelpath);
    printf("0000=====");
    int ret3 =yolo.load_param(mgr, parampath);
    if (ret3 != 0) {
        printf("ERROR: Failed to load param file, code=%d\n", ret3);
    } else{
        printf("param ok=====");
    }
    int ret4=yolo.load_model(mgr, modelpath);
    if (ret4 != 0) {
        printf("ERROR: Failed to load model file, code=%d\n", ret4);
    }else{
        printf("model ok=====");
    }
    printf("100000=====");

    target_size = _target_size;
    mean_vals[0] = _mean_vals[0];
    mean_vals[1] = _mean_vals[1];
    mean_vals[2] = _mean_vals[2];
    norm_vals[0] = _norm_vals[0];
    norm_vals[1] = _norm_vals[1];
    norm_vals[2] = _norm_vals[2];

    return 0;
}

int YoloV8::detect(const cv::Mat& rgb, std::vector<Object>& objects, float prob_threshold, float nms_threshold)
{
    int width = rgb.cols;
    int height = rgb.rows;

    // pad to multiple of 32
    int w = width;
    int h = height;
    float scale = 1.f;
    if (w > h)
    {
        scale = (float)target_size / w;
        w = target_size;
        h = h * scale;
    }
    else
    {
        scale = (float)target_size / h;
        h = target_size;
        w = w * scale;
    }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(rgb.data, ncnn::Mat::PIXEL_RGB2BGR, width, height, w, h);

    // pad to target_size rectangle
    int wpad = (w + 31) / 32 * 32 - w;
    int hpad = (h + 31) / 32 * 32 - h;
    ncnn::Mat in_pad;
    ncnn::copy_make_border(in, in_pad, hpad / 2, hpad - hpad / 2, wpad / 2, wpad - wpad / 2, ncnn::BORDER_CONSTANT, 0.f);

    in_pad.substract_mean_normalize(0, norm_vals);

    ncnn::Extractor ex = yolo.create_extractor();
//printf("3333===")
//    ex.input("images", in_pad);
//    std::vector<Object> proposals;
//    ncnn::Mat out;
//    ex.extract("output0", out);

    ex.input("in0", in_pad);
    std::vector<Object> proposals;
    ncnn::Mat out;
    ex.extract("out0", out);

//    int ret = ex.extract("out0", out);
//    if (ret != 0) {
//        printf("ERROR: extract, error code: %d\n", ret);
//    }else{
//        printf("OK: extract");
//    }
    //    printf("4444===")
    std::vector<int> strides = {8, 16, 32}; // might have stride=64
    std::vector<GridAndStride> grid_strides;
    generate_grids_and_stride(in_pad.w, in_pad.h, strides, grid_strides);
    generate_proposals(grid_strides, out, prob_threshold, proposals);

    // sort all proposals by score from highest to lowest
    qsort_descent_inplace(proposals);

    // apply nms with nms_threshold
    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_threshold);

    int count = picked.size();

    objects.resize(count);
    for (int i = 0; i < count; i++)
    {
        objects[i] = proposals[picked[i]];

        // adjust offset to original unpadded
        float x0 = (objects[i].rect.x - (wpad / 2)) / scale;
        float y0 = (objects[i].rect.y - (hpad / 2)) / scale;
        float x1 = (objects[i].rect.x + objects[i].rect.width - (wpad / 2)) / scale;
        float y1 = (objects[i].rect.y + objects[i].rect.height - (hpad / 2)) / scale;

        // clip
        x0 = std::max(std::min(x0, (float)(width - 1)), 0.f);
        y0 = std::max(std::min(y0, (float)(height - 1)), 0.f);
        x1 = std::max(std::min(x1, (float)(width - 1)), 0.f);
        y1 = std::max(std::min(y1, (float)(height - 1)), 0.f);

        objects[i].rect.x = x0;
        objects[i].rect.y = y0;
        objects[i].rect.width = x1 - x0;
        objects[i].rect.height = y1 - y0;
    }

    // sort objects by area
    struct
    {
        bool operator()(const Object& a, const Object& b) const
        {
            return a.rect.area() > b.rect.area();
        }
    } objects_area_greater;
    std::sort(objects.begin(), objects.end(), objects_area_greater);

    return 0;
}

//int YoloV8::draw(cv::Mat& rgb, const std::vector<Object>& objects)
int YoloV8::draw(cv::Mat& rgb, const std::vector<Object>& objects, std::function<void(int)> callback)
{
    static const unsigned char colors[19][3] = {
        { 54,  67, 244},
        { 99,  30, 233},
        {176,  39, 156},
        {183,  58, 103},
        {181,  81,  63},
        {243, 150,  33},
        {244, 169,   3},
        {212, 188,   0},
        {136, 150,   0},
        { 80, 175,  76},
        { 74, 195, 139},
        { 57, 220, 205},
        { 59, 235, 255},
        {  7, 193, 255},
        {  0, 152, 255},
        { 34,  87, 255},
        { 72,  85, 121},
        {158, 158, 158},
        {139, 125,  96}
    };

    int color_index = 0;

    for (size_t i = 0; i < objects.size(); i++)
    {
        const Object& obj = objects[i];

//         fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.label, obj.prob,
//                 obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

        const unsigned char* color = colors[color_index % 19];
        color_index++;

        cv::Scalar cc(color[0], color[1], color[2]);
        cv::rectangle(rgb, obj.rect, cc, 2);

//        { // 扩大范围
//            int width = 1280;
//            int height = 720;
//            int fl= (int) MIN(obj.rect.x, obj.rect.width*0.4);
//            int fr= (int) MIN(width- obj.rect.x - obj.rect.width, obj.rect.width*0.4);
//            int ft= (int) MIN(obj.rect.y, obj.rect.height*0.4);
//            int fb= (int) MIN(height-obj.rect.y- obj.rect.height, obj.rect.height*0.4);
//            cv::Rect_<int> newRect(obj.rect.x-fl, obj.rect.y-ft, obj.rect.width+fl+fr, obj.rect.height+ft+fb);
//            cv::Scalar cc2(100,  100, 100);
//            cv::rectangle(rgb, newRect, cc2, 2);
//        }

        char text[256];
        sprintf(text, "%s %.1f%%", class_names[obj.label], obj.prob * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        int x = obj.rect.x;
        int y = obj.rect.y - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > rgb.cols)
            x = rgb.cols - label_size.width;

        cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)), cc, -1);

        cv::Scalar textcc = (color[0] + color[1] + color[2] >= 381) ? cv::Scalar(0, 0, 0) : cv::Scalar(255, 255, 255);

//        cv::putText(rgb, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5, textcc, 1);
        cv::putText(rgb, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 1.3, textcc, 2);

//        drawTextWithCustomFont(rgb, text, x, y, face);
    }

    return 0;
}
