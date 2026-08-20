
# Fralix‑Player 🎬
> 自研Windows播放器，Duilib + FFmpeg + SDL3

**简体中文** | [English](README.md)

## ✨ 功能特性
- FFmpeg解码音视频  V8.1.2
- SDL3音频输出，支持运行时音量调节 `SDL_SetAudioStreamGain`  V3.4.14
- Duilib UI界面
- 支持常见媒体格式

## 🚀 快速构建
- download ffmpeg
	https://github.com/FFmpeg/FFmpeg/releases/tag/n8.1.2
	https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-8.1.2-full_build.7z

- download SDL3
	https://github.com/libsdl-org/SDL/releases   
	SDL3-devel-3.4.14-VC.zip
	
- download duilib 
	git https://github.com/duilib/duilib.git

- 你需要把dll目录下所有库文件，和整个res目录拷贝到Fralix-Player.exe运行目录下
-   目录结构为：
--   Fralix-Player
--   xxx.dll
--   res


```bash

git clone https://github.com/qazwsxwtc/Fralix‑Player.git
cd Fralix‑Player

make build
cd build
cmake .. -G "Visual Studio 17 2022"


# VS2022 x64 编译