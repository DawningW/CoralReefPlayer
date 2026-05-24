import { hilog } from '@kit.PerformanceAnalysisKit';
import { Transport, Format, Event, Option, PlayerController, PlayerView, versionCode, versionStr } from 'coralreefplayer';

const TAG = 'crpdemo';

@Entry
@Component
struct Index {
  promptAction = this.getUIContext().getPromptAction();
  @State url: string = 'rtsp://127.0.0.1:8554/test';
  player: PlayerController = new PlayerController();

  toast(message: string) {
    this.promptAction.showToast({
      message: message
    });
  }

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
              if (event == Event.ERROR) {
                this.toast("错误!");
              } else if (event == Event.START) {
                this.toast("开始拉流");
              } else if (event == Event.PLAYING) {
                this.toast("正在播放");
              } else if (event == Event.END) {
                this.toast("视频流已到达末尾");
              } else if (event == Event.STOP) {
                this.toast("停止拉流");
              } else if (event == Event.VIDEO_EXTRADATA) {
                let extraData = data as ArrayBuffer;
                this.toast("获取到视频参数集, 长度" + extraData.byteLength + "字节");
              }
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