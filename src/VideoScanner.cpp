#include "VideoScanner.h"
#include <sys/stat.h> // 用于 stat
// 定义最小文件大小阈值 (1 MB = 1024 * 1024 bytes)
static const long long MIN_VIDEO_SIZE_BYTES = 1024LL * 1024LL;

// 【新增】获取文件大小（字节）
static long long GetFileSize(const std::string& filePath) {
#ifdef _WIN32
	struct _stat64 fileStat;
	if (_stat64(filePath.c_str(), &fileStat) == 0) {
		return fileStat.st_size;
	}
#else
	struct stat fileStat;
	if (stat(filePath.c_str(), &fileStat) == 0) {
		return fileStat.st_size;
	}
#endif
	return -1; // 错误
}


// 初始化静态常量
const std::set<std::string> CVideoScanner::VIDEO_EXTENSIONS = {

	".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm",
	".m4v", ".mpg", ".mpeg", ".3gp", ".3g2", ".rm", ".rmvb",
	".ts", ".m2ts", ".mts", ".f4v", ".f4p", ".asf"
};

// 【单例核心】实现 GetInstance
// 使用 Magic Static (C++11 起保证线程安全)
CVideoScanner& CVideoScanner::GetInstance() {
	static CVideoScanner instance;
	return instance;
}

// 私有构造函数
CVideoScanner::CVideoScanner() {
	// 初始化逻辑
}

CVideoScanner::~CVideoScanner() {
	StopScan(); // 确保退出时停止扫描
}

std::string CVideoScanner::GetLowerExtension(const std::string& filePath) {
    size_t pos = filePath.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = filePath.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool CVideoScanner::IsVideoFile(const std::string& filePath) {
	// 1. 获取扩展名
	std::string ext = GetLowerExtension(filePath);

	// 【新增】显式排除常见的非视频双后缀干扰项
	// 虽然 .ts 是视频，但 .d.ts, .map.js 等不是
	if (ext == ".ts") {
		// 如果是 .ts 结尾，检查它是否真的是视频 TS 流
		// 简单的启发式判断：视频 TS 文件通常大于 100KB
		if (filePath.find_last_of(".d.ts") != std::string::npos)
		{
            return false;
		}
	}

	// 2. 检查是否在白名单中
	if (VIDEO_EXTENSIONS.find(ext) == VIDEO_EXTENSIONS.end()) {
		return false;
	}

	// 3. 检查文件大小 (过滤小于 1MB 的文件)
	long long size = GetFileSize(filePath);
	if (size < 0 || size < MIN_VIDEO_SIZE_BYTES) {
		return false;
	}

	return true;
}

// 【新增】获取所有盘符 (Windows)
std::vector<std::string> CVideoScanner::GetAllDrives() {
    std::vector<std::string> drives;
#ifdef _WIN32
    DWORD logicalDrives = GetLogicalDrives();
    if (logicalDrives == 0) return drives;

    for (int i = 0; i < 26; ++i) {
        if (logicalDrives & (1 << i)) {
            char driveName[4] = { static_cast<char>('A' + i), ':', '\\', '\0' };
            
            // 可选：只扫描固定磁盘 (Fixed Drive)，跳过光驱、U盘等
            UINT driveType = GetDriveTypeA(driveName);
            if (driveType == DRIVE_FIXED) {
                drives.push_back(std::string(driveName));
            }
        }
    }
#else
    // Linux/Mac: 通常扫描 /mnt, /media, 或 /
    drives.push_back("/"); 
#endif
    return drives;
}

void CVideoScanner::StopScan()
{
    m_isScanning.store(false);
}

bool CVideoScanner::IsScanning()
{   
    return m_isScanning.load();
}

void CVideoScanner::ScanDirectory(const std::string& path, int currentDepth, int maxDepth,
                                 std::vector<std::string>& results,
                                 std::function<void(const std::string&)> onFileFound) {
    if (maxDepth >= 0 && currentDepth > maxDepth) return;
    m_isScanning.store(true);
#ifdef _WIN32
    std::string searchPath = path + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string fileName = findData.cFileName;
        if (fileName == "." || fileName == "..") continue;

        // 【优化】跳过系统隐藏文件夹，避免权限错误和无限循环
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ||
            (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
            // 可选：如果想扫描隐藏文件，去掉这个判断
             if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
             // 如果是隐藏目录，通常也是系统目录，跳过
             continue;
        }
        if (!m_isScanning.load()) break;

        std::string fullPath = path + "\\" + fileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ScanDirectory(fullPath, currentDepth + 1, maxDepth, results, onFileFound);
        } else {
            if (IsVideoFile(fullPath)) {
                results.push_back(fullPath);
                if (onFileFound) {
                    onFileFound(fullPath);
                }
            }
        }
    } while (FindNextFileA(hFind, &findData) != 0);
    FindClose(hFind);

#else
    DIR* dir = opendir(path.c_str());
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;
        if (fileName == "." || fileName == "..") continue;
        
        // Linux: 跳过隐藏文件夹
        if (fileName[0] == '.') continue;

        std::string fullPath = path + "/" + fileName;
        struct stat statBuf;
        if (stat(fullPath.c_str(), &statBuf) != 0) continue;

        if (S_ISDIR(statBuf.st_mode)) {
            ScanDirectory(fullPath, currentDepth + 1, maxDepth, results, onFileFound);
        } else if (S_ISREG(statBuf.st_mode)) {
            if (IsVideoFile(fullPath)) {
                results.push_back(fullPath);
                if (onFileFound) {
                    onFileFound(fullPath);
                }
            }
        }
    }
    closedir(dir);
#endif
    m_isScanning.store(false);
}

// 【新增】全盘扫描入口
void CVideoScanner::ScanAllDrives(std::function<void(const std::string&)> onFileFound) {
    std::vector<std::string> drives = GetAllDrives();
    std::vector<std::string> allVideos;
    std::mutex mtx;
    m_isScanning.store(true);
    std::vector<std::thread> threads;

    for (const auto& drive : drives) {

        if (!m_isScanning.load()) break;
        
        // 为每个磁盘启动一个线程并行扫描
        threads.emplace_back([&, drive]() {
            std::vector<std::string> localResults;
            try {
                // 深度设为 -1 表示无限制，或者设为 10 防止太深
                ScanDirectory(drive, 0, -1, localResults, nullptr); 
            } catch (...) {
                std::cerr << "Error scanning drive: " << drive << std::endl;
            }
           
            // 合并结果
            std::lock_guard<std::mutex> lock(mtx);
            allVideos.insert(allVideos.end(), localResults.begin(), localResults.end());
            
            // 如果提供了回调，可以在这里批量通知 UI，或者在 ScanDirectory 中实时通知
        });
    }

    // 等待所有线程结束
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // 排序最终结果
    std::sort(allVideos.begin(), allVideos.end());
    
    // 如果需要一次性返回所有结果，可以修改函数签名返回 vector
    // 这里为了演示异步回调，我们假设 onFileFound 是实时的
    // 注意：上面的线程内部没有调用 onFileFound，因为多线程并发调用 UI 回调不安全
    // 建议：在主线程中处理 allVideos
    m_isScanning.store(false);
}