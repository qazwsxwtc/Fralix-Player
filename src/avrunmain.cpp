#include "CVideoPlayerFrame.h"
#include "WndMediaDisplay.h"
#include "include\DuiLib\Core\UIDlgBuilder.h"

// 自定义控件创建回调
CControlUI* CustomControlFactory(LPCTSTR pstrClass, void* pManager)
{
	if (_tcscmp(pstrClass, DUI_CTR_MEDIADISPLAY) == 0)
	{
		return new CWndMediaDisplay();
	}
	return nullptr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	CPaintManagerUI::SetInstance(hInstance);
	//CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	//CDialogBuilder::RegisterCustomControl(CustomControlFactory);

	HRESULT Hr = ::CoInitialize(NULL);
	if (FAILED(Hr)) return 0;

	CVideoPlayerFrame* pFrame = new CVideoPlayerFrame();
	if (pFrame == NULL) return 0;

	// 创建窗口 (WS_OVERLAPPEDWINDOW 风格由 Duilib 内部模拟或指定)
	pFrame->Create(NULL, _T("Video Player"), UI_WNDSTYLE_FRAME, WS_EX_STATICEDGE | WS_EX_APPWINDOW, 0,0,600,800);
	pFrame->CenterWindow();
	pFrame->ShowWindow(true);

	// Duilib 的消息循环
	CPaintManagerUI::MessageLoop();

	::CoUninitialize();
	return 0;
}