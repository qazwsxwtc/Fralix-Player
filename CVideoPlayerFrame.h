#ifndef CVideoPlayerFrame_H
#define CVideoPlayerFrame_H


#include "..\DuiLib\UIlib.h"
#include <string>
#include <windows.h>
#include "CVideoRenderWnd.h"
#include "CFFmpegDecoder.h"
#include "WndMediaDisplay.h"

using namespace DuiLib;



class CVideoPlayerFrame : public WindowImplBase/*public CWindowWnd, public INotifyUI, , public IMessageFilterUI*/
{
public:
    CVideoPlayerFrame();
    virtual ~CVideoPlayerFrame();

	virtual CDuiString GetSkinFolder() override;
    virtual CDuiString GetSkinFile() override;
    UILIB_RESOURCETYPE GetResourceType() const override;
    // === CWindowWnd 接口实现 ===
    LPCTSTR GetWindowClassName() const override;
    UINT GetClassStyle() const override;
    void OnFinalMessage(HWND hWnd) override;
    virtual CControlUI* CreateControl(LPCTSTR pstrClass);
    // === 消息处理 ===
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    //LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // === UI 通知处理 (INotifyUI) ===
    void Notify(TNotifyUI& msg) override;

    // === 公共业务方法 ===
    void InitWindow();
    bool OpenFile(const std::wstring& filePath);
    void Play();
    void Pause();
    
    void Stop();
    void Seek(int nPos); // 0-1000
    void SetVolume(int nVol); // 0-100
    void ToggleFullScreen();
   
    void InitRun();

    void OnPlayEnd();
    //bool OpenFile(const std::wstring& filePath);

    void AdjustVideoLayout(int videoWidth, int videoHeight);

protected:
	// === 内部辅助方法 ===
	void UpdatePlayPauseIcon();
	void FormatTime(int nMs, std::wstring& strOut);
	void ResizeVideoWindow();
    void SetWindowSize(int nWidth, int nHeight);
    void OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void SeekAsync(int nPos); // 【新增】异步 Seek 入口
    virtual LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void ShowControlBar(bool bShow);
	void UpdateControlBarPos();      // 更新浮动位置

    void ShowDropHint(bool bShow);

	// 更新播放按钮 UI 状态
	void UpdatePlayButtonUI();

private:
    // === UI 控件指针缓存 ===
   // CPaintManagerUI m_PaintManager;
    
    // 视频渲染容器 (可能是 ActiveXUI 或自定义 HWND 容器)
    CControlUI* m_pVideoContainer; 
    CLabelUI*   m_pLblFilename;
    CLabelUI*   m_pLblCurrTime;
    CLabelUI*   m_pLblTotalTime;
    CSliderUI*  m_pSliderProgress;
    CSliderUI*  m_pSliderVolume;
    CButtonUI*  m_pBtnPlayPause;
    CButtonUI*  m_pBtnStop;
    CButtonUI*  m_pBtnOpen;
    CButtonUI*  m_pBtnFullScreen;

   CWndMediaDisplay* m_pMediaDisplay = nullptr;
    // === 播放状态 ===
    bool m_bIsPlaying;
    bool m_bIsFullScreen;
    int m_nTotalDuration = 0; // 毫秒
    //int m_nCurrentPos;    // 毫秒
    int m_nSeekDuration = 0;
    
    CLabelUI* m_pLblDropHint; // 【新增】拖放提示标签
    
    // 假设你有一个底层的播放器引擎类 (如 VLC, FFmpeg wrapper)
    // 这里用 void* 占位，实际开发中替换为你的播放器实例指针
    
    CFFmpegDecoder* m_pFFmpegEngine = nullptr;

	

	// 【新增】音频播放器
	CAudioPlayer m_audioPlayer; // 音频播放器实例
	

	int m_nPendingSeekPos = -1; // 待处理的 Seek 位置
   

	std::mutex m_seekMutex;   // 【新增】保护 Seek 状态
	bool m_bIsSeeking = false; // 【新增】标记是否正在 Seek


	CVerticalLayoutUI* m_pBottomCtr; // 底部控制栏指针
	bool m_bShowControlBar;          // 是否显示控制栏
   
	
	
};

#endif // CVideoPlayerFrame_H