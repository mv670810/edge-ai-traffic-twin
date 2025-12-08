import cv2
import subprocess
import time

# 設定影片來源與 RTSP 目標位址
VIDEO_SOURCE = "data/test_video.mp4"
RTSP_URL = "rtsp://127.0.0.1:8554/live"
FPS = 30

def main():
    print(f"Opening video: {VIDEO_SOURCE}")
    cap = cv2.VideoCapture(VIDEO_SOURCE)

    # 取得影片寬高
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    print(f"Video resolution: {width}x{height}")

    # 設定 FFmpeg 指令 (將影像 pipe 到 FFmpeg 進行 RTSP 推流)
    ffmpeg_cmd = [
        'ffmpeg',
        '-y',
        '-f', 'rawvideo',
        '-vcodec', 'rawvideo',
        '-pix_fmt', 'bgr24',
        '-s', f'{width}x{height}',
        '-r', str(FPS),
        '-i', '-',
        '-c:v', 'libx264',
        '-pix_fmt', 'yuv420p',
        '-preset', 'ultrafast',
        '-f', 'rtsp',
        RTSP_URL
    ]

    # 啟動 FFmpeg subprocess
    process = subprocess.Popen(ffmpeg_cmd, stdin=subprocess.PIPE)
    print(f"Streaming to {RTSP_URL} ... (Press Ctrl+C to stop)")

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("End of video, restarting...")
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                continue

            # 在這裡可以模擬感測器數據處理 (例如加時間戳記)
            # cv2.putText(frame, str(time.time()), (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            # 將影像寫入 FFmpeg Pipe
            process.stdin.write(frame.tobytes())

            # 控制發送速度，模擬真實 FPS
            time.sleep(1/FPS)

    except KeyboardInterrupt:
        print("Stopping stream...")
    finally:
        cap.release()
        process.stdin.close()
        process.wait()

if __name__ == "__main__":
    main()
