#ifndef _THREAD_CONTROL_H_
#define _THREAD_CONTROL_H_
#include "error.h"
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <map>
// #include "yolov7.h"
// class Thread_Control{

// private:
//     std::queue<std::pair<int, cv::Mat>> tasks;             // <id, img>用来存放任务
//     std::vector<std::shared_ptr<YOLOV7>> yolov7_instances; // 模型实例；每一个线程都一个这个智能指针；是不是可以共享，所以用shared_ptr，初始化十二遍
//     std::map<int, std::vector<Detection>> results;         // <id, objects>用来存放结果（检测框） results和img_results以及tasks都是线程间的公共资源
//     std::map<int, cv::Mat> img_results;                    // <id, img>用来存放结果（图片）
//     std::map<int, cv::Mat> raw_img;                        // <id, img>原始图片
//     std::vector<std::thread> threads;                      // 线程池
//     std::mutex mtx1;                                       // 锁
//     std::mutex mtx2;
//     std::condition_variable cv_task, cv_result;            // 条件变量，对应上面两把锁
//     bool stop;

//     void worker(int id);                                   // 实际工作的函数

// public:
//     Thread_Control();  // 构造函数，没写啥，直接：stop = false;
//     ~Thread_Control(); // 析构函数，释放所有的线程

//     nn_error_e setUp(std::string &model_path, int num_threads = 12);     // 初始化,每一个线程都是同一个模型的
//     nn_error_e submitTask(const cv::Mat &img, int id);                   // 提交任务，对应的图片和帧的ID，帧的ID是什么是怎么初始化的？，因为多线程不知道位置，都需要帧的ID表示位置
//     nn_error_e getTargetResult(std::vector<Detection> &objects, int id); // 获取结果（检测框）只需要检测框
//     nn_error_e getTargetImgResult(cv::Mat &img, int id);                 // 获取结果（图片）
//     nn_error_e getRawImage(cv::Mat &img, int id);                         // 获取原始图片
//     void stopAll();                         // <id, img>用来存放结果（图片）

// };


#include "yolov8.h"

#include <atomic>
#include <memory>





class Thread_Control{

private:
    std::queue<std::pair<int, cv::Mat>> tasks;             // <id, img>用来存放任务
    std::vector<std::shared_ptr<YOLOV8>> yolov8_instances; // 模型实例；每一个线程都一个这个智能指针；是不是可以共享，所以用shared_ptr，初始化十二遍
    std::map<int, std::vector<Detection>> results;         // <id, objects>用来存放结果（检测框） results和img_results以及tasks都是线程间的公共资源
    std::map<int, cv::Mat> img_results;                    // <id, img>用来存放结果（图片）
    std::map<int, cv::Mat> raw_img;                        // <id, img>原始图片
    std::vector<std::thread> threads;                      // 线程池
    std::mutex mtx1;                                       // 锁
    std::mutex mtx2;
    std::condition_variable cv_task, cv_result;            // 条件变量，对应上面两把锁
    bool stop;

    void worker(int id);                                   // 实际工作的函数

public:
    Thread_Control();  // 构造函数，没写啥，直接：stop = false;
    ~Thread_Control(); // 析构函数，释放所有的线程

    nn_error_e setUp(std::string &model_path, int num_threads = 12);     // 初始化,每一个线程都是同一个模型的
    nn_error_e submitTask(const cv::Mat &img, int id);                   // 提交任务，对应的图片和帧的ID，帧的ID是什么是怎么初始化的？，因为多线程不知道位置，都需要帧的ID表示位置
    nn_error_e getTargetResult(std::vector<Detection> &objects, cv::Mat &img , int id); // 获取结果（检测框）只需要检测框 , 原始相片
    nn_error_e getTargetImgResult(cv::Mat &img, int id);                 // 获取结果（图片）
    nn_error_e getRawImage(cv::Mat &img, int id);                         // 获取原始图片
    void stopAll();                         // <id, img>用来存放结果（图片）
    void clear();
    cv::Mat now_picture;
    int now_id;

    size_t map_size();

    std::queue<cv::Mat> imageQueue;

};

#endif