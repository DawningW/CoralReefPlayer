package gcrp

import (
	"testing"
	"time"
)

const (
	URL = "rtsp://127.0.0.1:8554/"
)

type MyCallback struct {
	onEvent func(event Event, data int64)
	onFrame func(isAudio bool, frame Frame)
}

func (cb MyCallback) OnEvent(event Event, data int64) {
	cb.onEvent(event, data)
}

func (cb MyCallback) OnFrame(isAudio bool, frame Frame) {
	cb.onFrame(isAudio, frame)
}

func TestPull(t *testing.T) {
	t.Logf("CoralReefPlayer version: %s (%d)", VersionStr(), VersionCode())
	hasFrame := false
	player := Create()
	defer player.Destroy()
	option := Option{
		Transport:   TRANS_UDP,
		VideoFormat: FORMAT_YUV420P,
	}
	player.Play(URL, option, MyCallback{
		onEvent: func(event Event, data int64) {
			t.Logf("event: %d, data: %d", event, data)
		},
		onFrame: func(isAudio bool, frame Frame) {
			if !isAudio {
				t.Logf("frame: %dx%d, format: %d", frame.Width, frame.Height, frame.Format)
			} else {
				t.Logf("audio: %dx%d, format: %d", frame.SampleRate, frame.Channels, frame.Format)
			}
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
