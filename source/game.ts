
let context: CanvasRenderingContext2D;
let module_instance: WebAssembly.Instance;
let memory: WebAssembly.Memory = new WebAssembly.Memory({initial: 1000, maximum: 1000})
let memoryView = new DataView(memory.buffer);

let malloc: (size: number) => number;
let zalloc: (size: number) => number;
let free: (size: number) => void;

let keystatePtr: number = 0;
let keystateLen: number = 0;

let buttonstatePtr: number = 0;
let buttonstateLen: number = 0;

let screenPtr: number = 0;
let screenLen: number = 0;

let mouseInside: boolean = false;

const audioContext: AudioContext = new AudioContext();

function getKeystateBuffer()
{
    return [keystatePtr, keystateLen];
}

function getButtonstateBuffer()
{
    return [buttonstatePtr, buttonstateLen];
}

function getScreenBuffer()
{
    return [screenPtr, screenLen/4];
}

async function update(timestamp: DOMHighResTimeStamp)
{
    window.requestAnimationFrame(update)

    const beg = Date.now();

    let on_frame: WebAssembly.ExportValue = module_instance.exports["on_frame"];
    (on_frame as any)();
    
    let src = new ImageData(new Uint8ClampedArray(memoryView.buffer).subarray(screenPtr, screenPtr + screenLen), context.canvas.width);
    let image = await createImageBitmap(src);
    context.drawImage(image, 0, 0, image.width, image.height);
    const end = Date.now();
    console.log("ms:", end - beg);
}

let mouseX: number = 0;
let mouseY: number = 0;

function mouseXY()
{
    return [mouseX, mouseY];
}

const page_size = 64*1024;

let audio: HTMLAudioElement[] = new Array<HTMLAudioElement>(0x10);
let audio_count = 0;

function playAudio(id: number, loop: boolean)
{
    audio[id].loop = loop;
    audio[id].play();
}

function pauseAudio(id: number)
{
    audio[id].pause();
}

function audioSetVolume(id: number, level: number)
{
    audio[id].volume = level;
}

function request_audio(uri_ptr: number, uri_len: number)
{
    const uri = new TextDecoder().decode(memory.buffer.slice(uri_ptr, uri_ptr + uri_len));

    console.log(uri);

    const audioElement = document.createElement("audio");
    audioElement.src = uri;

    const track = audioContext.createMediaElementSource(audioElement);
    track.connect(audioContext.destination);

    const res = audio_count++;
    audio[res] = audioElement;

    return res;
}

let imagePtrs: number[] = new Array<number>(0x100);
let images: HTMLImageElement[] = new Array<HTMLImageElement>(0x100);
let imageState: boolean[] = new Array<boolean>(0x100);
let imageWidth: number[] = new Array<number>(0x100);
let imageHeight: number[] = new Array<number>(0x100);
let images_count = 0;

function request_image(uri_ptr: number, uri_len: number)
{
    const uri = new TextDecoder().decode(memory.buffer.slice(uri_ptr, uri_ptr + uri_len));

    console.log(uri);

    const res = images_count++;
    images[res] = new Image();
    
    imageState[res] = false;
    images[res].src = uri;

    return res;
}

function image_ready(res: number)
{
    if (!imageState[res] && images[res].complete)
    {
        let imageCanvas = document.createElement("canvas") as HTMLCanvasElement;
        imageCanvas.width = images[res].width;
        imageCanvas.height = images[res].height;
        imageWidth[res] = imageCanvas.width; 
        imageHeight[res] = imageCanvas.height; 
        console.log(imageCanvas.width, imageCanvas.height);

        let imageCanvasContext = imageCanvas.getContext("2d") as CanvasRenderingContext2D;
        imageCanvasContext.drawImage(images[res], 0, 0);
        let imageData: Uint8ClampedArray = imageCanvasContext.getImageData(0, 0, imageCanvas.width, imageCanvas.height).data;

        imagePtrs[res] = malloc(imageData.length);
        console.log(imageData.length);
        console.log(imageCanvas.width*imageCanvas.height*4);

        for (let i = 0; i < imageCanvas.width*imageCanvas.height*4; ++i)
        {
            memoryView.setUint8(imagePtrs[res] + i, imageData[i]);
        }

        delete images[res];
        
        imageState[res] = true;
    }
    return imageState[res];
}

function get_image(id: number)
{
    return [imagePtrs[id], imageWidth[id], imageHeight[id]];
}

function printStr(ptr: number, len: number)
{
    const uri = new TextDecoder().decode(memory.buffer.slice(ptr, ptr + len));

    console.log(uri);
}

const keyMap: Map<string, number> = new Map<string, number>([
    ["Escape", 0x0001	],
    ["Digit0", 0x0002	],
    ["Digit1", 0x0003	],
    ["Digit2", 0x0004	],
    ["Digit3", 0x0005	],
    ["Digit4", 0x0006	],
    ["Digit5", 0x0007	],
    ["Digit6", 0x0008	],
    ["Digit7", 0x0009	],
    ["Digit8", 0x000A	],
    ["Digit9", 0x000B	],
    ["Minus", 0x000C	],
    ["Equal", 0x000D	],
    ["Backspace", 0x000E	],
    ["Tab", 0x000F	],
    ["KeyQ", 0x0010	],
    ["KeyW", 0x0011	],
    ["KeyE", 0x0012	],
    ["KeyR", 0x0013	],
    ["KeyT", 0x0014	],
    ["KeyY", 0x0015	],
    ["KeyU", 0x0016	],
    ["KeyI", 0x0017	],
    ["KeyO", 0x0018	],
    ["KeyP", 0x0019	],
    ["BracketLeft", 0x001A	],
    ["BracketRight", 0x001B	],
    ["Enter", 0x001C	],
    ["ControlLeft", 0x001D	],
    ["KeyA", 0x001E	],
    ["KeyS", 0x001F	],
    ["KeyD", 0x0020	],
    ["KeyF", 0x0021	],
    ["KeyG", 0x0022	],
    ["KeyH", 0x0023	],
    ["KeyJ", 0x0024	],
    ["KeyK", 0x0025	],
    ["KeyL", 0x0026	],
    ["Semicolon", 0x0027	],
    ["Quote", 0x0028	],
    ["Backquote", 0x0029	],
    ["ShiftLeft", 0x002A	],
    ["Backslash", 0x002B	],
    ["KeyZ", 0x002C	],
    ["KeyX", 0x002D	],
    ["KeyC", 0x002E	],
    ["KeyV", 0x002F	],
    ["KeyB", 0x0030	],
    ["KeyN", 0x0031	],
    ["KeyM", 0x0032	],
    ["Comma", 0x0033	],
    ["Period", 0x0034	],
    ["Slash", 0x0035	],
    ["ShiftRight", 0x0036	],
    ["NumpadMultiply", 0x0037	],
    ["AltLeft", 0x0038	],
    ["Space", 0x0039	],
    ["CapsLock", 0x003A	],
    ["F1", 0x003B	],
    ["F2", 0x003C	],
    ["F3", 0x003D	],
    ["F4", 0x003E	],
    ["F5", 0x003F	],
    ["F6", 0x0040	],
    ["F7", 0x0041	],
    ["F8", 0x0042	],
    ["F9", 0x0043	],
    ["F10", 0x0044	],
    ["Pause", 0x0045	],
    ["ScrollLock", 0x0046	],
    ["Numpad7", 0x0047	],
    ["Numpad8", 0x0048	],
    ["Numpad9", 0x0049	],
    ["NumpadSubtract", 0x004A	],
    ["Numpad4", 0x004B	],
    ["Numpad5", 0x004C	],
    ["Numpad6", 0x004D	],
    ["NumpadAdd", 0x004E	],
    ["Numpad1", 0x004F	],
    ["Numpad2", 0x0050	],
    ["Numpad3", 0x0051	],
    ["Numpad0", 0x0052	],
    ["NumpadDecimal", 0x0053	],
    ["IntlBackslash", 0x0056	],
    ["F11", 0x0057	],
    ["F12", 0x0058	],
    ["IntlYen", 0x007D	],
    ["MediaTrackPrevious", 0xE010	],
    ["MediaTrackNext", 0xE019	],
    ["NumpadEnter", 0xE01C	],
    ["ControlRight", 0xE01D	],
    ["AudioVolumeMute", 0xE020	],
    ["MediaPlayPause", 0xE022	],
    ["MediaStop", 0xE024	],
    ["AudioVolumeDown", 0xE02E	], 
    ["AudioVolumeUp", 0xE030	],
    ["BrowserHome", 0xE032	],
    ["NumpadDivide", 0xE035	],
    ["PrintScreen", 0xE037	],
    ["AltRight", 0xE038	],
    ["NumLock", 0xE045	],
    ["Pause", 0xE046 ],
    ["Home", 0xE047	],
    ["ArrowUp", 0xE048	],
    ["PageUp", 0xE049	],
    ["ArrowLeft", 0xE04B	],
    ["ArrowRight", 0xE04D	],
    ["End", 0xE04F	],
    ["ArrowDown", 0xE050	],
    ["PageDown", 0xE051	],
    ["Insert", 0xE052	],
    ["Delete", 0xE053	],
    ["MetaLeft", 0xE05B	],
    ["MetaRight", 0xE05C	],
    ["ContextMenu", 0xE05D	],
    ["BrowserSearch", 0xE065	],
    ["BrowserFavorites", 0xE066	],
    ["BrowserRefresh", 0xE067	],
    ["BrowserStop", 0xE068	],
    ["BrowserForward", 0xE069	],
    ["BrowserBack", 0xE06A	],
]);

async function main()
{
    console.clear();
    let canvas = document.getElementById("screen") as HTMLCanvasElement;

    context = canvas.getContext("2d") as CanvasRenderingContext2D;

    if (context)
    {
        console.log("context is good!");

        canvas.addEventListener("mouseenter", (event: MouseEvent)=>{
            mouseInside = true;
        });

        canvas.addEventListener("mouseleave", (event: MouseEvent)=>{
            mouseInside = false;
        });

        canvas.addEventListener("mousemove", (event: MouseEvent)=>{ 
            mouseX = event.clientX - canvas.offsetLeft; 
            mouseY = event.clientY - canvas.offsetTop; 
        });

        canvas.addEventListener("mousedown", (event: MouseEvent)=>{ 
            memoryView.setUint8(buttonstatePtr + event.button, 1);
        });

        canvas.addEventListener("mouseup", (event: MouseEvent)=>{ 
            memoryView.setUint8(buttonstatePtr + event.button, 0);
        });

        canvas.addEventListener("keydown", (event: KeyboardEvent)=>{ 
            {
                const code = keyMap.get(event.code);
                if (code != undefined)
                {
                    memoryView.setUint8(keystatePtr + code, 1);
                }
            }
        });
        
        canvas.addEventListener("keyup", (event: KeyboardEvent)=>{ 
            {
                const code = keyMap.get(event.code);
                if (code != undefined)
                {
                    memoryView.setUint8(keystatePtr + code, 0);
                }
            }
        });

        canvas.oncontextmenu = ()=>{ 
            return false;
        };

        const wasm_imports: WebAssembly.Imports = {
            env: { 
                "sinf": (arg: number) => Math.sin(arg), 
                "cosf": (arg: number) => Math.cos(arg), 
                "acosf": (arg: number) => Math.acos(arg),
                "asinf": (arg: number) => Math.asin(arg),
                "sqrtf": (arg: number) => Math.sqrt(arg),
                "sqrti": (arg: number) => Math.sqrt(arg),
                "floor": (arg: number) => Math.floor(arg),
                "ceil": (arg: number) => Math.ceil(arg),
                "mod": (arg0: number, arg1: number) => arg0 % arg1,
                "cursor_xy": () => mouseXY(),
                
                "request_image": (ptr: number, len: number) => request_image(ptr, len),
                "image_ready": (arg: number) => image_ready(arg),
                "get_image": (arg: number) => get_image(arg),
                
                "request_audio": (ptr: number, len: number) => request_audio(ptr, len),
                "audio_play": (id: number, loop: boolean) => playAudio(id, loop),
                "audio_pause": (id: number) => pauseAudio(id),
                "audio_set_volume": (id: number, level: number) => audioSetVolume(id, level),

                "print_i32": (arg: number) => console.log(arg), 
                "print_i32_array": (arg0: number, arg1: number) => console.log(new Int32Array(memory.buffer).subarray(arg1/4, arg1/4 + arg0)), 
                "print_f32_array": (arg0: number, arg1: number) => console.log(new Float32Array(memory.buffer).subarray(arg1/4, arg1/4 + arg0)), 
                "print_u32": (arg: number) => console.log(arg), 
                "print_f32": (arg: number) => console.log(arg), 
                "print_str": (ptr: number, len: number) => printStr(ptr, len), 
                "console_clear": () => console.clear(), 
                "cursor_inside": () => mouseInside,
                "get_keystate_buffer": () => getKeystateBuffer(),
                "get_buttonstate_buffer": () => getButtonstateBuffer(),
                "get_screen_buffer": () => getScreenBuffer(),
                "memory": memory
            }
        };

        let module = await WebAssembly.compile(await (await fetch("./script/main.wasm")).arrayBuffer());

        module_instance = await WebAssembly.instantiate(module, wasm_imports);
        malloc = module_instance.exports["malloc"] as (size: number) => number;
        zalloc = module_instance.exports["zalloc"] as (size: number) => number;
        free = module_instance.exports["free"] as (size: number) => number;

        {
            const len = 512;
            const ptr = zalloc(len);

            keystatePtr = ptr;
            keystateLen = len;
        }

        {
            const len = 8;
            const ptr = zalloc(len);

            buttonstatePtr = ptr;
            buttonstateLen = len;
        }

        {
            const len = canvas.width*canvas.height*4;
            const ptr = zalloc(len);

            screenPtr = ptr;
            screenLen = len;
        }

        let export_main: WebAssembly.ExportValue = module_instance.exports["entry"];
        (export_main as any)(canvas.width, canvas.height);

        window.requestAnimationFrame(update)
    }

    window.onbeforeunload = (e)=>{
        e = e || window.event;

        if(e)
        {
            free(screenPtr);
            free(keystatePtr);
        }
    };

}