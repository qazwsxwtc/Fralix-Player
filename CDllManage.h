#ifndef CDllManage_H
#define CDllManage_H


#include <windows.h>
#include <string>
#include <stdexcept>

class CDllManage
{
public:
    CDllManage();
    ~CDllManage();

    // 禁止拷贝，防止多次释放同一个 HMODULE
    CDllManage(const CDllManage&) = delete;
    CDllManage& operator=(const CDllManage&) = delete;

    /**
     * @brief 加载指定的 DLL 文件
     * @param dllPath DLL 文件的完整路径或文件名
     * @return true 如果加载成功，false 如果失败
     */
    bool Load(const std::wstring& dllPath);
    
    /**
     * @brief 卸载当前加载的 DLL
     */
    void Free();

    /**
     * @brief 检查 DLL 是否已加载
     */
    bool IsLoaded() const;

    /**
     * @brief 从 DLL 中获取函数指针 (ANSI 版本)
     * @param funcName 函数名称
     * @return 函数指针，如果未找到返回 nullptr
     */
    FARPROC GetProcAddress(LPCSTR funcName) const;

    /**
     * @brief 从 DLL 中获取函数指针 (Unicode 版本，通常用于导出名也是 Unicode 的情况，较少见)
     */
    FARPROC GetProcAddressW(LPCWSTR funcName) const;

    /**
     * @brief 模板方法：获取类型安全的函数指针
     * @tparam T 函数指针类型，例如 typedef int (*MyFunc)(int, int);
     * @param funcName 函数名称
     * @return 类型化的函数指针
     * @throws std::runtime_error 如果 DLL 未加载或函数未找到
     */
    template <typename T>
    T GetFunction(LPCSTR funcName) const;

private:
    HMODULE m_hModule;
};

// 模板实现必须放在头文件中
template <typename T>
T CDllManage::GetFunction(LPCSTR funcName) const
{
    if (!m_hModule) {
        throw std::runtime_error("DLL is not loaded.");
    }
    
    FARPROC proc = ::GetProcAddress(m_hModule, funcName);
    if (!proc) {
        throw std::runtime_error("Function not found in DLL.");
    }
    
    return reinterpret_cast<T>(proc);
}

#endif // CDllManage_H


/*
* 假设你有一个 VideoCore.dll，里面导出了以下函数：
* // VideoCore.h (DLL 端)
* 
extern "C" __declspec(dllexport) int InitPlayer(void* hWnd);
extern "C" __declspec(dllexport) void PlayVideo();

#include "CDllManage.h"
#include <iostream>

// 定义函数指针类型
typedef int (*InitPlayerFunc)(void* hWnd);
typedef void (*PlayVideoFunc)();

void TestDllLoading()
{
	CDllManage dllManager;

	// 1. 加载 DLL
	if (!dllManager.Load(L"VideoCore.dll")) {
		std::wcerr << L"Failed to load DLL: " << GetLastError() << std::endl;
		return;
	}

	try {
		// 2. 获取函数指针 (类型安全)
		InitPlayerFunc pInit = dllManager.GetFunction<InitPlayerFunc>("InitPlayer");
		PlayVideoFunc pPlay = dllManager.GetFunction<PlayVideoFunc>("PlayVideo");

		// 3. 调用函数
		if (pInit) {
			int result = pInit(nullptr); // 传入窗口句柄等参数
			std::cout << "Init result: " << result << std::endl;
		}

		if (pPlay) {
			pPlay();
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	// 4. 析构时自动 FreeLibrary，或者手动调用 dllManager.Free();
}

*/