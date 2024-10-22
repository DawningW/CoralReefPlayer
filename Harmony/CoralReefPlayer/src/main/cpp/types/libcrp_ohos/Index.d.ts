export function create(): any;
export function destroy(handle: any): void;
export function auth(handle: any, username: string, password: string, isMD5: boolean): void;
export function play(handle: any, url: string, option: any, callback: any): void;
export function replay(handle: any): void;
export function stop(handle: any): void;
export function versionCode(): number;
export function versionStr(): string;
