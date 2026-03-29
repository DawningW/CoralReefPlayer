package gcrp

/*
#cgo CFLAGS: -I.
#cgo LDFLAGS: -L. -lCoralReefPlayer
#cgo darwin LDFLAGS: -Wl,-rpath,.
#cgo linux LDFLAGS: -Wl,-rpath,$ORIGIN
#include <stdlib.h>
#include "coralreefplayer.h"
extern void goCallback(enum Event, void*, void*);
static int frame_get_width(struct Frame* frame) { return frame->width; }
static int frame_get_height(struct Frame* frame) { return frame->height; }
*/
import "C"
import (
	"sync"
	"unsafe"
)

type Transport int

const (
	TRANS_UDP Transport = iota
	TRANS_TCP
)

type Format int

const (
	FORMAT_YUV420P Format = iota
	FORMAT_NV12
	FORMAT_NV21
	FORMAT_RGB24
	FORMAT_BGR24
	FORMAT_ARGB32
	FORMAT_RGBA32
	FORMAT_ABGR32
	FORMAT_BGRA32
)

const (
	FORMAT_U8 Format = iota
	FORMAT_S16
	FORMAT_S32
	FORMAT_F32
)

type Event int

const (
	EVENT_NEW_FRAME Event = iota
	EVENT_ERROR
	EVENT_START
	EVENT_PLAYING
	EVENT_END
	EVENT_STOP
	EVENT_NEW_AUDIO
	EVENT_VIDEO_EXTRADATA
	EVENT_AUDIO_EXTRADATA
)

type Option struct {
	Transport   Transport
	Width       int
	Height      int
	VideoFormat Format
	HWDevice    string
	EnableAudio bool
	SampleRate  int
	Channels    int
	AudioFormat Format
	Timeout     int64
}

type Frame struct {
	Width      int
	Height     int
	SampleRate int
	Channels   int
	Format     Format
	Data       [4][]byte
	Stride     [4]int
	PTS        uint64
}

type Callback interface {
	OnEvent(event Event, data any)
	OnFrame(isAudio bool, frame Frame)
}

type Player struct {
	handle     C.crp_handle
	callbackId int
}

var mutex sync.RWMutex
var callbacks = make(map[int]Callback)
var lastId = 0

//export goCallback
func goCallback(event C.enum_Event, data unsafe.Pointer, userData unsafe.Pointer) {
	id := int(uintptr(userData))
	mutex.RLock()
	callback, ok := callbacks[id]
	mutex.RUnlock()
	if !ok {
		return
	}

	if event == C.CRP_EV_NEW_FRAME {
		frame := (*C.struct_Frame)(data)
		height := C.frame_get_height(frame)
		callback.OnFrame(false, Frame{
			int(C.frame_get_width(frame)),
			int(height),
			0, 0,
			Format(frame.format),
			[4][]byte{
				C.GoBytes(unsafe.Pointer(frame.data[0]), C.int(frame.stride[0]*height)),
				C.GoBytes(unsafe.Pointer(frame.data[1]), C.int(frame.stride[1]*height/2)),
				C.GoBytes(unsafe.Pointer(frame.data[2]), C.int(frame.stride[2]*height/2)),
				C.GoBytes(unsafe.Pointer(frame.data[3]), C.int(frame.stride[3]*height/2)),
			},
			[4]int{int(frame.stride[0]), int(frame.stride[1]), int(frame.stride[2]), int(frame.stride[3])},
			uint64(frame.pts),
		})
	} else if event == C.CRP_EV_NEW_AUDIO {
		frame := (*C.struct_Frame)(data)
		callback.OnFrame(true, Frame{
			0, 0,
			int(C.frame_get_width(frame)),
			int(C.frame_get_height(frame)),
			Format(frame.format),
			[4][]byte{
				C.GoBytes(unsafe.Pointer(frame.data[0]), C.int(frame.stride[0])),
				{},
				{},
				{},
			},
			[4]int{int(frame.stride[0]), int(frame.stride[1]), int(frame.stride[2]), int(frame.stride[3])},
			uint64(frame.pts),
		})
	} else if event == C.CRP_EV_VIDEO_EXTRADATA || event == C.CRP_EV_AUDIO_EXTRADATA {
		ed := (*C.struct_ExtraData)(data)
		callback.OnEvent(Event(event), C.GoBytes(unsafe.Pointer(ed.data), C.int(ed.size)))
	} else {
		callback.OnEvent(Event(event), int64(uintptr(data)))
	}
}

func Create() *Player {
	return &Player{C.crp_create(), 0}
}

func (player *Player) Destroy() {
	C.crp_destroy(player.handle)
	player.handle = nil

	mutex.Lock()
	delete(callbacks, player.callbackId)
	mutex.Unlock()
}

func (player *Player) Auth(username, password string, isMd5 bool) {
	cusername := C.CString(username)
	defer C.free(unsafe.Pointer(cusername))
	cpassword := C.CString(password)
	defer C.free(unsafe.Pointer(cpassword))

	C.crp_auth(player.handle, cusername, cpassword, C.bool(isMd5))
}

func (player *Player) Play(url string, option Option, callback Callback) {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))
	chwdevice := C.CString(option.HWDevice)
	defer C.free(unsafe.Pointer(chwdevice))

	coption := C.struct_Option{
		transport: C.int(option.Transport),
		video: C.struct___1{
			width:     C.int(option.Width),
			height:    C.int(option.Height),
			format:    C.int(option.VideoFormat),
			hw_device: *(*[32]C.char)(unsafe.Pointer(chwdevice)),
		},
		enable_audio: C.bool(option.EnableAudio),
		audio: C.struct___2{
			sample_rate: C.int(option.SampleRate),
			channels:    C.int(option.Channels),
			format:      C.int(option.AudioFormat),
		},
		timeout: C.int64_t(option.Timeout),
	}

	mutex.Lock()
	id := lastId
	player.callbackId = id
	callbacks[id] = callback
	lastId += 1
	mutex.Unlock()

	C.crp_play(player.handle, curl, &coption, C.crp_callback(C.goCallback), unsafe.Pointer(uintptr(id)))
}

func (player *Player) Replay() {
	C.crp_replay(player.handle)
}

func (player *Player) Stop() {
	C.crp_stop(player.handle)
}

func VersionCode() int {
	return int(C.crp_version_code())
}

func VersionStr() string {
	return C.GoString(C.crp_version_str())
}
