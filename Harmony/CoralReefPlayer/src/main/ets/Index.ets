import crp from 'libcrp_ohos.so'

export enum Transport {
  UDP = 0,
  TCP = 1,
}

export enum Format {
  YUV420P = 0,
  NV12 = 1,
  NV21 = 2,
  RGB24 = 3,
  BGR24 = 4,
  ARGB32 = 5,
  RGBA32 = 6,
  ABGR32 = 7,
  BGRA32 = 8,

  U8 = 0,
  S16 = 1,
  S32 = 2,
  F32 = 3,
}

export enum Event {
  NEW_FRAME = 0,
  ERROR = 1,
  START = 2,
  PLAYING = 3,
  END = 4,
  STOP = 5,
  NEW_AUDIO = 6,
}

export interface Option {
  transport?: Transport;
  width?: number;
  height?: number;
  video_format?: Format;
  hw_device?: string;
  enable_audio?: boolean;
  sample_rate?: number;
  channels?: number;
  audio_format?: Format;
  timeout?: number;
}

export interface Frame {
  width?: number;
  height?: number;
  sample_rate?: number;
  channels?: number;
  format: Format;
  data: ArrayBuffer | ArrayBuffer[];
  stride: number | number[];
  pts: number;
}

export class Player {
  handle?: Object;
  callback?: Object;

  constructor() {
    this.handle = crp.create()
    this.callback = undefined
  }
  release(): void {
    crp.destroy(this.handle)
    this.handle = undefined
    this.callback = undefined
  }
  auth(username: string, password: string, isMD5: boolean): void {
    crp.auth(this.handle, username, password, isMD5)
  }
  play(url: string, option: Option, callback: (event: Event, data: number | Frame) => void): void {
    this.callback = callback // avoid GC
    crp.play(this.handle, url, option, callback)
  }
  replay(): void {
    crp.replay(this.handle)
  }
  stop(): void {
    crp.stop(this.handle)
  }
}

export {
  versionCode,
  versionStr
} from 'libcrp_ohos.so'
