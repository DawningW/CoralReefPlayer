import sys
import os
import ctypes
import numpy as np
from numpy._typing import NDArray

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PATH_VAR = 'PATH' if os.name == 'nt' else 'LD_LIBRARY_PATH'
if os.name == 'nt' and sys.version_info[:2] >= (3, 8):
    os.add_dll_directory(BASE_DIR)
os.environ[PATH_VAR] = BASE_DIR + os.pathsep + os.environ[PATH_VAR]

# ctypes.util.find_library('CoralReefPlayer')
dll = ctypes.CDLL('CoralReefPlayer')

CRP_WIDTH_AUTO = 0
CRP_HEIGHT_AUTO = 0
CRP_EV_NEW_FRAME = 0

class Transport:
    UDP = ctypes.c_int(0)
    TCP = ctypes.c_int(1)

class Format:
    YUV420P = ctypes.c_int(0)
    RGB24 = ctypes.c_int(1)
    BGR24 = ctypes.c_int(2)
    ARGB32 = ctypes.c_int(3)
    RGBA32 = ctypes.c_int(4)
    ABGR32 = ctypes.c_int(5)
    BGRA32 = ctypes.c_int(6)

class Frame(ctypes.Structure):
    _fields_ = (
        ('width', ctypes.c_int),
        ('height', ctypes.c_int),
        ('format', ctypes.c_int),
        ('data', ctypes.POINTER(ctypes.c_ubyte) * 4),
        ('linesize', ctypes.c_int * 4),
        ('pts', ctypes.c_ulonglong),
    )
Frame_p = ctypes.POINTER(Frame)

Handle = ctypes.c_void_p
Callback = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_void_p)

dll.crp_create.argtypes = []
dll.crp_create.restype = Handle
def create() -> Handle:
    return dll.crp_create()

dll.crp_destroy.argtypes = [Handle]
dll.crp_destroy.restype = None
def destroy(handle: Handle) -> None:
    dll.crp_destroy(handle)

dll.crp_play.argtypes = [Handle, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, Callback]
dll.crp_play.restype = None
def play(handle: Handle, url: bytes, transport: int, width: int, height: int, format: int, callback: Callback) -> None:
    dll.crp_play(handle, url, transport, width, height, format, callback)

def frame_to_mat(frame: Frame) -> NDArray[np.uint8]:
    image = np.ctypeslib.as_array(frame.data[0], shape=(frame.height, frame.width, 3))
    return image.copy()
