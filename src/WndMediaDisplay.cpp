#include "WndMediaDisplay.h"

// 【修正】全局窗口过程
LRESULT CALLBACK VideoWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWndMediaDisplay* pThis = reinterpret_cast<CWndMediaDisplay*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	// 1. 处理创建消息，保存指针
	if (uMsg == WM_NCCREATE) {
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		pThis = reinterpret_cast<CWndMediaDisplay*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	if (!pThis) {
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	// 2. 处理需要透传给父窗口的鼠标消息
	switch (uMsg) {
	case WM_LBUTTONDBLCLK:
	{
		HWND hParent = ::GetParent(hWnd);
		if (hParent)
		{
			// 触发主窗口的最大化/还原切换
			::SendMessage(hParent, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
		return 0; // 消息已处理，不再向下传递
	}
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	
	case WM_RBUTTONDBLCLK:
	{
		// 【关键】坐标转换！
		// lParam 中的坐标是相对于当前子窗口 (hWnd) 的
		// 我们需要将其转换为相对于父窗口 (GetParent) 的坐标，Duilib 才能正确命中测试
		POINT pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		// 将点从子窗口映射到父窗口
		::MapWindowPoints(hWnd, ::GetParent(hWnd), &pt, 1);

		// 重新打包 lParam
		LPARAM newLParam = MAKELPARAM(pt.x, pt.y);

		// 发送消息给父窗口
		::SendMessage(::GetParent(hWnd), uMsg, wParam, newLParam);

		// 注意：对于鼠标消息，通常返回 0 即可，表示消息已处理/转发
		return 0;
	}
	break;

	case WM_PAINT:
	{
		// 处理视频绘制逻辑
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);

		// 如果有原始数据，进行绘制
		if (pThis->GetRawData() && pThis->GetRawWidth() > 0 && pThis->GetRawHeight() > 0)
		{
			RECT rc;
			::GetClientRect(hWnd, &rc);
			int dstW = rc.right - rc.left;
			int dstH = rc.bottom - rc.top;

			if (dstW > 0 && dstH > 0)
			{
				HDC hTempDC = ::CreateCompatibleDC(ps.hdc);
				HBITMAP hTempBitmap = ::CreateCompatibleBitmap(ps.hdc, pThis->GetRawWidth(), pThis->GetRawHeight());
				::SelectObject(hTempDC, hTempBitmap);

				BITMAPINFO bmi;
				ZeroMemory(&bmi, sizeof(BITMAPINFO));
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = pThis->GetRawWidth();
				bmi.bmiHeader.biHeight = -pThis->GetRawHeight(); // Top-down
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 24;
				bmi.bmiHeader.biCompression = BI_RGB;

				::SetDIBitsToDevice(hTempDC, 0, 0, pThis->GetRawWidth(), pThis->GetRawHeight(),
					0, 0, 0, pThis->GetRawHeight(),
					pThis->GetRawData(), &bmi, DIB_RGB_COLORS);

				::SetStretchBltMode(ps.hdc, HALFTONE);
				::StretchBlt(ps.hdc, 0, 0, dstW, dstH,
					hTempDC, 0, 0, pThis->GetRawWidth(), pThis->GetRawHeight(),
					SRCCOPY);

				::DeleteObject(hTempBitmap);
				::DeleteDC(hTempDC);
			}
		}

		// 在 VideoWndProc 的 WM_PAINT 分支中
		if (pThis->GetShowHint()) {
			const wchar_t* hint = L"拖拽视频文件到此处...";
			HFONT hFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
			HFONT hOldFont = (HFONT)SelectObject(ps.hdc, hFont);
			SetBkMode(ps.hdc, TRANSPARENT);
			SetTextColor(ps.hdc, RGB(255, 255, 255));
			

			RECT rc;
			GetClientRect(hWnd, &rc);
			DrawText(ps.hdc, hint, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			SelectObject(ps.hdc, hOldFont);
			DeleteObject(hFont);
		}

		EndPaint(hWnd, &ps);
		return 0;
	}
	break;

	default:
		break;
	}

	// 3. 其他消息交给默认窗口过程或类成员处理
	// 这里直接调用 DefWindowProc 是最安全的，避免递归调用 HandleMessage 导致栈溢出
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CWndMediaDisplay::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

CWndMediaDisplay::CWndMediaDisplay()
	: m_hVideoWnd(nullptr)
	, m_hMemDC(nullptr)
	, m_hBitmap(nullptr)
	, m_pRawData(nullptr)
	, m_nRawWidth(0)
	, m_nRawHeight(0)
	, m_rawFormat(PIXEL_FORMAT_RGB24)
	, m_bVideoVisible(true)
	, m_nWidth(0)
	, m_nHeight(0)
{
}

CWndMediaDisplay::~CWndMediaDisplay()
{
    DestroyRenderWindow();
	if (m_pRawData) {
		delete[] m_pRawData;
		m_pRawData = nullptr;
	}
}

void CWndMediaDisplay::DoInit()
{
    __super::DoInit();
    CreateVideoWindow();
}

void CWndMediaDisplay::DoUninit()
{
    DestroyRenderWindow();
    //__super::DoUninit();
}

bool CWndMediaDisplay::CreateVideoWindow()
{
    if (m_hVideoWnd) return true;

    // 获取 Duilib 主窗口句柄
    HWND hWndParent = m_pManager->GetPaintWindow();
    if (!hWndParent) return false;

    // 创建子窗口
    // WS_CHILD | WS_CLIPSIBLINGS 是关键
    m_hVideoWnd = ::CreateWindowEx(
        WS_EX_TRANSPARENT,
        _T("STATIC"), // 使用静态控件类，轻量且无需注册
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, 0, 0,
        hWndParent,
        NULL,
        NULL,
        NULL
    );

    if (m_hVideoWnd)
    {
		// 【关键】存储 this 指针，以便在窗口过程中访问
		SetWindowLongPtr(m_hVideoWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		// 【关键】子类化窗口过程，以处理 WM_PAINT
		SetWindowLongPtr(m_hVideoWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(VideoWndProc));

        // 初始化 GDI 资源
        HDC hDC = ::GetDC(m_hVideoWnd);
        m_hMemDC = ::CreateCompatibleDC(hDC);
        ::ReleaseDC(m_hVideoWnd, hDC);

        // 初始位置同步
        RECT rc = GetPos();
        ::SetWindowPos(m_hVideoWnd, NULL, rc.left, rc.top, 
                       rc.right - rc.left, rc.bottom - rc.top, 
                       SWP_NOZORDER | SWP_NOACTIVATE);
        return true;
    }

    return false;
}

void CWndMediaDisplay::DestroyRenderWindow()
{
	if (m_hMemDC) {
		::DeleteDC(m_hMemDC);
		m_hMemDC = nullptr;
	}
	if (m_hBitmap) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}
	if (m_hVideoWnd) {
		::DestroyWindow(m_hVideoWnd);
		m_hVideoWnd = nullptr;
	}
}

void CWndMediaDisplay::SetPos(RECT rc, bool bNeedInvalidate)
{
    __super::SetPos(rc, bNeedInvalidate);

    if (m_hVideoWnd)
    {
        // 同步子窗口位置
        ::SetWindowPos(m_hVideoWnd, NULL, rc.left, rc.top, 
                       rc.right - rc.left, rc.bottom - rc.top, 
                       SWP_NOZORDER | SWP_NOACTIVATE);
        
        // 更新内部尺寸记录
        m_nWidth = rc.right - rc.left;
        m_nHeight = rc.bottom - rc.top;

		// 【关键】窗口大小改变后，立即触发重绘以拉伸视频
		if (m_pRawData) {
			::InvalidateRect(m_hVideoWnd, NULL, FALSE);
		}
    }
}

void CWndMediaDisplay::Paint(HDC hDC, const RECT& rcPaint)
{
    // 关键：如果视频可见，我们不绘制任何背景，让底层的 HWND 显示视频
    // 这样可以避免 Duilib 重绘背景导致视频闪烁
    if (m_bVideoVisible && m_hVideoWnd)
    {
        return;
    }

    // 如果视频不可见或窗口未创建，绘制黑色背景
    CRenderEngine::DrawColor(hDC, rcPaint, 0xFF000000);
}

void CWndMediaDisplay::SetVideoVisible(bool bVisible)
{
    m_bVideoVisible = bVisible;
    if (m_hVideoWnd)
    {
        ::ShowWindow(m_hVideoWnd, bVisible ? SW_SHOW : SW_HIDE);
    }
    Invalidate(); // 触发 Duilib 重绘以清除或显示背景
}

void CWndMediaDisplay::RenderFrame(const uint8_t* pData, int width, int height, VideoPixelFormat format)
{
    if (!m_hVideoWnd || !pData || width <= 0 || height <= 0) return;

	// 1. 保存原始数据
	int dataSize = width * height * 3; // 假设 RGB24/BGR24

	// 如果尺寸变了，重新分配缓冲区
	if (m_nRawWidth != width || m_nRawHeight != height) {
		if (m_pRawData) delete[] m_pRawData;
		m_pRawData = new uint8_t[dataSize];
		m_nRawWidth = width;
		m_nRawHeight = height;
		m_rawFormat = format;
	}

	memcpy(m_pRawData, pData, dataSize);

	// 2. 更新当前控件尺寸（用于拉伸计算）
	RECT rc;
	::GetClientRect(m_hVideoWnd, &rc);
	m_nWidth = rc.right - rc.left;
	m_nHeight = rc.bottom - rc.top;

	// 3. 触发子窗口重绘
	::InvalidateRect(m_hVideoWnd, NULL, FALSE);

    /*
    m_nWidth = width;
    m_nHeight = height;

    // 执行 GDI 绘制
    DrawFrameGDI(pData, width, height, format);

    // 触发子窗口重绘
    ::InvalidateRect(m_hVideoWnd, NULL, FALSE);*/
}

HDC CWndMediaDisplay::GetMemDC()
{
    return m_hMemDC;
}

HBITMAP CWndMediaDisplay::GetBitmap()
{
    return m_hBitmap;
}

int CWndMediaDisplay::GetWidth()
{
    return m_nWidth;
}

int CWndMediaDisplay::GetHeight()
{
    return m_nHeight;
}

uint8_t* CWndMediaDisplay::GetRawData()
{
	return m_pRawData;
}

int CWndMediaDisplay::GetRawWidth()
{
	return m_nRawWidth;
}

int CWndMediaDisplay::GetRawHeight()
{
	return m_nRawHeight;
}


int CWndMediaDisplay::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0; 
}

void CWndMediaDisplay::DrawFrameGDI(const uint8_t* pData, int width, int height, VideoPixelFormat format)
{
    if (!m_hMemDC) return;

    // 1. 创建或重置兼容位图
    if (m_hBitmap) ::DeleteObject(m_hBitmap);
    
    HDC hScreenDC = ::GetDC(m_hVideoWnd);
    m_hBitmap = ::CreateCompatibleBitmap(hScreenDC, width, height);
    ::ReleaseDC(m_hVideoWnd, hScreenDC);
    
    ::SelectObject(m_hMemDC, m_hBitmap);

    // 2. 将数据写入位图
    if (format == PIXEL_FORMAT_RGB24 /*|| format == PIXEL_FORMAT_BGR24*/)
    {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // 顶向下 DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        // 注意：如果是 BGR24，Windows DIB 原生支持，直接绘制即可
        // 如果是 RGB24，可能需要交换 R/B 通道，或者在 FFmpeg 端输出 BGR24
        ::SetDIBitsToDevice(m_hMemDC, 0, 0, width, height, 0, 0, 0, height, pData, &bmi, DIB_RGB_COLORS);
    }
    else
    {
        // 其他格式暂不支持，填充灰色
        HBRUSH hBrush = ::CreateSolidBrush(RGB(50, 50, 50));
        RECT rc = {0, 0, width, height};
        ::FillRect(m_hMemDC, &rc, hBrush);
        ::DeleteObject(hBrush);
    }
}

// 如果需要处理子窗口的 WM_PAINT，可以子类化窗口过程，但通常 InvalidateRect + BitBlt 足够
// 这里为了简单，我们假设 Duilib 的主窗口消息循环会处理子窗口的重绘请求
// 如果视频画面不更新，可能需要手动处理 m_hVideoWnd 的 WM_PAINT



// 获取是否显示提示
bool CWndMediaDisplay::GetShowHint()
{
	return m_bShowHint;
}
