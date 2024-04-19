from typing import Optional, Union, List, TypedDict, Callable, NewType
from typing_extensions import Buffer

Transport = NewType("Transport", int)
TRANS_UDP: Transport
TRANS_TCP: Transport

Format = NewType("Format", int)
FORMAT_YUV420P: Format
FORMAT_NV12: Format
FORMAT_NV21: Format
FORMAT_RGB24: Format
FORMAT_BGR24: Format
FORMAT_ARGB32: Format
FORMAT_RGBA32: Format
FORMAT_ABGR32: Format
FORMAT_BGRA32: Format
FORMAT_U8: Format
FORMAT_S16: Format
FORMAT_S32: Format
FORMAT_F32: Format

Event = NewType("Event", int)
EVENT_NEW_FRAME: Event
EVENT_ERROR: Event
EVENT_START: Event
EVENT_PLAYING: Event
EVENT_END: Event
EVENT_STOP: Event
EVENT_NEW_AUDIO: Event

class Option(TypedDict):
    transport: Transport
    width: int
    height: int
    video_format: Format
    hw_device: str
    enable_audio: bool
    sample_rate: int
    channels: int
    audio_format: Format
    timeout: int

class Frame(TypedDict):
    width: Optional[int]
    height: Optional[int]
    sample_rate: Optional[int]
    channels: Optional[int]
    format: Format
    data: Buffer | List[Buffer]
    pts: int

class Player:
    def __init__(self) -> None: ...
    def release(self) -> None: ...
    def auth(self, username: str, password: str, is_md5: bool) -> None: ...
    def play(self, url: str, option: Option, callback: Callable[[Event, Union[int, Frame]], None]) -> None: ...
    def replay(self) -> None: ...
    def stop(self) -> None: ...

def version_code() -> int: ...
def version_str() -> str: ...
