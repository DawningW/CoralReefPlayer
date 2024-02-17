import UIKit
import CoralReefPlayer

class ViewController: UIViewController {

    let url = "rtsp://127.0.0.1/main.sdp"
    let player = Player()
    
    @IBOutlet weak var imagePlayer: UIImageView!
    
    
    deinit {
        player.stop()
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        NSLog("CoralReefPlayer version: \(versionStr()) (\(versionCode()))")
        player.play(url: url, transport: Transport.UDP, width: 0, height: 0, format: Format.RGBA32, callback: { (event, data) in
            NSLog("event: \(event)")
            if event == Event.NEW_FRAME {
                let frame = data as! Frame

                let colorSpace = CGColorSpaceCreateDeviceRGB()
                let bitmapInfo = CGBitmapInfo(rawValue: CGBitmapInfo.byteOrder32Big.rawValue | CGImageAlphaInfo.premultipliedLast.rawValue)
                let providerRef = CGDataProvider(data: NSData(bytes: frame.data.baseAddress, length: frame.data.count) as CFData)!
                let cgImage = CGImage(width: frame.width, height: frame.height, bitsPerComponent: 8, bitsPerPixel: 32, bytesPerRow: frame.linesize, space: colorSpace, bitmapInfo: bitmapInfo, provider: providerRef, decode: nil, shouldInterpolate: true, intent: .defaultIntent)!

                DispatchQueue.main.async {
                    self.imagePlayer.image = UIImage(cgImage: cgImage)
                }
            }
        })
    }


}

