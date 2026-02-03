
cmake_minimum_required(VERSION 3.20.1)
project("yolo_ai")

message("Current directory: ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(freetype SHARED IMPORTED)
set_target_properties(freetype PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libfreetype.so)
include_directories(${CMAKE_SOURCE_DIR}/libs/android-freetype-master/include)

set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/libs/opencv-4.5.5-android-sdk/OpenCV-android-sdk/sdk/native/jni)
find_package(OpenCV REQUIRED)
set(ncnn_DIR ${CMAKE_SOURCE_DIR}/libs/ncnn-20240820-android-vulkan-shared/${ANDROID_ABI})

include_directories(
        ${OpenCV_DIR}/include
        ${ncnn_DIR}/include/ncnn
        power/include
)

link_directories(
        ${ncnn_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

# 查找opencv路径下所需的静态库
file(GLOB STATIC_LIBS2 ${OpenCV_DIR}/native/staticlibs/arm64-v8a/*.a)  # opencv需要的第三方静态库


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        libncnn.cpp
        yolo.cpp
        safe/libsafe.cpp
        safe/yolov8.cpp
#        power_jni.cpp
#        power/yolov8_detect.cpp
#        power/yolov8_pose.cpp
#        power/clipper.cpp
#        power/ppocr_det.cpp
#        power/ppocr_cls.cpp
#        power/ppocr_rec.cpp
#        power/ppocr.cpp
#        power/stop_send_card_rec.cpp
#        power/device_status_rec.cpp
#        power/high_voltage_cabinet_rec.cpp
#        power/low_voltage_cabinet_rec.cpp
        callsmoke/libcall.cpp
        callsmoke/yolocall.cpp
        callsmoke/libsmoke.cpp
        callsmoke/yolosmoke.cpp
        uniform/lib_uniform.cpp
        uniform/yolo_uniform.cpp
        hiv_z_vest/lib_hivz_vest.cpp
        hiv_z_vest/yolo_hivz_vest.cpp
        )
target_link_libraries(${CMAKE_PROJECT_NAME}
        freetype
        ncnn
        android
        log
        ${STATIC_LIBS2}
        ${OpenCV_LIBS}
        )


#set(ncnn_DIR ${CMAKE_SOURCE_DIR}/ncnn-20240410-android-vulkan/${ANDROID_ABI}/lib/cmake/ncnn)
#find_package(ncnn REQUIRED)
#
#add_library(ncnnyolov7 SHARED yoloncnn.cpp yolo.cpp ndkcamera.cpp)
#
#target_link_libraries(ncnnyolov7 ncnn ${OpenCV_LIBS} camera2ndk mediandk)
