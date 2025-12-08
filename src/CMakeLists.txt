cmake_minimum_required(VERSION 3.10)
project(TrafficTwin)

set(CMAKE_CXX_STANDARD 17)

# --- 1. 新增: 明確指定 CUDA 路徑 (NVIDIA Docker 標準位置) ---
include_directories(/usr/local/cuda/include)
link_directories(/usr/local/cuda/lib64)

# --- 2. 尋找套件 ---
find_package(OpenCV REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# --- 3. TensorRT 路徑 ---
include_directories(/usr/include/x86_64-linux-gnu/)
link_directories(/usr/lib/x86_64-linux-gnu/)

# --- 4. 定義執行檔 ---
add_executable(traffic_app main.cpp)

# --- 5. 連結函式庫 (新增 cudart) ---
target_link_libraries(traffic_app
    ${OpenCV_LIBS}
    cudart             # <--- 關鍵：連結 CUDA Runtime
    nvinfer
    nvonnxparser
    paho-mqttpp3
    paho-mqtt3a
    nlohmann_json::nlohmann_json
)
