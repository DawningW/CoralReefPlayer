const path = require('path');
const root = path.resolve(__dirname, '..');
const PATH_VAR = function() {
    if (process.platform === 'win32') {
        return 'PATH';
    } else if (process.platform === 'darwin') {
        return 'DYLD_LIBRARY_PATH';
    } else {
        return 'LD_LIBRARY_PATH';
    }
}();
process.env[PATH_VAR] = `${process.env[PATH_VAR]}${path.delimiter}${path.resolve(root, 'prebuilds')}`;

const addon = require('node-gyp-build')(root);

const Transport = {
    UDP: 0,
    TCP: 1,
}

const Format = {
    YUV420P: 0,
    NV12: 1,
    NV21: 2,
    RGB24: 3,
    BGR24: 4,
    ARGB32: 5,
    RGBA32: 6,
    ABGR32: 7,
    BGRA32: 8,

    U8: 0,
    S16: 1,
    S32: 2,
    F32: 3,
}

const Event = {
    NEW_FRAME: 0,
    ERROR: 1,
    START: 2,
    PLAYING: 3,
    END: 4,
    STOP: 5,
    NEW_AUDIO: 6,
}

class Player {
    constructor() {
        this.handle = addon.create()
        this.callback = null
    }
    release() {
        addon.destroy(this.handle)
        this.handle = null
        this.callback = null
    }
    auth(username, password, isMD5) {
        addon.auth(this.handle, username, password, isMD5)
    }
    play(url, option, callback) {
        this.callback = callback // avoid GC
        addon.play(this.handle, url, option, callback)
    }
    replay() {
        addon.replay(this.handle)
    }
    stop() {
        addon.stop(this.handle)
    }
}

module.exports = {
    Transport,
    Format,
    Event,
    Player,
    versionCode: addon.versionCode,
    versionStr: addon.versionStr,
}
