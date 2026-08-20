#include "VolumePopupWnd.h"
#include <sstream>

#ifndef UI_WNDSTYLE_POPUP
#define UI_WNDSTYLE_POPUP       (WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#endif
#include "CCommonTypedef.h"

CVolumePopupWnd::CVolumePopupWnd()
{
}

CVolumePopupWnd::~CVolumePopupWnd()
{
}

LPCTSTR CVolumePopupWnd::GetWindowClassName() const
{
    return _T("VolumePopupWnd");
}

UINT CVolumePopupWnd::GetClassStyle() const
{
    return CS_DBLCLKS;
}

void CVolumePopupWnd::OnFinalMessage(HWND hWnd)
{
    delete this;
}

// 【实现】显示窗口
void CVolumePopupWnd::ShowVolume(HWND hParent, RECT rcAnchor, int nCurrentVol)
{
	m_hParentWnd = hParent;
	m_nLastVolume = nCurrentVol;

	// 1. 如果窗口尚未创建，则创建并初始化
	if (!m_hWnd) {
		Create(hParent, _T(""), UI_WNDSTYLE_POPUP, WS_EX_TOPMOST);

		m_PaintManager.Init(m_hWnd);
		CDialogBuilder builder;
		// 假设 xml 在 res 目录下
		CControlUI* pRoot = builder.Create(_T("volume_popup.xml"), (LPCTSTR)_T("res\\"), nullptr, &m_PaintManager);
		if (!pRoot) {
			// 尝试当前目录
			pRoot = builder.Create(_T("volume_popup.xml"), (LPCTSTR)nullptr, nullptr, &m_PaintManager);
		}

		if (pRoot) {
			m_PaintManager.AttachDialog(pRoot);
			m_PaintManager.AddNotifier(this);

			m_pSlider = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("slider_vol_popup")));
			m_pLblPercent = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_vol_percent")));
			m_pBtnMute = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_mute")));
		}
	}

	// 2. 更新 UI 状态
	if (m_pSlider) {
		m_pSlider->SetValue(nCurrentVol);
	}
	UpdatePercentText(nCurrentVol);

	// 3. 更新位置并显示
	UpdatePosition(rcAnchor);
	ShowWindow(SW_SHOW);
	SetForegroundWindow(m_hWnd); // 确保获得焦点以便检测 WM_KILLFOCUS
}

// 【实现】隐藏窗口
void CVolumePopupWnd::HideVolume()
{
	if (m_hWnd && IsWindowVisible(m_hWnd)) {
		ShowWindow(SW_HIDE);
	}
}

// 【辅助】更新窗口位置
void CVolumePopupWnd::UpdatePosition(RECT rcAnchor)
{
	if (!m_hWnd) return;

	int nWidth = 10;
	int nHeight = 100;

	// 将锚点坐标转换为屏幕坐标
	POINT ptTopLeft = { rcAnchor.left, rcAnchor.top };
	POINT ptBottomRight = { rcAnchor.right, rcAnchor.bottom };
	ClientToScreen(m_hParentWnd, &ptTopLeft);
	ClientToScreen(m_hParentWnd, &ptBottomRight);
	RECT rcScreenAnchor = { ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y };

	int x = rcScreenAnchor.left + (rcScreenAnchor.right - rcScreenAnchor.left - nWidth) / 2;
	int y = rcScreenAnchor.top - nHeight - 5;

	// 边界检查
	RECT rcWorkArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

	if (y < rcWorkArea.top) {
		y = rcScreenAnchor.bottom + 5;
	}
	if (x < rcWorkArea.left) x = rcWorkArea.left;
	if (x + nWidth > rcWorkArea.right) x = rcWorkArea.right - nWidth;
	if (y + nHeight > rcWorkArea.bottom) y = rcWorkArea.bottom - nHeight;

	SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, nWidth, nHeight, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CVolumePopupWnd::ShowAtPoint(HWND hParent, RECT rcAnchor, int nCurrentVol)
{
	m_nLastVolume = nCurrentVol;

	// 1. 创建窗口
	// 使用 WS_POPUP 风格，不带标题栏
	if (!Create(hParent, _T(""), UI_WNDSTYLE_POPUP, WS_EX_TOPMOST)) {
		return;
	}

	// 2. 初始化 XML
	m_PaintManager.Init(m_hWnd);
	CDialogBuilder builder;
	// 假设 xml 在 exe 同级目录的 res 文件夹下
	CControlUI* pRoot = builder.Create(_T("volume_popup.xml"), (LPCTSTR)_T("res\\"), nullptr, &m_PaintManager);

	if (!pRoot) {
		// 调试用：如果加载失败，尝试当前目录
		pRoot = builder.Create(_T("volume_popup.xml"), (LPCTSTR)nullptr, nullptr, &m_PaintManager);
		if (!pRoot) {
			MessageBox(hParent, _T("Cannot find volume_popup.xml"), _T("Error"), MB_OK);
			DestroyWindow(m_hWnd);
			return;
		}
	}

	m_PaintManager.AttachDialog(pRoot);
	m_PaintManager.AddNotifier(this);

	// 3. 绑定控件
	m_pSlider = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("slider_vol_popup")));
	/*m_pLblPercent = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_vol_percent")));
	m_pBtnMute = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_mute")));*/

	if (m_pSlider) {
		m_pSlider->SetValue(nCurrentVol);
	}
	UpdatePercentText(nCurrentVol);

	// 4. 计算位置
	int nWidth = 10;
	int nHeight = 100;

	// 将锚点坐标转换为屏幕坐标
	POINT ptTopLeft = { rcAnchor.left, rcAnchor.top };
	POINT ptBottomRight = { rcAnchor.right, rcAnchor.bottom };
	ClientToScreen(hParent, &ptTopLeft);
	ClientToScreen(hParent, &ptBottomRight);
	RECT rcScreenAnchor = { ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y };

	int x = rcScreenAnchor.left + (rcScreenAnchor.right - rcScreenAnchor.left - nWidth) / 2;
	int y = rcScreenAnchor.top - nHeight - 5;

	// 边界检查
	RECT rcWorkArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

	if (y < rcWorkArea.top) {
		y = rcScreenAnchor.bottom + 5;
	}
	if (x < rcWorkArea.left) x = rcWorkArea.left;
	if (x + nWidth > rcWorkArea.right) x = rcWorkArea.right - nWidth;
	if (y + nHeight > rcWorkArea.bottom) y = rcWorkArea.bottom - nHeight;

	// 5. 显示窗口
	SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, nWidth, nHeight, SWP_SHOWWINDOW | SWP_NOACTIVATE);

	// 确保窗口获得焦点以便接收鼠标消息（可选，取决于你是否希望点击外部关闭）
	// SetForegroundWindow(m_hWnd); 
}

int CVolumePopupWnd::GetVolume() const
{
    if (m_pSlider) return m_pSlider->GetValue();
    return m_nLastVolume;
}

void CVolumePopupWnd::UpdatePercentText(int vol)
{
    if (m_pLblPercent) {
        std::wstring str = std::to_wstring(vol) + L"%";
        m_pLblPercent->SetText(str.c_str());
    }
    if (m_pBtnMute) {
        if (vol == 0) m_pBtnMute->SetText(_T("🔇"));
        else if (vol < 50) m_pBtnMute->SetText(_T("🔉"));
        else m_pBtnMute->SetText(_T("🔊"));
    }
}

LRESULT CVolumePopupWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    BOOL bHandled = TRUE;

    switch (uMsg)
    {
    case WM_KILLFOCUS:
        // 失去焦点时自动关闭（可选）
        // PostQuitMessage(0); 
        break;
    case WM_MOUSEACTIVATE:
        // 防止点击窗口外部时立即关闭，根据需要处理
        break;
    default:
        bHandled = FALSE;
    }

    if (bHandled) return lRes;
    if (m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

void CVolumePopupWnd::Notify(TNotifyUI& msg)
{
    if (msg.sType == _T("valuechanged"))
    {
        if (msg.pSender == m_pSlider)
        {
            int vol = m_pSlider->GetValue();
            UpdatePercentText(vol);
			::PostMessage(m_hParentWnd, WM_VOLUME_CHANGED, vol, 0);
            // 这里可以发送消息给主窗口，或者主窗口轮询 GetVolume()
        }
    }
    else if (msg.sType == _T("click"))
    {
        if (msg.pSender == m_pBtnMute)
        {
            int vol = m_pSlider->GetValue();
            if (vol > 0) {
                m_nLastVolume = vol;
                m_pSlider->SetValue(0);
                UpdatePercentText(0);
            } else {
                m_pSlider->SetValue(m_nLastVolume > 0 ? m_nLastVolume : 50);
                UpdatePercentText(m_nLastVolume);
            }
        }
    }
}