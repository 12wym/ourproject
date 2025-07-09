#include <iostream>
#include <thread>
#include "uart/uart.h"
#include "can/can.h"
#include "utils/logging.h"
#include "task/Thread_control.h"
#include "bytetrack/BYTETracker.h"
#include "streamer/streamer.h"

int tag = 0;
static int g_frame_start_id = 0; // 读取视频帧的索引
static int g_frame_end_id = 0;   // 模型处理完的索引


static int g_frame_start_id_jiangluo = 0; // 读取视频帧的索引
static int g_frame_end_id_jiangluo = 0;   // 模型处理完的索引

static int g_frame_start_id_hongwai = 0; // 读取视频帧的索引
static int g_frame_end_id_hongwai = 0;   // 模型处理完的索引

cv::Mat output;

using namespace streamer;
extern int udp_socket_1;
extern int udp_socket_2;


int can_status = 0;
int now_status = can_status;
extern std::mutex lock_can;
extern bool data_ready;
extern std::condition_variable cv_can;

cv::Size frameSize1(1920, 1080);
cv::VideoWriter writer1("output1.avi", cv::VideoWriter::fourcc('M','J','P','G'), 25, frameSize1);



// 创建线程池1
static Thread_Control *g_pool = nullptr;
bool end = false;
static Thread_Control *g_pool_hongwai = nullptr;

static Thread_Control *g_pool_jiangluo = nullptr;

std::mutex streamer_lock;

int getClassId(const std::string& className) {  
    // 这里是一个简单的映射示例，实际情况中你可能需要根据className查找实际的classId  
    if (className == "vehicle") return 0;  
    if (className == "person") return 1;  

    return -1; // 如果未找到匹配项，返回一个错误值  
}

void process_frame(const cv::Mat &in, cv::Mat &out , int video_type)
{
    //in.copyTo(out);
    if(video_type == 0)
    {
        cv::cvtColor(in, out, cv::COLOR_YUV2BGR_NV12);
    }
    else
    {
        in.copyTo(out);
    }
}


void stream_frame(Streamer &streamer, const cv::Mat &image)
{
    streamer.stream_frame(image.data);
}




void test_bytetrack(cv::Mat& frame, std::vector<track_in>& results, BYTETracker& tracker , Streamer& streamer)
{
    std::vector<track_in> objects;


    for (track_in dr : results)
    {
        // std::cout << "IIIIIIIIIIIIII" << std::endl;

        if(dr.classId == 0) //person
        {
            objects.push_back(dr);
        }
        
        if(dr.classId == 1) //person
        {
            objects.push_back(dr);
        }
    }

    std::vector<STrack> output_stracks = tracker.update(objects);
    for (unsigned long i = 0; i < output_stracks.size(); i++)
    {

        std::vector<float> tlwh = output_stracks[i].tlwh;
        bool vertical = tlwh[2] / tlwh[3] > 1.6;
        if (tlwh[2] * tlwh[3] > 20 && !vertical)
        {
            cv::Scalar s = tracker.get_color(output_stracks[i].track_id);
            cv::putText(frame, cv::format("%d", output_stracks[i].track_id), cv::Point(tlwh[0], tlwh[1] - 5),
                    0, 0.6, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            cv::rectangle(frame, cv::Rect(tlwh[0], tlwh[1], tlwh[2], tlwh[3]), s, 2);
        }
    }
    stream_frame(streamer , frame); 

}



void get_results_kejian(int width , int height , int fps, BYTETracker& tracker , Streamer& streamer)
{
    // int64_t frame_id = 0;
    // 记录开始时间
    auto start_all = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    while (true)
    {
        cv::Mat img;
        std::vector<Detection> object;
        if(can_status == 0)
        // 结果
        { 
            size_t current_size = g_pool->map_size();
            auto ret = g_pool->getTargetImgResult(img, g_frame_end_id);  // 自己业务上可能就是需要下面的，这边是可视化的，根据ID去找可视化哪一帧
            
            //获取原始图片和id
            
            auto ret2 = g_pool->getTargetResult(object, img ,g_frame_end_id);


            // img = g_pool->now_picture;
            g_frame_end_id++;
        }
        else if(can_status == 1)
        {   
            size_t current_size = g_pool_jiangluo->map_size();
            auto ret = g_pool_jiangluo->getTargetImgResult(img, g_frame_end_id_jiangluo);  // 自己业务上可能就是需要下面的，这边是可视化的，根据ID去找可视化哪一帧
            
            //获取原始图片和id
            
            auto ret2 = g_pool_jiangluo->getTargetResult(object, img ,g_frame_end_id_jiangluo);


            // img = g_pool->now_picture;
            g_frame_end_id_jiangluo++;

        }
        else if(can_status == 2)
        {
            size_t current_size = g_pool_hongwai->map_size();

            //std::cout << "1     " << current_size << std::endl;


            auto ret = g_pool_hongwai->getTargetImgResult(img, g_frame_end_id_hongwai);  // 自己业务上可能就是需要下面的，这边是可视化的，根据ID去找可视化哪一帧
            
            //获取原始图片和id
            
            auto ret2 = g_pool_hongwai->getTargetResult(object, img ,g_frame_end_id_hongwai);


            g_frame_end_id_hongwai++;
        }
        std::vector<track_in> track_message;
        // 遍历 object，并将信息拷贝到 track_message
        for (int i = 0; i < object.size(); i++) {
            track_in track;
            track.classId = object[i].class_id;
            track.confidence = object[i].confidence;
            track.box = object[i].box;

            // 将 track 添加到 track_message
            track_message.push_back(track);


        }

        test_bytetrack( img, track_message ,tracker , streamer);
        // 如果读取完毕，且模型处理完毕，结束
        // if (end && ret2 != NN_SUCCESS )
        // {
        //     g_pool->stopAll();
        //     break;
        // }


        // 算法2：计算超过 1s 一共处理了多少张图片
        frame_count++;
        // all end
        auto end_all = std::chrono::high_resolution_clock::now();
        auto elapsed_all_2 = std::chrono::duration_cast<std::chrono::microseconds>(end_all - start_all).count() / 1000.f;
        // 每隔1秒打印一次，多线程必须这么计算平均帧率
        if (elapsed_all_2 > 1000)
        {
            NN_LOG_INFO("Method2 Time:%fms, FPS:%f, Frame Count:%d", elapsed_all_2, frame_count / (elapsed_all_2 / 1000.0f), frame_count);
            frame_count = 0;
            start_all = std::chrono::high_resolution_clock::now();
        }
    }
    // 结束所有线程
    g_pool->stopAll();
    g_pool_hongwai->stopAll();
    g_pool_jiangluo->stopAll();
    NN_LOG_INFO("Get results end.");

}






// 读取视频帧，提交任务
void read_stream(const char *video_file , const char* video_file2 , const char* video_file3)
{
    // 读取视频
    cv::VideoCapture cap(video_file);
    if (!cap.isOpened())
    {
        NN_LOG_ERROR("Failed to open video file: %s", video_file);
    }
    // 获取视频尺寸、帧率
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps = cap.get(cv::CAP_PROP_FPS);
    NN_LOG_INFO("Video size: %d x %d, fps: %d", width, height, fps);

    // 读取视频
    cv::VideoCapture cap2(video_file2);
    if (!cap2.isOpened())
    {
        NN_LOG_ERROR("Failed to open video file: %s", video_file2);
    }
    // 获取视频尺寸、帧率
    int width_2 = cap2.get(cv::CAP_PROP_FRAME_WIDTH);
    int height_2 = cap2.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps_2 = cap2.get(cv::CAP_PROP_FPS);
    NN_LOG_INFO("Video size: %d x %d, fps: %d", width_2, height_2, fps_2);

    // 读取视频
    cv::VideoCapture cap3(video_file3);
    if (!cap3.isOpened())
    {
        NN_LOG_ERROR("Failed to open video file: %s", video_file3);
    }
    // 获取视频尺寸、帧率
    int width_3 = cap3.get(cv::CAP_PROP_FRAME_WIDTH);
    int height_3 = cap3.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps_3 = cap3.get(cv::CAP_PROP_FPS);
    NN_LOG_INFO("Video size: %d x %d, fps: %d", width_3, height_3, fps_3);

    // 画面
    cv::Mat img;
    cv::Mat img2;
    cv::Mat img3(height_3, width_3, CV_8UC2);


    cv::Mat bgr_image;

    // 创建 1920x1080 的输出图像
    
    while (true)
    {

        if(now_status != can_status){
            if(can_status == 0)
            {
                g_pool->clear();
                g_frame_start_id = 0;
                g_frame_end_id = 0;
            }
            else if(can_status == 1)
            {
                g_pool_jiangluo->clear();
                g_frame_start_id_jiangluo = 0; // 读取视频帧的索引
                g_frame_end_id_jiangluo = 0;   // 模型处理完的索引
            }
            else if(can_status == 2)
            {
                g_pool_hongwai->clear();
                g_frame_start_id_hongwai = 0;
                g_frame_end_id_hongwai = 0;
            }

            now_status = can_status;
        }

       if(can_status == 0)
       {    // 读取视频帧
            cap >> img;
            // cv::cvtColor(img, img, cv::COLOR_YUV2BGR_NV12);
            if (img.empty())
            {
                NN_LOG_INFO("Video end.");
                // 结束所有线程
                // sleep 5s
                // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                end = true;
                break;  // 跳出while循环
            }
            g_pool->submitTask(img.clone(), g_frame_start_id++);  // 帧的ID不断自增

       }
       else if(can_status == 1){
            cap2 >> img2;
            cv::cvtColor(img2, img2, cv::COLOR_YUV2BGR_NV12);
            if (img2.empty())
            {
                NN_LOG_INFO("Video end.");
                // 结束所有线程
                // sleep 5s
                // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                end = true;
                break;  // 跳出while循环
            }
            g_pool_jiangluo->submitTask(img2.clone(), g_frame_start_id_jiangluo++);  // 帧的ID不断自增


       }
       else if(can_status == 2){
            cap3 >> img3;
            // cv::Mat output_image(1080, 1920, CV_8UC3);
            // cv::resize(img3, img3, cv::Size(1920, 1080));
            // cv::cvtColor(img3, img3, cv::COLOR_YUV2BGR_YUYV);
            // cv::resize(img3, img3, output_image.size());

            cv::Mat img_rgb;
            // cv::cvtColor(img3, img_rgb, cv::COLOR_BGR2RGB);
            cv::cvtColor(img3, img_rgb, cv::COLOR_YUV2BGR_YUYV);
            // resize img
            cv::Mat img_resized;
            // resize img
            cv::resize(img_rgb, img_resized, cv::Size(1920, 1080), 0, 0, cv::INTER_LINEAR);
            if (img_resized.empty())
            {
                NN_LOG_INFO("Video end.");
                // 结束所有线程
                // sleep 5s
                // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                end = true;
                break;  // 跳出while循环
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            g_pool_hongwai->submitTask(img_resized, g_frame_start_id_hongwai++);  // 帧的ID不断自增
            //output_image.release();
            

       }
       
        
       
    }
    // 释放资源
    cap.release();
    cap2.release();
    cap3.release();
}


void init_udp_socket()
{
    udp_socket_1 = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_1 == -1) {
        std::cerr << "Error creating UDP socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    // 第二个UDP连接
    udp_socket_2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_2 == -1) {
        std::cerr << "Error creating UDP socket" << std::endl;
        exit(EXIT_FAILURE);
    }

}


int main(){

    //bytetrack
    int fps=30;
    BYTETracker bytetracker(fps, 30);
    Uart uart_light;
    CAN CAN_control;

    const char* _camera_2 = "/home/elf/fly_demo1/video/172.mp4";
    const char* _camera_3= "/home/elf/fly_demo1/video/172.mp4";
    const char* _camera_4 = "/home/elf/fly_demo1/video/WIN_20240409_16_44_35_Pro.mp4";

    //视频流初始化
    int stream_fps = 30;
    int bitrate = 2000000;

    Streamer streamer;
    StreamerConfig streamer_config(1920, 1080,
                                   1920, 1080,
                                   stream_fps, bitrate);
    //h264Debug
    //streamer.enable_av_debug_log();

    streamer.init(streamer_config);

    size_t streamed_frames = 0;

    init_udp_socket();
    

    
    Thread_Control thread_yolov7;   
    // std::string model_str = "/home/firefly/fly_debug/fly_demo1/rknn_models/yolov8s-leakey-relu-1280.rknn";
    std::string model_str = "/home/elf/fly_demo1/rknn_models/yolov8s-leakey-relu-1280.rknn";
    // 实例化线程池，这边的g_pool是一个指针：static Yolov5ThreadPool *g_pool = nullptr;
    g_pool = new Thread_Control();
    g_pool->setUp(model_str, 6);


    // std::string model_str_jiangluo = "/home/firefly/fly_debug/fly_demo1/rknn_models/yolov8s-leakey-relu-1280.rknn";
    std::string model_str_jiangluo = "/home/elf/fly_demo1/rknn_models/yolov8s-leakey-relu-1280.rknn";
    // 实例化线程池，这边的g_pool是一个指针：static Yolov5ThreadPool *g_pool = nullptr;
    g_pool_jiangluo = new Thread_Control();
    g_pool_jiangluo->setUp(model_str_jiangluo, 6);


    Thread_Control thread_yolov8;   
    // std::string model_str_hongwai = "/home/firefly/fly_debug/fly_demo1/rknn_models/hongwai.rknn";
    std::string model_str_hongwai = "/home/elf/fly_demo1/rknn_models/hongwai.rknn";
    // 实例化线程池，这边的g_pool是一个指针：static Yolov5ThreadPool *g_pool = nullptr;
    g_pool_hongwai = new Thread_Control();
    g_pool_hongwai->setUp(model_str_hongwai, 6);

    std::thread read_stream_thread(read_stream, _camera_2 , _camera_3 , _camera_4);
    // std::thread read_stream_thread(read_stream, _camera_1);

    std::thread result_thread([&]() {
        get_results_kejian(1920, 1080, 30, bytetracker , streamer);
    });

    read_stream_thread.join();
    result_thread.join();

    // camera.Camera_destroy(cap1 , cap2);

    return 0;
}
