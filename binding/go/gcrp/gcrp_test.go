package gcrp

import (
	"testing"
	"time"
)

const (
	URL = "rtsp://127.0.0.1/main.sdp"
)

type MyCallback struct {
	onEvent func(event Event, data int64)
	onFrame func(frame Frame)
}

func (cb MyCallback) OnEvent(event Event, data int64) {
	cb.onEvent(event, data)
}

func (cb MyCallback) OnFrame(frame Frame) {
	cb.onFrame(frame)
}

func TestPull(t *testing.T) {
	t.Logf("CoralReefPlayer version: %s (%d)", VersionStr(), VersionCode())
	hasFrame := false
	player := Create()
	defer player.Destroy()
	player.Play(URL, TRANS_UDP, 0, 0, FORMAT_RGB24, MyCallback{
		onEvent: func(event Event, data int64) {
			t.Logf("event: %d, data: %d", event, data)
		},
		onFrame: func(frame Frame) {
			t.Logf("frame: %dx%d, format: %d", frame.Width, frame.Height, frame.Format)
			hasFrame = true
		},
	})
	for i := 0; i < 10; i++ {
		time.Sleep(1 * time.Second)
		if hasFrame {
			break
		}
	}
	if !hasFrame {
		t.Error("No frame received")
	}
}
