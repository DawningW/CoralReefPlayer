const init = require("./coralreefplayer.js");

var Module = null;
init().then((instance) => {
    Module = instance;
});

class Player {
    constructor() {
        this.handle = Module.create();
        this.callback = null;
    }
    release() {
        Module.destroy(this.handle);
        this.handle = null;
        this.callback = null;
    }
    auth(username, password, isMD5) {
        Module.auth(this.handle, username, password, isMD5);
    }
    play(url, option, callback) {
        this.callback = callback; // avoid GC
        Module.play(this.handle, url, option, callback);
    }
    replay() {
        Module.replay(this.handle);
    }
    stop() {
        Module.stop(this.handle);
    }
}

module.exports = {
    get Transport() { return Module.Transport },
    get Format() { return Module.Format },
    get Event() { return Module.Event },
    Player: Player,
    versionCode: () => Module.version_code(),
    versionStr: () => Module.version_str(),
};
