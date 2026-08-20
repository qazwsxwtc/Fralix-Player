#pragma once
#ifdef _MSC_VER
#include <windows.h>
#include <mmsystem.h>
#include <queue>
#include <mutex>
#include <atomic>
#include "AudioRingBuffer.h"

class CAudioPlayerWin {
public:
	CAudioPlayerWin();
	~CAudioPlayerWin();

	bool Init(int sampleRate, int channels);
	void Stop();
	void FeedData(const uint8_t* data, int size);
	double GetPlayedSeconds() const;

private:
	static void CALLBACK WaveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	void PlaybackLoop();

	HWAVEOUT m_hWaveOut;
	WAVEFORMATEX m_wfex;

	AudioRingBuffer m_ringBuffer;
	std::thread* m_pThread;
	std::atomic<bool> m_bPlaying;

	// 【关键】使用两个 Header 交替发送，确保连续性
	static const int HEADER_COUNT = 2;
	WAVEHDR m_headers[HEADER_COUNT];
	// 每个 Header 的缓冲区大小，建议 4KB - 8KB
	static const int BUFFER_SIZE = 4096;
	uint8_t m_headerBuffers[HEADER_COUNT][BUFFER_SIZE];
};

#endif // _MSC_VER