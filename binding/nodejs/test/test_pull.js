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
    };
    player.play(url, option, (event, data) => {
        if (event == crp.Event.NEW_FRAME || event == crp.Event.NEW_AUDIO) {
            if (event == crp.Event.NEW_FRAME) {
                console.log(`frame: ${data.width}x${data.height}, format: ${data.format}, pts: ${data.pts}`);
            } else {
                console.log(`audio: ${data.sample_rate}x${data.channels}, format: ${data.format}, pts: ${data.pts}`);
            }
            hasFrame = true;
        } else if (event == crp.Event.VIDEO_EXTRADATA || event == crp.Event.AUDIO_EXTRADATA) {
            console.log(`received ${event == crp.Event.VIDEO_EXTRADATA ? 'video' : 'audio'} extra data, size: ${data.length}`);
        } else {
            console.log(`event: ${event}, data: ${data}`);
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
