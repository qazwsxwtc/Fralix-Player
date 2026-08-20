#include "ConfigManager.h"

CConfigManager::CConfigManager() {
    // 可以在这里从配置文件（如 ini/json）加载初始值
}

CConfigManager::~CConfigManager() {
    // 可以在这里保存配置到文件
}

// --- 实现 ---

void CConfigManager::SetVolume(int vol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    m_nVolume = vol;
}

int CConfigManager::GetVolume() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nVolume;
}

void CConfigManager::SetPlaybackRate(double rate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (rate < 0.25) rate = 0.25;
    if (rate > 4.0) rate = 4.0;
    m_dPlaybackRate = rate;
}

double CConfigManager::GetPlaybackRate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dPlaybackRate;
}

void CConfigManager::SetLastPlayedFile(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_strLastPlayedFile = path;
}

std::wstring CConfigManager::GetLastPlayedFile() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_strLastPlayedFile;
}

void CConfigManager::SetAutoPlayNext(bool bAuto) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bAutoPlayNext = bAuto;
}

bool CConfigManager::IsAutoPlayNext() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bAutoPlayNext;
}