import XCTest
@testable import CoralReefPlayer

final class CoralReefPlayerTests: XCTestCase {
    
    let url = "rtsp://127.0.0.1:8554/"

    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        NSLog("CoralReefPlayer version: \(versionStr()) (\(versionCode()))")
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testPull() throws {
        var hasFrame = false
        let player = Player()
        let option = Option(
            transport: Transport.UDP,
            videoFormat: VideoFormat.YUV420P
        )
        player.play(url: url, option: option, callback: { (event, data) in
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

