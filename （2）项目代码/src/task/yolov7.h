

#ifndef RK3588_DEMO_YOLOV7_H
#define RK3588_DEMO_YOLOV7_H

#include "types/yolo_datatype.h"
#include "engine/engine.h"
#include "process/preprocess.h"

class YOLOV7{

private:
    //智能指针
    std::shared_ptr<NNEngine> engine_;
    //模型输入
    tensor_data_s input_tensor_;
    //图像变化参数
    LetterBoxInfo letterbox_info_;
    //模型输出数据
    std::vector<tensor_data_s> output_tensors_;
    //量化参数
    std::vector<int32_t> out_zps_;
    std::vector<float> out_scales_;



public:
    YOLOV7();
    ~YOLOV7();
    //模型加载
    nn_error_e LoadModel(const char* model_path);
    // 运行模型
    nn_error_e Run(const cv::Mat &img, std::vector<Detection> &objects); 
    


private:
    nn_error_e Preprocess(const cv::Mat &img, const std::string process_type,cv::Mat &image_letterbox);   // 图像预处理
    nn_error_e Inference();                                                      // 推理
    nn_error_e Postprocess(const cv::Mat &img, std::vector<Detection> &objects); // 后处理



};


#endif // RK3588_DEMO_YOLOV5_H
