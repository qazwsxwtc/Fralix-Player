#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional> // 【新增】用于 std::function

extern "C" {
#include <libavcodec/avcodec.h>
}


// 简单的线程安全帧队列模板（带大小限制）
template<typename T>
class FrameQueue {
public:
	// 构造函数，默认无限制 (-1)，也可以指定最大大小
	explicit FrameQueue(int maxSize = -1)
		: m_maxSize(maxSize), m_abort(false) {}

	void setMaxSize(int maxSize) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_maxSize = maxSize;
	}

	// 【修改】推入时检查大小，如果满了则阻塞等待
	void push(T item) {
		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_maxSize > 0) {
			m_notFullCond.wait(lock, [this] {
				return static_cast<int>(m_queue.size()) < m_maxSize || m_abort;
			});
		}

		if (m_abort) {
			return; // 【关键】如果已中止，丢弃数据并返回，避免推入无效数据
		}

		m_queue.push(std::move(item));
		m_cond.notify_one();
	}

	bool pop(T& item) {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cond.wait(lock, [this] { return !m_queue.empty() || m_abort; });

		if (m_abort && m_queue.empty()) {
			return false;
		}

		item = std::move(m_queue.front());
		m_queue.pop();

		// 【关键】通知生产者：队列已有空间
		if (m_maxSize > 0) {
			m_notFullCond.notify_one();
		}

		return true;
	}

	// 【新增】支持中断的 Pop
	// interruptCallback: 返回 true 表示应该中断等待
	T popWithInterrupt(std::function<bool()> interruptCallback = nullptr) {
		std::unique_lock<std::mutex> lock(m_mutex);

		// 等待：直到队列不为空 OR 被中止 OR 中断回调返回 true
		m_cond.wait(lock, [this, &interruptCallback] {
			return !m_queue.empty() || m_abort || (interruptCallback && interruptCallback());
		});

		if (m_queue.empty()) {
			return T(); // 返回默认构造的对象（对于指针通常是 nullptr）
		}

		T item = std::move(m_queue.front());
		m_queue.pop();

		// 通知生产者有空位
		if (m_maxSize > 0) {
			m_notFullCond.notify_one();
		}

		return item;
	}

	void abort() {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_abort = true;
		m_cond.notify_all();    // 唤醒所有消费者
		m_notFullCond.notify_all(); // 唤醒所有生产者
	}

	void clear() {
		std::unique_lock<std::mutex> lock(m_mutex);
		while (!m_queue.empty()) {
			// 如果是 AVFrame*，需要释放内存
			// 这里假设 T 是指针类型，需要根据实际类型调整
			if constexpr (std::is_pointer_v<T>) {
				if (m_queue.front()) {
					// 如果是 AVFrame*, 调用 av_frame_free
					// 如果是其他类型，适当处理
					// 这里为了通用性，可能需要特化或者用户保证外部释放
					// 对于 AVFrame*:
					// av_frame_free(&m_queue.front()); 
				}
			}
			m_queue.pop();
		}
		m_abort = false;
		m_notFullCond.notify_all(); // 唤醒可能阻塞在 push 的生产者
		m_cond.notify_all();
	}

	size_t size() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cond;          // 消费者等待队列非空
	std::condition_variable m_notFullCond;   // 【新增】生产者等待队列非满
	int m_maxSize;                           // 【新增】最大队列大小
	bool m_abort;
};

class PacketQueue {
public:
    static const int UNLIMITED = -1;

    PacketQueue() : m_abort(false), m_maxSize(UNLIMITED) {}

    ~PacketQueue() {
        clear();
    }

    // 设置最大队列大小 (-1 表示无限制)
    void setMaxSize(int maxSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxSize = maxSize;
    }

    // 【线程安全】推入数据包（阻塞式）
    // 当队列已满且设置了 m_maxSize 时，生产者会阻塞等待，实现背压
    void push(AVPacket* pkt) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_abort) {
            if (pkt) av_packet_free(&pkt);
            return;
        }

        // 如果设置了最大限制，等待直到队列不满
        // 但 nullptr (EOF 哨兵) 不受限制，确保终止信号能送达消费者
        if (m_maxSize > 0 && pkt != nullptr) {
            m_notFullCond.wait(lock, [this] {
                return (int)m_queue.size() < m_maxSize || m_abort;
            });
        }

        if (m_abort) {
            if (pkt) av_packet_free(&pkt);
            return;
        }

        m_queue.push(pkt);
        m_cond.notify_one(); // 唤醒一个等待的消费者线程
    }

    // 【线程安全】弹出数据包 (阻塞式)
    // 如果队列为空，线程会休眠直到有数据或 abort 被调用
    bool pop(AVPacket*& pkt) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // 等待：直到队列不为空 或者 被中止
        m_cond.wait(lock, [this] { 
            return !m_queue.empty() || m_abort; 
        });
        
        if (m_abort && m_queue.empty()) {
            pkt = nullptr;
            return false;
        }
        
        pkt = m_queue.front();
        m_queue.pop();

        // 【关键】通知生产者：队列已有空间
        if (m_maxSize > 0) {
            m_notFullCond.notify_one();
        }

        return true;
    }

	// 【新增】支持中断的 Pop
	// interruptCallback: 返回 true 表示应该中断等待
	AVPacket* Pop(std::function<bool()> interruptCallback = nullptr);

    // 【线程安全】中止队列，唤醒所有等待线程并释放队列内剩余数据包
    void abort() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            AVPacket* pkt = m_queue.front();
            m_queue.pop();
            if (pkt) av_packet_free(&pkt);
        }
        m_abort = true;
        m_cond.notify_all();        // 唤醒所有阻塞在 pop 的消费者
        m_notFullCond.notify_all();  // 唤醒所有阻塞在 push 的生产者
    }

    // 【线程安全】清空队列
    void clear() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            AVPacket* pkt = m_queue.front();
            m_queue.pop();
            if (pkt) av_packet_free(&pkt);
        }
        m_abort = false;
        // 【关键】唤醒所有可能阻塞在 push() 上的生产者
        m_notFullCond.notify_all();
    }

    int size() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<int>(m_queue.size());
    }

private:
    std::queue<AVPacket*> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;          // 消费者等待队列非空
    std::condition_variable m_notFullCond;   // 生产者等待队列非满
    bool m_abort;
    int  m_maxSize;  // 最大队列大小，UNLIMITED(-1) 表示无限制
};