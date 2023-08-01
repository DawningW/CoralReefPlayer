import time
import queue
import coralreefplayer as crp
import cv2

images = queue.Queue(5)

def on_frame(event, data):
    if event == crp.CRP_EV_NEW_FRAME:
        frame = crp.Frame.from_address(data)
        image = crp.frame_to_mat(frame)
        image = cv2.resize(image, (640, 360))
        images.put(image)

if __name__ == '__main__':
    cv2.namedWindow('CoralReefPlayerDemo')
    player = crp.create()
    callback = crp.Callback(on_frame)
    crp.play(player, b'rtsp://127.0.0.1/main.sdp', crp.Transport.UDP,
            crp.CRP_WIDTH_AUTO, crp.CRP_HEIGHT_AUTO, crp.Format.BGR24, callback)
    
    while True:
        if not images.empty():
            image = images.get(timeout=0.033)
            cv2.imshow('CoralReefPlayerDemo', image)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        
    crp.destroy(player)
    cv2.destroyAllWindows()
