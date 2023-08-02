from setuptools import setup

setup(
    name='CoralReefPlayer',
    version='0.1',
    description='A player for push low delay RTSP live stream',
    author='Wu Chen',
    author_email='wc@mail.dawncraft.cc',
    url='http://nas.oureda.cn:8418/wc/CoralReefCamCpp',
    packages=['coralreefplayer'],
    package_dir={'coralreefplayer': 'coralreefplayer'},
    package_data={'coralreefplayer': ['*.dll', '*.so', '*.dylib']},
    include_package_data=True,
    install_requires=[
        'numpy'
    ]
)
