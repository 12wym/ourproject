# 设置最低版本号
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
# 设置项目名称
project(rk3588-demo VERSION 0.0.1 LANGUAGES CXX)

# 输出系统信息
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")

# 设置编译器
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)

# 设置库架构
set(LIB_ARCH "aarch64")
set(DEVICE_NAME "RK3588")

#  rknn_api 文件夹路径
set(RKNN_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/librknn_api)
#  rknn_api include 路径
set(RKNN_API_INCLUDE_PATH ${RKNN_API_PATH}/include)
#  rknn_api lib 路径
set(RKNN_API_LIB_PATH ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)




# 寻找OpenCV库，使用自定义的OpenCV_DIR
set(3RDPARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)


#  rknn_api 文件夹路径
set(RGA_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rga/RK3588)
#  rknn_api include 路径
set(RGA_DIR ${RGA_API_PATH}/include)
#  rknn_api lib 路径
set(RGA_LIB ${RGA_API_PATH}/lib/Linux/${LIB_ARCH}/librga.so)


# 查找头文件路径
find_path(RGA_INCLUDE_DIR NAMES rga.h PATHS ${RGA_DIR})

# 查找库文件路径
find_library(RGA_LIBRARY NAMES rga PATHS ${RGA_LIB})

# 检查头文件和库文件是否找到
if(RGA_INCLUDE_DIR AND RGA_LIBRARY)
    message(STATUS "RGA API found.")
    message(STATUS "Include directory: ${RGA_INCLUDE_DIR}")
    message(STATUS "Library: ${RGA_LIBRARY}")

    # 如果你想链接这个库，可以将它添加到你的目标中
    # target_include_directories(your_target PRIVATE ${RGA_INCLUDE_DIR})
    # target_link_libraries(your_target PRIVATE ${RGA_LIBRARY})
else()
    message(FATAL_ERROR "RGA API not found. Check the paths and verify if the library exists.")
endif()
# set(RGA_DIR ${3RDPARTY_PATH}/rga/${DEVICE_NAME})
# set(RGA_LIB ${RGA_DIR}/lib/Linux/${LIB_ARCH}/librga.so)
# set(OpenCV_DIR ${3RDPARTY_PATH}/opencv/opencv-linux-${LIB_ARCH}/share/OpenCV)
find_package(OpenCV REQUIRED)
# 输出OpenCV信息
message(STATUS "include path: ${OpenCV_INCLUDE_DIRS}")

# find engin
find_package (Eigen3 3.3 REQUIRED NO_MODULE)


##Gstreamer
# Find the GStreamer package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

# Add include directories
include_directories(${GST_INCLUDE_DIRS})

# Add library directories
link_directories(${GST_LIBRARY_DIRS})
# Add compiler flags for GStreamer
add_definitions(${GST_CFLAGS_OTHER})

#ffmpeg
# aux_source_directory会查找指定目录下的所有源文件，并将结果存入指定变量名
# 这里是将当前目录下的所有源文件存进变量SRC_LIST
aux_source_directory(. SRC_LIST)
# 设置ffmpeg依赖库及头文件所在目录，并存进指定变量
set(ffmpeg_libs_DIR /usr/local/ffmpeg/lib)
set(ffmpeg_headers_DIR /usr/local/ffmpeg/include)
#对于find_package找不到的外部依赖库，可以用add_library添加
# SHARED表示添加的是动态库
# IMPORTED表示是引入已经存在的动态库
add_library( avcodec SHARED IMPORTED )

#指定所添加依赖库的导入路径
set_target_properties( avcodec PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavcodec.so )

add_library( avfilter SHARED IMPORTED )
set_target_properties( avfilter PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavfilter.so )

add_library( swresample SHARED IMPORTED )
set_target_properties( swresample PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libswresample.so )

add_library( swscale SHARED IMPORTED )
set_target_properties( swscale PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libswscale.so )

add_library( avformat SHARED IMPORTED )
set_target_properties( avformat PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavformat.so )

add_library( avutil SHARED IMPORTED )
set_target_properties( avutil PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavutil.so )

include_directories( ${OpenCV_INCLUDE_DIRS} ${ffmpeg_headers_DIR} )
link_directories(${OpenCV_LIBRARY_DIRS} ${ffmpeg_libs_DIR} )


# 用来搜索头文件的目录
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${RKNN_API_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RGA_DIR}/include
)

# 构建预处理和后处理库
add_library(nn_process SHARED
            src/process/preprocess.cpp
            src/process/yolov8_postprocess.cpp
            src/process/yolov5_postprocess.cpp
)
# 链接库
target_link_libraries(nn_process
    ${OpenCV_LIBS}
    ${RGA_LIB}
)

# 构建自定义封装API库
add_library(rknn_engine SHARED src/engine/rknn_engine.cpp) 
# 链接库
target_link_libraries(rknn_engine
    ${RKNN_API_LIB_PATH}
)
# # yolov7_lib
# add_library(yolov7_lib SHARED src/task/yolov7.cpp)
# # 链接库
# target_link_libraries(yolov7_lib
#     rknn_engine
#     nn_process
# )

# yolov7_lib
add_library(yolov8_lib SHARED src/task/yolov8.cpp)
# 链接库
target_link_libraries(yolov8_lib
    rknn_engine
    nn_process
)

# draw_lib
add_library(draw_lib SHARED src/draw/cv_draw.cpp)
# 链接库
target_link_libraries(draw_lib
    ${OpenCV_LIBS}
)

# can_lib
add_library(can_lib SHARED src/can/can.cpp)
# 链接库
target_link_libraries(can_lib
)

# uart_lib
add_library(uart_lib SHARED src/uart/uart.cpp)
# 链接库
target_link_libraries(uart_lib
)

# streamer_lib

add_library(streamer_lib SHARED src/streamer/streamer.cpp)
# 链接库
target_link_libraries(streamer_lib
    ${ffmpeg_libs_DIR}
    ${GST_LIBRARIES}
)

# bytetrack_lib

add_library(bytetrack_lib SHARED 
    src/bytetrack/BytekalmanFilter.cpp
    src/bytetrack/BYTETracker.cpp
    src/bytetrack/lapjv.cpp
    src/bytetrack/STrack.cpp
    src/bytetrack/utils.cpp
)
# 链接库
target_link_libraries(bytetrack_lib
    Eigen3::Eigen
)


# 测试自yolov5 thread pool
add_executable(yolo11_thread_pool 
    src/main.cpp
    src/task/Thead_control.cpp
    )

# 链接库
target_link_libraries(yolo11_thread_pool
        draw_lib
        yolov8_lib
        uart_lib
        can_lib
        streamer_lib
        bytetrack_lib
        Threads::Threads
        Eigen3::Eigen
)

add_custom_target(clear
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/*
	)

