#ifndef _CCOMMONTYPEDEF_H_
#define _CCOMMONTYPEDEF_H_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <cctype>
#include <codecvt> // C++17 deprecated but still works, or use iconv/widechar_to_utf8 on linux

// 定时器 ID
#define TIMER_ID_UPDATE_PROGRESS 1001
#define TIMER_ID_SEEK_DEBOUNCE 1002
#define TIMER_ID_HIDE_BOTTOM 1003
#define TIMER_INTERVAL_MS 500 // 每500ms更新一次进度条

#define WM_USER_RENDER_FRAME (WM_USER + 1000 )
#define WM_USER_PLAY_END (WM_USER + 1001 )
#define WM_USER_PLAY_AUDIO (WM_USER + 1002 )
#define WM_USER_SEEK_COMPLETE (WM_USER + 1003)
#define WM_USER_SCAN_COMPLETE (WM_USER + 1004)
#define WM_ADDLISTITEM (WM_USER + 1005)

// 在 CFFmpegDecoder.cpp 顶部或 .h文件中
const int MAX_VIDEO_PACKET_QUEUE_SIZE = 100;
const int MAX_AUDIO_PACKET_QUEUE_SIZE = 100;
const int MAX_VIDEO_FRAME_QUEUE_SIZE = 3;   // 已解码帧，少一点以降低延迟
const int MAX_AUDIO_FRAME_QUEUE_SIZE = 15;  // 已解码音频，多一点以防卡顿

// 简单的像素格式枚举，实际项目中可能使用更复杂的结构
enum VideoPixelFormat {
	PIXEL_FORMAT_RGB24,
	PIXEL_FORMAT_RGB32,
	PIXEL_FORMAT_YUV420P,
	PIXEL_FORMAT_NV12
};

enum class VideoRenderMode {
    RENDER_MODE_OPENGL,
    RENDER_MODE_DIRECTX,
    RENDER_MODE_D3D11
};

enum class VideoRenderType {
    RENDER_TYPE_WINDOW,
    RENDER_TYPE_PIPE
};  

enum class VideoRenderState {//播放状态
    RENDER_STATE_INIT,
    RENDER_STATE_PLAYING,
    RENDER_STATE_PAUSED,
    RENDER_STATE_STOPPED
};


enum class ContainerFormatsType {
    CONTAINER_FORMAT_MP4,
    CONTAINER_FORMAT_AVI,
    CONTAINER_FORMAT_MKV,
    CONTAINER_FORMAT_MOV,
    CONTAINER_FORMAT_TS,
    CONTAINER_FORMAT_M2TS,
    CONTAINER_FORMAT_MTS,
    CONTAINER_FORMAT_MPEG,
    CONTAINER_FORMAT_FLV,
    CONTAINER_FORMAT_WMV,
    CONTAINER_FORMAT_WEBM,
    CONTAINER_FORMAT_OGG,
    CONTAINER_FORMAT_M4A,
    CONTAINER_FORMAT_M4V,
};

enum class VideoDecoderType {
    D_FFMPEG,
	D_H263CODEC,//H.263,H263,H263+,H263++,H263-2000,H263-2001,H263-2002,H263-2003,H263-2004,H263-2005,H263-2006,H263-2007,H263-2008,H263-2009,H263-2010,H263-2011,H263-2012,H263-2013,H263-2014,H263-2015,H263-2016,H263-2017,H263-2018,H263-2019,H263-2020
    D_H264CODEC,//AVC
    D_H265CODEC,//HEVC
	D_X264CODEC,//X264,H264,AVC
	D_X265CODEC,//X265,H265,HEVC
    D_AV1CODEC,//AV1
    D_VP8CODEC,//VP8
    D_VP9CODEC,//VP9
	D_MPEG1CODEC,//MPEG1
    D_MPEG2CODEC,//MPEG2
    D_MPEG4CODEC,//MPEG4
    D_AVICODEC,//AVI,XVID/DIVX
    D_WMVCODEC,//WMV,WMV1,WMV2,WMV3,WVC1,WMVA,WMVP,WMVX,WMVQ,WMVJ,WMVZ,WMVB,WMVG,WMVH,WMVI,WMVJ,WMVK,WMVL,WMMV,WMMQ,WMMR,WMMT,WMMU,WMMV,WMMW,WMMX,WMMY,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,WMMZ,
    D_VC1CODEC,//VC1,WVC1,WMV3
    D_AVSCODEC,//AVS,Avisynth, 
    D_MJPEGCODEC,//MJPEG,MJPG,Motion JPEG
    D_MJPEG2000CODEC,//MJ2,MJP2,Motion JPEG 2000
    D_DNXHDCODEC,// 专业音频格式，影视后期素材

};

enum class AudioDecoderType {
    D_FFMPEG,
    D_PCMODEC,
    D_OPUSCODEC,
    D_MP3CODEC,
    D_AACODEC,
    D_AC3CODEC,
    D_VORBISCODEC,
    D_FLACCODEC,
    D_DTSCODEC,
    D_EAC3CODEC,
    D_TRUEHDCODEC,

};

// 3. 【关键】回到主线程更新 UI
  // 使用 PostMessage 确保线程安全
struct SeekResult {
	int pos;
	int targetMs;
};

/**
 * @brief wstring (UTF-16) 转换为 string (UTF-8)
 * @details 用于将 Duilib 编辑框内容转换为 Git 命令所需的 UTF-8 字符串
 */
std::string WStringToUTF8(const std::wstring& wstr);

std::wstring UTF8ToWString(const std::string& str);





#endif
