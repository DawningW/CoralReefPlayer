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
    RGB24: 1,
    BGR24: 2,
    ARGB32: 3,
    RGBA32: 4,
    ABGR32: 5,
    BGRA32: 6,
}

const Event = {
    NEW_FRAME: 0,
    ERROR: 1,
    START: 2,
    PLAYING: 3,
    END: 4,
    STOP: 5,
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
    play(url, transport, width, height, format, callback) {
        this.callback = callback // avoid GC
        addon.play(this.handle, url, transport, width, height, format, callback)
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
