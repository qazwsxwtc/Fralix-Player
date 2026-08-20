#pragma once
#include <string>
#include <mutex>

class CConfigManager {
public:
    // 【关键】获取单例实例
    static CConfigManager& GetInstance() {
        static CConfigManager instance;
        return instance;
    }

    // 禁止拷贝和赋值
    CConfigManager(const CConfigManager&) = delete;
    CConfigManager& operator=(const CConfigManager&) = delete;

    // --- 全局配置项 ---

    // 音量 (0-100)
    void SetVolume(int vol);
    int GetVolume() const;

    // 播放倍速 (0.25 - 4.0)
    void SetPlaybackRate(double rate);
    double GetPlaybackRate() const;

    // 最后播放的文件路径
    void SetLastPlayedFile(const std::wstring& path);
    std::wstring GetLastPlayedFile() const;

    // 是否自动连播
    void SetAutoPlayNext(bool bAuto);
    bool IsAutoPlayNext() const;

private:
    CConfigManager(); // 私有构造函数
    ~CConfigManager();

    mutable std::mutex m_mutex; // 线程安全锁

    // 成员变量
    int m_nVolume = 100;
    double m_dPlaybackRate = 1.0;
    std::wstring m_strLastPlayedFile;
    bool m_bAutoPlayNext = true;
};