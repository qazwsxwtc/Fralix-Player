#include "CVideoPlayerFrame.h"
#include "CCommonTypedef.h"

#include "CVideoPlayerFrame.h"
#include <sstream>
#include <iomanip>
#include <commctrl.h>
#include "WndMediaDisplay.h"
#include "..\DuiLib\Core\UIDlgBuilder.h" // 确保包含此头文件


// 自定义控件创建回调
// 【新增】自定义控件创建回调类
class CCustomControlCallback : public IDialogBuilderCallback
{
public:
	virtual CControlUI* CreateControl(LPCTSTR pstrClass) override
	{
		if (_tcscmp(pstrClass, _T("MediaDisplay")) == 0) // 确保这里与 XML 中的标签名一致
		{
			return new CWndMediaDisplay();
		}
		return nullptr;
	}
};

CVideoPlayerFrame::CVideoPlayerFrame()
    : m_bIsPlaying(false)
    , m_bIsFullScreen(false)
    , m_nTotalDuration(0)
    , m_pVideoContainer(nullptr)
    , m_pLblFilename(nullptr)
    , m_pLblCurrTime(nullptr)
    , m_pLblTotalTime(nullptr)
    , m_pSliderProgress(nullptr)
    , m_pSliderVolume(nullptr)
    , m_pBtnPlayPause(nullptr)
    , m_pBtnStop(nullptr)
    , m_pBtnOpen(nullptr)
    , m_pBtnFullScreen(nullptr)
{

}

CVideoPlayerFrame::~CVideoPlayerFrame()
{
    Stop();
}

CDuiString CVideoPlayerFrame::GetSkinFolder()
{
	// 返回皮肤文件夹路径
	// 通常相对于可执行文件目录
	// 如果 res 文件夹在 exe 同级目录下，直接返回 "res"
	return _T("res");
}

CDuiString CVideoPlayerFrame::GetSkinFile()
{
	// 返回具体的 XML 文件名
	return _T("video_player.xml");
}

UILIB_RESOURCETYPE CVideoPlayerFrame::GetResourceType() const
{
	return UILIB_FILE;
}

LPCTSTR CVideoPlayerFrame::GetWindowClassName() const
{
    return _T("VideoPlayerFrame");
}

UINT CVideoPlayerFrame::GetClassStyle() const
{
    return CS_DBLCLKS;
}

void CVideoPlayerFrame::OnFinalMessage(HWND hWnd)
{
    delete this;
}



DuiLib::CControlUI* CVideoPlayerFrame::CreateControl(LPCTSTR pstrClass)
{
    if (_tcscmp(pstrClass, _T("MediaDisplay")) == 0) // 确保这里与 XML 中的标签名一致
    {
        return new CWndMediaDisplay();
    }
    //else if (_tcscmp(pstrClass, _T("Slider")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CSliderUI();
    //}
    //else if (_tcscmp(pstrClass, _T("Button")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CButtonUI();
    //}
    //else if (_tcscmp(pstrClass, _T("Label")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CLabelUI();
    //}
    //else if (_tcscmp(pstrClass, _T("Progress")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CProgressUI();
    //}
    //else if (_tcscmp(pstrClass, _T("Text")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CTextUI();
    //}
    //else if (_tcscmp(pstrClass, _T("Edit")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CEditUI();
    //}
    //else if (_tcscmp(pstrClass, _T("ComboUI")) == 0) // 确保这里与 XML 中的标签名一致
    //{
    //    return new CComboUI();
    //}
	
    //return WindowImplBase::CreateControl();
    return WindowImplBase::CreateControl(pstrClass);
}

LRESULT CVideoPlayerFrame::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    BOOL bHandled = TRUE;

    switch (uMsg)
    {
	case WM_KEYDOWN:
		if (wParam == VK_SPACE)
		{
			if (m_bIsPlaying)
				Pause();
			else
				Play();
			return 0;
		}
		break;
	case WM_MOUSEMOVE:
	{
        if (m_bIsPlaying)
        {
			if (!m_bShowControlBar && m_pBottomCtr) {
				ShowControlBar(true);
			}

			// 重置自动隐藏定时器
			KillTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM);
			SetTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM, 5000, NULL); // 3秒后隐藏
        }
		else
		{
			if (m_pBottomCtr && !m_pBottomCtr->IsVisible()) {
				ShowControlBar(true);
			}
		}
		
	}
    
	break;
	case WM_DROPFILES:
	{
		HDROP hDropInfo = (HDROP)wParam;
		UINT nFileCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0); // 获取文件数量

		if (nFileCount > 0)
		{
			WCHAR szFilePath[MAX_PATH] = { 0 };
			// 获取第一个文件的路径
			if (DragQueryFile(hDropInfo, 0, szFilePath, MAX_PATH) > 0)
			{
				// 调用打开文件逻辑
				OpenFile(szFilePath);
			}
		}

		DragFinish(hDropInfo); // 释放内存
		lRes = 0;
		break;
	}
    case WM_CLOSE:
        OnClose(uMsg, wParam, lParam);
        break;
    case WM_SIZE:
		lRes = OnSize(uMsg, wParam, lParam);
		break;
    case WM_TIMER:
		if (wParam == TIMER_ID_HIDE_BOTTOM) {
			// 定时器触发，隐藏控制栏
			if (m_bIsPlaying) {
				ShowControlBar(false);
			}
			KillTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM);
		}
        lRes = OnTimer(uMsg, wParam, lParam);
        break;
    case WM_DESTROY:
		::KillTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS);
		::KillTimer(m_hWnd, TIMER_ID_SEEK_DEBOUNCE); // 【新增】
        break;
	
		// 【新增】处理视频帧渲染消息
	case WM_USER_RENDER_FRAME:
	{
		uint8_t* pData = (uint8_t*)wParam;
		int lParamVal = (int)lParam;
		int w = lParamVal & 0xFFFF;
		int h = (lParamVal >> 16) & 0xFFFF;
         
		if (pData && m_pMediaDisplay) {
			// 调用渲染窗口的绘制函数
			// 假设 CVideoRenderWnd 有一个 RenderFrame 方法接受 RGB24 数据
            RECT rc = m_pMediaDisplay->GetPos();
            m_pMediaDisplay->RenderFrame(pData, w, h, PIXEL_FORMAT_RGB24);

			// 释放拷贝的数据
			delete[] pData;
		}
		lRes = 0;
		break;
	}
	case WM_USER_PLAY_END:
	{
		// 在主线程中处理播放结束逻辑
		OnPlayEnd();
		lRes = 0;
		break;
	}
    case WM_USER_PLAY_AUDIO:
    {
		break;
	}
	case WM_USER_SEEK_COMPLETE:
	{
		SeekResult* result = (SeekResult*)wParam;
		if (result) {
			// 在主线程更新 UI
			m_nSeekDuration = result->targetMs;

			if (m_pLblCurrTime) {
				std::wstring strTime;
				FormatTime(result->targetMs, strTime);
				m_pLblCurrTime->SetText(strTime.c_str());
			}

			if (m_pSliderProgress) {
				m_pSliderProgress->SetValue(result->pos);
			}
			m_audioPlayer.Resume();
			// 恢复进度条定时器
			SetTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS, TIMER_INTERVAL_MS, NULL);
			
			
			delete result;
		}

		// 释放 Seek 锁
		{
			std::lock_guard<std::mutex> lock(m_seekMutex);
			m_bIsSeeking = false;
		}
		break;
	}
	case WM_USER_SCAN_COMPLETE:
	{
		//std::vector<std::string>* pFiles = reinterpret_cast<std::vector<std::string>*>(wParam);
		//if (pFiles) {
		//	for (const auto& path : *pFiles) {
		//		// 转换为 wstring
		//		std::wstring wPath(path.begin(), path.end()); // 简单转换，建议用 MultiByteToWideChar
		//		AddToPlaylist(wPath);
		//	}
		//	delete pFiles;

		//	
		//}
		//lRes = 0;
		break;
	}
	case WM_ADDLISTITEM:
	{
		lRes = OnAddListItem(uMsg, wParam, lParam, bHandled); 

		break;
	}
    default:
        bHandled = FALSE;
    }

   /* if ( ) return lRes;*/
    if (m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
    return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
}

LRESULT CVideoPlayerFrame::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //LRESULT lRes = __super::OnSize(uMsg, wParam, lParam);
    if(wParam == SIZE_MINIMIZED) return 0;

	// 只需要调用这个即可，因为它直接基于窗口大小计算，不依赖 Duilib 布局状态
	ResizeVideoWindow();
    UpdateControlBarPos();
	// 同时通知 Duilib 更新其他控件（如按钮位置等，如果它们也是自适应的）
	m_PaintManager.NeedUpdate();

    if (m_bIsPlaying)
    {
		int vWidth = m_pFFmpegEngine->GetWidth();
		int vHeight = m_pFFmpegEngine->GetHeight();

		// 立即调整一次布局，防止第一帧出现前窗口大小不对
		AdjustVideoLayout(vWidth, vHeight);
    }

	// 每次窗口大小改变，重新计算底部控制栏的位置
	UpdateControlBarPos();
   
    return 0;
}


//
void CVideoPlayerFrame::ShowControlBar(bool bShow)
{
	if (m_pBottomCtr) {
		m_pBottomCtr->SetVisible(bShow);
		m_bShowControlBar = bShow;

		// 如果显示，确保位置正确
		if (bShow) {
			UpdateControlBarPos();
		}
	}
}

void CVideoPlayerFrame::UpdateControlBarPos()
{
	if (!m_pBottomCtr) return;

	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);

	// 防止无效矩形
	if (rcClient.right <= rcClient.left || rcClient.bottom <= rcClient.top) return;

	int nHeight = 60; // 确保这里和 XML 中的高度一致

	// 1. 设置底部控制栏位置
	m_pBottomCtr->SetPos(RECT{ 0, rcClient.bottom - nHeight, rcClient.right, rcClient.bottom });
	//m_pBottomCtr->SetVisible(true); // 确保可见

	// 2. 【关键】调整视频窗口大小，避开底部控制栏
	if (m_pMediaDisplay) {
		HWND hVideoWnd = m_pMediaDisplay->GetRenderHwnd(); // 假设你有这个接口
		if (hVideoWnd) {
			// 视频窗口的高度 = 客户区高度 - 控制栏高度
			int videoHeight = rcClient.bottom - rcClient.top - nHeight;

			::SetWindowPos(hVideoWnd, NULL,
				0, 0,
				rcClient.right - rcClient.left,
				videoHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	// 【新增】更新拖放提示标签的位置
	if (m_pLblDropHint && m_pLblDropHint->IsVisible()) {
		m_pLblDropHint->SetPos(RECT{ 0, 0, rcClient.right, rcClient.bottom - nHeight });
	}
}

LRESULT CVideoPlayerFrame::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (wParam == TIMER_ID_UPDATE_PROGRESS)
    {
        if (m_pFFmpegEngine)
        {
          
			double currentSec = m_audioPlayer.GetPlayedSeconds()*1000 + m_nSeekDuration;
			double totalSec = m_pFFmpegEngine->GetDuration();
            // 模拟进度增加 (仅用于演示，实际请替换为引擎数据)
            // m_nCurrentPos += 500; 
            // if (m_nCurrentPos > m_nTotalDuration) m_nCurrentPos = m_nTotalDuration;
 
            // 更新 UI
            if (m_pSliderProgress && m_nTotalDuration > 0)
            { 
                int nPercent = static_cast<int>((static_cast<double>(currentSec) / m_nTotalDuration) * 1000);
                if (nPercent > 1000) nPercent = 1000;
                
                // 避免滑块拖动时冲突，只在非用户交互时更新
                // 这里简化处理，直接设置值
                m_pSliderProgress->SetValue(nPercent);
            }
			// 更新时间标签，例如 "01:23 / 05:45"
			//UpdateTimeLabel(currentSec, totalSec);
            if (m_pLblCurrTime)
            {
                std::wstring strTime;
                int iMiSec = static_cast<int>(currentSec);
                FormatTime(iMiSec, strTime);
                m_pLblCurrTime->SetText(strTime.c_str());
            }
        }
    }
	else if (wParam == TIMER_ID_SEEK_DEBOUNCE)
	{
		// 定时器触发，执行真正的 Seek
		KillTimer(m_hWnd, TIMER_ID_SEEK_DEBOUNCE);

        
		if (m_nPendingSeekPos != -1) {
			int posToSeek = m_nPendingSeekPos;
			m_nPendingSeekPos = -1;

			// 【关键】启动后台线程执行耗时的 Seek 操作
			std::thread([this, posToSeek]() {
				this->SeekAsync(posToSeek);
			}).detach(); // 分离线程，让它在后台运行
		}
       // SetTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS, TIMER_INTERVAL_MS, NULL);
	}
    return 0;
}
LRESULT CVideoPlayerFrame::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

#if defined(WIN32)
	BOOL bZoomed = ::IsZoomed(m_hWnd);
	LRESULT lRes = CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	if (::IsZoomed(m_hWnd) != bZoomed)
	{
		if (!bZoomed)
		{
			/*CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(kMaxButtonControlName));
			if (pControl) pControl->SetVisible(false);
			pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(kRestoreButtonControlName));
			if (pControl) pControl->SetVisible(true);
		*/
        }
		else
		{
			/*CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(kMaxButtonControlName));
			if (pControl) pControl->SetVisible(true);
			pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(kRestoreButtonControlName));
			if (pControl) pControl->SetVisible(false);*/
		}
	}
#else
	return __super::OnSysCommand(uMsg, wParam, lParam, bHandled);
#endif

	return 0;


}
void CVideoPlayerFrame::InitRun()
{
	// 缓存控件指针

	m_pLblFilename = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_filename")));
	m_pLblCurrTime = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_curr_time")));
	m_pLblTotalTime = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_total_time")));
	m_pSliderProgress = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("slider_progress")));
	m_pSliderVolume = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("slider_volume")));
	m_pBtnPlayPause = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_play_pause")));
	m_pBtnStop = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_stop")));
	m_pBtnOpen = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_open")));
	m_pBtnFullScreen = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_fullscreen")));

	// 初始化播放列表
	m_pPlaylistList = static_cast<CListUI*>(m_PaintManager.FindControl(_T("playlist_list")));
	m_pPlaylistPanel = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("playlist_panel")));

	if (!m_pPlaylistList) {
		OutputDebugString(_T("ERROR: Playlist List not found!\n"));
	}
	if (!m_pPlaylistPanel) {
		OutputDebugString(_T("ERROR: Playlist Panel not found!\n"));
	}

	m_pBottomCtr = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(_T("bottomctr")));
	if (m_pBottomCtr) {
		// 设置为浮动
		//m_pBottomCtr->SetFloat(true);
		// 初始隐藏
		m_pBottomCtr->SetVisible(true);
		m_bShowControlBar = false; 
	}


	// 初始状态：未播放，显示提示
	ShowDropHint(true);


	// 查找自定义媒体控件
	m_pMediaDisplay = static_cast<CWndMediaDisplay*>(m_PaintManager.FindControl(_T("video_display")));

	if (m_pMediaDisplay)
	{
		HWND hVideoWnd = m_pMediaDisplay->GetRenderHwnd();
		if (hVideoWnd)
		{
			// 在这里初始化 FFmpeg 或 VLC，并将视频输出到这个 hVideoWnd
			// 例如：m_pFFmpegEngine->SetOutputWindow(hVideoWnd);
		}
	}
	// 初始状态
	UpdatePlayPauseIcon();
	// 【关键】初始状态：未播放，所以不需要自动隐藏
	m_bShowControlBar = true;
	m_bIsPlaying = false; // 确保播放状态初始为假
	
	OnScanAllLibrary();
}

void CVideoPlayerFrame::InitWindow()
{
	OutputDebugString(_T("InitWindow Called!\n")); // 检查是否被调用

	CControlUI* pRoot = m_PaintManager.GetRoot();
	if (!pRoot) {
		OutputDebugString(_T("ERROR: Root is NULL! XML Load Failed.\n"));
		return;
	}

	// 【新增】启用文件拖放支持
	DragAcceptFiles(m_hWnd, TRUE);

    InitRun();
	
}

void CVideoPlayerFrame::Notify(TNotifyUI& msg)
{
    if (_tcsicmp(msg.sType, _T("windowinit")) == 0)
    {

    }
    else if (msg.sType == _T("click"))
    {
        if (msg.pSender == m_pBtnPlayPause)
        {
			if (m_bIsPlaying)
			{
				if (m_bIsPaused)
				{
					Play();
					m_bIsPaused = false;
				}
				else
				{
					Pause();
					m_bIsPaused = true;
				}
			}
        }
        else if (msg.pSender == m_pBtnStop)
        {
            Stop();
        }
        else if (msg.pSender == m_pBtnOpen)
        {
            // 打开文件对话框
            OPENFILENAME ofn;
            WCHAR szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = m_hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Video Files\0*.mp4;*.avi;*.mkv;*.flv;*.rmvb\0All Files\0*.*\0\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                OpenFile(szFile);
            }
        }
        else if (msg.pSender == m_pBtnFullScreen)
        {
            ToggleFullScreen();
        }
        else if (msg.pSender->GetName() == _T("closebtn"))
        {
            PostQuitMessage(0);
        }
        else if (msg.pSender->GetName() == _T("minbtn"))
        {
            SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
        else if (msg.pSender->GetName() == _T("maxbtn"))
        {
            SendMessage(WM_SYSCOMMAND, IsZoomed(m_hWnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
        }
		else if (msg.pSender->GetName() == _T("btn_toggle_playlist") ||
			msg.pSender->GetName() == _T("btn_close_playlist"))
		{
			TogglePlaylistVisibility();
		}
    }
    else if (msg.sType == _T("valuechanged"))
    {
        if (msg.pSender == m_pSliderProgress)
        {
            KillTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS);
            int nPos = m_pSliderProgress->GetValue();
			// 【优化】防抖处理：不立即 Seek，而是记录位置并重启定时器
			m_nPendingSeekPos = nPos;

			// 杀死旧的定时器
			

			// 启动一个新的短定时器（例如 200ms）
			// 如果用户在 200ms 内继续拖动，这个定时器会被再次杀死并重启
			SetTimer(m_hWnd, TIMER_ID_SEEK_DEBOUNCE, 200, NULL);
        }
        else if (msg.pSender == m_pSliderVolume)
        {
            int nVol = m_pSliderVolume->GetValue();
            SetVolume(nVol);
        }
    }
	else if (_tcsicmp(msg.sType, _T("itemclick")) == 0)
	{
		if (msg.pSender) {
			// 获取点击的 Item
			CListLabelElementUI* pItem = static_cast<CListLabelElementUI*>(msg.pSender);
			if (pItem) {
				int index = static_cast<int>(pItem->GetTag());
				PlayByIndex(index);
			}
		}
	}
}

void CVideoPlayerFrame::Play()
{

	if (!m_pFFmpegEngine) return;

	if (m_pFFmpegEngine->IsPaused())
	{
		// 如果是从暂停状态恢复
		m_audioPlayer.Resume();
		m_pFFmpegEngine->Resume();
		m_bIsPlaying = true;

		// 恢复定时器
		SetTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS, TIMER_INTERVAL_MS, NULL);

		// 启动音频播放器（如果之前暂停时停止了SDL）
		// m_audioPlayer.Resume(); // 如果 CAudioPlayer 也有暂停逻辑
	}
	else if (!m_bIsPlaying)
	{
		// 如果是初次播放（OpenFile 后）
		// OpenFile 中已经调用了 StartPlayback，这里通常不需要再次调用
		// 除非 Stop() 被调用过。
		m_bIsPlaying = true;
		SetTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS, TIMER_INTERVAL_MS, NULL);
	}

	UpdatePlayPauseIcon();
}

void CVideoPlayerFrame::Pause()
{
	if (!m_pFFmpegEngine || !m_bIsPlaying) return;

	m_audioPlayer.Pause();
	// 调用解码器暂停
	m_pFFmpegEngine->Pause();
	m_bIsPlaying = true; // 标记为非播放状态（用于UI显示）

	// 停止进度条更新定时器
	KillTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS);

	// 可选：暂停音频播放器
	// 

	UpdatePlayPauseIcon();
}

void CVideoPlayerFrame::Stop()
{
 
    m_bIsPlaying = false;
    m_nSeekDuration = 0;
	// 【新增】停止时，强制显示控制栏
	
    UpdatePlayPauseIcon();
    
	// 【新增】停止时，强制显示控制栏
	//KillTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM);
    KillTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS);

	KillTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM);
	ShowControlBar(true);


    if (m_pSliderProgress) m_pSliderProgress->SetValue(0);
    if (m_pLblCurrTime) m_pLblCurrTime->SetText(L"00:00:00");

	
	
	// 释放解码器
	if (m_pFFmpegEngine) {
        m_pFFmpegEngine->StopPlayback();
		delete m_pFFmpegEngine;
		m_pFFmpegEngine = nullptr;
	}
    ShowDropHint(true);
}

void CVideoPlayerFrame::Seek(int nPos)
{
	if (!m_pFFmpegEngine || m_nTotalDuration <= 0) return;

	// nPos is 0-1000
	double percent = static_cast<double>(nPos) / 1000.0;
	int targetMs = static_cast<int>(m_nTotalDuration * percent);

	// 执行 Seek
	m_pFFmpegEngine->Seek(targetMs);

	// 更新 UI
    m_nSeekDuration = targetMs;

	if (m_pLblCurrTime)
	{
		std::wstring strTime;
		FormatTime(targetMs, strTime);
		m_pLblCurrTime->SetText(strTime.c_str());
	}

	// 更新滑块位置（防止拖动后回弹）
	if (m_pSliderProgress) {
		m_pSliderProgress->SetValue(nPos);
	}
}

void CVideoPlayerFrame::SetVolume(int nVol)
{
    
}

void CVideoPlayerFrame::ToggleFullScreen()
{
    m_bIsFullScreen = !m_bIsFullScreen;

    if (m_bIsFullScreen)
    {
        // 进入全屏
        LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        SetWindowLong(m_hWnd, GWL_STYLE, style);
        
        ShowWindow(SW_MAXIMIZE);
        
        // 隐藏控制栏 (可选，通过设置控件 visible=false)
        if (m_PaintManager.GetRoot()) {
             // 简单起见，这里不动态隐藏控件，实际项目中可以查找底部 Layout 并隐藏
        }
    }
    else
    {
        // 退出全屏
        LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
        style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        SetWindowLong(m_hWnd, GWL_STYLE, style);
        
        ShowWindow(SW_RESTORE);
        ShowControlBar(true);

		// 【关键】退出全屏时，用户通常期望看到控制栏，所以强制显示
		ShowControlBar(true);
		// 重置定时器，让用户有5秒时间操作
		KillTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM);
		SetTimer(m_hWnd, TIMER_ID_HIDE_BOTTOM, 5000, NULL);
    }
    
    // 刷新布局
    m_PaintManager.NeedUpdate();
    ResizeVideoWindow();
    UpdateControlBarPos();
}

void CVideoPlayerFrame::UpdatePlayPauseIcon()
{
	if (!m_pBtnPlayPause) return;

	if (m_bIsPlaying)
	{
		// 显示暂停图标
		m_pBtnPlayPause->SetNormalImage(_T("file='res/pause.png'"));
		m_pBtnPlayPause->SetHotImage(_T("file='res/pause_hot.png'"));
		m_pBtnPlayPause->SetAttribute(_T("tooltip"), _T("暂停 (Space)"));
	}
	else
	{
		// 显示播放图标
		m_pBtnPlayPause->SetNormalImage(_T("file='res/play.png'"));
		m_pBtnPlayPause->SetHotImage(_T("file='res/play_hot.png'"));
		m_pBtnPlayPause->SetAttribute(_T("tooltip"), _T("播放 (Space)"));
	}
}



void CVideoPlayerFrame::FormatTime(int nMs, std::wstring& strOut)
{
    int seconds = nMs / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;

    std::wstringstream ss;
    //if (hours > 0)
    {
        ss << std::setfill(L'0') << std::setw(2) << hours << L":"
           << std::setfill(L'0') << std::setw(2) << minutes << L":"
           << std::setfill(L'0') << std::setw(2) << seconds;
    }
   // else
    //{
     //   ss << std::setfill(L'0') << std::setw(2) << minutes << L":"
      //     << std::setfill(L'0') << std::setw(2) << seconds;
   // }
    strOut = ss.str();
}

void CVideoPlayerFrame::ResizeVideoWindow()
{

    if (!m_pVideoContainer) return;
 
}

bool CVideoPlayerFrame::OpenFile(const std::wstring& filePath)
{
    if (filePath.empty()) return false;

    // 1. 清理之前的资源
    Stop();
   

    if (m_pFFmpegEngine) {
        delete m_pFFmpegEngine;
        m_pFFmpegEngine = nullptr;
    }

    // 2. 转换路径
    std::string utf8Path = WStringToUTF8(filePath);

    // 3. 创建并初始化解码器
    m_pFFmpegEngine = new CFFmpegDecoder();
    m_pFFmpegEngine->SetAudioPlayer(&m_audioPlayer); // 
    if (!m_pFFmpegEngine->Open(utf8Path)) {
        delete m_pFFmpegEngine;
        m_pFFmpegEngine = nullptr;
        MessageBox(m_hWnd, L"无法打开视频文件", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    ShowDropHint(false);
    // 获取媒体信息更新 UI
    m_nTotalDuration = (int)(m_pFFmpegEngine->GetDuration())*1000; // 转换为毫秒

    if (m_pLblTotalTime) {
        std::wstring strTime;
        FormatTime(m_nTotalDuration, strTime);
        m_pLblTotalTime->SetText(strTime.c_str());
    }
    if (m_pLblFilename) {
        size_t pos = filePath.find_last_of(L"\\/");
        std::wstring fileName = (pos != std::wstring::npos) ? filePath.substr(pos + 1) : filePath;
        m_pLblFilename->SetText(fileName.c_str());
    }

	// 假设你有一个进度条控件 m_pProgress
	if (m_pSliderProgress) {
		// 设置进度条范围为 0 - 总秒数 * 1000 (毫秒) 或者直接用秒
        m_pSliderProgress->SetValue(0);
	}


    // 【关键】获取视频原始宽高，并在第一次渲染前调整布局
    int vWidth = m_pFFmpegEngine->GetWidth();
    int vHeight = m_pFFmpegEngine->GetHeight();

    SetWindowSize(vWidth, vHeight + 60);
    // 立即调整一次布局，防止第一帧出现前窗口大小不对
    AdjustVideoLayout(vWidth, vHeight);

    m_pFFmpegEngine->SetAudioCallback([this](uint8_t* data, int nb_samples, int channels, int sample_rate) {
        if (m_pMediaDisplay) 
		{
			m_audioPlayer.FeedData(data, nb_samples);

        }
    });


    // 4. 设置视频回调
    m_pFFmpegEngine->SetVideoCallback([this](uint8_t* data, int w, int h, int linesize) {
        if (m_pMediaDisplay) {
            // 【关键】如果视频分辨率发生变化（极少见），或者为了确保布局正确，可以再次调用
           // 为了性能通常只在第一帧或分辨率改变时调用 AdjustVideoLayout
            m_pMediaDisplay->RenderFrame(data, w, h, PIXEL_FORMAT_RGB24);
        }
    });

    // 【新增】设置播放结束回调
    m_pFFmpegEngine->SetPlayEndCallback([this]() {
        // 注意：这个回调在解码线程中执行！
        // 必须 PostMessage 到主线程处理 UI
        ::PostMessage(m_hWnd, WM_USER_PLAY_END, 0, 0);
    });

    m_audioPlayer.Init(44100, 2);
    
 
    // 5. 启动解码线程
   
    if (m_pFFmpegEngine) {
        m_pFFmpegEngine->StartPlayback();
    }
    m_bIsPlaying = true;
    UpdatePlayPauseIcon();

        // 启动进度条定时器
    SetTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS, TIMER_INTERVAL_MS, NULL);

     return true;
 }


void CVideoPlayerFrame::OnPlayEnd()
{
    // 1. 停止定时器
    KillTimer(m_hWnd, TIMER_ID_UPDATE_PROGRESS);
    
    // 2. 更新 UI 状态
    m_bIsPlaying = false;
	m_bIsPaused = false;
    UpdatePlayPauseIcon();
  
    // 3. 重置进度条
    if (m_pSliderProgress) {
        m_pSliderProgress->SetValue(0);
    }
    if (m_pLblCurrTime) {
        m_pLblCurrTime->SetText(L"00:00:00");
    }

    ShowDropHint(true);

	Stop();


	if (m_pFFmpegEngine) {
		delete m_pFFmpegEngine;
		m_pFFmpegEngine = nullptr;
	}
	// 4. 【新增】自动播放下一个
	PlayNextInPlaylist();
}

void CVideoPlayerFrame::PlayNextInPlaylist()
{
	if (m_playlistPaths.empty()) return;

	int nextIndex = m_nCurrentPlayIndex + 1;

	// 循环播放：如果是最后一个，回到第一个
	if (nextIndex >= (int)m_playlistPaths.size()) {
		nextIndex = 0;
	}

	// 播放
	PlayByIndex(nextIndex);
}

void CVideoPlayerFrame::AdjustVideoLayout(int videoWidth, int videoHeight)
{
    return; // 暂时禁用布局调整，避免黑边问题
	if (!m_pMediaDisplay || videoWidth <= 0 || videoHeight <= 0) return;

	// 1. 获取主窗口客户区大小
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);

	// 2. 计算可用显示区域
	// 假设底部控制栏高度为 50 (根据你的 XML <HorizontalLayout height="50">)
	const int BOTTOM_BAR_HEIGHT = 50;

	int availW = rcClient.right - rcClient.left;
	int availH = rcClient.bottom - rcClient.top - BOTTOM_BAR_HEIGHT;

	if (availW <= 0 || availH <= 0) return;

	// 3. 计算宽高比
	float videoRatio = (float)videoWidth / (float)videoHeight;
	float availRatio = (float)availW / (float)availH;

	int finalW, finalH, offsetX, offsetY;

	// 4. 决定缩放策略：保持宽高比，适应较短的边
	if (videoRatio > availRatio)
	{
		// 视频更宽，以宽度为准
		finalW = availW;
		finalH = (int)(availW / videoRatio);
		offsetX = 0;
		offsetY = (availH - finalH) / 2; // 垂直居中
	}
	else
	{
		// 视频更高，以高度为准
		finalH = availH;
		finalW = (int)(availH * videoRatio);
		offsetX = (availW - finalW) / 2; // 水平居中
		offsetY = 0;
	}

	// 5. 直接调整内部渲染窗口 (HWND) 的位置和大小
	HWND hVideoWnd = m_pMediaDisplay->GetRenderHwnd();
	if (hVideoWnd)
	{
		// 坐标相对于主窗口客户区左上角
		::SetWindowPos(hVideoWnd, NULL,
			rcClient.left + offsetX,
			rcClient.top + offsetY,
			finalW, finalH,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}

}

void CVideoPlayerFrame::SetWindowSize(int nWidth, int nHeight)
{
    if (m_hWnd == nullptr) return;

	// 获取当前屏幕工作区大小
	RECT rcWorkArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

	// 计算居中位置
	int x = rcWorkArea.left + (rcWorkArea.right - rcWorkArea.left - nWidth) / 2;
	int y = rcWorkArea.top + (rcWorkArea.bottom - rcWorkArea.top - nHeight) / 2;

	if (x < 0)
	{
        x = 0;
	}
	if (y < 0)
	{
        y = 0;
	}

	// 设置窗口位置和大小
	::SetWindowPos(m_hWnd, NULL, x, y, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);

    //// 获取当前窗口位置
    //RECT rcWnd;
    //::GetWindowRect(m_hWnd, &rcWnd);
    //
    //// 计算新的左上角坐标，保持窗口中心不变
    //int cx = (rcWnd.left + rcWnd.right) / 2;
    //int cy = (rcWnd.top + rcWnd.bottom) / 2;
    //
    //int newX = cx - nWidth / 2;
    //int newY = cy - nHeight / 2;

    //// 调整窗口大小
    //::SetWindowPos(m_hWnd, NULL, newX, newY, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

void CVideoPlayerFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(m_pFFmpegEngine)m_pFFmpegEngine->Close();
	// 2. 停止 SDL 音频
	//if (m_audioPlayer.IsInitialized()) { // 假设你有这个判断方法
	m_audioPlayer.Stop();
	//}

	

    OnPlayEnd();
	// 3. 关闭窗口
	PostQuitMessage(0);
}
 

void CVideoPlayerFrame::SeekAsync(int nPos)
{
    // 【可选】性能监控
    auto start = std::chrono::high_resolution_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_seekMutex);
        if (m_bIsSeeking) return; // 如果上一次 Seek 还没完成，忽略本次请求（或排队）
        m_bIsSeeking = true;
    }

    if (!m_pFFmpegEngine || m_nTotalDuration <= 0) {
        std::lock_guard<std::mutex> lock(m_seekMutex);
        m_bIsSeeking = false;
        return;
    }

    // 1. 计算目标时间
    double percent = static_cast<double>(nPos) / 1000.0;
    int targetMs = static_cast<int>(m_nTotalDuration * percent);

	
    // 2. 【耗时操作】在后台线程执行底层 Seek
    // 这包括 av_seek_frame, flush_buffers, 以及 CAudioPlayer::Restart()
    m_pFFmpegEngine->Seek(targetMs);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "[Perf] Async Seek execution time: " << duration.count() << " ms" << std::endl;

  
	// 3. 确保音频播放器也重置
	m_audioPlayer.Flush(); // 或者 Restart()
	m_bIsPaused = false;
	m_bIsPlaying = true;
	UpdatePlayPauseIcon();

    SeekResult* result = new SeekResult{ nPos, targetMs };
	
    // 定义一个自定义消息用于更新 UI
    // #define WM_USER_SEEK_COMPLETE (WM_USER + 102)
    ::PostMessage(m_hWnd, WM_USER_SEEK_COMPLETE, (WPARAM)result, 0);
}

//
void CVideoPlayerFrame::ShowDropHint(bool bShow) {
	if (m_pMediaDisplay) {
		m_pMediaDisplay->ShowHint(bShow);
	}
}

void CVideoPlayerFrame::ScanLibrary() {
//	// 1. 选择扫描根目录 (例如用户的主目录或特定文件夹)
//#ifdef _WIN32
//	std::string root = "D:\\Videos"; // Windows 示例
//#else
//	std::string root = "/home/user/Videos"; // Linux/Mac 示例
//#endif
//
//	std::cout << "Start scanning..." << std::endl;
//
//	// 2. 执行扫描 (限制深度为 5 层，防止扫描整个磁盘耗时过长)
//	std::vector<std::string> videos = VideoScanner::Scan(root, 5);
//
//	std::cout << "Found " << videos.size() << " video files." << std::endl;
//
//	// 3. 处理结果 (例如添加到播放列表)
//	for (const auto& path : videos) {
//		std::wstring wPath(path.begin(), path.end()); // 简单转换，实际建议用 UTF-8 转换工具
//		std::wcout << L"  - " << wPath << std::endl;
//
//		// m_playlist.Add(wPath); 
//	}
}


void CVideoPlayerFrame::TogglePlaylistVisibility()
{
	if (m_pPlaylistPanel) {
		bool isVisible = m_pPlaylistPanel->IsVisible();
		m_pPlaylistPanel->SetVisible(!isVisible);

		// 关键：通知 PaintManager 重新计算布局
		m_PaintManager.NeedUpdate();

		// 如果视频正在播放，可能需要调整视频窗口大小以填补空缺
		if (m_bIsPlaying && m_pFFmpegEngine) {
			AdjustVideoLayout(m_pFFmpegEngine->GetWidth(), m_pFFmpegEngine->GetHeight());
		}
		// 然后手动同步 HWND
			if (m_pMediaDisplay) {
				RECT rc = m_pMediaDisplay->GetPos(); // 获取 Duilib 计算后的新位置
				HWND hVideoWnd = m_pMediaDisplay->GetRenderHwnd();
				if (hVideoWnd) {
					::SetWindowPos(hVideoWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
				}
			}
	}
}


//
void CVideoPlayerFrame::PlayByIndex(int index)
{
    if (index < 0 || index >= (int)m_playlistPaths.size()) return;
    
    m_nCurrentPlayIndex = index;
    OpenFile(m_playlistPaths[index]);
    //UpdatePlaylistUI(); // 更新高亮
}

LRESULT CVideoPlayerFrame::OnAddListItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	std::string* pStrPath = reinterpret_cast<std::string*>(lParam);
	if (!pStrPath) return 0;

	// 2. 获取字符串内容
	std::string strPath = *pStrPath;

	// 3. 【关键】释放内存，防止内存泄漏
	delete pStrPath;

	std::wstring path = UTF8ToWString(strPath);

	if (std::find(m_playlistPaths.begin(), m_playlistPaths.end(), path) != m_playlistPaths.end())
	{
		//m_playlistPaths.find(path) == m_playlistPaths.end()
		return -1;
	}
	m_playlistPaths.push_back(path);
	size_t pos = path.find_last_of(L"\\/");
	std::wstring fileName = (pos != std::wstring::npos) ? path.substr(pos + 1) : path;
	// 6. 创建并添加列表项
	if (m_pPlaylistList) {
		CListLabelElementUI* pListElement = new CListLabelElementUI();
        pListElement->SetTag(m_playlistPaths.size() - 1);
		pListElement->SetText(fileName.c_str());
		pListElement->SetToolTip(path.c_str());
		//pListElement->SetUserData(fileName.c_str());
		pListElement->SetBkColor(0xFFFFFFFF);
		pListElement->SetAttribute(_T("textcolor"), _T("#FF000000"));
		//pListElement->SetAttribute(_T("textcolor"), _T("#FF000000"));
		// 设置颜色回调（可选，确保文字可见）
		//pListElement->SetTextCallback([](CControlUI* pCtrl, int nIndex, UINT uState) -> DWORD {
		//	if (uState & UISTATE_SELECTED) return 0xFF00BFFF;
		//	if (uState & UISTATE_HOT) return 0xFFDDDDDD;
		//	return 0xFFFFFFFF; // 默认白色
		//});

		m_pPlaylistList->Add(pListElement);
	}
	return 0;
}

void CVideoPlayerFrame::OnScanLibraryOne()
{
	
}

// 扫描文件
void CVideoPlayerFrame::OnScanAllLibrary()
{
	// 启动后台线程
	std::thread([this]() {
		std::vector<std::string> drives = CVideoScanner::GetAllDrives();
		std::vector<std::string> allVideos;
		std::mutex mtx;

		std::vector<std::thread> threads;
		for (const auto& drive : drives) {
			threads.emplace_back([&, drive]() {
				std::vector<std::string> local;
				// 扫描该磁盘，深度限制为 5 层以加快速度，或者 -1 全盘
				CVideoScanner::ScanDirectory(drive, 0, -1, local, nullptr);

				std::lock_guard<std::mutex> lock(mtx);
				allVideos.insert(allVideos.end(), local.begin(), local.end());
			});
		}
		 
		for (auto& t : threads) if (t.joinable()) t.join();

		/*std::sort(allVideos.begin(), allVideos.end());*/


		for (auto& path : allVideos)
		{
			auto * pData = new std::string(std::move(path));
			::PostMessage(m_hWnd, WM_ADDLISTITEM, 0, (LPARAM)pData);
		}
		// 发送完成消息，携带所有文件路径
		// 注意：大数据量拷贝可能慢，建议用指针或共享指针
		//auto* pData = new std::vector<std::string>(std::move(allVideos));
		

	}).detach();
}