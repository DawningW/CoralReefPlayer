export enum Transport {
    UDP,
    TCP,
}

export enum Format {
    YUV420P,
    RGB24,
    BGR24,
    ARGB32,
    RGBA32,
    ABGR32,
    BGRA32,
}

export enum Event {
    NEW_FRAME,
    ERROR,
    START,
    PLAYING,
    END,
    STOP,
}

export interface Frame {
    width: number;
    height: number;
    format: Format;
    data: Buffer;
    linesize: number;
    pts: number;
}

export class Player {
    constructor();
    release(): void;
    auth(username: string, password: string, isMD5: boolean): void;
    play(url: string, transport: Transport, width: number, height: number, format: Format,
        callback: (event: Event, data: number | Frame) => void): void;
    replay(): void;
    stop(): void;
}

export function versionCode(): number;
export function versionStr(): string;
