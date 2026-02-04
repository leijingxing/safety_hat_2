# Safety Hat 2

## 项目简介
Safety Hat 2 是一个基于 Android 的工业安全 HUD 应用，集成 CameraX 预览、
NCNN 模型推理与可选 RTMP 推流，面向手机与边缘设备场景。

## 功能概览
- 相机预览 + HUD 叠加
- 安全帽/抽烟/打电话/工装/反光衣等检测
- 违规列表（最多 8 条）与提示高亮
- 可选 RTMP 推流

## 目录结构
- `app/`：UI、业务逻辑、CameraX、推理与推流
- `libncnn/`：NCNN 模型与 JNI 封装
- `librtmps/`：RTMP 推流 C/C++ 实现

## 关键入口
- UI 入口：`app/src/main/java/com/lei/safety_hat_2/ui/hud/HudScreen.kt`
- 推理流程：`app/src/main/java/com/lei/safety_hat_2/data/repository/NcnnCascadeAiRepository.kt`
- 推流编码：`app/src/main/java/com/lei/safety_hat_2/data/rtmp/RtmpStreamer.kt`

## 推流配置
修改 RTMP 地址与流 ID：
- `app/src/main/java/com/lei/safety_hat_2/MainActivity.kt`

## 模型说明
- 见 `app/src/main/assets/MODEL_README.md`

