#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <opencv2/opencv.hpp>
#include "VideoCodec.h"
#include "RtpReceiver.h"

int g_width = 1280;
int g_height = 720;

/// Frame mutex.
std::mutex g_frameMutex;
/// Sync cond variable for putting frame.
std::condition_variable g_condVar;
/// Cond variable mutex.
std::mutex g_condVarMtx;
/// Cond variable flag for read frames.
std::atomic<bool> g_condVarFlag{false};
cr::video::Frame g_sharedFrame(g_width, g_height, cr::video::Fourcc::H264);


void testRtpSenderThreadFunc();

void rtpReceiverThreadFunc();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow mainWindow;

    mainWindow.setWindowTitle("RtpPlayer");
    mainWindow.resize(g_width, g_height);
    mainWindow.show();

    // Create a label to display the video
    QLabel *label = new QLabel(&mainWindow);
    mainWindow.setCentralWidget(label);

    // Start rtp sender thread
    std::thread rtpSenderThread(testRtpSenderThreadFunc);
    rtpSenderThread.detach();

    // Start rtp receiver thread
    std::thread rtpReceiverThread(rtpReceiverThreadFunc);
    rtpReceiverThread.detach();

    cr::video::Frame frame(g_width, g_height, cr::video::Fourcc::RGB24);

    while(true)
    {
        // Check if there is no ready data and wait.
        std::unique_lock lk(g_condVarMtx);
        if (!g_condVarFlag.load())
        {
            while (!g_condVarFlag.load())
            {
                // Wait for the until notified. It can be notified by the producer thread or by the destructor.
                g_condVar.wait(lk);
            }
        }
        if (!g_condVarFlag.load())
            continue;

        // Copy frame.
        g_frameMutex.lock();
        frame = g_sharedFrame;
        g_frameMutex.unlock();
        g_condVarFlag.store(false); // Reset the flag.

        QImage img(frame.data, frame.width, frame.height, QImage::Format_RGB888);
        label->setPixmap(QPixmap::fromImage(img));
        mainWindow.repaint();
        app.processEvents();
    }
    return 0;
}

void rtpReceiverThreadFunc()
{
    RtpReceiver rtpReceiver;
    VideoCodec videoCodec;

    cr::video::Frame receivedFrame(g_width, g_height, cr::video::Fourcc::H264);
    cr::video::Frame decodedFrame(g_width, g_height, cr::video::Fourcc::RGB24);
    if (!rtpReceiver.init("127.0.0.1", 5004))
    {
        std::cout << "Failed to initialize rtp receiver" << std::endl;
        exit(-1);
    }

    while(true)
    {
        if (!rtpReceiver.getFrame(receivedFrame))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::cout << "Failed to get frame from rtp stream" << std::endl;
            continue;
        }

        if (!videoCodec.decode(receivedFrame, decodedFrame))
        {
            std::cout << "Failed to decode frame" << std::endl;
            continue;
        }

        // Copy frame.
        g_frameMutex.lock();
        g_sharedFrame = decodedFrame;
        g_frameMutex.unlock();

        std::unique_lock lk(g_condVarMtx);
        g_condVarFlag.store(true);
        g_condVar.notify_one();
        lk.unlock();
    }
}


void testRtpSenderThreadFunc()
{
    // Open video file
    std::string inputFile = "../../test.mp4";
    cv::VideoCapture cap(inputFile);
    
    if (!cap.isOpened()) 
    {
        std::cerr << "Error: Could not open video file " << inputFile << std::endl;
        exit(-1);
    }

    // Get video properties
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);

    // GStreamer pipeline for RTP streaming
    std::string pipeline = "appsrc ! "
                           "videoconvert !  video/x-raw,format=I420,width=" + std::to_string(width) +
                           ",height=" + std::to_string(height) + ",framerate=" + std::to_string(int(fps)) + "/1 ! "
                           "x264enc bitrate=500 sliced-threads=false threads=1 key-int-max=30 ! "
                           "rtph264pay config-interval=1 pt=96 ! "
                           "udpsink host=127.0.0.1 port=5004";

    // Open the video writer with the GStreamer pipeline
    cv::VideoWriter writer(pipeline, cv::CAP_GSTREAMER, 0, fps, cv::Size(width, height), true);
    
    if (!writer.isOpened()) 
    {
        std::cerr << "Error: Could not open video writer with GStreamer pipeline" << std::endl;
        exit(-1);
    }

    std::cout << "Streaming " << inputFile << " over RTP..." << std::endl;

    cv::Mat frame;
    while (true)
    {
        // Read frame from video file
        cap >> frame;
        if (frame.empty()) 
        {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            std::cout << "End of video file, restarting..." << std::endl;
            continue;
        }

        // Write frame to GStreamer pipeline
        writer.write(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // Cleanup
    cap.release();
    writer.release();
}