import { PlayerController } from './PlayerController';

@Component
export struct PlayerView {
  @Require surfaceId: string;
  controller: PlayerController = new PlayerController();

  build() {
    XComponent({
      id: this.surfaceId,
      type: XComponentType.SURFACE,
      controller: this.controller.xcomponentController,
    })
  }
}