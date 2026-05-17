import { renderYUVOnSurface } from 'libcrp_ohos.so';
import { Player, Event, Option, Frame, EventData } from './Player';

class PlayerXComponentController extends XComponentController {
  surfaceId?: string;

  onSurfaceCreated(surfaceId: string): void {
    this.surfaceId = surfaceId;
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {}

  onSurfaceDestroyed(surfaceId: string): void {
    this.surfaceId = undefined;
  }
}

export class PlayerController {
  public readonly xcomponentController: PlayerXComponentController = new PlayerXComponentController();
  private player: Player = new Player();

  setAuth(username: string, password: string, isMD5: boolean = false): void {
    this.player.auth(username, password, isMD5);
  }

  release(): void {
    this.player.release();
  }

  play(url: string, options?: Option, callback?: (event: Event, data: EventData) => void): void {
    this.player.play(url, options || {}, (event, data) => {
      if (callback) {
        callback(event, data);
      }
      if (event == Event.NEW_FRAME) {
        if (this.xcomponentController.surfaceId) {
          renderYUVOnSurface(this.xcomponentController.surfaceId, data as Frame);
        }
      }
    });
  }

  replay(): void {
    this.player.replay();
  }

  stop(): void {
    this.player.stop();
  }
}