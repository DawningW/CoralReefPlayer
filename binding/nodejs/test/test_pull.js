const crp = require('../lib/index.js');
const assert = require('assert');

console.log(`CoralReefPlayer version: ${crp.versionStr()} (${crp.versionCode()})`);

const url = 'rtsp://127.0.0.1/main.sdp';

const wait = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function testPull() {
    let hasFrame = false;
    let player = crp.create();
    crp.play(player, url, crp.Transport.UDP, 0, 0, crp.Format.RGBA32,
        (event, data) => {
            console.log(`event: ${event}`);
            if (event == crp.Event.NEW_FRAME) {
                hasFrame = true;
            } else if (event == crp.Event.ERROR) {
                console.error('error');
            }
        });
    for (let i = 0; i < 10; i++) {
        await wait(1000);
        if (hasFrame) {
            break;
        }
    }
    crp.destroy(player);
    assert(hasFrame);
}

module.exports = testPull;
