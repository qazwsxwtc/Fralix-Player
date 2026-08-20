#ifndef __FFMPEG_H__
#define __FFMPEG_H__

// 【跨平台兼容】如果编译器支持 fopen_s (MSVC)，则定义宏，否则使用标准 fopen
#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS
    // MSVC 特有的安全警告禁用
#endif

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
}

#include <string>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <cstdio> // for FILE*

#include "AudioPlayer.h"
#include "PacketQueue.h"

// 回调定义
typedef std::function<void(uint8_t* data, int width, int height, int linesize)> VideoFrameCallback;
typedef std::function<void(uint8_t* data, int nb_samples, int channels, int sample_rate)> AudioFrameCallback;
typedef std::function<void()> PlayEndCallback;

// 视频帧数据结构
struct VideoFrameData {
    uint8_t* data[4];
    int linesize[4];
    int width;
    int height;
    double pts;

    VideoFrameData(AVFrame* frame, double timestamp) {
        width = frame->width;
        height = frame->height;
        pts = timestamp;

        int planes = av_pix_fmt_count_planes((AVPixelFormat)frame->format);
        if (planes <= 0) planes = 1;

        for (int i = 0; i < 4; i++) {
            linesize[i] = frame->linesize[i];
            if (i < planes && frame->data[i]) {
                data[i] = new uint8_t[frame->linesize[i] * frame->height];
                memcpy(data[i], frame->data[i], frame->linesize[i] * frame->height);
            } else {
                data[i] = nullptr;
                linesize[i] = 0;
            }
        }
    }

    ~VideoFrameData() {
        for (int i = 0; i < 4; i++) {
            delete[] data[i];
        }
    }

    // 禁止拷贝
    VideoFrameData(const VideoFrameData&) = delete;
    VideoFrameData& operator=(const VideoFrameData&) = delete;

    // 移动构造
    VideoFrameData(VideoFrameData&& other) noexcept {
        for (int i = 0; i < 4; i++) {
            data[i] = other.data[i];
            linesize[i] = other.linesize[i];
            other.data[i] = nullptr;
        }
        width = other.width;
        height = other.height;
        pts = other.pts;
    }
};

// 音频帧数据结构
struct AudioFrameData {
    uint8_t* data;
    int size;
    double pts;
    bool ownsData;

    AudioFrameData(uint8_t* rawData, int dataSize, double timestamp, bool takeOwnership = false)
        : data(nullptr), size(dataSize), pts(timestamp), ownsData(takeOwnership)
    {
        if (takeOwnership) {
            data = rawData;
        } else {
            if (rawData) {
                data = new uint8_t[dataSize];
                memcpy(data, rawData, dataSize);
            }
        }
    }

    ~AudioFrameData() {
        if (data) {
            if (ownsData) av_free(data);
            else delete[] data;
            data = nullptr;
        }
    }

    // 禁止拷贝
    AudioFrameData(const AudioFrameData&) = delete;
    AudioFrameData& operator=(const AudioFrameData&) = delete;

    // 移动构造
    AudioFrameData(AudioFrameData&& other) noexcept
        : data(other.data), size(other.size), pts(other.pts), ownsData(other.ownsData)
    {
        other.data = nullptr;
        other.ownsData = false;
    }

    // 移动赋值
    AudioFrameData& operator=(AudioFrameData&& other) noexcept {
        if (this != &other) {
            if (data) {
                if (ownsData) av_free(data);
                else delete[] data;
            }
            data = other.data;
            size = other.size;
            pts = other.pts;
            ownsData = other.ownsData;
            other.data = nullptr;
            other.ownsData = false;
        }
        return *this;
    }
};

class CFFmpegDecoder
{
public:
    CFFmpegDecoder();
    ~CFFmpegDecoder();

    bool Open(const std::string& filename);
    void Close();

    void SetVideoCallback(VideoFrameCallback cb);
    void SetAudioCallback(AudioFrameCallback cb);
    void SetPlayEndCallback(PlayEndCallback cb);

    int GetWidth() const { return m_videoWidth; }
    int GetHeight() const { return m_videoHeight; }
    double GetDuration() const { return m_duration; }

    bool StartPlayback();
    void StopPlayback();

    void SetAudioSpeed(double speed);
    double GetAudioSpeed() const { return m_audioSpeed; }

    void SetAudioPlayer(CAudioPlayer* player) { m_pAudioPlayer = player; }

    double GetAudioClock() const {
        if (m_pAudioPlayer) return m_pAudioPlayer->GetPreciseClock();
        return m_audioClock.load();
    }

    bool Seek(int64_t targetMs);
    //void SeekNew(int64_t targetMs); // 新增 Seek 接口

    // 获取/设置音频播放延迟补偿（秒）
    double GetAudioPlayAdjust() const { return m_audioPlayAjust; }
    void SetAudioPlayAdjust(double adjustSec) { m_audioPlayAjust = adjustSec; }

	void Pause();       // 【新增】暂停
	void Resume();      // 【新增】恢复
	bool IsPaused() const { return m_bPaused; } // 【新增】获取状态

	//void SetPlaybackRate(double rate); // rate: 0.5, 1.0, 2.0 etc.
	//double GetPlaybackRate() const { return m_playbackRate; }
    double GetFps() const; // 【新增】获取视频帧率
private:
    // 线程函数
    void ReadLoop();
    void VideoDecodeLoop();
    void AudioDecodeLoop();
    void VideoShowLoop();
    void AudioPlayLoop();

    // 初始化辅助函数
    bool InitFormatContext(const std::string& filename);
    bool InitVideoDecoder();
    bool InitAudioDecoder();
    bool ConvertVideoFrame(AVFrame* pFrame);
    bool ResampleAudioFrame(AVFrame* pFrame);
    void ResetPauseState(); // 内部辅助函数
private:
    // FFmpeg 上下文
    AVFormatContext* m_pFormatCtx;
    AVCodecContext* m_pVideoCodecCtx;
    AVCodecContext* m_pAudioCodecCtx;
    SwsContext* m_pSwsCtx;
    SwrContext* m_pSwrCtx;

    // 帧缓冲
    AVFrame* m_pVideoFrame;
    AVFrame* m_pRGBFrame;
    uint8_t* m_pRGBBuffer;
    AVFrame* m_pAudioFrame;

    // 流信息
    int m_videoStreamIdx;
    int m_audioStreamIdx;
    int m_videoWidth;
    int m_videoHeight;
    double m_duration;

    // 回调
    VideoFrameCallback m_fnVideoCb = nullptr;
    AudioFrameCallback m_fnAudioCb = nullptr;
    PlayEndCallback m_fnPlayEndCb = nullptr;

    // 线程
    std::thread* m_pReadThread = nullptr;
    std::thread* m_pVideoThread = nullptr;
    std::thread* m_pAudioThread = nullptr;
    std::thread* m_pVideoShowThread = nullptr;
    std::thread* m_pAudioPlayThread = nullptr;

    // 队列
    PacketQueue m_videoQueue;
    PacketQueue m_audioQueue;
    FrameQueue<std::unique_ptr<VideoFrameData>> m_videoShowQueue;
    FrameQueue<std::unique_ptr<AudioFrameData>> m_audioPlayQueue;

    // 同步原语
    std::mutex m_mutex;
    std::mutex m_readMutex;
    std::mutex m_seekControlMutex;
    std::condition_variable m_seekCondVar;//seek ，


	std::mutex m_pauseMutex;
	std::condition_variable m_pauseCondVar;//pause ，resume 应该都是互斥的
    std::atomic<bool> m_bPaused = false; // 【新增】暂停状态


    // 状态标志
    bool m_bOpened = false;
    bool m_bStopThreads = false;
    bool m_isReadPaused = false;
    std::atomic<bool> m_bSeeking = false;
    std::atomic<bool> m_bNeedCalibrateOffset = false;
    std::atomic<bool> m_bTest = false;

    // 时钟与同步
    std::atomic<double> m_audioSpeed = 1.0;
    std::atomic<double> m_audioClock = 0.0;
    std::atomic<double> m_seekOffset = 0.0;
    std::atomic<double> m_firstAudioPts = 0.0;
    
    // 音频硬件缓冲补偿（秒），用于音视频同步
    std::atomic<double> m_audioPlayAjust = 0.04; 
    
    std::chrono::steady_clock::time_point m_startTime;

    
    // 其他
    CAudioPlayer* m_pAudioPlayer = nullptr;
    FILE* m_debugFile = nullptr;

	//double m_playbackRate = 1.0;
	//std::mutex m_rateMutex;

    double m_fps = 30.0;   // 【新增】缓存帧率，默认30
};

#endif // __FFMPEG_H__