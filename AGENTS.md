# Safety Hat 2 - Agent 协作指南

## 1. 项目定位
Android 工业安全 HUD 应用，核心能力包含：
- 实时相机预览与检测框叠加（Compose）。
- 基于 NCNN 的级联违规检测。
- 可选 RTMP 实时推流。

## 2. 模块边界
- `app/`
  - 负责 UI、ViewModel、Camera 帧接入、AI 事件消费、推流控制。
- `libncnn/`
  - 负责模型与 JNI 绑定，输出检测标签和候选框。
- `librtmps/`
  - 负责 RTMP 链路与码流写入（Native 层）。
- `libuvccamera/`
  - 负责 UVC 相机能力支持。

## 3. 核心链路（必须保持）
1. 相机帧进入应用层（NV21/RGBA）。
2. `SafeMod` 先做人员/安全帽检测。
3. 基于人员区域裁剪后触发抽烟/打电话/工装/反光衣模型。
4. 统一产出 `AiEvent`，驱动 HUD 及违规统计。
5. 推流开启时，NV21 经 MediaCodec 编码后进入 `librtmps.NativeLib.writeVideo`。

## 4. 修改建议
- 优先在 `app` 层做业务编排，不要随意改 Native 接口签名。
- 检测模型标签新增/变更时，务必同步：
  - `NcnnCascadeAiRepository.mapLabel`
  - `HudViewModel.extractViolations`
- 推流协议或编码策略变更时，保持 `RtmpRepository -> RtmpStreamer -> NativeLib` 接口语义一致。

## 5. 代码风格
- 只添加“解释意图”的注释，不写逐行翻译式注释。
- 与性能相关逻辑（限频、裁剪、缓存）必须保留注释说明。
- 遇到线程/协程切换，注释里说明“为什么在该线程执行”。

## 6. 关键文件
- UI 入口：`app/src/main/java/com/lei/safety_hat_2/ui/hud/HudScreen.kt`
- 状态编排：`app/src/main/java/com/lei/safety_hat_2/ui/hud/HudViewModel.kt`
- AI 级联：`app/src/main/java/com/lei/safety_hat_2/data/repository/NcnnCascadeAiRepository.kt`
- RTMP 推流：`app/src/main/java/com/lei/safety_hat_2/data/rtmp/RtmpStreamer.kt`
- 启动配置：`app/src/main/java/com/lei/safety_hat_2/MainActivity.kt`

## 7. 常见任务入口
- 修改 RTMP 地址/流 ID：
  - `app/src/main/java/com/lei/safety_hat_2/MainActivity.kt`
- 调整模型文档：
  - `app/src/main/assets/MODEL_README.md`
- 调整违规统计口径：
  - `app/src/main/java/com/lei/safety_hat_2/ui/hud/HudViewModel.kt`
