cmake_minimum_required(VERSION 3.15)
project(Fralix-Player LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# 1. 查找依赖库
# ------------------------------------------------------------------

# --- 查找 FFmpeg ---
# 如果 FFmpeg 安装在非标准路径，请取消注释并设置以下变量：
# set(FFMPEG_ROOT "D:/ffmpeg/build") 
# set(ENV{PKG_CONFIG_PATH} "${FFMPEG_ROOT}/lib/pkgconfig")

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FFMPEG_LIBS 
        libavformat 
        libavcodec 
        libswscale 
        libswresample 
        libavutil
    )
endif()

# 如果 pkg-config 没找到，尝试使用 find_path/find_library (Windows 常用方式)
if(NOT FFMPEG_LIBS_FOUND)
    # 【修改点】将头文件路径设置为当前目录下的 include，库路径设置为当前目录下的 lib
    set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE PATH "FFmpeg include directory")
    set(FFMPEG_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib" CACHE PATH "FFmpeg lib directory")

    find_path(FFMPEG_AVFORMAT_INCLUDE_DIR NAMES libavformat/avformat.h PATHS ${FFMPEG_INCLUDE_DIR})
    find_library(FFMPEG_AVFORMAT_LIB NAMES avformat PATHS ${FFMPEG_LIB_DIR})
    find_library(FFMPEG_AVCODEC_LIB NAMES avcodec PATHS ${FFMPEG_LIB_DIR})
    find_library(FFMPEG_SWSCALE_LIB NAMES swscale PATHS ${FFMPEG_LIB_DIR})
    find_library(FFMPEG_SWRESAMPLE_LIB NAMES swresample PATHS ${FFMPEG_LIB_DIR})
    find_library(FFMPEG_AVUTIL_LIB NAMES avutil PATHS ${FFMPEG_LIB_DIR})

    if(FFMPEG_AVFORMAT_INCLUDE_DIR AND FFMPEG_AVFORMAT_LIB)
        set(FFMPEG_INCLUDE_DIRS ${FFMPEG_AVFORMAT_INCLUDE_DIR})
        set(FFMPEG_LIBRARIES 
            ${FFMPEG_AVFORMAT_LIB}
            ${FFMPEG_AVCODEC_LIB}
            ${FFMPEG_SWSCALE_LIB}
            ${FFMPEG_SWRESAMPLE_LIB}
            ${FFMPEG_AVUTIL_LIB}
        )
        message(STATUS "Found FFmpeg manually: ${FFMPEG_INCLUDE_DIRS}")
    else()
        message(FATAL_ERROR "Could not find FFmpeg. Please check ${FFMPEG_INCLUDE_DIR} and ${FFMPEG_LIB_DIR}.")
    endif()
else()
    # 使用 pkg-config 的结果
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBS_INCLUDE_DIRS})
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBS_LIBRARIES})
    message(STATUS "Found FFmpeg via pkg-config")
endif()

# --- 查找 SDL3 ---
# SDL3 通常提供 CMake Config 文件
find_package(SDL3 CONFIG QUIET)
if(SDL3_FOUND)
    message(STATUS "Found SDL3 via Config: ${SDL3_DIR}")
else()
    # 如果找不到 Config，尝试查找头文件和库
    # 【修改点】将头文件路径设置为当前目录下的 include，库路径设置为当前目录下的 lib
    set(SDL3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE PATH "SDL3 include directory")
    set(SDL3_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib" CACHE PATH "SDL3 lib directory")
    
    # 注意：SDL3 的头文件通常在 include/SDL3/SDL.h，所以 find_path 需要正确配置
    find_path(SDL3_INCLUDE_DIR NAMES SDL3/SDL.h PATHS ${SDL3_INCLUDE_DIR})
    find_library(SDL3_LIBRARY NAMES SDL3 PATHS ${SDL3_LIB_DIR})

    if(SDL3_INCLUDE_DIR AND SDL3_LIBRARY)
        add_library(SDL3::SDL3 UNKNOWN IMPORTED)
        set_target_properties(SDL3::SDL3 PROPERTIES
            IMPORTED_LOCATION "${SDL3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
        )
        message(STATUS "Found SDL3 manually: ${SDL3_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "Could not find SDL3. Please check ${SDL3_INCLUDE_DIR} and ${SDL3_LIB_DIR}.")
    endif()
endif()

# --- 查找 Duilib (假设 Duilib 是一个静态库或动态库) ---
# 这里假设你已经编译好了 Duilib 库，名为 DuiLib
# 你需要根据实际编译出的 Duilib 库路径进行修改
# 如果 Duilib 也遵循同样的 include/lib 结构，可以取消注释下面的行并注释掉原有的 set
# set(DUILIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Root directory of Duilib")
set(DUILIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include/DuiLib" CACHE PATH "Root directory of Duilib")
set(DUILIB_LIB_NAME "DuiLib" CACHE STRING "Name of the Duilib library file")

# 尝试查找 Duilib 头文件
find_path(DUILIB_INCLUDE_DIR NAMES Core/UIBase.h PATHS ${DUILIB_ROOT})
# 尝试查找 Duilib 库文件 (假设在 build 目录下或者当前的 lib 目录)
# 【可选修改】如果 Duilib 库也在当前目录的 lib 下，可以添加 ${CMAKE_CURRENT_SOURCE_DIR}/lib 到 PATHS
find_library(DUILIB_LIBRARY NAMES ${DUILIB_LIB_NAME} PATHS ${DUILIB_ROOT}/bin ${DUILIB_ROOT}/lib ${DUILIB_ROOT}/build/bin ${DUILIB_ROOT}/build/lib ${CMAKE_CURRENT_SOURCE_DIR}/lib NO_DEFAULT_PATH)

if(NOT DUILIB_INCLUDE_DIR OR NOT DUILIB_LIBRARY)
    message(WARNING "Duilib not found automatically. Please check DUILIB_ROOT and ensure Duilib is compiled.")
    # 为了演示，我们暂时添加一个假的目标，实际使用时请修正路径
    # set(DUILIB_INCLUDE_DIR "${DUILIB_ROOT}")
    # set(DUILIB_LIBRARY "${DUILIB_ROOT}/bin/DuiLib.lib") 
endif()

# ------------------------------------------------------------------
# 2. 定义可执行文件
# ------------------------------------------------------------------

# 收集源文件
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# 排除不需要的文件（如果有）
# list(REMOVE_ITEM SOURCES ...)

#add_executable(${PROJECT_NAME} ${SOURCES})
add_executable(${PROJECT_NAME} WIN32 avrunmain.cpp ${SOURCES})
# 设置字符集为 Unicode
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)
# 在 add_executable 之后，target_link_libraries 之前或之后
target_compile_definitions(${PROJECT_NAME} PRIVATE UILIB_STATIC)

# ------------------------------------------------------------------
# 3. 包含目录与链接库
# ------------------------------------------------------------------

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${DUILIB_INCLUDE_DIR}
    # SDL3::SDL3 已经通过 target_link_libraries 自动包含其 include dir
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FFMPEG_LIBRARIES}
    SDL3::SDL3
    ${DUILIB_LIBRARY}
)

# Windows 特定设置
if(WIN32)
    # 如果是 MSVC，可能需要链接一些系统库
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ws2_32 
        strmiids 
        mfplat 
        mfuuid 
        ole32 
        uuid
        # 【新增】以下是 Duilib 和 Windows GUI 应用必需的库
        Comctl32      # 解决 InitCommonControls
        User32        # 解决 TrackMouseEvent 等
        Gdi32         # Duilib 绘图需要
        Shell32       # Duilib 常用
        OleAut32      # Duilib 常用
        Imm32
    )
    
endif()

# Linux/macOS 特定设置
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
endif()

message(STATUS "Project configured successfully.")