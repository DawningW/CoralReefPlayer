import UIKit
import CoralReefPlayer

class ViewController: UIViewController {

    let url = "rtsp://127.0.0.1:8554/"
    let player = Player()
    
    @IBOutlet weak var imagePlayer: UIImageView!
    
    
    deinit {
        player.stop()
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        NSLog("CoralReefPlayer version: \(versionStr()) (\(versionCode()))")
        let option = Option(
            transport: Transport.UDP,
            videoFormat: VideoFormat.RGBA32
        )
        player.play(url: url, option: option, callback: { (event, data) in
            NSLog("event: \(event)")
            if event == Event.NEW_FRAME {
                let frame = data as! Frame

                let colorSpace = CGColorSpaceCreateDeviceRGB()
                let bitmapInfo = CGBitmapInfo(rawValue: CGBitmapInfo.byteOrder32Big.rawValue | CGImageAlphaInfo.premultipliedLast.rawValue)
                let providerRef = CGDataProvider(data: NSData(bytes: frame.data[0].baseAddress, length: frame.data[0].count) as CFData)!
                let cgImage = CGImage(width: frame.width, height: frame.height, bitsPerComponent: 8, bitsPerPixel: 32, bytesPerRow: frame.stride[0], space: colorSpace, bitmapInfo: bitmapInfo, provider: providerRef, decode: nil, shouldInterpolate: true, intent: .defaultIntent)!

                DispatchQueue.main.async {
                    self.imagePlayer.image = UIImage(cgImage: cgImage)
                }
            }
        })
    }


}

