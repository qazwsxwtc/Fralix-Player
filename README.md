# Fralix‑Player
Self‑developed Windows media player, based on Duilib + FFmpeg + SDL3

**English** | [简体中文](README.zh.md)

## Features
- FFmpeg media decode V8.1.2
- SDL3 audio output  V3.4.14
- Duilib UI

## Quick Start
- please set FFMPEG_INCLUDE_DIR / FFMPEG_LIB_DIR / SDL3_INCLUDE_DIR / 
	SDL3_LIB_DIR/DUILIB_INCLUDE_DIR/DUILIB_LIBRARY path first.

download ffmpeg https://github.com/FFmpeg/FFmpeg/releases/tag/n8.1.2
	https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-8.1.2-full_build.7z

download SDL3 https://github.com/libsdl-org/SDL/releases   
	SDL3-devel-3.4.14-VC.zip


```bash
git https://github.com/duilib/duilib.git
 set duilib path

git clone https://github.com/qazwsxwtc/Fralix‑Player.git
cd Fralix‑Player

make build
cd build
cmake .. -G "Visual Studio 17 2022"


# VS2022 x64 compile