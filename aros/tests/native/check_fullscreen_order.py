#!/usr/bin/env python3
"""Exercise the production AROS reopen decision with SDL3's pre-event flags."""
from pathlib import Path
import argparse, subprocess, tempfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);a=p.parse_args()
s=a.source.read_text();start=s.index('void \nAROS_OpenWindows(');end=s.index('\nstatic bool',start);fn=s[start:end]
pos_start=s.index('bool\nAROS_SetWindowPosition(');pos_end=s.index('\nvoid\nAROS_SetWindowMinimumSize',pos_start);position_fn=s[pos_start:pos_end]
pre=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#define D(x)
#define SDL_WINDOW_EXTERNAL 8
#define SDL_WINDOW_FULLSCREEN 1
#define SDL_AROS_WINDOW_SHOWN 4
#define SDL_AROS_WINDOW_FULLSCREEN 1
#define SDL_AROS_WINDOW_FULLSCREEN_DESKTOP 2
typedef struct { unsigned flags; void *internal; int x,y; } SDL_Window;
typedef struct { int Width,Height; } NativeWindow;
typedef struct { int BarHeight; } NativeScreen;
struct video_data;
typedef struct { SDL_Window *window; unsigned winflags; NativeWindow *win; struct video_data *videodata; } SDL_WindowData;
typedef struct video_data { SDL_WindowData *windowlist; void *CustomScreen; NativeScreen *WScreen; } SDL_VideoData;
typedef struct { void *internal; } SDL_VideoDevice;
#define ForeachNode(list,item) for(item=*(list);item;item=0)
static int reopened,moves;
#define MAX(a,b) ((a)>(b)?(a):(b))
static void ChangeWindowBox(NativeWindow *w,int x,int y,int width,int height){(void)w;(void)x;(void)y;(void)width;(void)height;moves++;}
static void AROS_RecreateWindow(SDL_VideoDevice *d,SDL_Window *w){(void)d;(void)w;reopened++;}
static void AROS_ShowWindow_Internal(SDL_VideoDevice *d,SDL_Window *w){(void)d;(void)w;}
'''
post=r'''
int main(void){
 SDL_Window w={0};SDL_WindowData wd={&w,SDL_AROS_WINDOW_SHOWN|SDL_AROS_WINDOW_FULLSCREEN,(void*)1};
 SDL_VideoData vd={&wd};SDL_VideoDevice dev={&vd};
 AROS_OpenWindows(&dev);
 if(reopened!=1 || w.flags!=0){fprintf(stderr,"FAIL enter: native reopens=%d, logical flags=%u\n",reopened,w.flags);return 1;}
 reopened=0;w.flags=SDL_WINDOW_FULLSCREEN;wd.winflags=SDL_AROS_WINDOW_SHOWN|SDL_AROS_WINDOW_FULLSCREEN_DESKTOP;
 AROS_OpenWindows(&dev);
 if(reopened!=1 || w.flags!=SDL_WINDOW_FULLSCREEN){fprintf(stderr,"FAIL leave: native reopens=%d, logical flags=%u\n",reopened,w.flags);return 1;}
 w.internal=&wd;wd.videodata=&vd;NativeScreen screen={19};vd.WScreen=&screen;
 AROS_SetWindowPosition(&dev,&w);
 if(moves){fprintf(stderr,"FAIL fullscreen restore-position moved native window\n");return 1;}
 puts("PASS: fullscreen native transition precedes SDL3 logical flag events");return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(pre+fn+position_fn+post)
 subprocess.run(['cc','-std=c99',str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
