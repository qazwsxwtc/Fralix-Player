#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

// 简单的环形缓冲区实现
// 优化的环形缓冲区
class AudioRingBuffer {
public:
	explicit AudioRingBuffer(int sizeBytes)
		: m_size(sizeBytes), m_read(0), m_write(0), m_count(0), m_abort(false) {
		m_data = new uint8_t[m_size];
		memset(m_data, 0, m_size); // 【关键】初始化为0，防止初始噪音
	}
	~AudioRingBuffer() { delete[] m_data; }

	void write(const uint8_t* data, int len) {
		if (len <= 0) return;
		std::unique_lock<std::mutex> lock(m_mutex);
		m_notFull.wait(lock, [this, len] { return m_size - m_count >= len || m_abort; });
		if (m_abort) return;

		// 处理回绕写入
		int firstChunk = min(len, m_size - m_write);
		memcpy(m_data + m_write, data, firstChunk);
		if (len > firstChunk) {
			memcpy(m_data, data + firstChunk, len - firstChunk);
		}

		m_write = (m_write + len) % m_size;
		m_count += len;
		m_notEmpty.notify_one();
	}

	// 返回实际读取到的字节数
	int read(uint8_t* data, int len) {
		if (len <= 0) return 0;
		std::unique_lock<std::mutex> lock(m_mutex);

		// 如果数据不足，等待直到有足够的数或者中止
		// 注意：为了避免死锁，如果 abort 了，我们读取剩余所有数据
		m_notEmpty.wait(lock, [this, len] { return m_count >= len || m_abort; });

		if (m_count == 0) return 0;

		// 如果中止且数据不足 len，只读取剩余的
		int toRead = min(len, m_count);

		// 处理回绕读取
		int firstChunk = min(toRead, m_size - m_read);
		memcpy(data, m_data + m_read, firstChunk);
		if (toRead > firstChunk) {
			memcpy(data + firstChunk, m_data, toRead - firstChunk);
		}

		m_read = (m_read + toRead) % m_size;
		m_count -= toRead;

		m_notFull.notify_one();
		return toRead;
	}

	void abort() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_abort = true;
		m_notFull.notify_all();
		m_notEmpty.notify_all();
	}

	void clear() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_read = 0; m_write = 0; m_count = 0;
		memset(m_data, 0, m_size);
		m_abort = false;
	}

private:
	uint8_t* m_data;
	int m_size;
	int m_read, m_write, m_count;
	std::mutex m_mutex;
	std::condition_variable m_notFull;
	std::condition_variable m_notEmpty;
	bool m_abort;
	mutable std::atomic<double> m_cachedClock;
	mutable uint64_t m_lastUpdateTicks;
};
