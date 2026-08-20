#include "CDllManage.h"

CDllManage::CDllManage()
    : m_hModule(nullptr)
{
}

CDllManage::~CDllManage()
{
    Free();
}

bool CDllManage::Load(const std::wstring& dllPath)
{
    // 如果已经加载了其他的 DLL，先卸载
    if (m_hModule) {
        Free();
    }

    // 使用 LoadLibraryW 加载 Unicode 路径
    m_hModule = ::LoadLibraryW(dllPath.c_str());

    return (m_hModule != nullptr);
}

void CDllManage::Free()
{
    if (m_hModule) {
        ::FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }
}

bool CDllManage::IsLoaded() const
{
    return (m_hModule != nullptr);
}

FARPROC CDllManage::GetProcAddress(LPCSTR funcName) const
{
    if (!m_hModule) {
        return nullptr;
    }
    return ::GetProcAddress(m_hModule, funcName);
}

FARPROC CDllManage::GetProcAddressW(LPCWSTR funcName) const
{
    if (!m_hModule) {
        return nullptr;
    }
    // 注意：Windows API GetProcAddress 只接受 ANSI 字符串 (LPCSTR)。
    // 如果 DLL 导出的是 Unicode 名称（非常罕见），可能需要特殊处理。
    // 通常我们先将 LPCWSTR 转换为 LPCSTR (UTF-8 或 ANSI) 再调用。
    // 这里为了简单和兼容性，通常建议上层调用者传入 ANSI 名称，或者使用 GetFunction 模板。
    
    // 简单的转换示例 (仅适用于 ASCII 函数名)
    char buffer[256] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, funcName, -1, buffer, sizeof(buffer), NULL, NULL);
    return ::GetProcAddress(m_hModule, buffer);
}