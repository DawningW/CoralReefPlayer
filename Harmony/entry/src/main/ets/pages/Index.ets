import { hilog } from '@kit.PerformanceAnalysisKit';
import { Transport, Format, Option, PlayerController, PlayerView, versionCode, versionStr } from 'coralreefplayer';

const TAG = 'crpdemo';

@Entry
@Component
struct Index {
  @State url: string = 'rtsp://127.0.0.1:8554/test';
  player: PlayerController = new PlayerController();

  build() {
    Column() {
      Row() {
        TextInput({placeholder: 'stream url', text: this.url})
          .layoutWeight(4)
          .showUnderline(true)
          .onChange((value) => {
            this.url = value;
          })
        Button('Play!')
          .layoutWeight(1)
          .fontSize(18)
          .onClick(() => {
            let option: Option = {
              transport: Transport.UDP,
              video_format: Format.RGBA32,
            };
            this.player.play(this.url, option, (event, data) => {
              hilog.info(0x0000, TAG, 'event: %{public}d', event);
            })
          })
      }.width('100%')
      Blank()
      PlayerView({
        surfaceId: 'Player',
        controller: this.player,
      }).width('100%').aspectRatio(16 / 9)
      Blank()
      Text(`CoralReefPlayer: ${versionStr()} (${versionCode()})`)
    }
    .height('100%')
    .width('100%')
  }
}