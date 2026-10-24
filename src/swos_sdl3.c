/* Native SDL3 implementation of the game's narrow SDL platform boundary. */
#ifdef SWOS_SDL3_BUILD
#define SWOS_SDL3_IMPLEMENTATION
#include "swos_sdl.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifdef __AROS__
#include <stdlib.h>
#include <proto/dos.h>
#include <dos/var.h>
/* SDL's AROS backend has a private environment. The engine also uses C getenv,
 * which reads AmigaDOS variables; update both at this compatibility boundary. */
int SwosSdlSetEnv(const char *name, const char *value, int overwrite) {
  if (!name || !*name || SDL_strchr(name, '=') || !value)
    return -1;
  const char *effective = value;
  if (!overwrite) {
    const char *existing = getenv(name);
    if (existing)
      effective = existing;
  }
  /* getenv storage is reused by the CRT. Keep the selected value stable while
   * SDL creates its environment and allocates the new entry. */
  char *copy = SDL_strdup(effective);
  if (!copy)
    return -1;
  int rc = -1;
  if (SetVar(name, copy, -1, LV_VAR | GVF_LOCAL_ONLY)) {
    rc = SDL_setenv_unsafe(name, copy, 1);
    if (rc == 0 &&
        !SDL_SetEnvironmentVariable(SDL_GetEnvironment(), name, copy, true))
      rc = -1;
  }
  SDL_free(copy);
  return rc;
}
#endif
char *SwosSdlBasePath(void) {
  const char *p = SDL_GetBasePath();
  return p ? SDL_strdup(p) : NULL;
}
SDL_Texture *SwosSdlCreateTexture(SDL_Renderer *r, SDL_PixelFormat f,
                                  SDL_TextureAccess a, int w, int h) {
  SDL_Texture *t = SDL_CreateTexture(r, f, a, w, h);
  if (t)
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
  return t;
}
int SwosSdlInit(Uint32 f) { return SDL_Init(f) ? 0 : -1; }
int SwosSdlInitSubSystem(Uint32 f) { return SDL_InitSubSystem(f) ? 0 : -1; }
SDL_DisplayID SwosSdlDisplayId(int index) {
  int n = 0;
  SDL_DisplayID *ids = SDL_GetDisplays(&n);
  SDL_DisplayID id = ids && index >= 0 && index < n ? ids[index] : 0;
  SDL_free(ids);
  return id;
}
int SwosSdlNumDisplays(void) {
  int n = 0;
  SDL_DisplayID *ids = SDL_GetDisplays(&n);
  if (!ids)
    return -1;
  SDL_free(ids);
  return n;
}
int SwosSdlDisplayBounds(int i, SDL_Rect *r) {
  return SDL_GetDisplayBounds(SwosSdlDisplayId(i), r) ? 0 : -1;
}
int SwosSdlDisplayUsableBounds(int i, SDL_Rect *r) {
  return SDL_GetDisplayUsableBounds(SwosSdlDisplayId(i), r) ? 0 : -1;
}
static int copy_mode(const SDL_DisplayMode *s, SwosDisplayMode *d) {
  if (!s || !d)
    return -1;
  d->w = s->w;
  d->h = s->h;
  d->refresh_rate = (int)(s->refresh_rate + 0.5f);
  return 0;
}
int SwosSdlCurrentDisplayMode(int i, SwosDisplayMode *m) {
  return copy_mode(SDL_GetCurrentDisplayMode(SwosSdlDisplayId(i)), m);
}
int SwosSdlDesktopDisplayMode(int i, SwosDisplayMode *m) {
  return copy_mode(SDL_GetDesktopDisplayMode(SwosSdlDisplayId(i)), m);
}
int SwosSdlWindowDisplayIndex(SDL_Window *w) {
  SDL_DisplayID id = SDL_GetDisplayForWindow(w);
  int n = 0, found = -1;
  SDL_DisplayID *ids = SDL_GetDisplays(&n);
  for (int i = 0; ids && i < n; ++i)
    if (ids[i] == id) {
      found = i;
      break;
    }
  SDL_free(ids);
  return found;
}
SDL_Window *SwosSdlCreateWindow(const char *t, int x, int y, int w, int h,
                                Uint32 flags) {
  SDL_Window *win = SDL_CreateWindow(t, w, h, flags);
  if (win)
    SDL_SetWindowPosition(win, x, y);
  return win;
}
SDL_Renderer *SwosSdlCreateRenderer(SDL_Window *w, int index, Uint32 flags) {
  (void)index;
  const char *name = flags & SDL_RENDERER_SOFTWARE
                         ? "software"
                         : SDL_GetHint(SDL_HINT_RENDER_DRIVER);
  SDL_Renderer *r = SDL_CreateRenderer(w, name);
  if (r && (flags & SDL_RENDERER_PRESENTVSYNC))
    SDL_SetRenderVSync(r, 1);
  return r;
}
int SwosSdlRendererInfo(SDL_Renderer *r, SwosRendererInfo *i) {
  if (!r || !i)
    return -1;
  i->name = SDL_GetRendererName(r);
  int v = 0;
  SDL_GetRenderVSync(r, &v);
  i->flags =
      SDL_RENDERER_TARGETTEXTURE | (v ? SDL_RENDERER_PRESENTVSYNC : 0) |
      (i->name && !SDL_strcmp(i->name, "software") ? SDL_RENDERER_SOFTWARE
                                                   : SDL_RENDERER_ACCELERATED);
  return 0;
}
static SDL_FRect *frect(const SDL_Rect *r, SDL_FRect *f) {
  if (!r)
    return NULL;
  *f = (SDL_FRect){r->x, r->y, r->w, r->h};
  return f;
}
int SwosSdlRenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s,
                      const SDL_Rect *d) {
  SDL_FRect sf, df;
  return SDL_RenderTexture(r, t, frect(s, &sf), frect(d, &df)) ? 0 : -1;
}
int SwosSdlRenderCopyF(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s,
                       const SDL_FRect *d) {
  SDL_FRect sf;
  return SDL_RenderTexture(r, t, frect(s, &sf), d) ? 0 : -1;
}
int SwosSdlReadPixels(SDL_Renderer *r, const SDL_Rect *rect, Uint32 format,
                      void *pixels, int pitch) {
  SDL_Surface *s = SDL_RenderReadPixels(r, rect);
  if (!s)
    return -1;
  bool ok = SDL_ConvertPixels(s->w, s->h, s->format, s->pixels, s->pitch,
                              format, pixels, pitch);
  SDL_DestroySurface(s);
  return ok ? 0 : -1;
}
SDL_JoystickID SwosSdlJoystickId(int index) {
  int n = 0;
  SDL_JoystickID *ids = SDL_GetJoysticks(&n);
  SDL_JoystickID id = ids && index >= 0 && index < n ? ids[index] : 0;
  SDL_free(ids);
  return id;
}
static int joystick_index(SDL_JoystickID id) {
  int n = 0, found = -1;
  SDL_JoystickID *ids = SDL_GetJoysticks(&n);
  for (int i = 0; ids && i < n; ++i)
    if (ids[i] == id) {
      found = i;
      break;
    }
  SDL_free(ids);
  return found;
}
int SwosSdlNumJoysticks(void) {
  int n = 0;
  SDL_JoystickID *ids = SDL_GetJoysticks(&n);
  if (!ids)
    return -1;
  SDL_free(ids);
  return n;
}
SDL_Gamepad *SwosSdlOpenGamepad(int i) {
  return SDL_OpenGamepad(SwosSdlJoystickId(i));
}
SDL_Joystick *SwosSdlOpenJoystick(int i) {
  return SDL_OpenJoystick(SwosSdlJoystickId(i));
}
const char *SwosSdlJoystickNameForIndex(int i) {
  return SDL_GetJoystickNameForID(SwosSdlJoystickId(i));
}
bool SwosSdlIsGamepad(int i) { return SDL_IsGamepad(SwosSdlJoystickId(i)); }
int SwosSdlAttachVirtual(SDL_JoystickType t, int axes, int buttons, int hats) {
  SDL_VirtualJoystickDesc d;
  SDL_INIT_INTERFACE(&d);
  d.type = t;
  d.naxes = axes;
  d.nbuttons = buttons;
  d.nhats = hats;
  SDL_JoystickID id = SDL_AttachVirtualJoystick(&d);
  return id ? joystick_index(id) : -1;
}
int SwosSdlPollEvent(SDL_Event *e) {
  if (!SDL_PollEvent(e))
    return 0;
  if (e && (e->type == SDL_EVENT_JOYSTICK_ADDED ||
            e->type == SDL_EVENT_GAMEPAD_ADDED))
    e->jdevice.which = joystick_index(e->jdevice.which);
  return 1;
}
/* One output device is owned by audio_sdl.c. Source-format queued bytes stay
 * in the stream; SDL3 performs device conversion after the existing mixer. */
static SDL_AudioStream *output;
SDL_AudioDeviceID SwosSdlOpenAudio(const char *name, int capture,
                                   const SwosAudioSpec *want,
                                   SwosAudioSpec *got, int flags) {
  (void)flags;
  if (name || capture || output) {
    SDL_SetError("SWOS supports one default playback stream");
    return 0;
  }
  SDL_AudioSpec spec = {want->format, want->channels, want->freq};
  output = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                     NULL, NULL);
  if (!output)
    return 0;
  SDL_AudioSpec physical;
  int frames = 0;
  SDL_AudioDeviceID device = SDL_GetAudioStreamDevice(output);
  if (SDL_GetAudioDeviceFormat(device, &physical, &frames)) {
    const char *hint = SDL_GetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES);
    fprintf(stderr, "[audio] driver=%s source=%dHz/%dch device=%dHz/%dch "
                    "frames=%d (%.2fms) sample-frames-hint=%s\n",
            SDL_GetCurrentAudioDriver(), spec.freq, spec.channels,
            physical.freq, physical.channels, frames,
            physical.freq > 0 ? frames * 1000.0 / physical.freq : 0.0,
            hint && *hint ? hint : "default");
  } else {
    fprintf(stderr, "[audio] device format query failed: %s\n", SDL_GetError());
  }
  if (got)
    *got = *want;
  return SDL_GetAudioStreamDevice(output);
}
void SwosSdlPauseAudio(SDL_AudioDeviceID id, int pause) {
  (void)id;
  if (output) {
    if (pause)
      SDL_PauseAudioStreamDevice(output);
    else
      SDL_ResumeAudioStreamDevice(output);
  }
}
void SwosSdlCloseAudio(SDL_AudioDeviceID id) {
  (void)id;
  SDL_DestroyAudioStream(output);
  output = NULL;
}
int SwosSdlQueueAudio(SDL_AudioDeviceID id, const void *p, Uint32 n) {
  (void)id;
  return output && n <= INT_MAX && SDL_PutAudioStreamData(output, p, (int)n)
             ? 0
             : -1;
}
Uint32 SwosSdlQueuedAudio(SDL_AudioDeviceID id) {
  (void)id;
  int n = output ? SDL_GetAudioStreamQueued(output) : 0;
  return n > 0 ? (Uint32)n : 0;
}
void SwosSdlClearAudio(SDL_AudioDeviceID id) {
  (void)id;
  if (output)
    SDL_ClearAudioStream(output);
}
SwosAudioSpec *SwosSdlLoadWav(const char *p, SwosAudioSpec *s, Uint8 **b,
                              Uint32 *n) {
  SDL_AudioSpec a;
  if (!SDL_LoadWAV(p, &a, b, n))
    return NULL;
  memset(s, 0, sizeof *s);
  s->format = a.format;
  s->channels = a.channels;
  s->freq = a.freq;
  return s;
}
int SwosSdlBuildAudioCvt(SwosAudioCVT *c, SDL_AudioFormat sf, Uint8 sc, int sr,
                         SDL_AudioFormat df, Uint8 dc, int dr) {
  if (!c || !sc || !dc || sr <= 0 || dr <= 0)
    return -1;
  memset(c, 0, sizeof *c);
  c->src = (SDL_AudioSpec){sf, sc, sr};
  c->dst = (SDL_AudioSpec){df, dc, dr};
  c->needed = sf != df || sc != dc || sr != dr;
  c->len_mult = 1;
  return c->needed;
}
int SwosSdlConvertAudio(SwosAudioCVT *c) {
  if (!c || c->len < 0)
    return -1;
  Uint8 *out = NULL;
  int len = 0;
  if (!SDL_ConvertAudioSamples(&c->src, c->buf, c->len, &c->dst, &out, &len))
    return -1;
  SDL_free(c->buf);
  c->buf = out;
  c->len_cvt = len;
  return 0;
}
#endif
