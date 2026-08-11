#ifndef CVideoRenderWnd_h
#define CVideoRenderWnd_h

#include "..\DuiLib\UIlib.h"
#include <windows.h>
#include "CCommonTypedef.h"

using namespace DuiLib;


class CVideoRenderWnd : public CWindowWnd
{
public:
    CVideoRenderWnd();
    virtual ~CVideoRenderWnd();

    // === CWindowWnd 接口实现 ===
    LPCTSTR GetWindowClassName() const override;
    UINT GetClassStyle() const override;
    void OnFinalMessage(HWND hWnd) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // === 初始化与销毁 ===
    // hWndParent: 父窗口句柄 (通常是 CVideoPlayerFrame 的 HWND)
    bool CreateRenderWindow(HWND hWndParent, const RECT& rcPos);
    void DestroyRenderWindow();

    // === 渲染接口 ===
    // 将视频帧数据绘制到窗口上
    // pData: 图像数据指针
    // width, height: 图像宽高
    // format: 像素格式
    void RenderFrame(const uint8_t* pData,int ix, int iy , int width, int height, VideoPixelFormat format);

    // === 窗口调整 ===
    void Resize(int ix, int iy, int width, int height);
    
    // 获取渲染窗口的 HDC (如果需要 GDI 绘制)
    HDC GetRenderDC();

private:
    // === 内部辅助 ===
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // 简单的 GDI 绘制实现 (实际项目中通常替换为 D3D/OpenGL)
    void DrawFrameGDI(const uint8_t* pData, int ix, int iy, int width, int height, VideoPixelFormat format);

    // === 成员变量 ===
    HBITMAP m_hBitmap;      // 用于 GDI 绘制的兼容位图
    HDC     m_hMemDC;       // 内存 DC
    int     m_nWidth;
    int     m_nHeight;
    
    // 背景色 (当没有视频时显示)
    COLORREF m_clrBackground;
};

#endif // CVideoRenderWnd_h