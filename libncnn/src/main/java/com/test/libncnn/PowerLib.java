package com.test.libncnn;  // 定义Java文件的包名，标识文件所属的包。

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import org.yaml.snakeyaml.Yaml;

import java.io.InputStream;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


// 定义 ReadYaml 类，用于封装读取和解析YAML文件的功能。
public class PowerLib {

    static {
        try {
            System.loadLibrary("c++_shared"); // 加载 c++_shared 库
            System.loadLibrary("ncnn"); // 加载 ncnn 库
            System.loadLibrary("yolo_ai"); // 加载 native-lib 库
        } catch (UnsatisfiedLinkError e) {
            Log.e("NativeLib", "Failed to load libraries: " + e.getMessage());
        }
    }
    private Context context;
    private AssetManager assetManager; // 定义AssetManager对象，用于访问assets目录
    String filePath;

    // 声明一个本地方法，将解析的具体数据传递到 C++ 层，包括 names 列表
//    public native void StopSendCardSendToCpp1(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void StopSendCardSendToCpp2(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void StopSendCardSendToCpp3(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void StopSendCardSendToCpp4(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//
//    public native void DeviceStatusSendToCpp1(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void DeviceStatusSendToCpp2(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void DeviceStatusSendToCpp3(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void DeviceStatusSendToCpp4(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//
//    public native void HVC_RecognitionSendToCpp1(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void HVC_RecognitionSendToCpp2(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void HVC_RecognitionSendToCpp3(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void HVC_RecognitionSendToCpp4(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void HVC_RecognitionSendToCpp5(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void HVC_RecognitionSendToCpp6(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//
//    public native void LVC_RecognitionSendToCpp1(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void LVC_RecognitionSendToCpp2(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void LVC_RecognitionSendToCpp3(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void LVC_RecognitionSendToCpp4(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);
//    public native void LVC_RecognitionSendToCpp5(AssetManager mgr, String paramPath, String binPath, String inputNode, String outputNode, int modelHeight, int modelWidth, boolean useVulkan, String[] names);

    public native void initParam(String jParamType,
                                 String jParamPath, String jBinPath,
                                 String jinputNode, String joutputNode,
                                 int jmodelHeight, int jmodelWidth,
                                 boolean juseVulkan, String[] jNames);

    public native HashMap<String, String> cppmain(AssetManager mgr, long rgbaPtr, int oper, int step);

    // 构造函数，传入AssetManager和YAML文件路径
    public PowerLib() {
    }
    public void setAssets(AssetManager assetManager) {
        this.assetManager = assetManager; // 将传入的AssetManager保存为类的成员变量
    }

    // 读取YAML文件的函数
    public Map<String, Object> _readConfig() {
        Map<String, Object> data = Collections.emptyMap();
        // 创建YAML解析器对象，用于读取YAML文件
        Yaml yaml = new Yaml();

        try {
            // 打开指定的YAML文件
            InputStream inputStream = assetManager.open(filePath);
            // 使用YAML解析器将输入流转换为Map对象
            data = yaml.load(inputStream);
        } catch (Exception e) {
            // 如果读取文件或解析过程中出现异常，打印异常信息
            e.printStackTrace();
        }
        return data;
    }

    public void readConfig(String paramType, String yamlFilePath) {
        filePath = yamlFilePath;
        Map<String, Object> configData = _readConfig();

        // 检查解析结果是否为空
        if (configData != null) {
            // 例如，获取model部分的数据
            Map<String, Object> modelData = (Map<String, Object>) configData.get("model");
            if (modelData != null) {
                String paramPath = (String) modelData.get("paramPath");
                String binPath = (String) modelData.get("binPath");
                String input_node = (String) modelData.get("input_node");
                String output_node = (String) modelData.get("output_node");
                int modelHeight = (int) modelData.get("model_height");
                int modelWidth = (int) modelData.get("model_width");
                // 将 use_vulkan 从整型转换为布尔值
                int useVulkanInt = (int) modelData.get("use_vulkan");
                boolean useVulkan = (useVulkanInt == 1);

                Log.d("YAML Config", "paramPath: " + paramPath);
                Log.d("YAML Config", "binPath: " + binPath);

                // 解析 classes 列表（names）
                List<String> names = (List<String>) configData.get("names");
                if (names != null) {
                    // 输出 names 列表
                    Log.d("YAML Config", "Names: " + names);

                    // 通过 JNI 将数据传递到 C++ 层
                    initParam(paramType, paramPath, binPath, input_node, output_node, modelHeight, modelWidth, useVulkan, names.toArray(new String[0]));
                }
            }
        } else {
            // 如果configData为空，表示解析失败
            Log.e("YAML Config", "Failed to load YAML data");
        }
    }

    public HashMap<String, String> onRecvImage(long nativeObj, int op, int step) {
        return cppmain(assetManager, nativeObj, op, step);
    }
}



