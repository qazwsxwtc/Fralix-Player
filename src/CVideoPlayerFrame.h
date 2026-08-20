#ifndef CVideoPlayerFrame_H
#define CVideoPlayerFrame_H


#include "include\DuiLib\UIlib.h"
#include <string>
#include <windows.h>
#include "CVideoRenderWnd.h"
#include "CFFmpegDecoder.h"
#include "WndMediaDisplay.h"
#include "VideoScanner.h"
#include "VolumePopupWnd.h"
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


    void ScanLibrary();

    void TogglePlaylistVisibility();
    void OnScanLibraryOne();//扫描所有文件，扫描一次，导出一次
    void OnScanAllLibrary();//扫描所有文件，一次性扫描出所有文件

    void SetPlaybackRate(double rate); //倍速播放
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

	//void Init Playlist();
	/*void AddToPlaylist(const std::wstring& filePath);
	void UpdatePlaylistUI();*/
	void PlayByIndex(int index);
    void PlayNextInPlaylist();
    LRESULT OnAddListItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void ToggleVolumePanel();

	void FastForward(int seconds = 10); // 快进，默认10秒
	void FastRewind(int seconds = 10);  // 快退，默认10秒

    void ShowVolumePopup();

private:
    // === UI 控件指针缓存 ===
   // CPaintManagerUI m_PaintManager;
    
    // 视频渲染容器 (可能是 ActiveXUI 或自定义 HWND 容器)
    CControlUI* m_pVideoContainer = nullptr;
    CLabelUI*   m_pLblFilename = nullptr;
    CLabelUI*   m_pLblCurrTime = nullptr;
    CLabelUI*   m_pLblTotalTime = nullptr;
    CSliderUI*  m_pSliderProgress = nullptr;
    //CSliderUI*  m_pSliderVolume = nullptr;
    CButtonUI*  m_pBtnPlayPause = nullptr;
    CButtonUI*  m_pBtnStop = nullptr;
    CButtonUI*  m_pBtnOpen = nullptr;
    CButtonUI*  m_pBtnFullScreen = nullptr;
    CListUI* m_pPlaylistList = nullptr;
    CVerticalLayoutUI* m_pPlaylistPanel = nullptr;
    //CVerticalLayoutUI* m_pVolumePanel = nullptr;
    CWndMediaDisplay* m_pMediaDisplay = nullptr;
    CLabelUI* m_pLblDropHint = nullptr; // 【新增】拖放提示标签
    CFFmpegDecoder* m_pFFmpegEngine = nullptr;
	
    CAudioPlayer m_audioPlayer; // 音频播放器实例
    CVerticalLayoutUI* m_pBottomCtr = nullptr; // 底部控制栏指针
    CButtonUI* m_pBtnVolume = nullptr;      // 音量按钮
    // === 播放状态 ===
    bool m_bIsPlaying = false;
    bool m_bIsPaused = false;
    bool m_bIsFullScreen = false;
    int m_nTotalDuration = 0; // 毫秒
    //int m_nCurrentPos;    // 毫秒
    int m_nSeekDuration = 0;
    
	int m_nPendingSeekPos = -1; // 待处理的 Seek 位置
  
	std::mutex m_seekMutex;   // 【新增】保护 Seek 状态
	bool m_bIsSeeking = false; // 【新增】标记是否正在 Seek
	bool m_bShowControlBar = true;          // 是否显示控制栏
   
    std::vector<std::wstring> m_playlistPaths;
    int  m_nCurrentPlayIndex = -1;
	

    int m_nCurrentPlayMs = 0;

	//double m_playbackRate = 1.0;          // 当前播放倍速
	ULONGLONG m_lastRenderTime = 0;       // 上一帧渲染的时间戳 (ms)
	double m_frameDurationMs = 33.33;     // 默认帧间隔 (假设30fps, 1000/30)
	std::mutex m_renderMutex;             // 保护渲染时间变量

    CVolumePopupWnd* m_pVolumepWnd = nullptr;
};

#endif // CVideoPlayerFrame_H