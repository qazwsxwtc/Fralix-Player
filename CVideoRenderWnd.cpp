#include "CVideoRenderWnd.h"
#include <stdint.h>

CVideoRenderWnd::CVideoRenderWnd()
    : m_hBitmap(nullptr)
    , m_hMemDC(nullptr)
    , m_nWidth(0)
    , m_nHeight(0)
    , m_clrBackground(RGB(0, 0, 0)) // 黑色背景
{
}

CVideoRenderWnd::~CVideoRenderWnd()
{
    DestroyRenderWindow();
}

LPCTSTR CVideoRenderWnd::GetWindowClassName() const
{
    return _T("VideoRenderWnd");
}

UINT CVideoRenderWnd::GetClassStyle() const
{
    return CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
}

void CVideoRenderWnd::OnFinalMessage(HWND hWnd)
{
    delete this;
}

LRESULT CVideoRenderWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    BOOL bHandled = TRUE;

    switch (uMsg)
    {
    case WM_CREATE:
        // 可以在这里初始化 D3D/OpenGL 上下文
        break;
    case WM_PAINT:
        lRes = OnPaint(uMsg, wParam, lParam);
        break;
    case WM_SIZE:
        lRes = OnSize(uMsg, wParam, lParam);
        break;
    case WM_ERASEBKGND:
        // 阻止默认擦除背景，减少闪烁
        return 1; 
    default:
        bHandled = FALSE;
    }

    if (bHandled) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

bool CVideoRenderWnd::CreateRenderWindow(HWND hWndParent, const RECT& rcPos)
{
    if (m_hWnd != nullptr) return true; // 已创建

    // 创建子窗口，样式为 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
    // WS_CLIPSIBLINGS 很重要，防止与其他子窗口绘制重叠
    Create(hWndParent, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, rcPos.left, rcPos.top, 
           rcPos.right - rcPos.left, rcPos.bottom - rcPos.top, NULL);
    
    if (m_hWnd == nullptr) return false;

    // 初始化 GDI 资源 (如果是用 D3D/OpenGL，在这里初始化 Device/Context)
    HDC hDC = GetDC(m_hWnd);
    m_hMemDC = CreateCompatibleDC(hDC);
    ReleaseDC(m_hWnd, hDC);

    return true;
}

void CVideoRenderWnd::DestroyRenderWindow()
{
    if (m_hMemDC)
    {
        DeleteDC(m_hMemDC);
        m_hMemDC = nullptr;
    }
    if (m_hBitmap)
    {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

void CVideoRenderWnd::RenderFrame(const uint8_t* pData, int ix, int iy, int width, int height, VideoPixelFormat format)
{
    if (!m_hWnd || !pData || width <= 0 || height <= 0) return;

    m_nWidth = width;
    m_nHeight = height;

    // 这里调用具体的绘制逻辑
    // 注意：在实际高性能播放器中，这里应该直接操作 D3D Texture 或 OpenGL Texture
    // 为了演示简单性和兼容性，这里使用 GDI BitBlt
    DrawFrameGDI(pData, ix, iy, width, height, format);
    
    // 触发重绘
    InvalidateRect(m_hWnd, NULL, FALSE);
}  

void CVideoRenderWnd::Resize(int ix, int iy, int width, int height)
{
    if (m_hWnd)
    {
        MoveWindow(m_hWnd, ix, iy, width, height, TRUE);
        m_nWidth = width;
        m_nHeight = height;
    }
}

HDC CVideoRenderWnd::GetRenderDC()
{
    return m_hMemDC;
}

// ============================================================================
// 内部辅助方法
// ============================================================================

LRESULT CVideoRenderWnd::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    BeginPaint(m_hWnd, &ps);

    // 如果内存 DC 中有内容，将其 BitBlt 到屏幕 DC
    if (m_hMemDC && m_hBitmap)
    {
        BitBlt(ps.hdc, 0, 0, m_nWidth, m_nHeight, m_hMemDC, 0, 0, SRCCOPY);
    }
    else
    {
        // 没有视频帧时，填充黑色背景
	   /* RECT rc;
		GetClientRect(m_hWnd, &rc);
		HBRUSH hBrush = CreateSolidBrush(m_clrBackground);
		FillRect(ps.hdc, &rc, hBrush);
		DeleteObject(hBrush);*/
    }

    EndPaint(m_hWnd, &ps);
    return 0;
}

LRESULT CVideoRenderWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 窗口大小改变时，可能需要重新创建 D3D SwapChain 或调整 OpenGL Viewport
    // 对于 GDI，不需要特殊处理，BitBlt 会自动拉伸或裁剪（取决于具体实现）
    return 0;
}

void CVideoRenderWnd::DrawFrameGDI(const uint8_t* pData, int ix, int iy, int width, int height, VideoPixelFormat format)
{
    if (!m_hMemDC) return;

    // 1. 创建或重置兼容位图
    // 注意：频繁创建 DeleteObject/CreateCompatibleBitmap 性能较差，实际项目应复用
    if (m_hBitmap) DeleteObject(m_hBitmap);
    
    HDC hScreenDC = GetDC(m_hWnd);
    m_hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    ReleaseDC(m_hWnd, hScreenDC);
    
    SelectObject(m_hMemDC, m_hBitmap);

    // 2. 将数据写入位图
    // 这里仅演示 RGB32 格式，其他格式需要转换
    if (format == PIXEL_FORMAT_RGB32)
    {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // 负数表示顶向下 DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(m_hMemDC, ix, iy, width, height, 0, 0, 0, height, pData, &bmi, DIB_RGB_COLORS);
    }
    else if (format == PIXEL_FORMAT_RGB24)
    {
        // RGB24 需要转换为 RGB32 或使用 StretchDIBits
        // 为简化代码，此处省略转换逻辑，实际需处理步长(padding)
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        // 计算每行字节数 (需4字节对齐)
        int lineBytes = (width * 3 + 3) & ~3;
        
        SetDIBitsToDevice(m_hMemDC, ix, iy, width, height, 0, 0, 0, height, pData, &bmi, DIB_RGB_COLORS);
    }
    else
    {
        // YUV 等其他格式需要软件转换为 RGB 后再绘制，或者使用 Shader (D3D/OpenGL)
        // 这里仅填充灰色表示不支持
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
        RECT rc = {ix, iy, width, height};
        FillRect(m_hMemDC, &rc, hBrush);
        DeleteObject(hBrush);
    }
}