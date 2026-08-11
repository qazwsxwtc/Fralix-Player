#pragma once
#include "..\DuiLib\UIlib.h"
#include "CCommonTypedef.h" // 假设这里定义了 VideoPixelFormat

using namespace DuiLib;

// 定义控件名称，用于 XML 解析
#define DUI_CTR_MEDIADISPLAY _T("MediaDisplay")

class CWndMediaDisplay : public CControlUI
{
public:
    CWndMediaDisplay();
    virtual ~CWndMediaDisplay();

    // 获取视频渲染窗口的句柄，供 FFmpeg/VLC 嵌入
    HWND GetRenderHwnd() const { return m_hVideoWnd; }

    // 设置视频可见性
    void SetVideoVisible(bool bVisible);

    // === 渲染接口 ===
    // 将视频帧数据绘制到窗口上
    void RenderFrame(const uint8_t* pData, int width, int height, VideoPixelFormat format);

	HDC     GetMemDC();         // 内存 DC
	HBITMAP    GetBitmap();
	int         GetWidth();
	int          GetHeight();
    uint8_t* GetRawData();
    int GetRawWidth();
    int GetRawHeight();

    int HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void ShowHint(bool bShow) { m_bShowHint = bShow; InvalidateRect(m_hVideoWnd, NULL, FALSE); }
    bool GetShowHint();
protected:
    // 创建视频渲染子窗口
    bool CreateVideoWindow();
    // 销毁视频渲染子窗口
    void DestroyRenderWindow(); // 改名以匹配实现

    // GDI 绘制辅助
    void DrawFrameGDI(const uint8_t* pData, int width, int height, VideoPixelFormat format);

    // Duilib 虚函数重载
    virtual void SetPos(RECT rc, bool bNeedInvalidate = true) ;
    virtual void Paint(HDC hDC, const RECT& rcPaint) ;
    virtual void DoInit() ;
    virtual void DoUninit() ;

private:
	HWND        m_hVideoWnd;
	HDC         m_hMemDC;
	HBITMAP     m_hBitmap;

	// 【新增】保存原始视频帧数据和尺寸
	uint8_t* m_pRawData;       // 原始 RGB/BGR 数据
	int         m_nRawWidth;      // 原始宽度
	int         m_nRawHeight;     // 原始高度
	VideoPixelFormat m_rawFormat; // 原始格式

    bool m_bShowHint = true;

	bool        m_bVideoVisible;
	int         m_nWidth;         // 当前窗口/控件宽度
	int         m_nHeight;        // 当前窗口/控件高度
};