import Foundation
import CoralReefPlayerIOS

public enum Transport : Int32 {
    case UDP = 0
    case TCP = 1
}

public enum VideoFormat : Int32 {
    case YUV420P = 0
    case NV12 = 1
    case NV21 = 2
    case RGB24 = 3
    case BGR24 = 4
    case ARGB32 = 5
    case RGBA32 = 6
    case ABGR32 = 7
    case BGRA32 = 8
}

public enum AudioFormat : Int32 {
    case U8 = 0
    case S16 = 1
    case S32 = 2
    case F32 = 3
}

public enum Event : Int32 {
    case NEW_FRAME = 0
    case ERROR = 1
    case START = 2
    case PLAYING = 3
    case END = 4
    case STOP = 5
    case NEW_AUDIO = 6
}

public struct Option {
    public var transport: Transport
    public var width: Int
    public var height: Int
    public var videoFormat: VideoFormat
    public var hwDevice: String
    public var enableAudio: Bool
    public var sampleRate: Int
    public var channels: Int
    public var audioFormat: AudioFormat
    public var timeout: Int64
    
    public init(transport: Transport = .UDP, width: Int = 0, height: Int = 0, videoFormat: VideoFormat = .YUV420P, hwDevice: String = "",
        enableAudio: Bool = false, sampleRate: Int = 0, channels: Int = 0, audioFormat: AudioFormat = .U8, timeout: Int64 = 0) {
        self.transport = transport
        self.width = width
        self.height = height
        self.videoFormat = videoFormat
        self.hwDevice = hwDevice
        self.enableAudio = enableAudio
        self.sampleRate = sampleRate
        self.channels = channels
        self.audioFormat = audioFormat
        self.timeout = timeout
    }
}

public struct Frame {
    public var width: Int = 0
    public var height: Int = 0
    public var sampleRate: Int = 0
    public var channels: Int = 0
    public var format: Int32
    public var data: [UnsafeBufferPointer<UInt8>]
    public var linesize: [Int]
    public var pts: UInt64
}

private func swift_callback(event: Int32, data: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
    let player = Unmanaged<Player>.fromOpaque(userData!).takeUnretainedValue()
    let ee = Event(rawValue: event)!
    if ee == Event.NEW_FRAME {
        let cframe = data!.bindMemory(to: CoralReefPlayerIOS.Frame.self, capacity: 1).pointee
        let frame = Frame(
            width: Int(cframe.width),
            height: Int(cframe.height),
            format: cframe.format,
            data: [
                UnsafeBufferPointer(start: cframe.data.0, count: Int(cframe.linesize.0 * cframe.height)),
                UnsafeBufferPointer(start: cframe.data.1, count: Int(cframe.linesize.1 * cframe.height / 2)),
                UnsafeBufferPointer(start: cframe.data.2, count: Int(cframe.linesize.2 * cframe.height / 2)),
                UnsafeBufferPointer(start: cframe.data.3, count: Int(cframe.linesize.3 * cframe.height / 2))
            ],
            linesize: [Int(cframe.linesize.0), Int(cframe.linesize.1), Int(cframe.linesize.2), Int(cframe.linesize.3)],
            pts: cframe.pts
        )
        player.callback(ee, frame)
    } else if ee == Event.NEW_AUDIO {
        let cframe = data!.bindMemory(to: CoralReefPlayerIOS.Frame.self, capacity: 1).pointee
        let frame = Frame(
            width: Int(cframe.width),
            height: Int(cframe.height),
            format: cframe.format,
            data: [
                UnsafeBufferPointer(start: cframe.data.0, count: Int(cframe.linesize.0)),
            ],
            linesize: [Int(cframe.linesize.0)],
            pts: cframe.pts
        )
        player.callback(ee, frame)
    } else {
        player.callback(ee, data == nil ? 0 : Int(bitPattern: data!))
    }
}

public class Player {

    var handle: crp_handle?
    var callback: (Event, Any) -> Void = { _, _ in }


    public init() {
        handle = crp_create()
    }

    deinit {
        if let handle = handle {
            crp_destroy(handle)
        }
    }

    public func auth(username: String, password: String, isMd5: Bool) {
        crp_auth(handle, username, password, isMd5)
    }

    public func play(url: String, option: Option, callback: @escaping (Event, Any) -> Void) {
        var opt = CoralReefPlayerIOS.Option(
            transport: option.transport.rawValue,
            video: CoralReefPlayerIOS.Option.__Unnamed_struct_video(
                width: Int32(option.width),
                height: Int32(option.height),
                format: option.videoFormat.rawValue,
                hw_device: (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            ),
            enable_audio: option.enableAudio,
            audio: CoralReefPlayerIOS.Option.__Unnamed_struct_audio(
                sample_rate: Int32(option.sampleRate),
                channels: Int32(option.channels),
                format: option.audioFormat.rawValue
            ),
            timeout: option.timeout
        )
        option.hwDevice.withCString { hw_device in
            withUnsafeMutablePointer(to: &opt.video.hw_device) { ptr in
                strncpy(ptr, hw_device, 32)
            }
        }
        self.callback = callback
        withUnsafeMutablePointer(to: &opt) { ptr in
            crp_play(handle, url, ptr, swift_callback, Unmanaged.passUnretained(self).toOpaque())
        }
    }

    public func replay() {
        crp_replay(handle)
    }

    public func stop() {
        crp_stop(handle)
    }
}

public func versionCode() -> Int {
    return Int(crp_version_code())
}

public func versionStr() -> String {
    return String(cString: crp_version_str())
}

