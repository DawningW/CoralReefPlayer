import { image } from '@kit.ImageKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { Transport, Format, Event, Option, Frame, Player, versionCode, versionStr } from 'coralreefplayer';

const TAG = 'crpdemo';

@Entry
@Component
struct Index {
  @State url: string = 'rtsp://127.0.0.1:8554/';
  @State image?: PixelMap = undefined;
  player?: Player;

  build() {
    Column() {
      Row() {
        TextInput({placeholder: 'stream url', text: this.url})
          .layoutWeight(4)
          .showUnderline(true)
          .onChange((value) => {
            this.url = value;
          })
        Button("Play!")
          .layoutWeight(1)
          .fontSize(18)
          .onClick(() => {
            let option: Option = {
              transport: Transport.UDP,
              video_format: Format.RGBA32,
            };
            this.player = new Player();
            this.player.play(this.url, option, (event, data) => {
              hilog.info(0x0000, TAG, 'event: %{public}d', event);
              if (event === Event.NEW_FRAME) {
                let frame = data as Frame;
                hilog.info(0x0000, TAG, 'frame: %{public}dx%{public}d, format: %{public}d',
                  frame.width, frame.height, frame.format);
                let imageData = frame.data as ArrayBuffer;
                image.createPixelMap(imageData, {
                  size: {
                    height: frame.height,
                    width: frame.width
                  },
                  pixelFormat: image.PixelMapFormat.RGBA_8888
                }).then((ret) => {
                  this.image = ret;
                });
              }
            });
          })
      }.width('100%')
      Blank()
      Image(this.image).width('100%').height('50%').objectFit(ImageFit.ScaleDown)
      Blank()
      Text(`CoralReefPlayer: ${versionStr()} (${versionCode()})`)
    }
    .height('100%')
    .width('100%')
  }
}