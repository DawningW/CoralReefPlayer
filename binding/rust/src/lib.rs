use ::std::os::raw::{c_char, c_int, c_uchar, c_uint, c_longlong, c_ulonglong, c_void};
use ::std::ffi::{CStr, CString};

#[repr(C)]
#[derive(Copy, Clone)]
struct COption {
    transport: c_int,
    video_width: c_int,
    video_height: c_int,
    video_format: c_int,
    hw_device: [c_char; 32],
    enable_audio: bool,
    audio_sample_rate: c_int,
    audio_channels: c_int,
    audio_format: c_int,
    timeout: c_longlong,
}

#[repr(C)]
#[derive(Copy, Clone)]
struct CFrame {
    width: c_int,
    height: c_int,
    format: c_uint,
    data: [*mut c_uchar; 4usize],
    linesize: [c_int; 4usize],
    pts: c_ulonglong,
}

#[allow(non_camel_case_types)]
type crp_handle = *mut c_void;
#[allow(non_camel_case_types)]
type crp_callback = extern "C" fn(event: c_int, data: *mut c_void, user_data: *mut c_void);

extern "C" {
    fn crp_create() -> crp_handle;
    fn crp_destroy(handle: crp_handle);
    fn crp_auth(handle: crp_handle, username: *const c_char, password: *const c_char, is_md5: bool);
    fn crp_play(handle: crp_handle, url: *const c_char, option: *mut COption, callback: crp_callback, user_data: *mut c_void);
    fn crp_replay(handle: crp_handle);
    fn crp_stop(handle: crp_handle);
    fn crp_version_code() -> c_int;
    fn crp_version_str() -> *const c_char;
}

#[derive(PartialEq, Clone, Copy, Default)]
pub enum Transport {
    #[default]
    UDP,
    TCP,
}

#[derive(PartialEq, Clone, Copy, Default)]
pub enum VideoFormat {
    #[default]
    YUV420P,
    NV12,
    NV21,
    RGB24,
    BGR24,
    ARGB32,
    RGBA32,
    ABGR32,
    BGRA32,
}

#[derive(PartialEq, Clone, Copy, Default)]
pub enum AudioFormat {
    #[default]
    U8,
	S16,
	S32,
	F32,
}

#[derive(PartialEq, Clone, Copy)]
pub enum Format {
    Video(VideoFormat),
    Audio(AudioFormat),
}

#[derive(PartialEq, Clone, Copy)]
pub enum Event {
    NewFrame,
    Error,
    Start,
    Playing,
    End,
    Stop,
    NewAudio,
}

#[derive(Default)]
pub struct Option {
    pub transport: Transport,
    pub width: i32,
    pub height: i32,
    pub video_format: VideoFormat,
    pub hw_device: String,
    pub enable_audio: bool,
    pub sample_rate: i32,
    pub channels: i32,
    pub audio_format: AudioFormat,
    pub timeout: i64,
}

pub struct Frame {
    pub width: i32,
    pub height: i32,
    pub sample_rate: i32,
    pub channels: i32,
    pub format: Format,
    pub data: [Box<[u8]>; 4],
    pub linesize: [i32; 4],
    pub pts: u64,
}

pub enum Data {
    EventCode(i64),
    Frame(Frame),
}

pub struct Player {
    handle: crp_handle,
    callback: Box<dyn Fn(Event, Data)>,
}

extern "C" fn rust_callback(event: c_int, data: *mut c_void, user_data: *mut c_void) {
    let player = unsafe { &*(user_data as *mut Player) };
    let ee = unsafe { ::std::mem::transmute::<_, Event>(event as i8) };
    if ee == Event::NewFrame {
        let cframe = unsafe { &*(data as *const CFrame) };
        let frame = Frame {
            width: cframe.width,
            height: cframe.height,
            sample_rate: 0,
            channels: 0,
            format: Format::Video(unsafe { ::std::mem::transmute::<_, VideoFormat>(cframe.format as i8) }),
            data: [
                unsafe { ::std::slice::from_raw_parts(cframe.data[0], (cframe.linesize[0] * cframe.height) as usize) }.to_vec().into_boxed_slice(),
                unsafe { ::std::slice::from_raw_parts(cframe.data[1], (cframe.linesize[1] * cframe.height / 2) as usize) }.to_vec().into_boxed_slice(),
                unsafe { ::std::slice::from_raw_parts(cframe.data[2], (cframe.linesize[2] * cframe.height / 2) as usize) }.to_vec().into_boxed_slice(),
                unsafe { ::std::slice::from_raw_parts(cframe.data[3], (cframe.linesize[3] * cframe.height / 2) as usize) }.to_vec().into_boxed_slice(),
            ],
            linesize: cframe.linesize,
            pts: cframe.pts,
        };
        (*player.callback)(ee, Data::Frame(frame));
    } else if ee == Event::NewAudio {
        let cframe = unsafe { &*(data as *const CFrame) };
        let frame = Frame {
            width: 0,
            height: 0,
            sample_rate: cframe.width,
            channels: cframe.height,
            format: Format::Audio(unsafe { ::std::mem::transmute::<_, AudioFormat>(cframe.format as i8) }),
            data: [
                unsafe { ::std::slice::from_raw_parts(cframe.data[0], cframe.linesize[0] as usize) }.to_vec().into_boxed_slice(),
                Box::new([]),
                Box::new([]),
                Box::new([]),
            ],
            linesize: cframe.linesize,
            pts: cframe.pts,
        };
        (*player.callback)(ee, Data::Frame(frame));
    } else {
        (*player.callback)(ee, Data::EventCode(data as i64));
    }
}

impl Player {
    pub fn new() -> Player {
        Player {
            handle: unsafe { crp_create() },
            callback: Box::new(|_, _| {}),
        }
    }

    pub fn destroy(&mut self) {
        unsafe { crp_destroy(self.handle) }
        self.handle = ::std::ptr::null_mut();
        self.callback = Box::new(|_, _| {});
    }

    pub fn auth(&self, username: &str, password: &str, is_md5: bool) {
        let username = CString::new(username).unwrap();
        let password = CString::new(password).unwrap();
        unsafe { crp_auth(self.handle, username.as_ptr(), password.as_ptr(), is_md5) }
    }

    pub fn play<F>(&mut self, url: &str, option: &Option, callback: F) where
        F: Fn(Event, Data) + 'static {
        let url = CString::new(url).unwrap();
        let mut opt = COption {
            transport: option.transport as i32,
            video_width: option.width,
            video_height: option.height,
            video_format: option.video_format as i32,
            hw_device: {
                let mut hw_device = [0; 32];
                let len = option.hw_device.len().min(hw_device.len());
                for i in 0..len {
                    hw_device[i] = option.hw_device.as_bytes()[i] as i8;
                }
                hw_device
            },
            enable_audio: option.enable_audio,
            audio_sample_rate: option.sample_rate,
            audio_channels: option.channels,
            audio_format: option.audio_format as i32,
            timeout: option.timeout,
        };
        self.callback = Box::new(callback);
        unsafe {
            let user_data = self as *mut Player as *mut c_void;
            crp_play(self.handle, url.as_ptr(), &mut opt as *mut COption, rust_callback, user_data);
        }
    }

    pub fn replay(&self) {
        unsafe { crp_replay(self.handle) }
    }

    pub fn stop(&self) {
        unsafe { crp_stop(self.handle) }
    }
}

pub fn version_code() -> i32 {
    unsafe { crp_version_code() }
}

pub fn version_str() -> String {
    let cstr = unsafe { CStr::from_ptr(crp_version_str()) };
    String::from_utf8_lossy(cstr.to_bytes()).to_string()
}
