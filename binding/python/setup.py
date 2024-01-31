#coding=utf-8
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext
from build_config import *

__version__ = CRP_VERSION

ext_modules = [
    Pybind11Extension(
        "coralreefplayer.cwrapper",
        ["src/main.cpp"],
        include_dirs=[CRP_INCLUDE_DIR],
        library_dirs=[CRP_LIBRARY_DIR],
        libraries=[CRP_LIBRARY]
    )
]

setup(
    name="coralreefplayer",
    version=__version__,
    description="Python binding for CoralReefPlayer",
    author="Wu Chen",
    author_email="wc@mail.dawncraft.cc",
    url="http://nas.oureda.cn:8418/ROV/CoralReefPlayer",
    packages=["coralreefplayer"],
    package_dir={"coralreefplayer": "coralreefplayer"},
    package_data={"coralreefplayer": ["*.dll", "*.so", "*.dylib"]},
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    python_requires=">=3.7"
)
