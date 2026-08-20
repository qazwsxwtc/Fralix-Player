#include "CCommonTypedef.h"
#include <windows.h>
#include <string>
//using namespace std;
/**
 * @brief wstring (UTF-16) 转换为 string (UTF-8)
 */
std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed == 0) return "";

    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring UTF8ToWString(const std::string& str)
{
	if (str.empty()) return L"";

	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	if (len <= 0) return L"";

	std::wstring wstr(len - 1, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], len);
	return wstr;

}
