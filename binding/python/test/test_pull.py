import time
import coralreefplayer as crp

print(f"CoralReefPlayer version: {crp.version_str()} ({crp.version_code()})")

url = "rtsp://127.0.0.1/main.sdp"

def on_event(event, data):
    global hasFrame
    print(f"event: {event}")
    if event == crp.EVENT_NEW_FRAME:
        hasFrame = True
    elif event == crp.EVENT_ERROR:
        print("error")

if __name__ == "__main__":
    hasFrame = False
    player = crp.create()
    crp.play(player, url, crp.TRANS_UDP, 0, 0, crp.FORMAT_BGR24, on_event)
    for i in range(10):
        time.sleep(1)
        if hasFrame:
            break
    crp.destroy(player)
    assert hasFrame
