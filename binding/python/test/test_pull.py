import time
import queue
import coralreefplayer as crp
import numpy as np
# import cv2
import pygame

print(f"CoralReefPlayer version: {crp.version_str()} ({crp.version_code()})")

url = "rtsp://127.0.0.1/main.sdp"
width = 1280
height = 720
images = queue.Queue(3)

def on_event(event, data):
    print(f"event: {event}")
    if event == 0:
        frame = np.array(data, copy=False)
        # frame = cv2.resize(frame, (width, height))
        images.put(frame)
    elif event == 1:
        print("error")

if __name__ == "__main__":
    pygame.init()
    pygame.display.set_caption("pyCoralReefPlayerDemo")
    screen = pygame.display.set_mode((width, height))
    player = crp.create()
    crp.play(player, url, crp.TRANS_UDP, width, height, crp.FORMAT_BGR24, on_event)
    
    running = True
    while running:
        if not images.empty():
            image = images.get(timeout=0.033)
            surface = pygame.image.frombuffer(image, (image.shape[1], image.shape[0]), "BGR")
            screen.blit(surface, (0, 0))
            pygame.display.flip()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
    
    crp.destroy(player)
    pygame.quit()
