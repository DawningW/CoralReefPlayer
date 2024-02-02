const path = require('path');
const root = path.resolve(__dirname, '..');
process.env.PATH = `${process.env.PATH}${path.delimiter}${path.resolve(root, 'prebuilds')}`;

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

module.exports = {
    Transport,
    Format,
    Event,
    ...addon
}
