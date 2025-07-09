#ifndef RK3588_DEMO_NN_DATATYPE_H
#define RK3588_DEMO_NN_DATATYPE_H

#include <opencv2/opencv.hpp>

typedef struct _nn_object_s {
    float x;
    float y;
    float w;
    float h;
    float score;
    int class_id;
} nn_object_s;

struct Detection
{
    int class_id{0};
    std::string className{};
    float confidence{0.0};
    cv::Scalar color{};
    cv::Rect box{};
};

// class detect_result
// {
// public:
//     int classId;
//     float confidence;
//     cv::Rect_<float> box;
// };



// class detect_result
// {
// public:
//     int classId;
//     float confidence;
//     cv::Rect_<float> box;
//     // 构造函数，用于从Detection对象创建detect_result对象  
//     detect_result(const Detection& det)  
//         : classId(det.class_id),  
//           confidence(det.confidence),  
//           box(static_cast<float>(det.box.x), static_cast<float>(det.box.y),  
//                static_cast<float>(det.box.width), static_cast<float>(det.box.height))  
//     {} 
// };


#endif //RK3588_DEMO_NN_DATATYPE_H
