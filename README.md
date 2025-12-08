# Edge-to-Cloud 3D Traffic Digital Twin System

![C++](https://img.shields.io/badge/C++-17-blue.svg?style=flat&logo=c%2B%2B)
![TensorRT](https://img.shields.io/badge/NVIDIA-TensorRT_8.6-green.svg?style=flat&logo=nvidia)
![Docker](https://img.shields.io/badge/Docker-Containerized-2496ED.svg?style=flat&logo=docker)
![Three.js](https://img.shields.io/badge/Frontend-Three.js-black.svg?style=flat&logo=three.js)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

> **A high-performance AIoT system that bridges Edge AI and Digital Twins.** > This project processes real-time traffic video streams on the edge using **C++** and **TensorRT**, achieving ultra-low latency inference (120+ FPS), and visualizes the results as a synchronized 3D Digital Twin in the browser via **MQTT**.

---

## 🚀 Key Features

* **Extreme Edge Performance:** optimized YOLOv8n inference pipeline using **C++** and **TensorRT (FP16)**, achieving **120+ FPS** on NVIDIA RTX 3060 Laptop GPU.
* **Microservices Architecture:** Fully containerized system using **Docker**, decoupling the AI Compute Node, Message Broker, and Visualization Service.
* **Real-time Digital Twin:** Interactive 3D visualization built with **Three.js**, synchronized with physical world events via **WebSockets**.
* **Robust Telemetry:** Utilizes **MQTT** for lightweight, decoupled, and reliable data transmission suitable for IoT networks.
* **Dynamic Shape Optimization:** Solved TensorRT dynamic shape compilation challenges to support variable input resolutions.

---

## 🏗️ System Architecture

The system follows a strict **Edge-to-Cloud** data flow pattern:

```mermaid
graph LR
    source["RTSP Stream Simulator"] -->|H.264 Video| ai_node("Edge AI Node (C++ / TensorRT)")
    
    subgraph Edge_Device [Edge Device]
        ai_node -->|1. Decode| opencv["OpenCV CUDA"]
        opencv -->|2. Inference| trt{"TensorRT Engine (FP16)"}
        trt -->|3. Serialize| json["JSON Payload"]
    end

    json -->|MQTT Publish| broker("Mosquitto Broker")
    
    subgraph User_Client [User Client]
        broker -->|WebSockets| dashboard["3D Web Dashboard (Three.js)"]
    end
```
* **Input Source:** Simulates an IP Camera RTSP stream.

* **Edge AI Node (C++):** Handles hardware-accelerated decoding, TensorRT inference, and data serialization.

* **Message Broker:** Eclipse Mosquitto bridging TCP (for C++) and WebSockets (for Web).

* **Digital Twin Dashboard:** Visualizes the structured data (bounding boxes, classes) in a 3D space.

---

## 🛠️ Tech Stack

|Component|Technology|
|-----|-----|
Language,"C++17, Python 3.8, JavaScript (ES6)"
AI Kernel,"NVIDIA TensorRT 8.6, CUDA 12.x, YOLOv8"
Computer Vision,OpenCV 4.5 (C++ API)
Infrastructure,"Docker, WSL2 (Ubuntu 22.04)"
Communication,"Eclipse Mosquitto (MQTT), Paho MQTT C++"
Visualization,Three.js (WebGL)

Quick Start
Prerequisites
NVIDIA GPU (Compute Capability 7.0+)

Docker & NVIDIA Container Toolkit

Linux Environment (Ubuntu 20.04+ or WSL2)

1.Clone the Repository
git clone [https://github.com/mv670810/edge-ai-traffic-twin.git](https://github.com/mv670810/edge-ai-traffic-twin.git)
cd edge-ai-traffic-twin

2.Build the AI Environment (Docker)
docker build -t traffic-cpp-env -f docker/Dockerfile.cpp .

3.Build TensorRT Engine
Place your yolov8n.onnx in the data/ folder. Then, compile the engine inside the container:

# Compiles ONNX to TensorRT Engine with FP16 precision and Dynamic Shapes support
docker run --rm --gpus all -v $(pwd):/app -w /app/src traffic-cpp-env \
    /usr/src/tensorrt/bin/trtexec \
    --onnx=/app/data/yolov8n.onnx \
    --saveEngine=/app/data/yolov8n.engine \
    --fp16 \
    --minShapes=images:1x3x640x640 \
    --optShapes=images:1x3x640x640 \
    --maxShapes=images:1x3x640x640

4.Launch the System
Step 1: Start MQTT Broker (Supports both TCP and WebSockets)
docker run -d --name mqtt-broker \
  -p 1883:1883 -p 9001:9001 \
  -v $(pwd)/docker/mosquitto/mosquitto.conf:/mosquitto/config/mosquitto.conf \
  eclipse-mosquitto

Step 2: Start Video Simulation
source .venv/bin/activate
python3 src/stream_sim.py

Step 3: Start AI Inference Node (C++)
docker run --rm --gpus all --net=host \
  -v $(pwd):/app -w /app/src \
  traffic-cpp-env \
  bash -c "mkdir -p build && cd build && cmake .. && make && ./traffic_app"

Step 4: Start Web Dashboard
cd web && python3 -m http.server 8000

Access the dashboard at http://localhost:8000.

Directory Structure
├── docker/
│   ├── Dockerfile.cpp        # C++ Development Environment Setup
│   └── mosquitto/            # MQTT Broker Configuration
├── src/
│   ├── main.cpp              # Core AI Logic (C++ & TensorRT)
│   ├── CMakeLists.txt        # Build Configuration
│   └── stream_sim.py         # RTSP Stream Simulator
├── web/
│   └── index.html            # 3D Digital Twin Interface (Three.js)
├── data/
│   └── yolov8n.onnx          # AI Model Source
└── README.md
































