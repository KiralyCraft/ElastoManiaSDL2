#pragma once
// Local validation ABI declarations for the installed SDL 2.32 runtime.
// Normal builds use the official SDL2 development headers instead.
#include <stdint.h>
typedef uint8_t Uint8; typedef uint16_t Uint16; typedef uint32_t Uint32; typedef uint64_t Uint64;
typedef int SDL_bool; typedef int SDL_Scancode; typedef Uint32 SDL_AudioDeviceID; typedef Uint16 SDL_AudioFormat;
typedef struct SDL_Window SDL_Window; typedef struct SDL_Renderer SDL_Renderer; typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Rect {int x,y,w,h;} SDL_Rect;
typedef struct SDL_version {Uint8 major,minor,patch;} SDL_version;
typedef union SDL_Event {Uint32 type; Uint8 padding[56]; void* alignment;} SDL_Event;
typedef struct SDL_AudioSpec {int freq; SDL_AudioFormat format; Uint8 channels,silence; Uint16 samples,padding; Uint32 size; void (*callback)(void*,Uint8*,int); void* userdata;} SDL_AudioSpec;
#define SDL_INIT_TIMER 0x1u
#define SDL_INIT_AUDIO 0x10u
#define SDL_INIT_VIDEO 0x20u
#define SDL_INIT_EVENTS 0x4000u
#define SDL_WINDOWPOS_CENTERED 0x2fff0000u
#define SDL_WINDOW_SHOWN 0x4u
#define SDL_WINDOW_RESIZABLE 0x20u
#define SDL_WINDOW_INPUT_FOCUS 0x200u
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x1001u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x2000u
#define SDL_RENDERER_SOFTWARE 1u
#define SDL_RENDERER_ACCELERATED 2u
#define SDL_RENDERER_PRESENTVSYNC 4u
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_PIXELFORMAT_ARGB8888 0x16362004u
#define SDL_QUIT 0x100u
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_MESSAGEBOX_ERROR 0x10u
#define SDL_MESSAGEBOX_WARNING 0x20u
#define AUDIO_S16SYS 0x8010u
#define SDL_FALSE 0
#define SDL_TRUE 1
extern "C" {
void SDL_SetMainReady(void);
int SDL_Init(Uint32); int SDL_InitSubSystem(Uint32); void SDL_Quit(void); const char* SDL_GetError(void);
SDL_bool SDL_SetHint(const char*, const char*); const char* SDL_GetPlatform(void); void SDL_GetVersion(SDL_version*);
SDL_Window* SDL_CreateWindow(const char*,int,int,int,int,Uint32); void SDL_DestroyWindow(SDL_Window*);
SDL_Renderer* SDL_CreateRenderer(SDL_Window*,int,Uint32); void SDL_DestroyRenderer(SDL_Renderer*);
SDL_Texture* SDL_CreateTexture(SDL_Renderer*,Uint32,int,int,int); void SDL_DestroyTexture(SDL_Texture*);
int SDL_RenderSetLogicalSize(SDL_Renderer*,int,int); int SDL_RenderSetIntegerScale(SDL_Renderer*,SDL_bool);
int SDL_UpdateTexture(SDL_Texture*,const SDL_Rect*,const void*,int);
int SDL_RenderClear(SDL_Renderer*); int SDL_RenderCopy(SDL_Renderer*,SDL_Texture*,const SDL_Rect*,const SDL_Rect*); void SDL_RenderPresent(SDL_Renderer*);
int SDL_RenderReadPixels(SDL_Renderer*,const SDL_Rect*,Uint32,void*,int); int SDL_GetRendererOutputSize(SDL_Renderer*,int*,int*);
void SDL_RenderWindowToLogical(SDL_Renderer*,int,int,float*,float*); void SDL_RenderLogicalToWindow(SDL_Renderer*,float,float,int*,int*);
int SDL_PollEvent(SDL_Event*); const Uint8* SDL_GetKeyboardState(int*); const char* SDL_GetScancodeName(SDL_Scancode);
Uint32 SDL_GetMouseState(int*,int*); void SDL_WarpMouseInWindow(SDL_Window*,int,int); int SDL_ShowCursor(int);
int SDL_SetWindowFullscreen(SDL_Window*,Uint32); Uint32 SDL_GetWindowFlags(SDL_Window*); void SDL_SetWindowTitle(SDL_Window*,const char*);
Uint32 SDL_GetTicks(void); Uint64 SDL_GetTicks64(void); Uint64 SDL_GetPerformanceCounter(void); Uint64 SDL_GetPerformanceFrequency(void); void SDL_Delay(Uint32);
char* SDL_GetPrefPath(const char*,const char*); char* SDL_GetBasePath(void); void SDL_free(void*);
int SDL_ShowSimpleMessageBox(Uint32,const char*,const char*,SDL_Window*);
SDL_AudioDeviceID SDL_OpenAudioDevice(const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int);
void SDL_PauseAudioDevice(SDL_AudioDeviceID,int); int SDL_QueueAudio(SDL_AudioDeviceID,const void*,Uint32); Uint32 SDL_GetQueuedAudioSize(SDL_AudioDeviceID); void SDL_ClearQueuedAudio(SDL_AudioDeviceID); void SDL_CloseAudioDevice(SDL_AudioDeviceID);
}
