package coralreefplayer_cj

import std.collection.HashMap

@C
struct COption {
    public var transport: Int32 = 0
    public var video_width: Int32 = 0
    public var video_height: Int32 = 0
    public var video_format: Int32 = 0
    public var hw_device = VArray<UInt8, $32>(repeat: 0)
    public var enable_audio: Bool = false
    public var audio_sample_rate: Int32 = 0
    public var audio_channels: Int32 = 0
    public var audio_format: Int32 = 0
    public var timeout: Int64 = 0
}

@C
struct CFrame {
    public var width: Int32 = 0
    public var height: Int32 = 0
    public var format: Int32 = 0
    public var data = VArray<CPointer<UInt8>, $4>(repeat: CPointer())
    public var stride = VArray<Int32, $4>(repeat: 0)
    public var pts: UInt64 = 0
}

@C
struct CExtraData {
    public var data = CPointer<UInt8>()
    public var size: Int32 = 0
}

type crp_handle = CPointer<Unit>
type crp_callback = CFunc<(event: Int32, data: CPointer<Unit>, user_data: CPointer<Unit>) -> Unit>

foreign func crp_create(): crp_handle
foreign func crp_destroy(handle: crp_handle): Unit
foreign func crp_auth(handle: crp_handle, username: CString, password: CString, is_md5: Bool): Unit
foreign func crp_play(handle: crp_handle, url: CString, option: CPointer<COption>, callback: crp_callback, user_data: CPointer<Unit>): Unit
foreign func crp_replay(handle: crp_handle): Unit
foreign func crp_stop(handle: crp_handle): Unit
foreign func crp_version_code(): Int32
foreign func crp_version_str(): CString

public const TRANSPORT_UDP: Int32 = 0
public const TRANSPORT_TCP: Int32 = 1

public const VIDEO_FORMAT_YUV420P: Int32 = 0
public const VIDEO_FORMAT_NV12: Int32 = 1
public const VIDEO_FORMAT_NV21: Int32 = 2
public const VIDEO_FORMAT_RGB24: Int32 = 3
public const VIDEO_FORMAT_BGR24: Int32 = 4
public const VIDEO_FORMAT_ARGB32: Int32 = 5
public const VIDEO_FORMAT_RGBA32: Int32 = 6
public const VIDEO_FORMAT_ABGR32: Int32 = 7
public const VIDEO_FORMAT_BGRA32: Int32 = 8

public const AUDIO_FORMAT_U8: Int32 = 0
public const AUDIO_FORMAT_S16: Int32 = 1
public const AUDIO_FORMAT_S32: Int32 = 2
public const AUDIO_FORMAT_F32: Int32 = 3

public const EVENT_NEW_FRAME: Int32 = 0
public const EVENT_ERROR: Int32 = 1
public const EVENT_START: Int32 = 2
public const EVENT_PLAYING: Int32 = 3
public const EVENT_END: Int32 = 4
public const EVENT_STOP: Int32 = 5
public const EVENT_NEW_AUDIO: Int32 = 6
public const EVENT_VIDEO_EXTRADATA: Int32 = 7
public const EVENT_AUDIO_EXTRADATA: Int32 = 8

public struct Options {
    public Options(
        var transport!: Int32 = TRANSPORT_UDP,
        var width!: Int32 = 0,
        var height!: Int32 = 0,
        var videoFormat!: Int32 = VIDEO_FORMAT_YUV420P,
        var hwDevice!: String = "",
        var enableAudio!: Bool = false,
        var sampleRate!: Int32 = 0,
        var channels!: Int32 = 0,
        var audioFormat!: Int32 = AUDIO_FORMAT_U8,
        var timeout!: Int64 = 0
    ) {}
}

public struct Frame {
    public Frame(
        var width!: Int32 = 0,
        var height!: Int32 = 0,
        var sampleRate!: Int32 = 0,
        var channels!: Int32 = 0,
        var format!: Int32 = 0,
        var data!: Array<Array<Byte>> = Array<Array<Byte>>(),
        var stride!: Array<Int32> = Array<Int32>(),
        var pts!: UInt64 = 0
    ) {}
}

public enum Data {
    | EventCode(Int64)
    | FrameData(Frame)
    | ExtraData(Array<UInt8>)
}

private let callbacks = HashMap<UIntNative, (Int32, Data) -> Unit>()

foreign func memcpy(dest: CPointer<Byte>, src: CPointer<Byte>, n: Int64): CPointer<Byte>
private func c_arr_copy(ptr: CPointer<UInt8>, size: Int64): Array<Byte> {
    let arr = Array<Byte>(size, repeat: 0)
    unsafe {
        let ptrHandle: CPointerHandle<Byte> = acquireArrayRawData(arr)
        memcpy(ptrHandle.pointer, ptr, size)
        releaseArrayRawData(ptrHandle)
    }
    return arr
}

@C
private func cj_callback(event: Int32, data: CPointer<Unit>, user_data: CPointer<Unit>) {
    let callback = callbacks.get(user_data.toUIntNative()).getOrThrow()
    if (event == EVENT_NEW_FRAME) {
        let cFrame = unsafe { CPointer<CFrame>(data).read() }
        let frame = Frame(
            width: cFrame.width,
            height: cFrame.height,
            format: cFrame.format,
            data: [
                c_arr_copy(cFrame.data[0], Int64(cFrame.stride[0] * cFrame.height)),
                c_arr_copy(cFrame.data[1], Int64(cFrame.stride[1] * (cFrame.height / 2))),
                c_arr_copy(cFrame.data[2], Int64(cFrame.stride[2] * (cFrame.height / 2))),
                c_arr_copy(cFrame.data[3], Int64(cFrame.stride[3] * (cFrame.height / 2)))
            ],
            stride: [cFrame.stride[0], cFrame.stride[1], cFrame.stride[2], cFrame.stride[3]],
            pts: cFrame.pts
        )
        callback(event, Data.FrameData(frame))
    } else if (event == EVENT_NEW_AUDIO) {
        let cFrame = unsafe { CPointer<CFrame>(data).read() }
        let frame = Frame(
            sampleRate: cFrame.width,
            channels: cFrame.height,
            format: cFrame.format,
            data: [
                c_arr_copy(cFrame.data[0], Int64(cFrame.stride[0] * cFrame.height)),
                Array(),
                Array(),
                Array()
            ],
            stride: [cFrame.stride[0], cFrame.stride[1], cFrame.stride[2], cFrame.stride[3]],
            pts: cFrame.pts
        )
        callback(event, Data.FrameData(frame))
    } else if (event == EVENT_VIDEO_EXTRADATA || event == EVENT_AUDIO_EXTRADATA) {
        let cExtraData = unsafe { CPointer<CExtraData>(data).read() }
        let extraData = c_arr_copy(cExtraData.data, Int64(cExtraData.size))
        callback(event, Data.ExtraData(extraData))
    } else {
        callback(event, Data.EventCode(unsafe { Int64(data.toUIntNative()) }))
    }
}

public class CoralReefPlayer {
    private var handle: crp_handle

    public init() {
        this.handle = unsafe { crp_create() }
    }

    public func release(): Unit {
        unsafe { crp_destroy(this.handle) }
        callbacks.remove(this.handle.toUIntNative())
        this.handle = CPointer()
    }

    public func auth(username: String, password: String, isMD5!: Bool = false): Unit {
        unsafe {
            let cUsername = LibC.mallocCString(username)
            let cPassword = LibC.mallocCString(password)
            crp_auth(this.handle, cUsername, cPassword, isMD5)
            LibC.free(cUsername)
            LibC.free(cPassword)
        }
    }

    public func play(url: String, option: Options, callback: (Int32, Data) -> Unit): Unit {
        unsafe {
            let cUrl = LibC.mallocCString(url)
            var cOption = COption()
            cOption.transport = option.transport
            cOption.video_width = option.width
            cOption.video_height = option.height
            cOption.video_format = option.videoFormat
            for (i in 0..min(option.hwDevice.size, cOption.hw_device.size - 1)) {
                cOption.hw_device[i] = option.hwDevice[i]
            }
            cOption.enable_audio = option.enableAudio
            cOption.audio_sample_rate = option.sampleRate
            cOption.audio_channels = option.channels
            cOption.audio_format = option.audioFormat
            cOption.timeout = option.timeout
            callbacks.add(this.handle.toUIntNative(), callback)
            crp_play(this.handle, cUrl, inout cOption, cj_callback, this.handle)
            LibC.free(cUrl)
        }
    }

    public func replay(): Unit {
        unsafe { crp_replay(this.handle) }
    }

    public func stop(): Unit {
        unsafe { crp_stop(this.handle) }
    }
}

public func versionCode(): Int32 {
    return unsafe { crp_version_code() }
}

public func versionStr(): String {
    return unsafe { crp_version_str().toString() }
}
