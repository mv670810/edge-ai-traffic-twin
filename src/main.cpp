#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>

// OpenCV & TensorRT
#include <opencv2/opencv.hpp>
#include <NvInfer.h>
#include <cuda_runtime_api.h>

// MQTT & JSON
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace nvinfer1;

// --- 參數設定 ---
const std::string ENGINE_PATH = "/app/data/yolov8n.engine";
// 使用 localhost，因為我們將使用 --net=host 模式運行容器
const std::string RTSP_URL = "rtsp://127.0.0.1:8554/live"; 
const std::string MQTT_SERVER = "tcp://127.0.0.1:1883";
const std::string MQTT_TOPIC = "traffic/sensor1";

// YOLOv8 輸入尺寸
const int INPUT_W = 640;
const int INPUT_H = 640;

// TensorRT Logger
class Logger : public ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING) std::cout << "[TRT] " << msg << std::endl;
    }
} gLogger;

int main() {
    std::cout << "=== AI Traffic Sensor Starting ===" << std::endl;

    // 1. 初始化 MQTT
    std::cout << ">>> Connecting to MQTT Broker..." << std::endl;
    mqtt::async_client cli(MQTT_SERVER, "traffic_ai_node");
    try {
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        cli.connect(connOpts)->wait();
        std::cout << ">>> MQTT Connected!" << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "!!! MQTT Connection Failed: " << exc.what() << std::endl;
        std::cerr << "    (Is Mosquitto running?)" << std::endl;
        return 1;
    }

    // 2. 載入 TensorRT Engine
    std::cout << ">>> Loading TensorRT Engine..." << std::endl;
    std::ifstream file(ENGINE_PATH, std::ios::binary);
    if (!file.good()) {
        std::cerr << "!!! Error loading engine file: " << ENGINE_PATH << std::endl;
        return 1;
    }
    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);
    std::vector<char> engineData(size);
    file.read(engineData.data(), size);

    std::unique_ptr<IRuntime> runtime(createInferRuntime(gLogger));
    std::unique_ptr<ICudaEngine> engine(runtime->deserializeCudaEngine(engineData.data(), size));
    std::unique_ptr<IExecutionContext> context(engine->createExecutionContext());

    // 3. 分配 GPU 記憶體
    void* buffers[2]; // 0: Input, 1: Output
    // Input: 1x3x640x640
    size_t inputSize = 1 * 3 * INPUT_H * INPUT_W * sizeof(float);
    cudaMalloc(&buffers[0], inputSize);

    // Output: YOLOv8 output [1, 84, 8400]
    int outputElements = 84 * 8400;
    size_t outputSize = outputElements * sizeof(float);
    cudaMalloc(&buffers[1], outputSize);
    
    std::vector<float> cpuOutput(outputElements);

    // 4. 開啟 RTSP 串流
    cv::VideoCapture cap(RTSP_URL);
    // 如果連不到 RTSP，自動切換回本機檔案模式 (方便測試)
    if (!cap.isOpened()) {
        std::cout << "!!! Cannot open RTSP stream: " << RTSP_URL << std::endl;
        std::cout << ">>> Switching to local video file for testing..." << std::endl;
        cap.open("../data/test_video.mp4");
        if(!cap.isOpened()) {
             std::cerr << "!!! Cannot open video file either." << std::endl;
             return 1;
        }
    }
    
    cv::Mat frame;
    std::cout << ">>> Starting Inference Loop..." << std::endl;

    while (cap.read(frame)) {
        auto start = std::chrono::high_resolution_clock::now();

        // --- Pre-process ---
        cv::Mat blob;
        cv::dnn::blobFromImage(frame, blob, 1.0/255.0, cv::Size(INPUT_W, INPUT_H), cv::Scalar(), true, false);
        cudaMemcpy(buffers[0], blob.ptr<float>(), inputSize, cudaMemcpyHostToDevice);

        // --- Inference ---
        context->enqueueV2(buffers, 0, nullptr);

        // --- Post-process ---
        cudaMemcpy(cpuOutput.data(), buffers[1], outputSize, cudaMemcpyDeviceToHost);

        // 解析 YOLO 輸出 (簡化版: 只取信心度 > 0.5 的物件)
        std::vector<int> indices;
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;

        float xFactor = (float)frame.cols / INPUT_W;
        float yFactor = (float)frame.rows / INPUT_H;
        float* data = cpuOutput.data();

        // YOLOv8 output layout: [class0_x, class0_y, ...][class1_x...]
        // 需要轉置處理，這裡為了效能簡化，直接掃描
        for (int i = 0; i < 8400; ++i) {
            float maxScore = 0.0f;
            int classId = -1;
            // 找最大信心度的類別 (4~83)
            for (int k = 4; k < 84; ++k) {
                float score = data[k * 8400 + i];
                if (score > maxScore) {
                    maxScore = score;
                    classId = k - 4;
                }
            }

            if (maxScore > 0.5) {
                float cx = data[0 * 8400 + i];
                float cy = data[1 * 8400 + i];
                float w  = data[2 * 8400 + i];
                float h  = data[3 * 8400 + i];

                int left = int((cx - 0.5 * w) * xFactor);
                int top  = int((cy - 0.5 * h) * yFactor);
                int width = int(w * xFactor);
                int height = int(h * yFactor);

                boxes.push_back(cv::Rect(left, top, width, height));
                confidences.push_back(maxScore);
                classIds.push_back(classId);
            }
        }
        
        cv::dnn::NMSBoxes(boxes, confidences, 0.5, 0.45, indices);

        // --- MQTT Publish ---
        if (indices.size() > 0) {
            json j;
            j["t"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            j["objs"] = json::array();
            
            for (int idx : indices) {
                json obj;
                obj["c"] = classIds[idx]; // Class ID
                obj["s"] = confidences[idx]; // Score
                // 簡單計算中心點
                obj["x"] = boxes[idx].x + boxes[idx].width / 2;
                obj["y"] = boxes[idx].y + boxes[idx].height / 2;
                j["objs"].push_back(obj);
            }
            
            std::string payload = j.dump();
            cli.publish(MQTT_TOPIC, payload, 0, false);
        }

        auto end = std::chrono::high_resolution_clock::now();
        float fps = 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // 每 30 幀印一次 Log，避免洗版
        static int frameCount = 0;
        if (frameCount++ % 30 == 0) {
            std::cout << "[AI Core] FPS: " << fps << " | Objects: " << indices.size() << std::endl;
        }
    }
    
    cudaFree(buffers[0]);
    cudaFree(buffers[1]);
    return 0;
}
