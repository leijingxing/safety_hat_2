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

#ifndef YOLOSMOKE_H
#define YOLOSMOKE_H

#include <iostream>
#include <opencv2/core/core.hpp>
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "yolo_smoke", __VA_ARGS__)
#include <net.h>

struct Object
{
    cv::Rect_<float> rect;
    int label;
    float prob;
    std::string classes;
};
struct GridAndStride
{
    int grid0;
    int grid1;
    int stride;
};
class YoloSmoke
{
public:
//  抽烟
    static constexpr const char* class_names[] = {
            "maybe_smoking",
            "normal",
            "smoking",
            "unknow",
    }; // 直接初始化
    struct Object
    {
        cv::Rect_<float> rect;
        int label;
        float prob;
        std::string classes;
    };
    struct GridAndStride
    {
        int grid0;
        int grid1;
        int stride;
    };
    YoloSmoke();

    int load(AAssetManager* mgr, const char* modeltype, int target_size, const float* mean_vals, const float* norm_vals, bool use_gpu = true);

    int detect(const cv::Mat& rgb, std::vector<Object>& objects, float prob_threshold = 0.25, float nms_threshold = 0.4);

    int draw(cv::Mat& rgb, const std::vector<Object>& objects);

private:
    ncnn::Net yolo;
    int target_size;
    float mean_vals[3];
    float norm_vals[3];
    ncnn::UnlockedPoolAllocator blob_pool_allocator;
    ncnn::PoolAllocator workspace_pool_allocator;
};

#endif // NANODET_H
