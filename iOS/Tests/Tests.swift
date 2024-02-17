import XCTest
@testable import CoralReefPlayer

final class CoralReefPlayerTests: XCTestCase {
    
    let url = "rtsp://127.0.0.1/main.sdp"

    override public init() {
        super.init()
        NSLog("CoralReefPlayer version: \(versionStr()) (\(versionCode()))")
    }

    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testPull() throws {
        var hasFrame = false
        let player = Player()
        player.play(url: url, transport: Transport.UDP, width: 0, height: 0, format: Format.RGB24, callback: { (event, data) in
            print("event: \(event)")
            if event == Event.NEW_FRAME {
                hasFrame = true
            }
        })
        for _ in 0...10 {
            sleep(1)
            if hasFrame {
                break
            }
        }
        XCTAssert(hasFrame)
    }


}

