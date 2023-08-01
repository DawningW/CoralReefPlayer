from setuptools import setup, find_packages

setup(
    name='CoralReefPlayer',
    version='0.1',
    description='A player for push low delay RTSP live stream',
    author='Wu Chen',
    author_email='wc@mail.dawncraft.cc',
    url='http://nas.oureda.cn:8418/wc/CoralReefCamCpp',
    packages=find_packages(),
    install_requires=[
        'numpy>=1.18.0'
    ]
)
