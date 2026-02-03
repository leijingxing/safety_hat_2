# Safety Hat 2 - Agent 指南

## 概览
Android 应用（Jetpack Compose HUD），包含 CameraX 预览、基于 NCNN 的安全检测，
以及可选的 RTMP 推流。本仓库是从 `safety_hat` 重构而来。

## 关键模块
- `app/`: Compose UI、CameraX、AI 流水线、RTMP 集成。
- `libncnn/`: NCNN 模型 + JNI 包装（安全帽/抽烟/打电话/工装/反光衣）。
- `librtmps/`: RTMP 发送（C/C++ + JNI）。

## 关键流程
- 相机帧（NV21）-> OpenCV RGBA -> `AiRepository.submitFrame` -> NCNN 级联。
- 检测结果 -> `AiEvent` -> HUD 叠加 + 违规列表。
- 推流（可选）：NV21 -> MediaCodec（H.264）-> `librtmps.NativeLib.writeVideo`。

## 已知约束
- GPU 模式默认关闭（稳定性优先）。
- RTMP 使用 H.264 Annex-B NALU 解析。
- 级联检测依赖 `SafeMod` 的人员标签作为下游模型入口。

## 开发备注
- Compose UI 入口：`app/src/main/java/com/lei/safety_hat_2/ui/hud/HudScreen.kt`
- AI 流水线：`app/src/main/java/com/lei/safety_hat_2/data/repository/NcnnCascadeAiRepository.kt`
- RTMP 编码：`app/src/main/java/com/lei/safety_hat_2/data/rtmp/RtmpStreamer.kt`

## 常用任务
- 修改 RTMP URL：`app/src/main/java/com/lei/safety_hat_2/MainActivity.kt`
- 添加模型文档：`app/src/main/assets/MODEL_README.md`
