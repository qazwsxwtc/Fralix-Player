#pragma once
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

class CVideoScanner {
public:
	// 【单例核心】获取唯一实例
	static CVideoScanner& GetInstance();

	// 禁止拷贝构造和赋值操作，确保单例唯一性
	CVideoScanner(const CVideoScanner&) = delete;
	CVideoScanner& operator=(const CVideoScanner&) = delete;

	// 视频扩展名集合 (保持 static const，因为它是常量数据)
	static const std::set<std::string> VIDEO_EXTENSIONS;

	// 【修改】变为非静态成员函数，通过实例调用
	void ScanAllDrives(std::function<void(const std::string&)> onFileFound);
	void ScanDirectory(const std::string& path, int currentDepth, int maxDepth,
		std::vector<std::string>& results,
		std::function<void(const std::string&)> onFileFound);

	bool IsVideoFile(const std::string& filePath);
	std::string GetLowerExtension(const std::string& filePath);
	std::vector<std::string> GetAllDrives();

	void StopScan();
	bool IsScanning();

private:
	// 【单例核心】私有构造函数
	CVideoScanner();
	~CVideoScanner();

	// 成员变量，不再需要 static
	std::atomic<bool> m_isScanning = false;
};