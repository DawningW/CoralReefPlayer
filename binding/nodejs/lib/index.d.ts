export enum Transport {
    UDP,
    TCP,
}

export enum Format {
    YUV420P = 0,
    NV12,
    NV21,
    RGB24,
    BGR24,
    ARGB32,
    RGBA32,
    ABGR32,
    BGRA32,

    U8 = 0,
    S16,
    S32,
    F32,
}

export enum Event {
    NEW_FRAME,
    ERROR,
    START,
    PLAYING,
    END,
    STOP,
    NEW_AUDIO,
}

export interface Option {
    transport: Transport;
    width: number;
    height: number;
    video_format: Format;
    hw_device: string;
    enable_audio: boolean;
    sample_rate: number;
    channels: number;
    audio_format: Format;
    timeout: number;
}

export interface Frame {
    width?: number;
    height?: number;
    sample_rate?: number;
    channels?: number;
    format: Format;
    data: Buffer | Buffer[];
    linesize: number | number[];
    pts: number;
}

export class Player {
    constructor();
    release(): void;
    auth(username: string, password: string, isMD5: boolean): void;
    play(url: string, option: Option, callback: (event: Event, data: number | Frame) => void): void;
    replay(): void;
    stop(): void;
}

export function versionCode(): number;
export function versionStr(): string;
