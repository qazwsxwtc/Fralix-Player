#pragma once
#include "include\DuiLib\UIlib.h"
using namespace DuiLib;

class CVolumePopupWnd : public CWindowWnd, public INotifyUI
{
public:
    CVolumePopupWnd();
    ~CVolumePopupWnd();


	// 【新增】显示窗口
	void ShowVolume(HWND hParent, RECT rcAnchor, int nCurrentVol);

	// 【新增】隐藏窗口
	void HideVolume();

    // 显示窗口在指定控件下方/上方
    void ShowAtPoint(HWND hParent, RECT rcAnchor, int nCurrentVol);

    LPCTSTR GetWindowClassName() const override;
    UINT GetClassStyle() const override;
    void OnFinalMessage(HWND hWnd) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    void Notify(TNotifyUI& msg) override;

    // 获取当前滑块的值的接口
    int GetVolume() const;

  
private:
	void UpdatePercentText(int vol);
	void UpdatePosition(RECT rcAnchor); // 辅助函数：更新位置


private:
    CPaintManagerUI m_PaintManager;
    CSliderUI*      m_pSlider = nullptr;
    CLabelUI*       m_pLblPercent = nullptr;
    CButtonUI*      m_pBtnMute = nullptr;
    int             m_nLastVolume = 100;
    HWND            m_hParentWnd; // 保存父窗口句柄
};

