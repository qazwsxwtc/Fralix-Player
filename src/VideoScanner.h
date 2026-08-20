#pragma once
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

class CVideoScanner {
public:
    static const std::set<std::string> VIDEO_EXTENSIONS;

    // 【新增】扫描所有磁盘的视频文件
    // callback: 每找到一个文件就回调一次，用于实时更新UI
    static void ScanAllDrives(std::function<void(const std::string&)> onFileFound);

    // 原有的单目录扫描
    static void ScanDirectory(const std::string& path, int currentDepth, int maxDepth, 
                              std::vector<std::string>& results, 
                              std::function<void(const std::string&)> onFileFound);
    
    static bool IsVideoFile(const std::string& filePath);
    static std::string GetLowerExtension(const std::string& filePath);
    
    // 【新增】获取所有盘符
    static std::vector<std::string> GetAllDrives();
};