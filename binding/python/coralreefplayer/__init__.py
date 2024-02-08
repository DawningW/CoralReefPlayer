import sys
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
if "win" in sys.platform:
    PATH_VAR = "PATH"
elif "darwin" in sys.platform:
    PATH_VAR = "DYLD_LIBRARY_PATH"
else:
    PATH_VAR = "LD_LIBRARY_PATH"
if os.name == "nt" and sys.version_info >= (3, 8):
    os.add_dll_directory(BASE_DIR)
os.environ[PATH_VAR] = BASE_DIR + os.pathsep + os.environ[PATH_VAR]
del BASE_DIR, PATH_VAR

from . import extension as ext

TRANS_UDP = 0
TRANS_TCP = 1

FORMAT_YUV420P = 0
FORMAT_RGB24 = 1
FORMAT_BGR24 = 2
FORMAT_ARGB32 = 3
FORMAT_RGBA32 = 4
FORMAT_ABGR32 = 5
FORMAT_BGRA32 = 6

EVENT_NEW_FRAME = 0
EVENT_ERROR = 1
EVENT_START = 2
EVENT_PLAYING = 3
EVENT_END = 4
EVENT_STOP = 5

Frame = ext.Frame

class Player():
    def __init__(self):
        self.handle = ext.create()
        self.callback = None

    def release(self):
        ext.destroy(self.handle)
        self.handle = None
        self.callback = None

    def auth(self, username, password, is_md5):
        ext.auth(self.handle, username, password, is_md5)

    def play(self, url, transport, width, height, format, callback):
        self.callback = callback
        ext.play(self.handle, url, transport, width, height, format, callback)

    def replay(self):
        ext.replay(self.handle)

    def stop(self):
        ext.stop(self.handle)

version_code = ext.version_code
version_str = ext.version_str

__version__ = version_str()
