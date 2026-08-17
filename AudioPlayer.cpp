#include "AudioPlayer.h"
#include <iostream>

CAudioPlayer::CAudioPlayer()
{
}

CAudioPlayer::~CAudioPlayer() {
    Stop();
}

bool CAudioPlayer::Init(int sampleRate, int channels) {
    if (m_isInitialized.load()) Stop();

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::cerr << "SDL_Init Audio Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = sampleRate;
    desiredSpec.channels = channels;
    desiredSpec.format = SDL_AUDIO_S16LE;
    
    m_audioDevID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desiredSpec);
    if (m_audioDevID == 0) {
        std::cerr << "SDL_OpenAudioDevice Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!SDL_GetAudioDeviceFormat(m_audioDevID, &m_spec, NULL)) {
         SDL_CloseAudioDevice(m_audioDevID);
         return false;
    }

    SDL_AudioSpec inputSpec;
    SDL_zero(inputSpec);
    inputSpec.freq = sampleRate;
    inputSpec.channels = channels;
    inputSpec.format = SDL_AUDIO_S16LE;

    m_pAudioStream = SDL_CreateAudioStream(&inputSpec, &m_spec);
    if (!m_pAudioStream) {
        SDL_CloseAudioDevice(m_audioDevID);
        return false;
    }

    if (!SDL_BindAudioStream(m_audioDevID, m_pAudioStream)) {
        SDL_DestroyAudioStream(m_pAudioStream);
        SDL_CloseAudioDevice(m_audioDevID);
        return false;
    }

    SDL_ResumeAudioDevice(m_audioDevID);
    
    m_totalBytesSent.store(0);
    m_lastQueriedBytes = 0;
    m_lastQueriedTime = 0.0;
    m_lastQueryTicks = SDL_GetPerformanceCounter();
    
    m_isInitialized.store(true);
    return true;
}

void CAudioPlayer::Stop() {
    if (!m_isInitialized.load()) return;
    if (m_pAudioStream) SDL_DestroyAudioStream(m_pAudioStream);
    if (m_audioDevID) SDL_CloseAudioDevice(m_audioDevID);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    m_isInitialized.store(false);
}


// 【新增】实现暂停
void CAudioPlayer::Pause()
{
	if (!m_isInitialized) return;

	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_bPaused) {
		// 第二个参数 true 表示暂停设备
		SDL_PauseAudioStreamDevice(m_pAudioStream);
		m_bPaused = true;
		std::cout << "[Audio] Paused." << std::endl;
	}
}

// 【新增】实现恢复
void CAudioPlayer::Resume()
{
	if (!m_isInitialized) return;

	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_bPaused) {
		// 第二个参数 false 表示恢复播放
		SDL_ResumeAudioStreamDevice(m_pAudioStream);
		m_bPaused = false;
		std::cout << "[Audio] Resumed." << std::endl;
	}
}



void CAudioPlayer::FeedData(const uint8_t* data, int size) {
    if (!m_isInitialized.load() || !data || size <= 0) return;
	std::lock_guard<std::mutex> lock(m_mutex);
    if (SDL_PutAudioStreamData(m_pAudioStream, data, size) < 0) {
        std::cerr << "SDL_PutAudioStreamData Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    // 【关键】只记录总发送量，不计算时间
    m_totalBytesSent.fetch_add(size);
}

double CAudioPlayer::GetPlayedSeconds() const {
	if (!m_isInitialized.load() || !m_pAudioStream) return 0.0;

	// 1. 获取已送入 SDL 流的总字节数 (输入格式)
	uint64_t sentBytes = m_totalBytesSent.load();

	// 2. 获取 SDL 内部缓冲区中尚未播放的字节数 (输出格式)
	int queuedBytes = SDL_GetAudioStreamAvailable(m_pAudioStream);
	if (queuedBytes < 0) queuedBytes = 0;

	// 3. 关键：获取输入和输出的规格，以处理重采样带来的字节数差异
	SDL_AudioSpec inSpec, outSpec;
	if (!SDL_GetAudioStreamFormat(m_pAudioStream, &inSpec, &outSpec)) {
		return 0.0;
	}

	// 计算输入和输出的字节率
	double inBytesPerSec = inSpec.freq * inSpec.channels * SDL_AUDIO_BITSIZE(inSpec.format) / 8.0;
	double outBytesPerSec = outSpec.freq * outSpec.channels * SDL_AUDIO_BITSIZE(outSpec.format) / 8.0;

	// 4. 计算时长
	// 总输入时长 - 剩余缓冲区的输出时长 = 已播放时长
	double totalInputDuration = (double)sentBytes / inBytesPerSec;
	double remainingOutputDuration = (double)queuedBytes / outBytesPerSec;

	double playedSec = totalInputDuration - remainingOutputDuration;

	// 防止负数（启动初期可能出现）
	if (playedSec < 0.0) playedSec = 0.0;

	return playedSec;
}


// 在 CAudioPlayer.cpp 中实现
double CAudioPlayer::GetPreciseClock() {
	if (!m_isInitialized.load() || !m_pAudioStream) return 0.0;

	// 1. 获取当前高性能计数器
	Uint64 nowTicks = SDL_GetPerformanceCounter();

	// 2. 获取流中剩余的字节数 (注意：这是输出格式的字节数)
	int bytesInStream = SDL_GetAudioStreamAvailable(m_pAudioStream);
	if (bytesInStream < 0) bytesInStream = 0;

	// 3. 获取总发送的输入字节数
	uint64_t sentInputBytes = m_totalBytesSent.load();

	// 【关键修正】计算输出字节率
	// 如果输入和输出采样率不同，sentInputBytes 不能直接减去 bytesInStream
	// 我们需要知道 SDL 流当前的转换比例。
	// 简单起见，我们假设 SDL3 内部处理了转换，我们可以通过查询流的实际输出来估算。

	// 更可靠的方法：使用 SDL_GetAudioStreamFormat 获取输出格式
	SDL_AudioSpec outSpec;
	if (!SDL_GetAudioStreamFormat(m_pAudioStream, NULL, &outSpec)) {
		// 如果获取失败，回退到 m_spec
		outSpec = m_spec;
	}

	double outputBytesPerSec = outSpec.freq * outSpec.channels * SDL_AUDIO_BITSIZE(outSpec.format) / 8.0;

	// 4. 估算已播放字节
	// 这里有一个难点：m_totalBytesSent 是输入字节，bytesInStream 是输出字节。
	// 除非我们知道确切的转换比例，否则直接减会出错。

	// 【替代方案】：由于 SDL3 AudioStream 是异步的，最准确的时钟其实应该由 
	// "最后放入流的时间点" + "流中剩余数据所需的播放时间" 来反推是不对的。
	// 正确的逻辑是：已播放时间 = (总发送输入字节 / 输入字节率) - (流中剩余输出字节 / 输出字节率) ? 
	// 不，这也很复杂。

	// 【最简且有效的方案】：
	// 对于大多数 PC 音频，输入输出采样率通常一致（或 SDL 自动处理）。
	// 如果存在重采样，SDL3 的 Stream 会尽量保持同步。
	// 我们改用基于“设备位置”的查询，如果 SDL3 支持的话。
	// 如果不支持，我们使用之前的逻辑，但必须确保单位一致。

	// 让我们尝试获取流的实际输出字节总数（如果 SDL3 提供此功能）。
	// 目前 SDL3 没有直接的 GetOutputBytesPlayed。

	// 回退到经过修正的字节计算：
	// 假设输入和输出时长一致。
	// 输入时长 = sentInputBytes / inputBytesPerSec
	// 剩余时长 = bytesInStream / outputBytesPerSec
	// 已播放时长 = 输入时长 - 剩余时长

	SDL_AudioSpec inSpec;
	SDL_GetAudioStreamFormat(m_pAudioStream, &inSpec, NULL);
	double inputBytesPerSec = inSpec.freq * inSpec.channels * SDL_AUDIO_BITSIZE(inSpec.format) / 8.0;

	double totalDurationSec = (double)sentInputBytes / inputBytesPerSec;
	double remainingDurationSec = (double)bytesInStream / outputBytesPerSec;

	double playedSec = totalDurationSec - remainingDurationSec;

	// 防止负数（启动初期可能出现）
	if (playedSec < 0.0) playedSec = 0.0;

	return playedSec;
}

void CAudioPlayer::Flush() {
	if (!m_isInitialized.load() || !m_pAudioStream) return;

	// 【强力模式】解绑流，清空，再重新绑定
	// 这会彻底清除硬件和软件缓冲中的所有残留数据

	// 1. 暂停设备
	SDL_PauseAudioDevice(m_audioDevID);

	// 2. 解绑流
	SDL_UnbindAudioStream(m_pAudioStream);

	// 3. 清空流
	SDL_FlushAudioStream(m_pAudioStream);

	// 4. 重新绑定
	SDL_BindAudioStream(m_audioDevID, m_pAudioStream);

	// 5. 重置时钟计数
	m_totalBytesSent.store(0);

	// 6. 恢复播放
	SDL_ResumeAudioDevice(m_audioDevID);

	std::cout << "Audio Player Flushed." << std::endl;

}

void CAudioPlayer::Restart() {
    if (!m_isInitialized.load()) return;

    // 1. 保存当前的配置参数
    int freq = m_spec.freq;
    int channels = m_spec.channels;
    // 注意：m_spec 是输出格式，我们需要知道输入格式以便重新创建 Stream
    // 假设输入格式固定为 S16LE, 44100, Stereo (根据你的 Init 函数)
    // 如果支持动态采样率，需要额外成员变量保存输入规格

    // 2. 停止并销毁现有资源
    // Stop() 内部会调用 SDL_CloseAudioDevice 和 SDL_DestroyAudioStream
    Stop(); 

    // 3. 重新初始化
    // 这里硬编码了 44100Hz Stereo S16LE，请根据你的实际需求调整
    // 如果你的播放器支持多种采样率，需要传入参数
    bool success = Init(44100, 2); 
    
    if (!success) {
        std::cerr << "Failed to restart audio device!" << std::endl;
    } else {
        std::cout << "Audio Device Restarted Successfully." << std::endl;
    }
}

double CAudioPlayer::GetCurrentLatencySeconds() const {
	if (!m_isInitialized.load() || !m_pAudioStream) return 0.0;

	// 1. 获取 SDL 流中尚未播放的字节数 (输出格式)
	int bytesInBuffer = SDL_GetAudioStreamAvailable(m_pAudioStream);
	if (bytesInBuffer < 0) bytesInBuffer = 0;

	// 2. 获取输出规格
	SDL_AudioSpec outSpec;
	if (!SDL_GetAudioStreamFormat(m_pAudioStream, NULL, &outSpec)) {
		outSpec = m_spec; // Fallback
	}

	// 3. 计算每秒字节数
	// BytesPerSecond = 采样率 * 通道数 * (位深/8)
	double bytesPerSec = outSpec.freq * outSpec.channels * (SDL_AUDIO_BITSIZE(outSpec.format) / 8.0);

	if (bytesPerSec <= 0) return 0.0;

	// 4. 计算延迟（秒）
	double latencySec = (double)bytesInBuffer / bytesPerSec;

	return latencySec;
}