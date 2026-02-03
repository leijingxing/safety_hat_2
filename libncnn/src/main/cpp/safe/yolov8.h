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

#ifndef YOLO_H
#define YOLO_H

#include <opencv2/core/core.hpp>
#include <android/sensor.h>

#include "net.h"
#include "ft2build.h"
#include FT_FREETYPE_H"freetype/freetype.h"

struct Object
{
    cv::Rect_<float> rect;
    int label;
    float prob;
};
struct GridAndStride
{
    int grid0;
    int grid1;
    int stride;
};
static const char* class_names[] = {
        "personup", // 站立 personup
        "persondown", // 跌倒 persondown
        "helmat",     // 有安全帽 helmat
        "nohelmat",   // 没有安全帽 nohelmat
        "landyard",   // 有安全绳 landyard
        "nolandyard", // 没有安全绳 nolandyard
};
class YoloV8
{
public:
    YoloV8();
    ~YoloV8();
    bool LoadFontFromAssets(const char* fontFileName, FT_Library* ftLibrary, FT_Face* face);

    int load(const char* modeltype, int target_size, const float* mean_vals, const float* norm_vals, bool use_gpu = true);

    int load(AAssetManager* mgr, const char* modeltype, int target_size, const float* mean_vals, const float* norm_vals, bool use_gpu = true);

    int detect(const cv::Mat& rgb, std::vector<Object>& objects, float prob_threshold = 0.5, float nms_threshold = 0.4);

//    int draw(cv::Mat& rgb, const std::vector<Object>& objects);
    int draw(cv::Mat& rgb, const std::vector<Object>& objects, std::function<void(int)> callback);

    bool outImageFlag = false;
    jobject byteBuffer = nullptr;

    ASensorManager* sensor_manager;
    mutable ASensorEventQueue* sensor_event_queue;
    const ASensor* accelerometer_sensor;
    mutable int accelerometer_orientation;
private:
    ncnn::Net yolo;
    int target_size;
    float mean_vals[3];
    float norm_vals[3];
    ncnn::UnlockedPoolAllocator blob_pool_allocator;
    ncnn::PoolAllocator workspace_pool_allocator;
    AAssetManager* assetsMgr = nullptr;

    FT_Library ftLibrary;
    FT_Face face;
};

#endif // NANODET_H
