# Safety Hat 2

Safety Hat 2 是一个基于 Android 的工业安全 HUD 应用，提供实时画面预览、
NCNN 级联检测与可选 RTMP 推流能力，适用于移动端与边缘设备巡检场景。

## 1. 核心能力
- CameraX/USB 相机预览 + HUD 叠加。
- 安全帽、抽烟、打电话、工装、反光衣等违规识别。
- 违规列表与近 1 分钟违规统计。
- 可选 RTMP 推流（MediaCodec H.264/H.265 编码）。

## 2. 技术架构
- `app/`：Compose UI、ViewModel、Camera 输入、AI 调度、RTMP 调用。
- `libncnn/`：NCNN 模型与 JNI 封装（SafeMod/SmokeMod/CallMod/UniformMod/VestMod）。
- `librtmps/`：RTMP 推流底层实现（C/C++ + JNI）。
- `libuvccamera/`：UVC 相机支持与底层渲染。

## 3. 关键流程
1. 相机帧（NV21/RGBA）进入应用层。
2. RGBA 帧送入 `AiRepository.submitFrame`，由 `SafeMod` 先做人/安全帽检测。
3. 基于人员框裁剪子区域，触发抽烟/打电话/工装/反光衣模型级联识别。
4. 检测结果汇总为 `AiEvent`，更新 HUD 叠加框与违规列表。
5. 若开启推流，NV21 帧进入 MediaCodec 编码后写入 RTMP。

## 4. 关键代码入口
- HUD 页面：`app/src/main/java/com/lei/safety_hat_2/ui/hud/HudScreen.kt`
- 状态与事件编排：`app/src/main/java/com/lei/safety_hat_2/ui/hud/HudViewModel.kt`
- AI 级联仓库：`app/src/main/java/com/lei/safety_hat_2/data/repository/NcnnCascadeAiRepository.kt`
- RTMP 推流器：`app/src/main/java/com/lei/safety_hat_2/data/rtmp/RtmpStreamer.kt`
- 启动配置：`app/src/main/java/com/lei/safety_hat_2/MainActivity.kt`

## 5. 配置说明
- RTMP 地址与流 ID：在 `MainActivity.kt` 的 `HudViewModelFactory` 参数中修改。
- GPU 开关：`HudViewModelFactory` 的 `useGpu` 参数（默认建议关闭，优先稳定）。
- 模型说明：`app/src/main/assets/MODEL_README.md`。

## 6. 开发与运行
```bash
./gradlew :app:assembleDebug
```

Windows PowerShell:
```powershell
.\gradlew.bat :app:assembleDebug
```

## 7. 注意事项
- 模型级联依赖 `SafeMod` 人员相关标签作为下游入口，若人员检出为空，下游模型不会触发。
- RTMP 发送使用 H.264/H.265 Annex-B NALU 处理链路。

