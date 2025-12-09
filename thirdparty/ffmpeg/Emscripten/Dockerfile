FROM emscripten/emsdk:4.0.21

ARG FFMPEG_VERSION=4.4.6
ARG PREFIX=/home/ffmpeg

RUN apt update && apt install -y git autoconf libtool build-essential cmake

COPY thirdparty/emfiberthreads /tmp/emfiberthreads

RUN cd /tmp/emfiberthreads && \
    emcmake cmake -B build -DCMAKE_INSTALL_PREFIX=${PREFIX} -DEMFIBERTHREADS_INSTALL=ON && \
    cmake --build build --target install

RUN cd /tmp/ && \
    wget http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz && \
    tar zxf ffmpeg-${FFMPEG_VERSION}.tar.gz && rm ffmpeg-${FFMPEG_VERSION}.tar.gz

ARG CFLAGS="-Oz -I${PREFIX}/include"
ARG LDFLAGS="$CFLAGS -lemfiberthreads -L${PREFIX}/lib -sINITIAL_MEMORY=33554432 -sALLOW_MEMORY_GROWTH=1"

COPY thirdparty/ffmpeg/Emscripten/ffmpeg4.patch /tmp/

RUN cd /tmp/ffmpeg-${FFMPEG_VERSION} && \
    patch -p1 < /tmp/ffmpeg4.patch && \
    emconfigure ./configure \
    --prefix=${PREFIX} \
    --target-os=none \
    --arch=x86_32 \
    --enable-cross-compile \
    --disable-debug \
    --disable-x86asm \
    --disable-inline-asm \
    --disable-stripping \
    --disable-programs \
    --disable-doc \
    --disable-all \
    --disable-network \
    --enable-avcodec \
    --enable-avformat \
    --enable-avfilter \
    --enable-avdevice \
    --enable-avutil \
    --enable-swresample \
    --enable-postproc \
    --enable-swscale \
    --enable-filters \
    --enable-protocol=file \
    --enable-decoder=h264,hevc,aac,*peg,opus,pcm* \
    --enable-demuxer=mov,matroska,mp3,ogg,rt* \
    --enable-muxer=*jpeg,mov,mp4,mpegts,matroska,ogg,rt* \
    --enable-gpl \
    # --enable-libx264 \
    # --enable-libx265 \
    --extra-cflags="$CFLAGS" \
    --extra-cxxflags="$CFLAGS" \
    --extra-ldflags="$LDFLAGS" \
    --nm=emnm \
    --ar=emar \
    --ranlib=emranlib \
    --cc=emcc \
    --cxx=em++ \
    --objcc=emcc \
    --dep-cc=emcc

RUN cd /tmp/ffmpeg-${FFMPEG_VERSION} && \
    emmake make -j8 && \
    emmake make install
