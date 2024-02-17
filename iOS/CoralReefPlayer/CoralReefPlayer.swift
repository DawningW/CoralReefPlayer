import Foundation
import CoralReefPlayerIOS

public enum Transport : Int32 {
    case UDP = 0
    case TCP = 1
}

public enum Format : Int32 {
    case YUV420P = 0
    case RGB24 = 1
    case BGR24 = 2
    case ARGB32 = 3
    case RGBA32 = 4
    case ABGR32 = 5
    case BGRA32 = 6
}

public enum Event : Int32 {
    case NEW_FRAME = 0
    case ERROR = 1
    case START = 2
    case PLAYING = 3
    case END = 4
    case STOP = 5
}

public struct Frame {
    public var width: Int
    public var height: Int
    public var format: Format
    public var data: UnsafeBufferPointer<UInt8>
    public var linesize: Int
    public var pts: UInt64
}

private func swift_callback(event: Int32, data: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
    let player = Unmanaged<Player>.fromOpaque(userData!).takeUnretainedValue()
    let ee = Event(rawValue: event)!
    if ee == Event.NEW_FRAME {
        let cframe = data!.bindMemory(to: CoralReefPlayerIOS.Frame.self, capacity: 1).pointee
        let buffer = UnsafeBufferPointer(start: cframe.data.0, count: Int(cframe.linesize.0 * cframe.height))
        let frame = Frame(width: Int(cframe.width), height: Int(cframe.height), format: Format(rawValue: cframe.format)!, data: buffer, linesize: Int(cframe.linesize.0), pts: cframe.pts)
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

    public func play(url: String, transport: Transport, width: Int, height: Int, format: Format, callback: @escaping (Event, Any) -> Void) {
        self.callback = callback
        crp_play(handle, url, transport.rawValue, Int32(width), Int32(height), format.rawValue, swift_callback, Unmanaged.passUnretained(self).toOpaque())
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

