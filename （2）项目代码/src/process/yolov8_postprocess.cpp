// // Copyright (c) 2021 by Rockchip Electronics Co., Ltd. All Rights Reserved.
// //
// // Licensed under the Apache License, Version 2.0 (the "License");
// // you may not use this file except in compliance with the License.
// // You may obtain a copy of the License at
// //
// //     http://www.apache.org/licenses/LICENSE-2.0
// //
// // Unless required by applicable law or agreed to in writing, software
// // distributed under the License is distributed on an "AS IS" BASIS,
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// // See the License for the specific language governing permissions and
// // limitations under the License.

// #include "yolov8_postprocess.h"

// #include <math.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/time.h>

// #include <set>
// #include <vector>
// namespace yolov8
// {

// #define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"

//       static const char *labels[OBJ_CLASS_NUM] = {
//        "person", "vehicle" };
//     //  static const char *labels[OBJ_CLASS_NUM] = {
//     //     "person", "bicycle", "car", "motorbike ", "aeroplane ", "bus ", "train", "truck ", "boat", "traffic light",
//     //     "fire hydrant", "stop sign ", "parking meter", "bench", "bird", "cat", "dog ", "horse ", "sheep", "cow", "elephant",
//     //     "bear", "zebra ", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite",
//     //     "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife ",
//     //     "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza ", "donut", "cake", "chair", "sofa",
//     //     "pottedplant", "bed", "diningtable", "toilet ", "tvmonitor", "laptop	", "mouse	", "remote ", "keyboard ", "cell phone", "microwave ",
//     //     "oven ", "toaster", "sink", "refrigerator ", "book", "clock", "vase", "scissors ", "teddy bear ", "hair drier", "toothbrush "};

//     // const int anchor0[6] = {10, 13, 16, 30, 33, 23};
//     // const int anchor1[6] = {30, 61, 62, 45, 59, 119};
//     // const int anchor2[6] = {116, 90, 156, 198, 373, 326};

//     inline static int clamp(float val, int min, int max) { return val > min ? (val < max ? val : max) : min; }

//     char *readLine(FILE *fp, char *buffer, int *len)
//     {
//         int ch;
//         int i = 0;
//         size_t buff_len = 0;

//         buffer = (char *)malloc(buff_len + 1);
//         if (!buffer)
//             return NULL; // Out of memory

//         while ((ch = fgetc(fp)) != '\n' && ch != EOF)
//         {
//             buff_len++;
//             void *tmp = realloc(buffer, buff_len + 1);
//             if (tmp == NULL)
//             {
//                 free(buffer);
//                 return NULL; // Out of memory
//             }
//             buffer = (char *)tmp;

//             buffer[i] = (char)ch;
//             i++;
//         }
//         buffer[i] = '\0';

//         *len = buff_len;

//         // Detect end
//         if (ch == EOF && (i == 0 || ferror(fp)))
//         {
//             free(buffer);
//             return NULL;
//         }
//         return buffer;
//     }

//     int readLines(const char *fileName, char *lines[], int max_line)
//     {
//         FILE *file = fopen(fileName, "r");
//         char *s;
//         int i = 0;
//         int n = 0;

//         if (file == NULL)
//         {
//             printf("Open %s fail!\n", fileName);
//             return -1;
//         }

//         while ((s = readLine(file, s, &n)) != NULL)
//         {
//             lines[i++] = s;
//             if (i >= max_line)
//                 break;
//         }
//         fclose(file);
//         return i;
//     }

//     int loadLabelName(const char *locationFilename, char *label[])
//     {
//         printf("loadLabelName %s\n", locationFilename);
//         readLines(locationFilename, label, OBJ_CLASS_NUM);
//         return 0;
//     }

//     static float
//     CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
//                      float ymax1)
//     {
//         float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
//         float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
//         float i = w * h;
//         float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
//         return u <= 0.f ? 0.f : (i / u);
//     }
//     void compute_dfl(float* tensor, int dfl_len, float* box){
//         for (int b=0; b<4; b++){
//             float exp_t[dfl_len];
//             float exp_sum=0;
//             float acc_sum=0;
//             for (int i=0; i< dfl_len; i++){
//                 exp_t[i] = exp(tensor[i+b*dfl_len]);
//                 exp_sum += exp_t[i];
//             }
            
//             for (int i=0; i< dfl_len; i++){
//                 acc_sum += exp_t[i]/exp_sum *i;
//             }
//             box[b] = acc_sum;
//         }
// }
//     static int
//     nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
//         int filterId, float threshold)
//     {
//         for (int i = 0; i < validCount; ++i)
//         {
//             if (order[i] == -1 || classIds[i] != filterId)
//             {
//                 continue;
//             }
//             int n = order[i];
//             for (int j = i + 1; j < validCount; ++j)
//             {
//                 int m = order[j];
//                 if (m == -1 || classIds[i] != filterId)
//                 {
//                     continue;
//                 }
//                 float xmin0 = outputLocations[n * 4 + 0];
//                 float ymin0 = outputLocations[n * 4 + 1];
//                 float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
//                 float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

//                 float xmin1 = outputLocations[m * 4 + 0];
//                 float ymin1 = outputLocations[m * 4 + 1];
//                 float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
//                 float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

//                 float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

//                 if (iou > threshold)
//                 {
//                     order[j] = -1;
//                 }
//             }
//         }
//         return 0;
//     }

//     static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
//     {
//         float key;
//         int key_index;
//         int low = left;
//         int high = right;
//         if (left < right)
//         {
//             key_index = indices[left];
//             key = input[left];
//             while (low < high)
//             {
//                 while (low < high && input[high] <= key)
//                 {
//                     high--;
//                 }
//                 input[low] = input[high];
//                 indices[low] = indices[high];
//                 while (low < high && input[low] >= key)
//                 {
//                     low++;
//                 }
//                 input[high] = input[low];
//                 indices[high] = indices[low];
//             }
//             input[low] = key;
//             indices[low] = key_index;
//             quick_sort_indice_inverse(input, left, low - 1, indices);
//             quick_sort_indice_inverse(input, low + 1, right, indices);
//         }
//         return low;
//     }

//     static float sigmoid(float x) { return 1.0 / (1.0 + expf(-x)); }

//     static float unsigmoid(float y) { return -1.0 * logf((1.0 / y) - 1.0); }

//     inline static int32_t __clip(float val, float min, float max)
//     {
//         float f = val <= min ? min : (val >= max ? max : val);
//         return f;
//     }

//     static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
//     {
//         float dst_val = (f32 / scale) + zp;
//         int8_t res = (int8_t)__clip(dst_val, -128, 127);
//         return res;
//     }
//     //     static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
//     // {
//     //     float dst_val = (f32 / scale) + zp;
//     //     int8_t res = (int8_t)__clip(dst_val, -256, 255);
//     //     return res;
//     // }

//     static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }

//     static int process_i8(int8_t *box_tensor, int32_t box_zp, float box_scale,
//                         int8_t *score_tensor, int32_t score_zp, float score_scale,
//                         int8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
//                         int grid_h, int grid_w, int stride, int dfl_len,
//                         std::vector<float> &boxes, 
//                         std::vector<float> &objProbs, 
//                         std::vector<int> &classId, 
//                         float threshold)
//     {
//         int validCount = 0;
//         int grid_len = grid_h * grid_w;
//         int8_t score_thres_i8 = qnt_f32_to_affine(threshold, score_zp, score_scale);
//         int8_t score_sum_thres_i8 = qnt_f32_to_affine(threshold, score_sum_zp, score_sum_scale);

//         for (int i = 0; i < grid_h; i++)
//         {
//             for (int j = 0; j < grid_w; j++)
//             {
//                 int offset = i* grid_w + j;
//                 int max_class_id = -1;

//                 // 通过 score sum 起到快速过滤的作用
//                 if (score_sum_tensor != nullptr){
//                     if (score_sum_tensor[offset] < score_sum_thres_i8){
//                         continue;
//                     }
//                 }

//                 int8_t max_score = -score_zp;
//                 for (int c= 0; c< OBJ_CLASS_NUM; c++){
//                     if ((score_tensor[offset] > score_thres_i8) && (score_tensor[offset] > max_score))
//                     {
//                         max_score = score_tensor[offset];
//                         max_class_id = c;
//                     }
//                     offset += grid_len;
//                 }

//                 // compute box
//                 if (max_score> score_thres_i8){
//                     offset = i* grid_w + j;
//                     float box[4];
//                     float before_dfl[dfl_len*4];
//                     for (int k=0; k< dfl_len*4; k++){
//                         before_dfl[k] = deqnt_affine_to_f32(box_tensor[offset], box_zp, box_scale);
//                         offset += grid_len;
//                     }
//                     compute_dfl(before_dfl, dfl_len, box);

//                     float x1,y1,x2,y2,w,h;
//                     x1 = (-box[0] + j + 0.5)*stride;
//                     y1 = (-box[1] + i + 0.5)*stride;
//                     x2 = (box[2] + j + 0.5)*stride;
//                     y2 = (box[3] + i + 0.5)*stride;
//                     w = x2 - x1;
//                     h = y2 - y1;
//                     boxes.push_back(x1);
//                     boxes.push_back(y1);
//                     boxes.push_back(w);
//                     boxes.push_back(h);

//                     objProbs.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
//                     classId.push_back(max_class_id);
//                     validCount ++;
//                 }
//             }
//         }
//         return validCount;
//     }

//     int
//     post_process(std::vector<tensor_data_s>& output_attrs ,int model_in_h, int model_in_w, float conf_threshold,
//                  float nms_threshold,float scale_w, float scale_h,std::vector<int32_t> &qnt_zps,
//                  std::vector<float> &qnt_scales, detect_result_group_t *group)
//     {
//         static int init = -1;
//         if (init == -1)
//         {
//             int ret = 0;
//             //            ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
//             if (ret < 0)
//             {
//                 return -1;
//             }

//             init = 0;
//         }
//         memset(group, 0, sizeof(detect_result_group_t));

//         std::vector<float> filterBoxes;
//         std::vector<float> objProbs;
//         std::vector<int> classId;
//         int validCount = 0;
//         int stride = 0;
//         int grid_h = 0;
//         int grid_w = 0;
//         // default 3 branch 循环处理三个分支
//         //dfl参数的长度，于位置相关，模型输出的第二个参数4*R,4*R/4=16
//         int dfl_len = output_attrs[0].attr.dims[1] /4;
//         //总输出张量数量的三分之一,为三个分支 = 9 /3 = 3
        
//         int output_per_branch = 9 / 3;
//         for (int i = 0; i < 3; i++)
//         {
//             //输出向量的首地址，两个量化参数
//             void *score_sum = nullptr;
//             int32_t score_sum_zp = 0;
//             float score_sum_scale = 1.0;
//             //这个参数仅仅进行二分类是否有无的判断
//             if (output_per_branch == 3){
//                 score_sum = output_attrs[i*output_per_branch + 2].data;
//                 score_sum_zp = qnt_zps[i*output_per_branch + 2];
//                 score_sum_scale = qnt_scales[i*output_per_branch + 2];
//             }
//             int box_idx = i*output_per_branch;
//             int score_idx = i*output_per_branch + 1;

//             grid_h = output_attrs[box_idx].attr.dims[2];
//             grid_w = output_attrs[box_idx].attr.dims[3];
//             stride = model_in_h / grid_h;
//             //outputs[0]对应box位置,outputs[1]对应类别，outputs[2]对应有无
//             //将后处理得到了坐标，置信度，类别存到filterBoxes, objProbs, classId,中


//             validCount += process_i8((int8_t *)output_attrs[box_idx].data, qnt_zps[box_idx], qnt_scales[box_idx],
//                                 (int8_t *)output_attrs[score_idx].data, qnt_zps[score_idx], qnt_scales[score_idx],
//                                 (int8_t *)score_sum, score_sum_zp, score_sum_scale,
//                                 grid_h, grid_w, stride, dfl_len, 
//                                 filterBoxes, objProbs, classId, conf_threshold);
    

//         }
        
//         // no object detect
//         if (validCount <= 0)
//         {
//             return 0;
//         }

//         std::vector<int> indexArray;
//         for (int i = 0; i < validCount; ++i)
//         {
//             indexArray.push_back(i);
//         }

//         quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

//         std::set<int> class_set(std::begin(classId), std::end(classId));

//         for (auto c : class_set)
//         {
//             nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
//         }

//         int last_count = 0;
//         group->count = 0;
//         /* box valid detect target */
//         for (int i = 0; i < validCount; ++i)
//         {
//             if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
//             {
//                 continue;
//             }
//             int n = indexArray[i];

//             float x1 = filterBoxes[n * 4 + 0];
//             float y1 = filterBoxes[n * 4 + 1];
//             float x2 = x1 + filterBoxes[n * 4 + 2];
//             float y2 = y1 + filterBoxes[n * 4 + 3];
//             int id = classId[n];
//             float obj_conf = objProbs[i];

//             group->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w) / scale_w);
//             group->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h) / scale_h);
//             group->results[last_count].box.right = (int)(clamp(x2, 0, model_in_w) / scale_w);
//             group->results[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h) / scale_h);
//             group->results[last_count].prop = obj_conf;
//             const char *label = labels[id];
//             strncpy(group->results[last_count].name, label, OBJ_NAME_MAX_SIZE);

//             // printf("result %2d: (%4d, %4d, %4d, %4d), %s\n", i, group->results[last_count].box.left,
//             // group->results[last_count].box.top,
//             //        group->results[last_count].box.right, group->results[last_count].box.bottom, label);
//             last_count++;
//         }
//         group->count = last_count;

//         return 0;
//     }

//     void deinitPostProcess()
//     {
//         //        for (int i = 0; i < OBJ_CLASS_NUM; i++) {
//         //            if (labels[i] != nullptr) {
//         //                free(labels[i]);
//         //                labels[i] = nullptr;
//         //            }
//         //        }
//     }

// }

// Copyright (c) 2021 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "yolov8_postprocess.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <set>
#include <vector>
namespace yolov8
{

#define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"

      static const char *labels[OBJ_CLASS_NUM] = {
       "person", "vehicle" };
    //  static const char *labels[OBJ_CLASS_NUM] = {
    //     "person", "bicycle", "car", "motorbike ", "aeroplane ", "bus ", "train", "truck ", "boat", "traffic light",
    //     "fire hydrant", "stop sign ", "parking meter", "bench", "bird", "cat", "dog ", "horse ", "sheep", "cow", "elephant",
    //     "bear", "zebra ", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite",
    //     "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife ",
    //     "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza ", "donut", "cake", "chair", "sofa",
    //     "pottedplant", "bed", "diningtable", "toilet ", "tvmonitor", "laptop	", "mouse	", "remote ", "keyboard ", "cell phone", "microwave ",
    //     "oven ", "toaster", "sink", "refrigerator ", "book", "clock", "vase", "scissors ", "teddy bear ", "hair drier", "toothbrush "};

    // const int anchor0[6] = {10, 13, 16, 30, 33, 23};
    // const int anchor1[6] = {30, 61, 62, 45, 59, 119};
    // const int anchor2[6] = {116, 90, 156, 198, 373, 326};

    inline static int clamp(float val, int min, int max) { return val > min ? (val < max ? val : max) : min; }

    char *readLine(FILE *fp, char *buffer, int *len)
    {
        int ch;
        int i = 0;
        size_t buff_len = 0;

        buffer = (char *)malloc(buff_len + 1);
        if (!buffer)
            return NULL; // Out of memory

        while ((ch = fgetc(fp)) != '\n' && ch != EOF)
        {
            buff_len++;
            void *tmp = realloc(buffer, buff_len + 1);
            if (tmp == NULL)
            {
                free(buffer);
                return NULL; // Out of memory
            }
            buffer = (char *)tmp;

            buffer[i] = (char)ch;
            i++;
        }
        buffer[i] = '\0';

        *len = buff_len;

        // Detect end
        if (ch == EOF && (i == 0 || ferror(fp)))
        {
            free(buffer);
            return NULL;
        }
        return buffer;
    }

    int readLines(const char *fileName, char *lines[], int max_line)
    {
        FILE *file = fopen(fileName, "r");
        char *s;
        int i = 0;
        int n = 0;

        if (file == NULL)
        {
            printf("Open %s fail!\n", fileName);
            return -1;
        }

        while ((s = readLine(file, s, &n)) != NULL)
        {
            lines[i++] = s;
            if (i >= max_line)
                break;
        }
        fclose(file);
        return i;
    }

    int loadLabelName(const char *locationFilename, char *label[])
    {
        printf("loadLabelName %s\n", locationFilename);
        readLines(locationFilename, label, OBJ_CLASS_NUM);
        return 0;
    }

    static float
    CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
                     float ymax1)
    {
        float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
        float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
        float i = w * h;
        float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
        return u <= 0.f ? 0.f : (i / u);
    }
    void compute_dfl(float* tensor, int dfl_len, float* box){
        for (int b=0; b<4; b++){
            float exp_t[dfl_len];
            float exp_sum=0;
            float acc_sum=0;
            for (int i=0; i< dfl_len; i++){
                exp_t[i] = exp(tensor[i+b*dfl_len]);
                exp_sum += exp_t[i];
            }
            
            for (int i=0; i< dfl_len; i++){
                acc_sum += exp_t[i]/exp_sum *i;
            }
            box[b] = acc_sum;
        }
}
    static int
    nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
        int filterId, float threshold)
    {
        for (int i = 0; i < validCount; ++i)
        {
            if (order[i] == -1 || classIds[i] != filterId)
            {
                continue;
            }
            int n = order[i];
            for (int j = i + 1; j < validCount; ++j)
            {
                int m = order[j];
                if (m == -1 || classIds[i] != filterId)
                {
                    continue;
                }
                float xmin0 = outputLocations[n * 4 + 0];
                float ymin0 = outputLocations[n * 4 + 1];
                float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
                float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

                float xmin1 = outputLocations[m * 4 + 0];
                float ymin1 = outputLocations[m * 4 + 1];
                float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
                float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

                float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

                if (iou > threshold)
                {
                    order[j] = -1;
                }
            }
        }
        return 0;
    }

    static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
    {
        float key;
        int key_index;
        int low = left;
        int high = right;
        if (left < right)
        {
            key_index = indices[left];
            key = input[left];
            while (low < high)
            {
                while (low < high && input[high] <= key)
                {
                    high--;
                }
                input[low] = input[high];
                indices[low] = indices[high];
                while (low < high && input[low] >= key)
                {
                    low++;
                }
                input[high] = input[low];
                indices[high] = indices[low];
            }
            input[low] = key;
            indices[low] = key_index;
            quick_sort_indice_inverse(input, left, low - 1, indices);
            quick_sort_indice_inverse(input, low + 1, right, indices);
        }
        return low;
    }

    static float sigmoid(float x) { return 1.0 / (1.0 + expf(-x)); }

    static float unsigmoid(float y) { return -1.0 * logf((1.0 / y) - 1.0); }

    inline static int32_t __clip(float val, float min, float max)
    {
        float f = val <= min ? min : (val >= max ? max : val);
        return f;
    }

    static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
    {
        float dst_val = (f32 / scale) + zp;
        int8_t res = (int8_t)__clip(dst_val, -128, 127);
        return res;
    }
    //     static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
    // {
    //     float dst_val = (f32 / scale) + zp;
    //     int8_t res = (int8_t)__clip(dst_val, -256, 255);
    //     return res;
    // }

    static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }

    static int process_i8(int8_t *box_tensor, int32_t box_zp, float box_scale,
                        int8_t *score_tensor, int32_t score_zp, float score_scale,
                        int8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                        int grid_h, int grid_w, int stride, int dfl_len,
                        std::vector<float> &boxes, 
                        std::vector<float> &objProbs, 
                        std::vector<int> &classId, 
                        float threshold)
    {
        int validCount = 0;
        int grid_len = grid_h * grid_w;
        int8_t score_thres_i8 = qnt_f32_to_affine(threshold, score_zp, score_scale);
        int8_t score_sum_thres_i8 = qnt_f32_to_affine(threshold, score_sum_zp, score_sum_scale);

        for (int i = 0; i < grid_h; i++)
        {
            for (int j = 0; j < grid_w; j++)
            {
                int offset = i* grid_w + j;
                int max_class_id = -1;

                // 通过 score sum 起到快速过滤的作用
                if (score_sum_tensor != nullptr){
                    if (score_sum_tensor[offset] < score_sum_thres_i8){
                        continue;
                    }
                }

                int8_t max_score = -score_zp;
                for (int c= 0; c< OBJ_CLASS_NUM; c++){
                    if ((score_tensor[offset] > score_thres_i8) && (score_tensor[offset] > max_score))
                    {
                        max_score = score_tensor[offset];
                        max_class_id = c;
                    }
                    offset += grid_len;
                }

                // compute box
                if (max_score> score_thres_i8){
                    offset = i* grid_w + j;
                    float box[4];
                    float before_dfl[dfl_len*4];
                    for (int k=0; k< dfl_len*4; k++){
                        before_dfl[k] = deqnt_affine_to_f32(box_tensor[offset], box_zp, box_scale);
                        offset += grid_len;
                    }
                    compute_dfl(before_dfl, dfl_len, box);

                    float x1,y1,x2,y2,w,h;
                    x1 = (-box[0] + j + 0.5)*stride;
                    y1 = (-box[1] + i + 0.5)*stride;
                    x2 = (box[2] + j + 0.5)*stride;
                    y2 = (box[3] + i + 0.5)*stride;
                    w = x2 - x1;
                    h = y2 - y1;
                    boxes.push_back(x1);
                    boxes.push_back(y1);
                    boxes.push_back(w);
                    boxes.push_back(h);

                    objProbs.push_back(deqnt_affine_to_f32(max_score, score_zp, score_scale));
                    classId.push_back(max_class_id);
                    validCount ++;
                }
            }
        }
        return validCount;
    }

    int
    post_process(std::vector<tensor_data_s>& output_attrs ,int model_in_h, int model_in_w, float conf_threshold,
                 float nms_threshold,float scale_w, float scale_h,std::vector<int32_t> &qnt_zps,
                 std::vector<float> &qnt_scales, detect_result_group_t *group)
    {
        static int init = -1;
        if (init == -1)
        {
            int ret = 0;
            //            ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
            if (ret < 0)
            {
                return -1;
            }

            init = 0;
        }
        memset(group, 0, sizeof(detect_result_group_t));

        std::vector<float> filterBoxes;
        std::vector<float> objProbs;
        std::vector<int> classId;
        int validCount = 0;
        int stride = 0;
        int grid_h = 0;
        int grid_w = 0;
        // default 3 branch 循环处理三个分支
        //dfl参数的长度，于位置相关，模型输出的第二个参数4*R,4*R/4=16
        int dfl_len = output_attrs[0].attr.dims[1] /4;
        //总输出张量数量的三分之一,为三个分支 = 9 /3 = 3
        int output_per_branch = 9 / 3;
        for (int i = 0; i < 3; i++)
        {
            //输出向量的首地址，两个量化参数
            void *score_sum = nullptr;
            int32_t score_sum_zp = 0;
            float score_sum_scale = 1.0;
            //这个参数仅仅进行二分类是否有无的判断
            if (output_per_branch == 3){
                score_sum = output_attrs[i*output_per_branch + 2].data;
                score_sum_zp = qnt_zps[i*output_per_branch + 2];
                score_sum_scale = qnt_scales[i*output_per_branch + 2];
            }
            int box_idx = i*output_per_branch;
            int score_idx = i*output_per_branch + 1;

            grid_h = output_attrs[box_idx].attr.dims[2];
            grid_w = output_attrs[box_idx].attr.dims[3];
            stride = model_in_h / grid_h;
            //outputs[0]对应box位置,outputs[1]对应类别，outputs[2]对应有无
            //将后处理得到了坐标，置信度，类别存到filterBoxes, objProbs, classId,中

            
            validCount += process_i8((int8_t *)output_attrs[box_idx].data, qnt_zps[box_idx], qnt_scales[box_idx],
                                (int8_t *)output_attrs[score_idx].data, qnt_zps[score_idx], qnt_scales[score_idx],
                                (int8_t *)score_sum, score_sum_zp, score_sum_scale,
                                grid_h, grid_w, stride, dfl_len, 
                                filterBoxes, objProbs, classId, conf_threshold);
    


        }
        
        // no object detect
        if (validCount <= 0)
        {
            return 0;
        }

        std::vector<int> indexArray;
        for (int i = 0; i < validCount; ++i)
        {
            indexArray.push_back(i);
        }

        quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

        std::set<int> class_set(std::begin(classId), std::end(classId));

        for (auto c : class_set)
        {
            nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
        }

        int last_count = 0;
        group->count = 0;
        /* box valid detect target */
        for (int i = 0; i < validCount; ++i)
        {
            if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
            {
                continue;
            }
            int n = indexArray[i];

            float x1 = filterBoxes[n * 4 + 0];
            float y1 = filterBoxes[n * 4 + 1];
            float x2 = x1 + filterBoxes[n * 4 + 2];
            float y2 = y1 + filterBoxes[n * 4 + 3];
            int id = classId[n];
            float obj_conf = objProbs[i];

            group->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w) / scale_w);
            group->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h) / scale_h);
            group->results[last_count].box.right = (int)(clamp(x2, 0, model_in_w) / scale_w);
            group->results[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h) / scale_h);
            group->results[last_count].prop = obj_conf;
            const char *label = labels[id];
            strncpy(group->results[last_count].name, label, OBJ_NAME_MAX_SIZE);

            // printf("result %2d: (%4d, %4d, %4d, %4d), %s\n", i, group->results[last_count].box.left,
            // group->results[last_count].box.top,
            //        group->results[last_count].box.right, group->results[last_count].box.bottom, label);
            last_count++;
        }
        group->count = last_count;

        return 0;
    }

    void deinitPostProcess()
    {
        //        for (int i = 0; i < OBJ_CLASS_NUM; i++) {
        //            if (labels[i] != nullptr) {
        //                free(labels[i]);
        //                labels[i] = nullptr;
        //            }
        //        }
    }

}