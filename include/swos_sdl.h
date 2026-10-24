#ifndef SWOS_SDL_H
#define SWOS_SDL_H
/* SDL2 reference or native SDL3 boundary. No SDL2 runtime is used by SDL3. */
#ifndef SWOS_SDL3_BUILD
#include <SDL.h>
#define SWOS_KEY_SYM(e) ((e).keysym.sym)
#define SWOS_KEY_SCAN(e) ((e).keysym.scancode)
#define SWOS_KEY_MOD(e) ((e).keysym.mod)
#define SWOS_KEY_DOWN(e) ((e).state)
#define SWOS_WINDOW_CASES case SDL_WINDOWEVENT:
#define SWOS_WINDOW_KIND(e) ((e).window.event)
#define SWOS_WINDOW_FIRST SDL_WINDOWEVENT
#define SWOS_WINDOW_LAST SDL_WINDOWEVENT
#define SWOS_BPP(s) ((s)->format->BytesPerPixel)
#define SWOS_PALETTE(s) ((s)->format->palette)
#else
#ifndef SDL_ENABLE_OLD_NAMES
#define SDL_ENABLE_OLD_NAMES
#endif
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
/* Preserve the existing call sites' error convention (0 succeeds). Only the
 * APIs actually used by this game are adapted; this is not SDL2 emulation. */
#define SDL_INIT_TIMER 0u
#define SDL_WINDOW_SHOWN 0u
#define SDL_WINDOW_FULLSCREEN_DESKTOP SDL_WINDOW_FULLSCREEN
#define SDL_RENDERER_SOFTWARE 1u
#define SDL_RENDERER_ACCELERATED 2u
#define SDL_RENDERER_PRESENTVSYNC 4u
#define SDL_RENDERER_TARGETTEXTURE 8u
#define SDL_PRESSED 1
#define SDL_RELEASED 0
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define SDL_HINT_RENDER_SCALE_QUALITY "SWOS_LEGACY_SCALE_QUALITY"
#define SWOS_KEY_SYM(e) ((e).key)
#define SWOS_KEY_SCAN(e) ((e).scancode)
#define SWOS_KEY_MOD(e) ((e).mod)
#define SWOS_KEY_DOWN(e) ((e).down)
#define SWOS_WINDOW_CASES                                                      \
  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:                                       \
  case SDL_EVENT_WINDOW_FOCUS_LOST:                                            \
  case SDL_EVENT_WINDOW_FOCUS_GAINED:
#define SWOS_WINDOW_KIND(e) ((e).type)
#define SDL_WINDOWEVENT_CLOSE SDL_EVENT_WINDOW_CLOSE_REQUESTED
#define SDL_WINDOWEVENT_FOCUS_LOST SDL_EVENT_WINDOW_FOCUS_LOST
#define SDL_WINDOWEVENT_FOCUS_GAINED SDL_EVENT_WINDOW_FOCUS_GAINED
#define SWOS_WINDOW_FIRST SDL_EVENT_WINDOW_FIRST
#define SWOS_WINDOW_LAST SDL_EVENT_WINDOW_LAST
#define SWOS_BPP(s) SDL_BYTESPERPIXEL((s)->format)
#define SWOS_PALETTE(s) SDL_GetSurfacePalette(s)
/* SDL3 moved controller event members to g* (device IDs are mapped below). */
#define cbutton gbutton
#define caxis gaxis
#define cdevice gdevice

typedef struct {
  int w, h, refresh_rate;
} SwosDisplayMode;
typedef struct {
  const char *name;
  Uint32 flags;
} SwosRendererInfo;
typedef struct {
  int freq;
  SDL_AudioFormat format;
  Uint8 channels;
  Uint16 samples;
  void *callback, *userdata;
} SwosAudioSpec;
typedef struct {
  int needed, len, len_mult, len_cvt;
  Uint8 *buf;
  SDL_AudioSpec src, dst;
} SwosAudioCVT;
int SwosSdlInit(Uint32 flags);
int SwosSdlInitSubSystem(Uint32 flags);
SDL_DisplayID SwosSdlDisplayId(int index);
int SwosSdlNumDisplays(void);
int SwosSdlDisplayBounds(int index, SDL_Rect *r);
int SwosSdlDisplayUsableBounds(int index, SDL_Rect *r);
int SwosSdlCurrentDisplayMode(int index, SwosDisplayMode *m);
int SwosSdlDesktopDisplayMode(int index, SwosDisplayMode *m);
int SwosSdlWindowDisplayIndex(SDL_Window *w);
SDL_Window *SwosSdlCreateWindow(const char *title, int x, int y, int w, int h,
                                Uint32 flags);
SDL_Renderer *SwosSdlCreateRenderer(SDL_Window *w, int index, Uint32 flags);
int SwosSdlRendererInfo(SDL_Renderer *r, SwosRendererInfo *i);
int SwosSdlRenderCopy(SDL_Renderer *, SDL_Texture *, const SDL_Rect *,
                      const SDL_Rect *);
int SwosSdlRenderCopyF(SDL_Renderer *, SDL_Texture *, const SDL_Rect *,
                       const SDL_FRect *);
int SwosSdlReadPixels(SDL_Renderer *, const SDL_Rect *, Uint32, void *, int);
int SwosSdlPollEvent(SDL_Event *);
int SwosSdlNumJoysticks(void);
SDL_JoystickID SwosSdlJoystickId(int index);
SDL_Gamepad *SwosSdlOpenGamepad(int index);
SDL_Joystick *SwosSdlOpenJoystick(int index);
const char *SwosSdlJoystickNameForIndex(int index);
bool SwosSdlIsGamepad(int index);
int SwosSdlAttachVirtual(SDL_JoystickType, int, int, int);
SDL_AudioDeviceID SwosSdlOpenAudio(const char *, int, const SwosAudioSpec *,
                                   SwosAudioSpec *, int);
void SwosSdlPauseAudio(SDL_AudioDeviceID, int);
void SwosSdlCloseAudio(SDL_AudioDeviceID);
int SwosSdlQueueAudio(SDL_AudioDeviceID, const void *, Uint32);
Uint32 SwosSdlQueuedAudio(SDL_AudioDeviceID);
void SwosSdlClearAudio(SDL_AudioDeviceID);
SwosAudioSpec *SwosSdlLoadWav(const char *, SwosAudioSpec *, Uint8 **,
                              Uint32 *);
int SwosSdlBuildAudioCvt(SwosAudioCVT *, SDL_AudioFormat, Uint8, int,
                         SDL_AudioFormat, Uint8, int);
int SwosSdlConvertAudio(SwosAudioCVT *);
static inline int SwosSdlLogicalSize(SDL_Renderer *r, int w, int h) {
  return SDL_SetRenderLogicalPresentation(
             r, w, h,
             w && h ? SDL_LOGICAL_PRESENTATION_LETTERBOX
                    : SDL_LOGICAL_PRESENTATION_DISABLED)
             ? 0
             : -1;
}
static inline void SwosSdlGetLogicalSize(SDL_Renderer *r, int *w, int *h) {
  SDL_RendererLogicalPresentation m;
  SDL_GetRenderLogicalPresentation(r, w, h, &m);
}
static inline int SwosSdlTryLock(SDL_Mutex *m) {
  return SDL_TryLockMutex(m) ? 0 : 1;
}
static inline SDL_Scancode SwosSdlScancode(SDL_Keycode k) {
  return SDL_GetScancodeFromKey(k, NULL);
}
static inline int SwosSdlShowCursor(int n) {
  bool old = SDL_CursorVisible();
  if (n >= 0) {
    if (n)
      SDL_ShowCursor();
    else
      SDL_HideCursor();
  }
  return old ? 1 : 0;
}
static inline Uint8 SwosSdlEventState(Uint32 e, int n) {
  bool old = SDL_EventEnabled(e);
  if (n >= 0)
    SDL_SetEventEnabled(e, n != 0);
  return old ? 1 : 0;
}
static inline SDL_Surface *SwosSdlSurfaceFrom(void *p, int w, int h, int depth,
                                              int pitch, Uint32 format) {
  (void)depth;
  return SDL_CreateSurfaceFrom(w, h, format, p, pitch);
}
char *SwosSdlBasePath(void);
SDL_Texture *SwosSdlCreateTexture(SDL_Renderer *, SDL_PixelFormat,
                                  SDL_TextureAccess, int, int);
static inline int SwosSdlLockTexture(SDL_Texture *t, const SDL_Rect *r,
                                     void **p, int *pitch) {
  return SDL_LockTexture(t, r, p, pitch) ? 0 : -1;
}

#ifdef __AROS__
int SwosSdlSetEnv(const char *n, const char *v, int overwrite);
#else
static inline int SwosSdlSetEnv(const char *n, const char *v, int overwrite) {
  int rc = SDL_setenv_unsafe(n, v, overwrite);
  if (rc == 0 &&
      !SDL_SetEnvironmentVariable(SDL_GetEnvironment(), n, v, overwrite != 0))
    return -1;
  return rc;
}
#endif
static inline int SwosSdlOutputSize(SDL_Renderer *r, int *w, int *h) {
  SDL_Texture *t = SDL_GetRenderTarget(r);
  if (t) {
    float tw, th;
    if (!SDL_GetTextureSize(t, &tw, &th))
      return -1;
    if (w)
      *w = (int)tw;
    if (h)
      *h = (int)th;
    return 0;
  }
  return SDL_GetRenderOutputSize(r, w, h) ? 0 : -1;
}
static inline int SwosSdlSetTarget(SDL_Renderer *r, SDL_Texture *t) {
  return SDL_SetRenderTarget(r, t) ? 0 : -1;
}
static inline int SwosSdlVirtualButton(SDL_Joystick *j, int n, Uint8 v) {
  return SDL_SetJoystickVirtualButton(j, n, v != 0) ? 0 : -1;
}

#ifndef SWOS_SDL3_IMPLEMENTATION
#undef SDL_CreateTexture
#define SDL_CreateTexture SwosSdlCreateTexture
#undef SDL_LockTexture
#define SDL_LockTexture SwosSdlLockTexture
#undef SDL_GetBasePath
#define SDL_GetBasePath SwosSdlBasePath
#undef SDL_setenv
#define SDL_setenv SwosSdlSetEnv
#undef SDL_GetRendererOutputSize
#define SDL_GetRendererOutputSize SwosSdlOutputSize
#undef SDL_SetRenderTarget
#define SDL_SetRenderTarget SwosSdlSetTarget
#undef SDL_JoystickSetVirtualButton
#define SDL_JoystickSetVirtualButton SwosSdlVirtualButton
#undef SDL_AudioSpec
#define SDL_AudioSpec SwosAudioSpec
#undef SDL_AudioCVT
#define SDL_AudioCVT SwosAudioCVT
#undef SDL_DisplayMode
#define SDL_DisplayMode SwosDisplayMode
#undef SDL_RendererInfo
#define SDL_RendererInfo SwosRendererInfo
#undef SDL_Init
#define SDL_Init SwosSdlInit
#undef SDL_InitSubSystem
#define SDL_InitSubSystem SwosSdlInitSubSystem
#undef SDL_GetNumVideoDisplays
#define SDL_GetNumVideoDisplays SwosSdlNumDisplays
#undef SDL_GetDisplayBounds
#define SDL_GetDisplayBounds SwosSdlDisplayBounds
#undef SDL_GetDisplayUsableBounds
#define SDL_GetDisplayUsableBounds SwosSdlDisplayUsableBounds
#undef SDL_GetCurrentDisplayMode
#define SDL_GetCurrentDisplayMode SwosSdlCurrentDisplayMode
#undef SDL_GetDesktopDisplayMode
#define SDL_GetDesktopDisplayMode SwosSdlDesktopDisplayMode
#undef SDL_GetWindowDisplayIndex
#define SDL_GetWindowDisplayIndex SwosSdlWindowDisplayIndex
#undef SDL_CreateWindow
#define SDL_CreateWindow SwosSdlCreateWindow
#undef SDL_CreateRenderer
#define SDL_CreateRenderer SwosSdlCreateRenderer
#undef SDL_GetRendererInfo
#define SDL_GetRendererInfo SwosSdlRendererInfo
#undef SDL_RenderCopy
#define SDL_RenderCopy SwosSdlRenderCopy
#undef SDL_RenderCopyF
#define SDL_RenderCopyF SwosSdlRenderCopyF
#undef SDL_RenderReadPixels
#define SDL_RenderReadPixels SwosSdlReadPixels
#undef SDL_RenderSetLogicalSize
#define SDL_RenderSetLogicalSize SwosSdlLogicalSize
#undef SDL_RenderGetLogicalSize
#define SDL_RenderGetLogicalSize SwosSdlGetLogicalSize
#undef SDL_TryLockMutex
#define SDL_TryLockMutex SwosSdlTryLock
#undef SDL_GetScancodeFromKey
#define SDL_GetScancodeFromKey SwosSdlScancode
#undef SDL_ShowCursor
#define SDL_ShowCursor SwosSdlShowCursor
#undef SDL_EventState
#define SDL_EventState SwosSdlEventState
#undef SDL_CreateRGBSurfaceWithFormatFrom
#define SDL_CreateRGBSurfaceWithFormatFrom SwosSdlSurfaceFrom
#undef SDL_PollEvent
#define SDL_PollEvent SwosSdlPollEvent
#undef SDL_NumJoysticks
#define SDL_NumJoysticks SwosSdlNumJoysticks
#undef SDL_JoystickGetDeviceInstanceID
#define SDL_JoystickGetDeviceInstanceID SwosSdlJoystickId
#undef SDL_GameControllerOpen
#define SDL_GameControllerOpen SwosSdlOpenGamepad
#undef SDL_JoystickOpen
#define SDL_JoystickOpen SwosSdlOpenJoystick
#undef SDL_JoystickNameForIndex
#define SDL_JoystickNameForIndex SwosSdlJoystickNameForIndex
#undef SDL_IsGameController
#define SDL_IsGameController SwosSdlIsGamepad
#undef SDL_JoystickAttachVirtual
#define SDL_JoystickAttachVirtual SwosSdlAttachVirtual
#undef SDL_OpenAudioDevice
#define SDL_OpenAudioDevice SwosSdlOpenAudio
#undef SDL_PauseAudioDevice
#define SDL_PauseAudioDevice SwosSdlPauseAudio
#undef SDL_CloseAudioDevice
#define SDL_CloseAudioDevice SwosSdlCloseAudio
#undef SDL_QueueAudio
#define SDL_QueueAudio SwosSdlQueueAudio
#undef SDL_GetQueuedAudioSize
#define SDL_GetQueuedAudioSize SwosSdlQueuedAudio
#undef SDL_ClearQueuedAudio
#define SDL_ClearQueuedAudio SwosSdlClearAudio
#undef SDL_LoadWAV
#define SDL_LoadWAV SwosSdlLoadWav
#undef SDL_FreeWAV
#define SDL_FreeWAV SDL_free
#undef SDL_BuildAudioCVT
#define SDL_BuildAudioCVT SwosSdlBuildAudioCvt
#undef SDL_ConvertAudio
#define SDL_ConvertAudio SwosSdlConvertAudio
#undef SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE 1
#undef SDL_AUDIO_ALLOW_SAMPLES_CHANGE
#define SDL_AUDIO_ALLOW_SAMPLES_CHANGE 2
#endif /* call-site names */
#endif
#endif
