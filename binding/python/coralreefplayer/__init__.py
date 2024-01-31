import sys
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PATH_VAR = "PATH" if os.name == "nt" else "LD_LIBRARY_PATH"
if os.name == "nt" and sys.version_info >= (3, 8):
    os.add_dll_directory(BASE_DIR)
os.environ[PATH_VAR] = BASE_DIR + os.pathsep + os.environ[PATH_VAR]
del BASE_DIR, PATH_VAR

from .cwrapper import *

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

__version__ = version_str()
