import init from "./coralreefplayer.js"

const Module = await init()

const Transport = Module.Transport
const Format = Module.Format
const Event = Module.Event

class Player {
    constructor() {
        this.handle = Module.create()
        this.callback = null
    }
    release() {
        Module.destroy(this.handle)
        this.handle = null
        this.callback = null
    }
    auth(username, password, isMD5) {
        Module.auth(this.handle, username, password, isMD5)
    }
    play(url, option, callback) {
        this.callback = callback // avoid GC
        Module.play(this.handle, url, option, callback)
    }
    replay() {
        Module.replay(this.handle)
    }
    stop() {
        Module.stop(this.handle)
    }
}

function versionCode() {
    return Module.version_code()
}

function versionStr() {
    return Module.version_str()
}

export { Transport, Format, Event, Player, versionCode, versionStr }
