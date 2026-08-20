#ifndef CVideoPlayerFrame_H
#define CVideoPlayerFrame_H


#include "include\DuiLib\UIlib.h"
#include <string>
#include <windows.h>
#include "CVideoRenderWnd.h"
#include "CFFmpegDecoder.h"
#include "WndMediaDisplay.h"
#include "VideoScanner.h"
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
private:
    // === UI 控件指针缓存 ===
   // CPaintManagerUI m_PaintManager;
    
    // 视频渲染容器 (可能是 ActiveXUI 或自定义 HWND 容器)
    CControlUI* m_pVideoContainer = nullptr;
    CLabelUI*   m_pLblFilename = nullptr;
    CLabelUI*   m_pLblCurrTime = nullptr;
    CLabelUI*   m_pLblTotalTime = nullptr;
    CSliderUI*  m_pSliderProgress = nullptr;
    CSliderUI*  m_pSliderVolume = nullptr;
    CButtonUI*  m_pBtnPlayPause = nullptr;
    CButtonUI*  m_pBtnStop = nullptr;
    CButtonUI*  m_pBtnOpen = nullptr;
    CButtonUI*  m_pBtnFullScreen = nullptr;
    CListUI* m_pPlaylistList = nullptr;
    CVerticalLayoutUI* m_pPlaylistPanel = nullptr;
    CVerticalLayoutUI* m_pVolumePanel = nullptr;
   CWndMediaDisplay* m_pMediaDisplay = nullptr;
    // === 播放状态 ===
    bool m_bIsPlaying;
    bool m_bIsPaused = false;
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
   
    std::vector<std::wstring> m_playlistPaths;
    int  m_nCurrentPlayIndex = -1;
	
	// ... 其他成员 ...
	CButtonUI* m_pBtnVolume;      // 音量按钮
	//CSliderUI* m_pSliderVolume;   // 音量滑动条
	int m_nCurrentVolume;         // 当前音量值 (0-100)
};

#endif // CVideoPlayerFrame_H