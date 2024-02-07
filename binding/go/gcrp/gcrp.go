package gcrp

// #cgo CFLAGS: -I.
// #cgo LDFLAGS: -L. -lCoralReefPlayer
// #include <stdlib.h>
// #include "coralreefplayer.h"
// extern void goCallback(enum Event, void*, void*);
import "C"
import "unsafe"

type Transport int

const (
	TRANS_UDP Transport = iota
	TRANS_TCP
)

type Format int

const (
	FORMAT_YUV420P Format = iota
	FORMAT_RGB24
	FORMAT_BGR24
	FORMAT_ARGB32
	FORMAT_RGBA32
	FORMAT_ABGR32
	FORMAT_BGRA32
)

type Event int

const (
	EVENT_NEW_FRAME Event = iota
	EVENT_ERROR
	EVENT_START
	EVENT_PLAYING
	EVENT_END
	EVENT_STOP
)

type Frame struct {
	Width    int
	Height   int
	Format   Format
	Data     [4][]byte
	LineSize [4]int
	PTS      uint64
}

type Callback interface {
	OnEvent(event Event, data int64)
	OnFrame(frame Frame)
}

type Player struct {
	handle     C.crp_handle
	callbackId int
}

var callbacks = make(map[int]Callback)
var lastId = 0

//export goCallback
func goCallback(event C.enum_Event, data unsafe.Pointer, userData unsafe.Pointer) {
	id := int(uintptr(userData))
	if event == C.CRP_EV_NEW_FRAME {
		frame := (*C.struct_Frame)(data)
		callbacks[id].OnFrame(Frame{
			int(frame.width),
			int(frame.height),
			Format(frame.format),
			[4][]byte{
				C.GoBytes(unsafe.Pointer(frame.data[0]), C.int(frame.linesize[0]*frame.height)),
				C.GoBytes(unsafe.Pointer(frame.data[1]), C.int(frame.linesize[1]*frame.height)),
				C.GoBytes(unsafe.Pointer(frame.data[2]), C.int(frame.linesize[2]*frame.height)),
				C.GoBytes(unsafe.Pointer(frame.data[3]), C.int(frame.linesize[3]*frame.height)),
			},
			[4]int{int(frame.linesize[0]), int(frame.linesize[1]), int(frame.linesize[2]), int(frame.linesize[3])},
			uint64(frame.pts),
		})
	} else {
		callbacks[id].OnEvent(Event(event), int64(uintptr(data)))
	}
}

func Create() Player {
	return Player{C.crp_create(), 0}
}

func (player Player) Destroy() {
	C.crp_destroy(player.handle)
	player.handle = nil
	delete(callbacks, player.callbackId)
}

func (player Player) Auth(username, password string, isMd5 bool) {
	cusername := C.CString(username)
	cpassword := C.CString(password)
	C.crp_auth(player.handle, cusername, cpassword, C.bool(isMd5))
	C.free(unsafe.Pointer(cusername))
	C.free(unsafe.Pointer(cpassword))
}

func (player Player) Play(url string, transport Transport, width, height int, format Format, callback Callback) {
	curl := C.CString(url)
	player.callbackId = lastId
	callbacks[lastId] = callback
	C.crp_play(player.handle, curl, C.int(transport), C.int(width), C.int(height), C.int(format), C.crp_callback(C.goCallback), unsafe.Pointer(uintptr(lastId)))
	lastId += 1
	C.free(unsafe.Pointer(curl))
}

func (player Player) Replay() {
	C.crp_replay(player.handle)
}

func (player Player) Stop() {
	C.crp_stop(player.handle)
}

func VersionCode() int {
	return int(C.crp_version_code())
}

func VersionStr() string {
	return C.GoString(C.crp_version_str())
}
