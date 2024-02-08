from typing import Union, Callable, NewType
from typing_extensions import Buffer

Transport = NewType("Transport", int)
TRANS_UDP: Transport
TRANS_TCP: Transport

Format = NewType("Format", int)
FORMAT_YUV420P: Format
FORMAT_RGB24: Format
FORMAT_BGR24: Format
FORMAT_ARGB32: Format
FORMAT_RGBA32: Format
FORMAT_ABGR32: Format
FORMAT_BGRA32: Format

Event = NewType("Event", int)
EVENT_NEW_FRAME: Event
EVENT_ERROR: Event
EVENT_START: Event
EVENT_PLAYING: Event
EVENT_END: Event
EVENT_STOP: Event

class Frame(Buffer):
    width: int
    height: int
    format: Format
    pts: int

class Player:
    def __init__(self) -> None: ...
    def release(self) -> None: ...
    def auth(self, username: str, password: str, is_md5: bool) -> None: ...
    def play(self, url: str, transport: Transport, width: int, height: int, format: Format,
             callback: Callable[[Event, Union[int, Frame]], str]) -> None: ...
    def replay(self) -> None: ...
    def stop(self) -> None: ...

def version_code() -> int: ...
def version_str() -> str: ...
