#!/usr/bin/env python3
"""Compile production guard functions against deterministic boundary doubles."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def function(path, signature):
    text = path.read_text()
    start = text.index(signature)
    body = text.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


p = argparse.ArgumentParser(description=__doc__)
p.add_argument('source', type=Path)
a = p.parse_args()
preamble = r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdlib.h>
#define SDL_MESSAGEBOX_ERROR 2
static jmp_buf fatal_return;
static int exit_status;
static void test_exit(int status) { exit_status=status; longjmp(fatal_return,1); }
#define exit test_exit
typedef unsigned Uint32;
#define SDL_WINDOW_FULLSCREEN 1
#define SDL_WINDOW_FULLSCREEN_DESKTOP 3
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define SDL_MESSAGEBOX_INFORMATION 1
static void aros_toggle_accel_fullscreen(void) {}
static int window, *s_window=&window, s_renderer_is_hw;
static int s_pinned_w=1008, s_pinned_h=816, _screen_width=336, _screen_height=272;
static int s_win_scale_current=3, s_fullscreen_mode;
#define PLATFORM_FULLSCREEN_EXCLUSIVE 1
static unsigned flags;
static int mutations, messages;
unsigned SDL_GetWindowFlags(void *w) { return flags; }
int SDL_SetWindowFullscreen(void *w,unsigned f) { mutations++; flags=f; return 0; }
void SDL_SetWindowBordered(void *w,bool f) { mutations++; }
void SDL_SetWindowSize(void *w,int x,int y) { mutations++; }
void SDL_ShowCursor(int f) { mutations++; }
int SDL_ShowSimpleMessageBox(unsigned f,const char*t,const char*m,void*w) { messages++; return 0; }
static const char *present[16];
int swos_data_exists(const char *p) {
    for (int i=0; present[i]; i++) if (!strcmp(present[i],p)) return 1;
    return 0;
}
'''
checks = r'''
int main(void) {
    /* The CPU path still enters and leaves fullscreen. */
    s_renderer_is_hw=0; flags=0;
    Platform_ToggleFullscreen();
    assert(mutations>0 && flags==SDL_WINDOW_FULLSCREEN_DESKTOP);
    Platform_ToggleFullscreen();
    assert(flags==0);
    /* Fresh, incomplete, shared OCS and mixed AGA installs. */
    assert(AssetsCheck_MissingRequired()!=NULL);
    present[0]="grafs/menus.raw";
    assert(AssetsCheck_MissingRequired()!=NULL);
    present[1]="grafs/menus2.raw"; present[2]="grafs/menubg.raw";
    /* Partial bootstrap: the old first-menu-only check missed this. */
    assert(AssetsCheck_MissingRequired()!=NULL);
    const char *banks[]={"cjcgrafs.raw","cjcbench.raw","cjcbits.raw",
        "cjcteam1.raw","cjcteam2.raw","cjcteam3.raw","cjcteamg.raw","soccer_s.raw"};
    static char paths[8][64];
    for (int i=0;i<8;i++) {
        snprintf(paths[i],sizeof paths[i],"grafs/%s",banks[i]);
        present[i+3]=paths[i];
    }
    assert(AssetsCheck_MissingRequired()==NULL);
    present[0]="grafs_aga/menus.raw";
    present[2]="grafs_aga/menubg.raw";
    assert(AssetsCheck_MissingRequired()==NULL);
    if (!setjmp(fatal_return)) {
        AssetsCheck_FatalMissing("grafs/cjcbits.raw");
        assert(!"fatal missing bank returned");
    }
    assert(exit_status==EXIT_FAILURE);
    puts("PASS: CPU roundtrip; absent/incomplete/OCS/mixed data");
}
'''
try:
    code = preamble + '\n' + function(a.source/'src/platform_sdl.c', 'void Platform_ToggleFullscreen(void)')
    code += '\n' + function(a.source/'src/assets_check.c', 'const char *AssetsCheck_MissingRequired(void)')
    code += '\n' + function(a.source/'src/assets_check.c', 'void AssetsCheck_FatalMissing(const char *filename)')
except ValueError as e:
    raise SystemExit('FAIL: production startup guard absent: ' + str(e))
with tempfile.TemporaryDirectory(prefix='swos-startup-') as tmp:
    c = Path(tmp)/'test.c'
    c.write_text(code+'\n'+checks)
    binary=Path(tmp)/'test'
    subprocess.run(['cc','-D__AROS__','-DSWOS_SDL3_BUILD',str(c),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,timeout=5)
