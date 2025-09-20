package coralreefplayer_cj

import std.unittest.*
import std.unittest.testmacro.*

@Test
func pullTest() {
    let url = "rtsp://127.0.0.1:8554/test"
    println("CoralReefPlayer version: ${versionStr()} (${versionCode()})")
    let hasFrame = Box(false)
    let player = CoralReefPlayer()
    let option = Options(
        transport: TRANSPORT_UDP,
        videoFormat: VIDEO_FORMAT_YUV420P
    )
    player.play(url, option) { event, data =>
        match (data) {
            case Data.FrameData(frame) =>
                if (event == EVENT_NEW_FRAME) {
                    println("frame: ${frame.width}x${frame.height}, format: ${frame.format}, pts: ${frame.pts}")
                } else if (event == EVENT_NEW_AUDIO) {
                    println("audio: ${frame.sampleRate}x${frame.channels}, format: ${frame.format}, pts: ${frame.pts}")
                }
                hasFrame.value = true
            case Data.ExtraData(data) =>
                let `type` = if (event == EVENT_VIDEO_EXTRADATA) { "video" } else { "audio" }
                println("received ${`type`} extra data, size: ${data.size}")
            case Data.EventCode(code) =>
                println("event: ${event}, data: ${code}")
        }
    }
    for (_ in 0..10) {
        sleep(1 * Duration.second)
        if (hasFrame.value) {
            break
        }
    }
    player.release()
    @Expect(hasFrame.value)
}
