use std::sync::{atomic::{AtomicBool, Ordering}, Arc};
use std::thread::sleep;
use std::time::Duration;
use coralreefplayer_rs as crp;

#[test]
fn test_pull() {
    let url = "rtsp://127.0.0.1:8554/";
    println!("CoralReefPlayer version: {} ({})", crp::version_str(), crp::version_code());
    let has_frame = Arc::new(AtomicBool::new(false));
    let has_frame_clone = has_frame.clone();
    let mut player = crp::Player::new();
    let mut option = crp::Option::default();
    option.transport = crp::Transport::UDP;
    option.video_format = crp::VideoFormat::YUV420P;
    player.play(url, &option, move |event, data| {
        match data {
            crp::Data::Frame(frame) => {
                match frame.format {
                    crp::Format::Video(format) => {
                        println!("frame: {}x{}, format: {}, pts: {}", frame.width, frame.height, format as i8, frame.pts);
                    }
                    crp::Format::Audio(format) => {
                        println!("audio: {}x{}, format: {}, pts: {}", frame.sample_rate, frame.channels, format as i8, frame.pts);
                    }
                }
                has_frame_clone.store(true, Ordering::Relaxed);
            }
            crp::Data::EventCode(code) => {
                println!("event: {}, data: {}", event as i8, code);
            }
        }
    });
    for _ in 0..10 {
        sleep(Duration::from_secs(1));
        if has_frame.load(Ordering::Relaxed) {
            break;
        }
    }
    player.destroy();
    assert!(has_frame.load(Ordering::Relaxed));
}
