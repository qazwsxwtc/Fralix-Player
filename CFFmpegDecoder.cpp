#include "CFFmpegDecoder.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "CCommonTypedef.h"

// 【新增】兼容 fopen_s (如果 WriteToFile 使用了它，建议在那里修改，这里仅做包含)
#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include "WriteToFile.h" 

CFFmpegDecoder::CFFmpegDecoder()
	: m_pFormatCtx(nullptr)
	, m_pVideoCodecCtx(nullptr)
	, m_pAudioCodecCtx(nullptr)
	, m_pSwsCtx(nullptr)
	, m_pSwrCtx(nullptr)
	, m_pVideoFrame(nullptr)
	, m_pRGBFrame(nullptr)
	, m_pRGBBuffer(nullptr)
	, m_pAudioFrame(nullptr)
	, m_videoStreamIdx(-1)
	, m_audioStreamIdx(-1)
	, m_videoWidth(0)
	, m_videoHeight(0)
	, m_duration(0)
	, m_fnVideoCb(nullptr)
	, m_fnAudioCb(nullptr)
	, m_fnPlayEndCb(nullptr)
	, m_bOpened(false)
	, m_pReadThread(nullptr)
	, m_pVideoThread(nullptr)
	, m_pAudioThread(nullptr)
	, m_bStopThreads(false)
{
}

CFFmpegDecoder::~CFFmpegDecoder()
{
    Close();
}

bool CFFmpegDecoder::Open(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bOpened) Close();

    if (!InitFormatContext(filename)) return false;
    
    // 查找流信息
    if (avformat_find_stream_info(m_pFormatCtx, nullptr) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        return false;
    }

    // 获取时长
    if (m_pFormatCtx->duration != AV_NOPTS_VALUE && m_pFormatCtx->duration > 0) {
        m_duration = (double)m_pFormatCtx->duration / AV_TIME_BASE;
	}
	else {
		// 如果容器没有提供时长，尝试通过流的 duration 估算
		m_duration = 0.0;
		for (unsigned int i = 0; i < m_pFormatCtx->nb_streams; i++) {
			if (m_pFormatCtx->streams[i]->duration > 0) {
				double streamDur = (double)m_pFormatCtx->streams[i]->duration * av_q2d(m_pFormatCtx->streams[i]->time_base);
				if (streamDur > m_duration) {
					m_duration = streamDur;
				}
			}
		}		
	}

    // 查找流索引
    for (unsigned int i = 0; i < m_pFormatCtx->nb_streams; i++) {
        if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIdx == -1) {
            m_videoStreamIdx = i;
        }
        else if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIdx == -1) {
            m_audioStreamIdx = i;
        }
    }

    if (m_videoStreamIdx != -1 && !InitVideoDecoder()) {
        std::cerr << "Failed to init video decoder." << std::endl;
    }

    if (m_audioStreamIdx != -1 && !InitAudioDecoder()) {
        std::cerr << "Failed to init audio decoder." << std::endl;
    }

    m_bOpened = true;
    return true;
}

void CFFmpegDecoder::Close()
{
    StopPlayback();

    std::lock_guard<std::mutex> lock(m_mutex);
    std::lock_guard<std::mutex> readLock(m_readMutex);

    if (m_pSwsCtx) { sws_freeContext(m_pSwsCtx); m_pSwsCtx = nullptr; }
    if (m_pSwrCtx) { swr_free(&m_pSwrCtx); m_pSwrCtx = nullptr; }

    if (m_pVideoFrame) av_frame_free(&m_pVideoFrame);
    if (m_pRGBFrame) av_frame_free(&m_pRGBFrame);
    if (m_pRGBBuffer) { av_free(m_pRGBBuffer); m_pRGBBuffer = nullptr; }
    if (m_pAudioFrame) av_frame_free(&m_pAudioFrame);
    
    if (m_pVideoCodecCtx) avcodec_free_context(&m_pVideoCodecCtx);
    if (m_pAudioCodecCtx) avcodec_free_context(&m_pAudioCodecCtx);

    if (m_pFormatCtx) {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = nullptr;
    }

    m_videoStreamIdx = -1;
    m_audioStreamIdx = -1;
}

bool CFFmpegDecoder::InitFormatContext(const std::string& filename)
{
    m_pFormatCtx = avformat_alloc_context();
    // FFmpeg 在 Linux/macOS 上也能正确处理 UTF-8 字符串
    if (avformat_open_input(&m_pFormatCtx, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return false;
    }
    return true;
}

bool CFFmpegDecoder::InitVideoDecoder()
{
    AVCodecParameters* codecParams = m_pFormatCtx->streams[m_videoStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Unsupported video codec!" << std::endl;
        return false;
    }

    m_pVideoCodecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(m_pVideoCodecCtx, codecParams) < 0) return false;
    if (avcodec_open2(m_pVideoCodecCtx, codec, nullptr) < 0) return false;

    m_videoWidth = m_pVideoCodecCtx->width;
    m_videoHeight = m_pVideoCodecCtx->height;

    m_pVideoFrame = av_frame_alloc();
    
    m_pRGBFrame = av_frame_alloc();
    m_pRGBFrame->format = AV_PIX_FMT_BGR24;
    m_pRGBFrame->width = m_videoWidth;
    m_pRGBFrame->height = m_videoHeight;
    
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, m_videoWidth, m_videoHeight, 1);
    m_pRGBBuffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(m_pRGBFrame->data, m_pRGBFrame->linesize, m_pRGBBuffer, 
        AV_PIX_FMT_BGR24, m_videoWidth, m_videoHeight, 1);

    m_pSwsCtx = sws_getContext(
        m_videoWidth, m_videoHeight, m_pVideoCodecCtx->pix_fmt,
        m_videoWidth, m_videoHeight, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    return true;
}

bool CFFmpegDecoder::InitAudioDecoder()
{
    AVCodecParameters* codecParams = m_pFormatCtx->streams[m_audioStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) return false;

    m_pAudioCodecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(m_pAudioCodecCtx, codecParams) < 0) return false;
    if (avcodec_open2(m_pAudioCodecCtx, codec, nullptr) < 0) return false;

    m_pAudioFrame = av_frame_alloc();

	// 分配重采样上下文
	m_pSwrCtx = swr_alloc();
	if (!m_pSwrCtx) return false;

	// 定义输出布局：立体声
	AVChannelLayout out_ch_layout;
	av_channel_layout_default(&out_ch_layout, 2);

	// 【FFmpeg 8.x 兼容写法】使用 av_opt_set_chlayout 和 av_opt_set_int
	// 注意：如果 av_opt_set_chlayout 报错，请检查是否包含了 <libavutil/opt.h> 
	// 并且链接了 avutil.lib
    int ret = 0;
	
	// 设置输入参数
    ret = av_opt_set_chlayout(m_pSwrCtx, "in_chlayout", &m_pAudioCodecCtx->ch_layout, 0);
    if (ret < 0) { swr_free(&m_pSwrCtx); return false; }

    ret = av_opt_set_int(m_pSwrCtx, "in_sample_rate", m_pAudioCodecCtx->sample_rate, 0);
    if (ret < 0) { swr_free(&m_pSwrCtx); return false; }

    ret = av_opt_set_sample_fmt(m_pSwrCtx, "in_sample_fmt", m_pAudioCodecCtx->sample_fmt, 0);
    if (ret < 0) { swr_free(&m_pSwrCtx); return false; }

    ret = av_opt_set_chlayout(m_pSwrCtx, "out_chlayout", &out_ch_layout, 0);
    if (ret < 0) { swr_free(&m_pSwrCtx); return false; }

    ret = av_opt_set_int(m_pSwrCtx, "out_sample_rate", 44100, 0);
    if (ret < 0) { swr_free(&m_pSwrCtx); return false; }

    ret = av_opt_set_sample_fmt(m_pSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    if (ret < 0) { swr_free(&m_pSwrCtx); return false; }

	// 初始化重采样器
    if (swr_init(m_pSwrCtx) < 0) {
        swr_free(&m_pSwrCtx);
        return false;
    }

    return true;
}

bool CFFmpegDecoder::ConvertVideoFrame(AVFrame* pFrame)
{
    if (!m_pSwsCtx) return false;

    sws_scale(m_pSwsCtx, pFrame->data, pFrame->linesize, 0, m_videoHeight,
              m_pRGBFrame->data, m_pRGBFrame->linesize);

    if (m_fnVideoCb) {
        m_fnVideoCb(m_pRGBBuffer, m_videoWidth, m_videoHeight, m_pRGBFrame->linesize[0]);
    }
    return true;
}

bool CFFmpegDecoder::ResampleAudioFrame(AVFrame* pFrame)
{
    if (!m_pSwrCtx || !pFrame) return false;

	// 1. 计算目标样本数
    int dst_nb_samples = av_rescale_rnd(swr_get_delay(m_pSwrCtx, pFrame->sample_rate) + pFrame->nb_samples,
        44100, pFrame->sample_rate, AV_ROUND_UP);

    if (dst_nb_samples <= 0) return false;

	// 2. 准备输出缓冲区
	// 注意：因为我们要的是 S16 Packed，所以只需要一个指针数组
	//uint8_t* outData[1];
	int bufferSize = dst_nb_samples * 2 * 2; // samples * channels(2) * bytes_per_sample(2)

	// 使用临时缓冲区，避免多线程冲突
    uint8_t* tempBuffer = (uint8_t*)av_malloc(bufferSize);
    if (!tempBuffer) return false;
    memset(tempBuffer, 0, bufferSize);

	// 3. 执行重采样
	// swr_convert 会自动处理 Planar -> Packed 的转换，只要 out_sample_fmt 设置为 S16
    int len = swr_convert(m_pSwrCtx, &tempBuffer, dst_nb_samples,
        (const uint8_t**)pFrame->extended_data, pFrame->nb_samples);

    if (len > 0) {
        int dataSize = len * 2 * 2;
		// 4. 发送给播放器
        m_fnAudioCb(tempBuffer, dataSize, 2, 44100);
    }

    av_free(tempBuffer);
    return true;
}

void CFFmpegDecoder::SetVideoCallback(VideoFrameCallback cb)
{
    m_fnVideoCb = cb;
}

void CFFmpegDecoder::SetAudioCallback(AudioFrameCallback cb)
{
    m_fnAudioCb = cb;
}

void CFFmpegDecoder::SetPlayEndCallback(PlayEndCallback cb)
{
    m_fnPlayEndCb = cb;
}

bool CFFmpegDecoder::StartPlayback()
{
    if (!m_bOpened) return false;

    m_bStopThreads = false;

	m_videoShowQueue.setMaxSize(MAX_VIDEO_FRAME_QUEUE_SIZE);
	m_audioPlayQueue.setMaxSize(MAX_AUDIO_FRAME_QUEUE_SIZE);
	m_videoQueue.setMaxSize(MAX_VIDEO_PACKET_QUEUE_SIZE);
	m_audioQueue.setMaxSize(MAX_AUDIO_PACKET_QUEUE_SIZE);
    
    m_videoQueue.clear();
    m_audioQueue.clear();
    m_videoShowQueue.clear();
    m_audioPlayQueue.clear();

    m_startTime = std::chrono::steady_clock::now();

    m_pReadThread = new std::thread([this]() { this->ReadLoop(); });
    m_pVideoThread = new std::thread([this]() { this->VideoDecodeLoop(); });
    if (m_audioStreamIdx != -1) {
        m_pAudioThread = new std::thread([this]() { this->AudioDecodeLoop(); });
    }
    m_pVideoShowThread = new std::thread([this]() { this->VideoShowLoop(); });
    if (m_audioStreamIdx != -1) {
        m_pAudioPlayThread = new std::thread([this]() { this->AudioPlayLoop(); });
    }

    return true;
}

void CFFmpegDecoder::StopPlayback()
{
	// 1. 设置停止标志
	m_bStopThreads = true;
	m_bOpened = false;
	// 【关键】唤醒可能因 Seek 而暂停的 ReadLoop
	{
		std::lock_guard<std::mutex> lock(m_seekControlMutex);
		m_isReadPaused = false;
		m_seekCondVar.notify_all();
	}
	// 2. 唤醒所有队列 (这会让 pop/push 立即返回)
	m_videoQueue.abort();
	m_audioQueue.abort();
	m_videoShowQueue.abort();
	m_audioPlayQueue.abort();

	// 3. 【关键顺序】先等待所有解码和播放线程完全退出
	// 这样可以确保没有线程再访问 SDL 或 FFmpeg 上下文
    auto waitAndDelete = [](std::thread*& t) {
        if (t) {
            if (t->joinable()) t->join();
            delete t;
            t = nullptr;
        }
    };

    waitAndDelete(m_pReadThread);
    waitAndDelete(m_pVideoThread);
    waitAndDelete(m_pAudioThread);
    waitAndDelete(m_pVideoShowThread);
    waitAndDelete(m_pAudioPlayThread);

	// 4. 【关键】所有线程都死后，再关闭 SDL 音频设备
    if (m_pAudioPlayer) {
        m_pAudioPlayer->Stop();
    }
}

void CFFmpegDecoder::ReadLoop()
{
    AVPacket* packet = av_packet_alloc();
    if (!packet) return;

    bool normalEof = false;

    while (!m_bStopThreads && m_bOpened) {
        // 【关键修改】检查是否需要暂停读取
        {
			// 【新增】检查暂停状态
			if (m_bPaused.load()) {
				std::unique_lock<std::mutex> lock(m_pauseMutex);
				m_pauseCondVar.wait(lock, [this] { return !m_bPaused.load() || m_bStopThreads; });
				continue; // 被唤醒后，如果是停止信号则退出，否则继续循环
			}

            std::unique_lock<std::mutex> lock(m_seekControlMutex);
            // 如果处于暂停状态，则等待，直到 Seek 完成通知
            m_seekCondVar.wait(lock, [this]() {
                return !m_isReadPaused || m_bStopThreads || !m_bOpened;
            });

			
            
            // 如果被停止或关闭，直接退出
            if (m_bStopThreads || !m_bOpened) break;
        }

        // 【原有逻辑】获取 readMutex 保护 FFmpeg 读取操作
        // 注意：这里依然需要 m_readMutex 来保护 av_read_frame 的线程安全
        {
            std::lock_guard<std::mutex> lock(m_readMutex);
            if (!m_pFormatCtx || m_bStopThreads || !m_bOpened) break;

            int ret = av_read_frame(m_pFormatCtx, packet);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    m_videoQueue.push(nullptr);
                    m_audioQueue.push(nullptr);
                    normalEof = true;
                    break;
                } else {
                    if (m_bStopThreads || !m_bOpened) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }

            if (m_bStopThreads || !m_bOpened) {
                av_packet_unref(packet);
                break;
            }

            if (packet->stream_index == m_videoStreamIdx) {
                AVPacket* videoPkt = av_packet_clone(packet);
                if (videoPkt) {
                    m_videoQueue.push(videoPkt);
                }
            } else if (packet->stream_index == m_audioStreamIdx) {
                AVPacket* audioPkt = av_packet_clone(packet);
                if (audioPkt) {
                    m_audioQueue.push(audioPkt);
                }
            }
			
            av_packet_unref(packet);
        }
    }
    
    av_packet_free(&packet);

    if (!normalEof) {
        m_videoQueue.abort();
        m_audioQueue.abort();
    }
}

void CFFmpegDecoder::VideoShowLoop()
{
    const double AV_SYNC_THRESHOLD_MIN = 0.01;
    const double AV_SYNC_THRESHOLD_MAX = 0.1;
    int consecutiveDrops = 0;
    const int MAX_CONSECUTIVE_DROPS = 10;
    //AVFrame* frame = nullptr;
    while (!m_bStopThreads) {

		if (m_bPaused.load()) {
			std::unique_lock<std::mutex> lock(m_pauseMutex);
			m_pauseCondVar.wait(lock, [this] { return !m_bPaused.load() || m_bStopThreads; });
			continue;
		}
        std::unique_ptr<VideoFrameData> videoData;

		// 【关键】定义中断回调
		auto interruptFn = [this]() {
			return m_bPaused.load() || m_bStopThreads;
		};
		// 1. 从帧队列获取帧（支持中断）
	   // 注意：这里调用的是我们刚刚在 FrameQueue 中添加的 popWithInterrupt
        videoData = m_videoShowQueue.popWithInterrupt(interruptFn);
        //if (!m_videoShowQueue.pop(videoData)) break;
        if (!videoData) continue;

		// 获取当前的 Seek 偏移量
        double offset = m_seekOffset.load();
        double audioClock = GetAudioClock();
        
        // 音频硬件缓冲补偿
        double diff = videoData->pts - audioClock - offset + m_audioPlayAjust;

        if (diff > AV_SYNC_THRESHOLD_MIN) {
            double waitTime = diff < 1.0 ? diff : 1.0;
            if (waitTime > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(waitTime * 1000)));
            }
            if (m_bStopThreads) break;

            double newDiff = videoData->pts - GetAudioClock() - offset + m_audioPlayAjust;
            if (newDiff < -AV_SYNC_THRESHOLD_MAX) {
                if (++consecutiveDrops < MAX_CONSECUTIVE_DROPS) continue;
            }
            consecutiveDrops = 0;
        }
        else if (diff < -AV_SYNC_THRESHOLD_MAX) {
            if (++consecutiveDrops >= MAX_CONSECUTIVE_DROPS) {
                consecutiveDrops = 0;
            } else {
                continue;
            }
        } else {
            consecutiveDrops = 0;
        }

        if (m_fnVideoCb && videoData->data[0]) {
            try {
                m_fnVideoCb(videoData->data[0], videoData->width, videoData->height, videoData->linesize[0]);
            } catch (...) {
                std::cerr << "Video Callback Exception!" << std::endl;
            }
        }
    }
}

void CFFmpegDecoder::VideoDecodeLoop()
{
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = nullptr;

    while (!m_bStopThreads) {
	
		// 定义中断回调：如果暂停或停止，则中断 Pop 的等待
		auto interruptFn = [this]() {
			return m_bPaused.load() || m_bStopThreads;
		};

        AVPacket* packet = m_videoQueue.Pop(interruptFn);
        if (!packet) break;

		// 【关键】如果正在 Seek，且收到包，先发送
        avcodec_send_packet(m_pVideoCodecCtx, packet);
        av_packet_free(&packet);

        while (avcodec_receive_frame(m_pVideoCodecCtx, frame) == 0) {
			// 【关键修复】Seek 后花屏与崩溃防护
			if (m_bSeeking.load()) {
				// 检查是否为关键帧 (I帧)
				// AV_FRAME_FLAG_KEY 是新 API，frame->key_frame 是旧 API，兼容写法
				bool isKeyFrame = (frame->flags & AV_FRAME_FLAG_KEY) /*|| frame->key_frame*/;

				if (!isKeyFrame) {
					// 如果不是关键帧，丢弃！因为参考帧可能已失效
					av_frame_unref(frame);
					continue;
				}
				else {
					// 找到关键帧，解除 Seek 状态
					m_bSeeking.store(false);
				}
            }

			// 1. 计算 PTS
			double pts = 0.0;
			if (frame->pts != AV_NOPTS_VALUE) {
				AVRational tb = m_pFormatCtx->streams[m_videoStreamIdx]->time_base;
				pts = frame->pts * av_q2d(tb);
			}

			// 2. 转换格式
			sws_scale(m_pSwsCtx, frame->data, frame->linesize, 0, m_videoHeight,
				m_pRGBFrame->data, m_pRGBFrame->linesize);

			// 3. 创建数据结构
			auto videoData = std::make_unique<VideoFrameData>(m_pRGBFrame, pts);

			// 4. 推入队列
			m_videoShowQueue.push(std::move(videoData));

			av_frame_unref(frame);
		}
	}
	av_frame_free(&frame);
	m_bSeeking.store(false); // 线程退出时重置
}

void CFFmpegDecoder::AudioPlayLoop()
{
    while (!m_bStopThreads) {
		if (m_bPaused.load()) {
			std::unique_lock<std::mutex> lock(m_pauseMutex);
			m_pauseCondVar.wait(lock, [this] { return !m_bPaused.load() || m_bStopThreads; });
			continue;
		}

        std::unique_ptr<AudioFrameData> audioData;
		auto interruptFn = [this]() {
			return m_bPaused.load() || m_bStopThreads;
		};
		// 1. 从帧队列获取帧（支持中断）
	   // 注意：这里调用的是我们刚刚在 FrameQueue 中添加的 popWithInterrupt
        audioData = m_audioPlayQueue.popWithInterrupt(interruptFn);
        //if (!m_audioPlayQueue.pop(audioData)) break;
        if (!audioData) continue;

		// 1. 发送给播放器
        if (m_pAudioPlayer) {
            m_pAudioPlayer->FeedData(audioData->data, audioData->size);
        }
        else if (m_fnAudioCb) {
			// 如果没有 SDL，使用旧的回调用法
            m_fnAudioCb(audioData->data, audioData->size / 4, 2, 44100);
        }
    }
}

void CFFmpegDecoder::AudioDecodeLoop()
{
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = nullptr;

    while (!m_bStopThreads) {
		
		// 定义中断回调：如果暂停或停止，则中断 Pop 的等待
		auto interruptFn = [this]() {
			return m_bPaused.load() || m_bStopThreads;
		};
        AVPacket* packet = m_audioQueue.Pop(interruptFn);
        if (!packet) break;

        avcodec_send_packet(m_pAudioCodecCtx, packet);
        av_packet_free(&packet);

        while (avcodec_receive_frame(m_pAudioCodecCtx, frame) == 0) {
			// 【关键】Seek 后校准 Offset
            if (m_bNeedCalibrateOffset.load()) {
                if (frame->pts != AV_NOPTS_VALUE) {
                    AVRational tb = m_pFormatCtx->streams[m_audioStreamIdx]->time_base;
                    double ptsSec = frame->pts * av_q2d(tb);
					// 记录第一帧的 PTS 作为基准
                    m_bNeedCalibrateOffset.store(false);// 校准完成
                    m_seekOffset.store(ptsSec);// Offset 等于首帧 PTS
                }
            }

			// 重采样
            int dst_nb_samples = av_rescale_rnd(swr_get_delay(m_pSwrCtx, frame->sample_rate) + frame->nb_samples,
                44100, frame->sample_rate, AV_ROUND_UP);

            if (dst_nb_samples <= 0) {
                av_frame_unref(frame);
                continue;
            }

            int bufferSize = dst_nb_samples * 2 * 2;
            uint8_t* tempBuffer = (uint8_t*)av_malloc(bufferSize);
            uint8_t* outData[1] = { tempBuffer };

            int len = swr_convert(m_pSwrCtx, outData, dst_nb_samples,
                (const uint8_t**)frame->extended_data, frame->nb_samples);

            if (len > 0) {
                int dataSize = len * 2 * 2;
                auto audioData = std::make_unique<AudioFrameData>(tempBuffer, dataSize, 0.0, true);
                m_audioPlayQueue.push(std::move(audioData));
            } else {
                av_free(tempBuffer);
            }
            av_frame_unref(frame);
        }
    }
    av_frame_free(&frame);
}

void CFFmpegDecoder::SetAudioSpeed(double speed)
{
    if (speed <= 0.0) speed = 0.01;// 防止除零或负数
    m_audioSpeed.store(speed);
}

bool CFFmpegDecoder::Seek(int64_t targetMs)
{
    if (!m_pFormatCtx || !m_bOpened) return false;

    Resume();


    // 1. 【阻塞 ReadLoop】
    // 锁定控制互斥量，设置暂停标志
    {
        std::lock_guard<std::mutex> controlLock(m_seekControlMutex);
        m_isReadPaused = true;
        // 注意：此时 ReadLoop 如果正在运行，会在下一次循环开始时阻塞在 wait()
        // 如果 ReadLoop 正持有 m_readMutex，我们需要确保它释放后才能继续
    }

    // 2. 【等待 ReadLoop 释放 m_readMutex】
    // 由于 ReadLoop 内部有 m_readMutex，我们需要获取它才能确保 ReadLoop 不在读取中
    // 这一步是隐式的：当我们下面获取 m_readMutex 时，如果 ReadLoop 还拿着它，我们会阻塞在这里
    // 一旦我们拿到了 m_readMutex，说明 ReadLoop 已经退出了它的临界区，并且由于 m_isReadPaused=true，它不会再进入
    std::lock_guard<std::mutex> lock(m_readMutex);

    // 3. 【执行 Seek 核心操作】
    
    // 设置标志位
    m_bNeedCalibrateOffset.store(true);
    m_bSeeking.store(true);
    m_audioClock.store(0.0);

    // 清空队列
    m_videoQueue.clear();
    m_audioQueue.clear();
    m_videoShowQueue.clear();
    m_audioPlayQueue.clear();

    // 重启音频
    if (m_pAudioPlayer) {
        m_pAudioPlayer->Restart();
    }

    // 刷新解码器
    if (m_pVideoCodecCtx) avcodec_flush_buffers(m_pVideoCodecCtx);
    if (m_pAudioCodecCtx) avcodec_flush_buffers(m_pAudioCodecCtx);

    // 执行 FFmpeg Seek
    int64_t targetTimestamp = (int64_t)targetMs * 1000;
    int ret = av_seek_frame(m_pFormatCtx, -1, targetTimestamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    if (ret < 0) {
        std::cerr << "Seek failed!" << std::endl;
        m_bSeeking.store(false);
        m_bNeedCalibrateOffset.store(false);
        
        // 即使失败，也要恢复 ReadLoop
        {
            std::lock_guard<std::mutex> controlLock(m_seekControlMutex);
            m_isReadPaused = false;
            m_seekCondVar.notify_all();
        }
        return false;
    }

    // 设置 Offset
    m_seekOffset.store((double)targetMs / 1000.0 + m_audioPlayAjust);

    // 4. 【唤醒 ReadLoop】
    {
        std::lock_guard<std::mutex> controlLock(m_seekControlMutex);
        m_isReadPaused = false;
        m_seekCondVar.notify_all(); // 唤醒等待中的 ReadLoop
    }

    // m_readMutex 会在函数结束时自动释放，ReadLoop 可以继续工作
    return true;
}

void CFFmpegDecoder::Pause()
{
	if (m_bPaused.exchange(true)) return; // 如果已经是暂停状态，直接返回
	std::cout << "[Decoder] Paused." << std::endl;
}

void CFFmpegDecoder::Resume()
{
	if (!m_bPaused.exchange(false)) return; // 如果已经是播放状态，直接返回
	m_pauseCondVar.notify_all(); // 唤醒所有等待的线程
	std::cout << "[Decoder] Resumed." << std::endl;
}

void CFFmpegDecoder::ResetPauseState()
{
	// 1. 强制设置为非暂停状态
	bool wasPaused = m_bPaused.exchange(false);

	// 2. 如果之前是暂停状态，必须唤醒所有被 condition_variable 阻塞的线程
	if (wasPaused) {
		m_pauseCondVar.notify_all();
	}
}