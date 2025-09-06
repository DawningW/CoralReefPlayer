import time
import coralreefplayer as crp

print(f"CoralReefPlayer version: {crp.version_str()} ({crp.version_code()})")

url = "rtsp://127.0.0.1:8554/test"

def on_event(event, data):
    global hasFrame
    if event == crp.EVENT_NEW_FRAME or event == crp.EVENT_NEW_AUDIO:
        if event == crp.EVENT_NEW_FRAME:
            print(f"frame: {data['width']}x{data['height']}, format: {data['format']}, pts: {data['pts']}")
        else:
            print(f"audio: {data['sample_rate']}x{data['channels']}, format: {data['format']}, pts: {data['pts']}")
        hasFrame = True
    elif event == crp.EVENT_VIDEO_EXTRADATA or event == crp.EVENT_AUDIO_EXTRADATA:
        print(f"received {'video' if event == crp.EVENT_VIDEO_EXTRADATA else 'audio'} extra data, size: {data.nbytes}")
    else:
        print(f"event: {event}, data: {data}")

if __name__ == "__main__":
    hasFrame = False
    player = crp.Player()
    option = {
        "transport": crp.TRANS_UDP,
        "video_format": crp.FORMAT_YUV420P,
    }
    player.play(url, option, on_event)
    for i in range(10):
        time.sleep(1)
        if hasFrame:
            break
    player.release()
    assert hasFrame
