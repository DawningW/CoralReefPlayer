import time
import coralreefplayer as crp

print(f"CoralReefPlayer version: {crp.version_str()} ({crp.version_code()})")

url = "rtsp://127.0.0.1:8554/"

def on_event(event, data):
    global hasFrame
    print(f"event: {event}")
    if event == crp.EVENT_NEW_FRAME or event == crp.EVENT_NEW_AUDIO:
        hasFrame = True
    elif event == crp.EVENT_ERROR:
        print("error")

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
