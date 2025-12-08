FROM nvcr.io/nvidia/tensorrt:23.08-py3

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libopencv-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/


WORKDIR /tmp
RUN git clone https://github.com/eclipse/paho.mqtt.c.git && \
    cd paho.mqtt.c && \
    cmake -Bbuild -H. \
        -DPAHO_ENABLE_TESTING=OFF \
        -DPAHO_BUILD_STATIC=ON \
        -DPAHO_BUILD_SHARED=ON \
        -DPAHO_WITH_SSL=OFF \
        -DPAHO_HIGH_PERFORMANCE=ON && \
    cmake --build build/ --target install && \
    ldconfig

RUN git clone https://github.com/eclipse/paho.mqtt.cpp.git && \
    cd paho.mqtt.cpp && \
    cmake -Bbuild -H. \
        -DPAHO_BUILD_STATIC=ON \
        -DPAHO_BUILD_SHARED=ON \
        -DPAHO_BUILD_DOCUMENTATION=FALSE \
        -DPAHO_WITH_SSL=OFF && \
    cmake --build build/ --target install && \
    ldconfig

WORKDIR /app
