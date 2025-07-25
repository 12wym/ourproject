
#include "yolov8.h"

#include <memory>

#include "utils/logging.h"
#include "process/preprocess.h"
#include "process/yolov8_postprocess.h"

#include <ctime>

void DetectionGrp2DetectionArray(yolov8::detect_result_group_t &det_grp, std::vector<Detection> &objects)
{
    // 根据当前系统时间生成随机数种子
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int i = 0; i < det_grp.count; i++)
    {
        Detection det;
        det.className = det_grp.results[i].name;

        det.box = cv::Rect(det_grp.results[i].box.left,
                           det_grp.results[i].box.top,
                           det_grp.results[i].box.right - det_grp.results[i].box.left,
                           det_grp.results[i].box.bottom - det_grp.results[i].box.top);

        det.confidence = det_grp.results[i].prop;
        det.class_id = 0;
        // generate random cv::Scalar color
        det.color = cv::Scalar(rand() % 255, rand() % 255, rand() % 255);
        objects.push_back(det);
    }
}

// 构造函数
YOLOV8::YOLOV8()
{
    engine_ = CreateRKNNEngine();
    input_tensor_.data = nullptr;
}
// 析构函数
YOLOV8::~YOLOV8()
{
    if (input_tensor_.data != nullptr)
    {
        free(input_tensor_.data);
        input_tensor_.data = nullptr;
    }
    for (auto &tensor : output_tensors_)
    {
        free(tensor.data);
        tensor.data = nullptr;
    }
}

nn_error_e YOLOV8::LoadModel(const char* model_path){

    auto ret = engine_->LoadModelFile(model_path);
    if(ret != NN_SUCCESS)
    {
        NN_LOG_ERROR("yolo load model file failed");
        return ret;
    }

    //获取输入信息
    
    auto input_shapes = engine_->GetInputShapes();

    //判断输入
    if(input_shapes.size() != 1){
        NN_LOG_ERROR("yolov7 input tensor number is error , is %ld " , input_shapes.size());
        return NN_RKNN_INPUT_ATTR_ERROR;
    }
    // 这个函数的作用是将神经网络张量属性（tensor_attr_s）转换为用于OpenCV图像输入的张量数据
    nn_tensor_attr_to_cvimg_input_data(input_shapes[0], input_tensor_);
    //分配内存空间
    input_tensor_.data = malloc(input_tensor_.attr.size);

    auto output_shapes = engine_->GetOutputShapes();

    //获取模型的输出存入,在yolov8中输出9个分支，每个分支是一个4维度的向量

    for(int i = 0 ; i < output_shapes.size() ; i++){
        tensor_data_s tensor;
        //输出张量的元素数量
        tensor.attr.n_elems = output_shapes[i].n_elems;
        //输出张量的维度数 , 3个大分支
        tensor.attr.n_dims = output_shapes[i].n_dims;
        //输出每个分支张量的每一个维度 eg 1x255x20x20
        for (int j = 0; j < output_shapes[i].n_dims; j++)
        {
            tensor.attr.dims[j] = output_shapes[i].dims[j];
        }
        if (output_shapes[i].type != NN_TENSOR_INT8)
        {
            NN_LOG_ERROR("yolo output tensor type is not int8, but %d", output_shapes[i].type);
            return NN_RKNN_OUTPUT_ATTR_ERROR;
        }
        tensor.attr.type = output_shapes[i].type;
        tensor.attr.index = i;
        tensor.attr.size = output_shapes[i].n_elems * nn_tensor_type_to_size(tensor.attr.type);
        tensor.data = malloc(tensor.attr.size);
        output_tensors_.push_back(tensor);
        out_zps_.push_back(output_shapes[i].zp);
        out_scales_.push_back(output_shapes[i].scale);

    }
    return NN_SUCCESS;


}

// 图像预处理
nn_error_e YOLOV8::Preprocess(const cv::Mat &img, const std::string process_type, cv::Mat &image_letterbox)
{

    // 预处理包含：letterbox、归一化、BGR2RGB、NCWH
    // 其中RKNN会做：归一化、NCWH转换（详见课程文档），所以这里只需要做letterbox、BGR2RGB

    // 比例
    float wh_ratio = (float)input_tensor_.attr.dims[2] / (float)input_tensor_.attr.dims[1];

    // lettorbox

    if (process_type == "opencv")
    {
        // BGR2RGB，resize，再放入input_tensor_中
        letterbox_info_ = letterbox(img, image_letterbox, wh_ratio);
        cvimg2tensor(image_letterbox, input_tensor_.attr.dims[2], input_tensor_.attr.dims[1], input_tensor_);
    }
    else if (process_type == "rga")
    {
        // rga resize
        letterbox_info_ = letterbox_rga(img, image_letterbox, wh_ratio);
        // save img
        // cv::imwrite("rga.jpg", image_letterbox);
        cvimg2tensor_rga(image_letterbox, input_tensor_.attr.dims[2], input_tensor_.attr.dims[1], input_tensor_);
    }
    
    

    return NN_SUCCESS;
}

nn_error_e YOLOV8::Inference(){

    std::vector<tensor_data_s> inputs;
    //图像数据被加载在input_tensor_中
    inputs.push_back(input_tensor_);
    engine_->Run(inputs, output_tensors_, false);
    return NN_SUCCESS;
}

nn_error_e YOLOV8::Run(const cv::Mat &img, std::vector<Detection> &objects)
{
    cv::Mat image_letterbox;
    // 预处理，支持opencv或rga
    // cv::cvtColor(img, img, cv::COLOR_YUV2BGR_NV12);
    //摄像头处理
    // cv::Mat rgb_image;
    // cv::cvtColor(img, rgb_image, cv::COLOR_YUV2BGR_NV12);
    // Preprocess(rgb_image, "opencv", image_letterbox);
    // //视频处理

    Preprocess(img, "opencv", image_letterbox);
    // Preprocess(img, "rga", image_letterbox);
    //推理
    Inference();
    Postprocess(image_letterbox, objects);

    return NN_SUCCESS;


}

void letterbox_decode(std::vector<Detection> &objects, bool hor, int pad)
{
    for (auto &obj : objects)
    {
        if (hor)
        {
            obj.box.x -= pad;
        }
        else
        {
            obj.box.y -= pad;
        }
    }
}


// 后处理
nn_error_e YOLOV8::Postprocess(const cv::Mat &img, std::vector<Detection> &objects)
{
    int height = input_tensor_.attr.dims[1];
    int width = input_tensor_.attr.dims[2];
    float scale_w = height * 1.f / img.cols; // 保证为浮点类型
    float scale_h = width * 1.f / img.rows;

    yolov8::detect_result_group_t detections;

    // yolov8::post_process((int8_t *)output_tensors_[0].data,
    //                      (int8_t *)output_tensors_[1].data,
    //                      (int8_t *)output_tensors_[2].data,
    //                      height, width,
    //                      BOX_THRESH, NMS_THRESH,
    //                      scale_w, scale_h,
    //                      out_zps_, out_scales_,
    //                      &detections);

    yolov8::post_process(output_tensors_ ,
                         height, width,
                         BOX_THRESH, NMS_THRESH,
                         scale_w, scale_h,
                         out_zps_, out_scales_,
                         &detections);


    DetectionGrp2DetectionArray(detections, objects);
    letterbox_decode(objects, letterbox_info_.hor, letterbox_info_.pad);

    return NN_SUCCESS;
}

