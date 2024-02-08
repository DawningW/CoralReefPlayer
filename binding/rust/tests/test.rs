use std::sync::{atomic::{AtomicBool, Ordering}, Arc};
use std::thread::sleep;
use std::time::Duration;
use coralreefplayer_rs as crp;

#[test]
fn test_pull() {
    let url = "rtsp://127.0.0.1/main.sdp";
    println!("CoralReefPlayer version: {} ({})", crp::version_str(), crp::version_code());
    let has_frame = Arc::new(AtomicBool::new(false));
    let mut player = crp::Player::new();
    let has_frame_clone = has_frame.clone();
    player.play(url, crp::Transport::UDP, 0, 0, crp::Format::RGB24,
                move |event, data| {
                    match data {
                        crp::Data::Frame(frame) => {
                            println!("frame: {}x{}, format: {}, pts: {}", frame.width, frame.height, frame.format as i8, frame.pts);
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
