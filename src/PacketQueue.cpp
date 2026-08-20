#include "PacketQueue.h"

AVPacket* PacketQueue::Pop(std::function<bool()> interruptCallback)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	// 等待条件：队列不为空 OR 被中止 OR 中断回调返回 true
	m_cond.wait(lock, [this, &interruptCallback] {
		return !m_queue.empty() || m_abort || (interruptCallback && interruptCallback());
	});

	// 如果是因为中断或中止且队列为空，返回 nullptr
	if (m_queue.empty()) {
		return nullptr;
	}

	AVPacket* pkt = m_queue.front();
	m_queue.pop();

	// 通知生产者有空位
	if (m_maxSize > 0) {
		m_notFullCond.notify_one();
	}

	return pkt;
}
