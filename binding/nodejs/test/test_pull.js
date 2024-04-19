const { format } = require('path');
const crp = require('../lib/index.js');
const assert = require('assert');

console.log(`CoralReefPlayer version: ${crp.versionStr()} (${crp.versionCode()})`);

const url = 'rtsp://127.0.0.1:8554/';

const wait = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function testPull() {
    let hasFrame = false;
    let player = new crp.Player();
    let option = {
        transport: crp.Transport.UDP,
        format: crp.Format.YUV420P,
    }
    player.play(url, option, (event, data) => {
        console.log(`event: ${event}`);
        if (event == crp.Event.NEW_FRAME || event == crp.Event.NEW_AUDIO) {
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
    player.release();
    assert(hasFrame);
}

module.exports = testPull;
