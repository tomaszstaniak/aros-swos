#!/usr/bin/env python3
"""Compile the real fullscreen lifecycle against native-window/GL ownership doubles."""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);a=p.parse_args()
def function(text,signature):
 start=text.index(signature);end=text.index('{',start)+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
s=(a.source/'src/platform_sdl.c').read_text()
pre=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#define SDL_WINDOW_FULLSCREEN 1
#define SDL_WINDOW_FULLSCREEN_DESKTOP 1
#define SDL_RENDERER_SOFTWARE 1
#define SDL_RENDERER_ACCELERATED 2
#define SDL_RENDERER_PRESENTVSYNC 4
#define SDL_RENDERER_TARGETTEXTURE 8
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define SDL_HINT_FRAMEBUFFER_ACCELERATION "framebuffer"
#define SDL_MESSAGEBOX_INFORMATION 1
#define SDL_MESSAGEBOX_ERROR 2
#define SDL_PIXELFORMAT_ARGB8888 1
#define SDL_TEXTUREACCESS_STREAMING 1
#define PLATFORM_FULLSCREEN_EXCLUSIVE 1
#define PLATFORM_TEX_RING 3
#define PLATFORM_TEX_H 288
typedef unsigned Uint32;
typedef struct {int hw;} SDL_Renderer;
typedef struct {SDL_Renderer *owner;} SDL_Texture;
typedef struct {unsigned flags;const char *name;} SDL_RendererInfo;
static SDL_Renderer renderer_store;
static SDL_Renderer *s_renderer=&renderer_store;
static SDL_Texture textures[4],*s_textures[3],*s_scale_target;
static int s_scale_n=2,s_scale_targets_ok=1,s_tex_idx,s_vsync_enabled=1,s_renderer_is_hw=1;
static int window,*s_window=&window,mutex,*s_render_mtx=&mutex;
static int s_pinned_w=724,s_pinned_h=544,_screen_width=362,_screen_height=272,s_win_scale_current=2,s_fullscreen_mode;
static unsigned flags;static int textures_alive,crt_alive=1,contexts_alive=1,mutations,creates,fail_gl,fail_textures,fail_all,quit,locked;
static int SDL_SetHint(const char*n,const char*v){assert(v[0]=='0');return 1;}
static const char *SDL_GetError(void){return "injected failure";}
static void SDL_LockMutex(void *m){assert(!locked);locked=1;}
static void SDL_UnlockMutex(void *m){assert(locked);locked=0;}
static unsigned SDL_GetWindowFlags(void *w){return flags;}
static int SDL_SetWindowFullscreen(void *w,unsigned f){assert(locked);assert(contexts_alive==0);assert(textures_alive==0 && !crt_alive);mutations++;flags=f;return 1;}
static void SDL_SetWindowBordered(void*w,bool f){assert(contexts_alive==0);mutations++;}
static void SDL_SetWindowSize(void*w,int x,int y){assert(contexts_alive==0);assert(x==724&&y==544);mutations++;}
static void SDL_ShowCursor(int f){}
static int SDL_ShowSimpleMessageBox(unsigned f,const char*t,const char*m,void*w){return 0;}
static void CrtEffects_Shutdown(void){assert(contexts_alive==1);crt_alive=0;}
static void SDL_DestroyTexture(SDL_Texture *t){assert(contexts_alive==1);assert(t->owner==s_renderer);textures_alive--;}
static void SDL_DestroyRenderer(SDL_Renderer *r){assert(textures_alive==0 && !crt_alive);assert(contexts_alive==1);contexts_alive=0;}
static SDL_Renderer *SDL_CreateRenderer(void*w,int i,unsigned f){assert(contexts_alive==0);creates++;if(fail_all || ((f&SDL_RENDERER_ACCELERATED)&&fail_gl)){fail_gl=0;return 0;}renderer_store.hw=!!(f&SDL_RENDERER_ACCELERATED);contexts_alive=1;return &renderer_store;}
static int SDL_GetRendererInfo(SDL_Renderer*r,SDL_RendererInfo*i){i->flags=SDL_RENDERER_TARGETTEXTURE|(r->hw?SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC:SDL_RENDERER_SOFTWARE);i->name=r->hw?"opengl":"software";return 0;}
static SDL_Texture *SDL_CreateTexture(SDL_Renderer*r,int f,int a,int w,int h){assert(contexts_alive==1);if(fail_textures && r->hw){fail_textures=0;return 0;}SDL_Texture*t=&textures[textures_alive++];t->owner=r;return t;}
static void apply_picture_scaling(void){assert(contexts_alive==1);}
static void Platform_RequestQuit(void){quit=1;}
static void seed(void){s_renderer=&renderer_store;renderer_store.hw=1;contexts_alive=1;textures_alive=4;crt_alive=1;for(int i=0;i<3;i++){s_textures[i]=&textures[i];textures[i].owner=s_renderer;}s_scale_target=&textures[3];textures[3].owner=s_renderer;s_renderer_is_hw=1;}
'''
code=pre
for sig in ['static void aros_release_renderer(void)','static int aros_restore_renderer(Uint32 flags)','static void aros_toggle_accel_fullscreen(void)']:
 if sig in s:code+='\n'+function(s,sig)
code+='\n'+function(s,'void Platform_ToggleFullscreen(void)')
code+=r'''
int main(void){
 seed();for(int i=0;i<12;i++){unsigned before=flags;Platform_ToggleFullscreen();assert(flags!=before);assert(contexts_alive==1&&textures_alive==3&&s_renderer&&s_textures[2]);assert(!locked);}
 assert(mutations>0&&!quit);fail_gl=1;Platform_ToggleFullscreen();assert(!s_renderer_is_hw&&s_renderer&&contexts_alive==1&&textures_alive==3);
 seed();fail_textures=1;Platform_ToggleFullscreen();assert(!s_renderer_is_hw&&contexts_alive==1&&textures_alive==3);
 seed();fail_all=1;Platform_ToggleFullscreen();assert(quit&&!s_renderer&&contexts_alive==0&&!locked);
 puts("PASS: 12 GL transitions; destruction precedes native mutation; textures rebuilt; CPU fallback; fatal failure exits cleanly");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(code);subprocess.run(['cc','-D__AROS__','-DSWOS_SDL3_BUILD',str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
