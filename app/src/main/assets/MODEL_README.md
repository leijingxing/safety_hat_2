智能安全帽模型说明

本目录存放端侧 AI 模型文件（NCNN / TorchScript 导出）。目前应用已接入：
- 安全帽/人员检测（SafeMod）
- 抽烟识别（SmokeMod）
- 打电话识别（CallMod）
- 工装识别（UniformMod）
- 反光衣识别（VestMod）

模型清单与用途
1) best.bin / best.param
   - 备注：历史模型，旧工程保留，当前未接入

2) safe_hat_detect_20250509_ncnn_s_320.bin / .param
   - 用途：安全帽/人员检测（当前接入）

3) safe_hat_smoking_20250515.bin / .param
   - 用途：抽烟识别（当前接入，级联裁剪输入）

4) cls_calling_241113_m.bin / .param
   - 用途：打电话识别（当前接入，级联裁剪输入）

5) cls_refjacket_241122_s_e14.ncnn.bin / .param
   - 用途：反光衣识别（当前接入，级联裁剪输入）

6) cls_workclothes_241123_s_e12_torchscript.ncnn.bin / .param
   - 用途：工装识别（当前接入，级联裁剪输入）

7) flames.bin / flames.param
   - 用途：火焰/火情识别
   - 状态：暂未接入（缺少 JNI/C++ 推理封装）

8) yolov7-tiny.bin / yolov7-tiny.param
   - 备注：历史模型，旧工程保留，当前未接入

说明
- 当前级联策略：先用安全帽/人员检测得到人体区域，再裁剪输入到抽烟/打电话/工装/反光衣模型。
- 若更换模型或标签，请同步更新标签映射与违规规则。
