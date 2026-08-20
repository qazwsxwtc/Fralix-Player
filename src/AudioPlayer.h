#ifndef _AUDIOPLAYER_H
#define _AUDIOPLAYER_H

// 【跨平台兼容】确保 SDL3 头文件正确包含
// 在 CMake 中通过 target_include_directories 设置 SDL3 的 include 路径后，
// #include <SDL3/SDL.h> 在 Windows, Linux, macOS 上均有效。
#include <SDL3/SDL.h>

#include <cstdint>
#include <atomic>
#include <mutex> // 虽然当前未直接使用，但作为音频播放器类，未来可能需要保护内部状态

class CAudioPlayer {
public:
    CAudioPlayer();
    ~CAudioPlayer();

    // 初始化音频设备
    // sampleRate: 采样率 (如 44100)
    // channels: 通道数 (如 2)
    bool Init(int sampleRate, int channels);
    
    // 停止并关闭音频设备
    void Stop();
    
    // 向音频流送入 PCM 数据
    // data: PCM 数据指针
    // size: 数据大小（字节）
    void FeedData(const uint8_t* data, int size);
    
    // 获取基于发送字节数计算的已播放秒数（不含硬件缓冲延迟修正）
    double GetPlayedSeconds() const;

    // 获取精确的音频时钟（建议用于音视频同步）
    // 注意：此方法可能包含对 SDL 状态的查询，建议在非实时线程或低频调用
    double GetPreciseClock();
    
    // 清空软件缓冲区，但不重置设备
    void Flush();
    
    // 【新增】彻底重启音频设备以清除硬件缓冲
    // 用于 Seek 操作后彻底消除残留声音
    void Restart(); 
    
    // 【新增】获取当前音频硬件/软件缓冲区的延迟时间（秒）
    // 用于动态补偿音视频同步
    double GetCurrentLatencySeconds() const;

	// 【新增】暂停和恢复接口
	void Pause();
	void Resume();

	void SetVolume(float vol); // vol 范围 0.0 - 1.0
	float GetVolume() const { return m_volume; }
    void OnAudioData(uint8_t* data, int size);
private:
    SDL_AudioDeviceID m_audioDevID = 0;
    SDL_AudioStream* m_pAudioStream = nullptr;
    SDL_AudioSpec     m_spec{}; // 使用 {} 初始化，确保跨平台兼容性
    
    std::atomic<bool>   m_isInitialized{false};
    std::atomic<uint64_t> m_totalBytesSent{0}; // 总共送入 SDL 流的字节数
    bool m_bPaused = false; // 【新增】记录暂停状态

    // 用于计算精确时钟的缓存变量
    // mutable 允许在 const 成员函数中修改这些缓存值
    mutable uint64_t    m_lastQueriedBytes = 0;
    mutable double      m_lastQueriedTime  = 0.0;
    mutable Uint64      m_lastQueryTicks   = 0;
    
    mutable std::mutex m_mutex;
	int m_sampleRate = 44100;
	int m_channels = 2;
    // 【可选】如果需要更严格的线程安全，可以添加一个互斥锁保护 m_spec 等状态
    // std::mutex m_mutex;

    float m_volume = 1.0f; // 默认最大音量
};

#endif // _AUDIOPLAYER_H