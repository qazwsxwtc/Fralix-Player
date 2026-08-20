#include "AudioPlayer_win.h"

#ifdef _MSC_VER

CAudioPlayerWin::CAudioPlayerWin()
	: m_ringBuffer(44100 * 2 * 2 * 0.5) // 0.5秒缓冲，平衡延迟与流畅度
	, m_pThread(nullptr)
	, m_bPlaying(false)
	, m_hWaveOut(nullptr)
{
}

CAudioPlayerWin::~CAudioPlayerWin() {
	Stop();
}

bool CAudioPlayerWin::Init(int sampleRate, int channels) {
	Stop(); // 确保之前已停止

	m_wfex.wFormatTag = WAVE_FORMAT_PCM;
	m_wfex.nChannels = channels;
	m_wfex.nSamplesPerSec = sampleRate;
	m_wfex.wBitsPerSample = 16;
	m_wfex.nBlockAlign = channels * 2;
	m_wfex.nAvgBytesPerSec = sampleRate * m_wfex.nBlockAlign;
	m_wfex.cbSize = 0;

	MMRESULT res = waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &m_wfex,
		(DWORD_PTR)WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if (res != MMSYSERR_NOERROR) {
		//std::cerr << "WaveOut Open Error: " << res << std::endl;
		return false;
	}

	// 准备 Header
	for (int i = 0; i < HEADER_COUNT; ++i) {
		ZeroMemory(&m_headers[i], sizeof(WAVEHDR));
		m_headers[i].lpData = (LPSTR)m_headerBuffers[i];
		m_headers[i].dwBufferLength = BUFFER_SIZE;
		m_headers[i].dwFlags = 0;

		// 【关键】预先将缓冲区填零，防止初始噪音
		memset(m_headerBuffers[i], 0, BUFFER_SIZE);

		waveOutPrepareHeader(m_hWaveOut, &m_headers[i], sizeof(WAVEHDR));
	}

	m_ringBuffer.clear();
	m_bPlaying = true;
	m_pThread = new std::thread([this]() { this->PlaybackLoop(); });

	return true;
}

void CAudioPlayerWin::Stop() {
	m_bPlaying = false;
	m_ringBuffer.abort();

	if (m_pThread) {
		if (m_pThread->joinable()) m_pThread->join();
		delete m_pThread;
		m_pThread = nullptr;
	}

	if (m_hWaveOut) {
		waveOutReset(m_hWaveOut); // 立即停止播放
		for (int i = 0; i < HEADER_COUNT; ++i) {
			waveOutUnprepareHeader(m_hWaveOut, &m_headers[i], sizeof(WAVEHDR));
		}
		waveOutClose(m_hWaveOut);
		m_hWaveOut = nullptr;
	}
}

void CAudioPlayerWin::FeedData(const uint8_t* data, int size) {
	if (m_bPlaying && data && size > 0) {
		m_ringBuffer.write(data, size);
	}
}

double CAudioPlayerWin::GetPlayedSeconds() const {
	if (!m_hWaveOut) return 0.0;
	MMTIME mmTime;
	mmTime.wType = TIME_BYTES;
	if (waveOutGetPosition(m_hWaveOut, &mmTime, sizeof(MMTIME)) == MMSYSERR_NOERROR) {
		return (double)mmTime.u.cb / m_wfex.nAvgBytesPerSec;
	}
	return 0.0;
}

void CAudioPlayerWin::PlaybackLoop() {
	while (m_bPlaying) {
		// 1. 寻找一个空闲的 Header (不在队列中)
		WAVEHDR* pHdr = nullptr;
		for (int i = 0; i < HEADER_COUNT; ++i) {
			if (!(m_headers[i].dwFlags & WHDR_INQUEUE)) {
				pHdr = &m_headers[i];
				break;
			}
		}

		if (!pHdr) {
			// 所有 Header 都在播放中，短暂休眠等待回调
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// 2. 从环形缓冲区读取数据填充 Header
		// 【关键】必须尝试读满整个 BUFFER_SIZE，否则声卡会播放旧数据或噪音
		int bytesRead = m_ringBuffer.read((uint8_t*)pHdr->lpData, BUFFER_SIZE);

		if (bytesRead > 0) {
			// 3. 【关键修复】如果没读满，剩余部分必须补零！
			if (bytesRead < BUFFER_SIZE) {
				memset((uint8_t*)pHdr->lpData + bytesRead, 0, BUFFER_SIZE - bytesRead);
			}

			// 设置实际有效的数据长度（虽然 buffer 是固定的，但告诉声卡有效长度更稳妥）
			// 注意：对于 PCM，通常直接发送整个预分配的 Buffer 即可，只要后面是0
			pHdr->dwBufferLength = BUFFER_SIZE;

			// 4. 发送给声卡
			MMRESULT res = waveOutWrite(m_hWaveOut, pHdr, sizeof(WAVEHDR));
			if (res != MMSYSERR_NOERROR) {
				//std::cerr << "WaveOut Write Error: " << res << std::endl;
			}
		}
		else {
			// 缓冲区空且未中止，说明解码跟不上，稍微等待
			// 此时 pHdr 是空的，不要发送，否则会产生静音或噪音
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void CALLBACK CAudioPlayerWin::WaveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	if (uMsg == WOM_DONE) {
		// 不需要做任何事，PlaybackLoop 会通过检查 WHDR_INQUEUE 标志来重用 Header
	}
}

#endif // _MSC_VER