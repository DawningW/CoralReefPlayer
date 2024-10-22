![logo](doc/logo.png)

# CoralReefPlayer

![License: MIT](https://img.shields.io/badge/license-MIT-blue) ![Branch: v1](https://img.shields.io/badge/branch-v1-red)

CoralReefPlayer 即珊瑚礁播放器，是一款使用 C++20 开发的跨平台流媒体播放器库，目前支持播放 RTSP 和 MJPEG over HTTP 流，可为基于网络进行视频流传输的机器人上位机提供可定制、高性能、低延迟的推拉流、编解码及录像能力。

CoralReefPlayer 支持 Windows、Linux、MacOS、Android、iOS 和纯血鸿蒙等主流操作系统，并且提供 C#、Java 和 Python 等语言的 binding，方便使用各种语言和框架开发上位机。

与现存的各种播放器库相比，CoralReefPlayer 具有以下特点：
- 使用 C++20 标准开发，代码简洁、高效
- 接口简单易用
- 所有主流操作系统和编程语言支持
- **遵循单缓冲区设计，专为低延迟场景优化**

CoralReefCam，中文名珊瑚礁™嘻屁屁高性能版，是 CoralReefPlayer 的示例项目，集成了 SDL、imgui、OpenCV 等库，可基于此项目开发高性能机器人上位机。目前作为监控软件使用，其最初的开发目的是用于解决拉流延迟问题。

![v0.2](doc/snapshot-0.2.png)

该库由大连理工大学 OurEDA 实验室开发，其 v1 版本较为稳定，已广泛应用在为某机构开发的水下机器人和各款用于比赛的机器人上位机中，可用于生产环境，配合实验室自研的 Rouring 框架（暂无开源计划）更可实现最低**66ms**的图传延迟。

如果您在项目或科研工作中用到了 CoralReefPlayer，欢迎在 Issues 中告诉我们。如果遇到了 bug 或有功能需求，也欢迎提交 Issue 或直接 PR，我们会有专人回答并解决您的问题。

## 依赖

随源码自带或推荐使用的第三方库版本如下所示，括号内为最低版本要求：

- CoralReefCam
    - CoralReefPlayer
        - [live555](http://www.live555.com/liveMedia/) 2023.11.30
        - [cpp-httplib](https://github.com/yhirose/cpp-httplib) 0.15.3
        - [FFmpeg](https://ffmpeg.org/) 4.4 (4.4+)
    - [SDL2](https://libsdl.org/) 2.30.2 (2.26.0+)
    - [imgui](https://github.com/ocornut/imgui) 1.90.5
    - [implot](https://github.com/epezent/implot) 0.16
    - （可选）[OpenCV](https://opencv.org/) 4.7.0 (4.4.0+)

## 编译指南

### Windows

前置条件：

- Visual Studio 2019 或以上版本，需要启用 `使用 C++ 的桌面开发`
- （可选）下载预编译的 OpenCV 库至 opencv 或任意目录中

编译步骤：

1. 直接使用 VS 打开本仓库
2. 右键根目录下的 CMakeLists.txt，选择 `CMake 设置` 打开设置界面
3. 填写必要的配置项，然后进行生成
4. 享受 CoralReefCamCpp 吧

如果不想使用 ninja 构建，也可使用 cmake-gui 生成 VS 工程，然后用 VS 打开工程进行构建

> [!TIP]
>
> 为降低开发和使用门槛，在 Windows 平台上所使用的第三方库均为预编译好的库，因此无需安装任何依赖即可一键编译。

### Linux

前置条件：

- CMake 3.21.0 或更高版本
- gcc 11 或更高版本
- 使用包管理器或源码编译安装下列库或它们的更高版本：
    - FFmpeg 4.4+
    - SDL2 2.26.0+
    - （可选）OpenCV 4.4.0+

编译步骤：

1. 使用终端进入本仓库根目录
2. 使用 `cmake-gui` 或 `ccmake` 配置项目，然后进行生成
3. 进入构建目录，执行 `make -j8` 进行构建
4. 执行 `make install` 安装至指定目录
5. 享受 CoralReefCamCpp 吧

### MacOS

前置条件：

- apple-clang 13.1.6 或更高版本
- 其余依赖同 Linux

编译步骤：同 Linux

### Android

前置条件：

- Android Studio
- SDK 33（最低要求 24）
- NDK 25.1.8937393
- CMake 3.22.1 或更高版本

编译步骤：

1. 使用 Android Studio 打开 Android 目录
2. 编译并运行 `app` 项目

### iOS

前置条件：

- Xcode 14.0 或更高版本
- iOS SDK 17.0（最低要求 14.0）
- CMake 3.21.0 或更高版本

编译步骤：

1. 进入 iOS 目录，执行 `prebuild.sh` 脚本
2. 使用 Xcode 打开 `CoralReefPlayer.xcodeproj`
3. 编译并运行 `Example` 目标

### HarmonyOS NEXT

前置条件：

- DevEco Studio 5.0.3.900 或更高版本
- HarmonyOS NEXT SDK 5.0.0.71（API 12）或更高版本

编译步骤：

1. 使用 DevEco Studio 打开 Harmony 目录
2. 编译并运行 `entry` 项目

### 交叉编译

CoralReefPlayer 支持交叉编译，可使用 CMake 的工具链文件进行交叉编译，工具链文件的编写请参考 `cmake/toolchains/aarch64-linux-gnu.toolchain.cmake`。

可使用 `-DCMAKE_TOOLCHAIN_FILE=<工具链文件路径>` 参数或 `cmake-gui` 工具指定工具链文件，其余编译步骤同 Linux。

### Other

目前 CoralReefPlayer 已适配全部主流操作系统，若您想添加对其他系统的支持，欢迎提交 PR

## bindings

> [!IMPORTANT]
>
> 目前各语言的 binding 只能在本地生成依赖库包，没有上传到包管理器仓库中，因此建议自行编译生成。

### Java

启用 `BUILD_JAVA_BINDING` 选项，然后运行 `java-package` 目标即可在 install 目录中生成 jar 包。

### C\#

启用 `BUILD_CSHARP_BINDING` 选项，并配置 dotnet 目标版本，然后运行 `csharp-package` 目标即可在 install 目录中生成 nuget 包。

### Python

启用 `BUILD_PYTHON_BINDING` 选项，然后运行 `python-package` 目标即可在 install 目录中生成 whl 包。

### Node.js

启用 `BUILD_JS_BINDING` 选项，然后运行 `node-package` 目标即可在 install 目录中生成 npm 包。

### Go

启用 `BUILD_GO_BINDING` 选项，然后运行 `go-package` 目标即可在 install 目录中生成包含 Go 代码的 tar.gz 压缩文件。

### Rust

启用 `BUILD_RUST_BINDING` 选项，然后运行 `rust-package` 目标即可在 install 目录中生成包含 Rust 代码的 zip 压缩文件。

## 文档

CoralReefPlayer 总体架构图：

![架构图](doc/architecture.png)

CoralReefPlayer API 文档：

```c
// coralreefplayer.h

struct Option
{
    int transport;          // 传输协议（仅 RTSP 有效），见 Transport 枚举定义
    struct
    {
        int width;          // 解码图像宽度，CRP_WIDTH_AUTO 为从码流自动获取
        int height;         // 解码图像高度，CRP_HEIGHT_AUTO 为从码流自动获取
        int format;         // 解码图像格式，见 Format 枚举定义
        char hw_device[32]; // 硬解码器名称，可取值请参考 ffmpeg 文档，为空则使用软解码器
    } video;
    bool enable_audio;      // 是否启用音频
    struct
    {
        int sample_rate;    // 解码音频采样率
        int channels;       // 解码音频声道数
        int format;         // 解码音频格式，见 Format 枚举定义
    } audio;
    int64_t timeout;        // 超时时间，单位为毫秒
};

struct Frame
{
    union
    {
        struct
        {
            int width;       // 图像宽度
            int height;      // 图像高度
        };
        struct
        {
            int sample_rate; // 采样率
            int channels;    // 声道数
        };
    };
    int format;              // 数据格式，见 Format 枚举定义
    uint8_t* data[4];        // 数据指针
    int stride[4];           // 数据跨度，即一行数据的字节数（由于内存对齐，可能大于图像宽度）
    uint64_t pts;            // 展示时间戳
};

/**
 * @brief 事件回调函数类型
 * @param event 事件类型，见 Event 枚举定义
 * @param data 事件数据，收到视频和音频数据时为 Frame 结构体指针，其他事件为 NULL
 * @param user_data crp_play 函数中传入的用户自定义数据
 */
typedef void (*crp_callback)(int event, void* data, void* user_data);

/**
 * @brief 创建播放器
 * @return 播放器句柄
 */
crp_handle crp_create();

/**
 * @brief 销毁播放器，会自动停止播放
 * @param handle 播放器句柄
 */
void crp_destroy(crp_handle handle);

/**
 * @brief 设置 RTSP/HTTP 认证
 * @param handle 播放器句柄
 * @param username 用户名
 * @param password 密码
 * @param is_md5 密码是否为 MD5，HTTP 认证时此参数无效
 */
void crp_auth(crp_handle handle, const char* username, const char* password, bool is_md5);

/**
 * @brief 开始播放指定 RTSP/HTTP 流，支持 H264/H265/MJPEG 和 AAC/OPUS/PCM 码流
 * @param handle 播放器句柄
 * @param url RTSP/HTTP 流地址
 * @param option 播放选项结构体指针，见 Option 结构体定义
 * @param callback 事件回调函数，函数签名见 crp_callback
 * @param user_data 用户自定义数据，会在调用回调时传入
 * @return 是否成功，若参数不正确则返回 false，其他错误会调用回调
 */
void crp_play(crp_handle handle, const char* url, const Option* option, crp_callback callback, void* user_data);

/**
 * @brief 重新播放之前播放的 RTSP/HTTP 流
 * @param handle 播放器句柄
 * @return 是否成功，若之前未播放过则返回 false，其他错误会调用回调
 */
bool crp_replay(crp_handle handle);

/**
 * @brief 停止播放指定 RTSP/HTTP 流
 * @param handle 播放器句柄
 */
void crp_stop(crp_handle handle);

/**
 * @brief 获取版本号
 */
int crp_version_code();

/**
 * @brief 获取版本号字符串
 */
const char* crp_version_str();
```

## 贡献

如果您想要为 CoralReefPlayer 添加更多功能，或者发现了 bug，欢迎向我们提交 PR。我们的社区准则和代码规范正在编写中，目前请尽量保证代码风格与现存代码一致。

## 更新日志

详细更新日志请查看本项目的 Release 页面。

## 版权声明

本项目由大连理工大学 OurEDA 实验室开发，其 v1 版本以 MIT 协议开源。根据 MIT 协议，作者不对此软件作任何形式的担保，您可以以任何形式自由使用本软件，但**需要**将该版权声明包含在使用此软件的软件中。

## 相关仓库

- [bubbler](https://github.com/xaxys/bubbler) - Bubbler，专为嵌入式场景优化的协议序列化/反序列化代码生成器。
- [OpenFinNAV](https://github.com/redlightASl/OpenFinNAV) - 鳍航，针对水下机器人（ROV/AUV）设计的飞控固件库。
