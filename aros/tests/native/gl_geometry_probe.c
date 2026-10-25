/* Bounded read-only AROS SDL3 window/renderer geometry diagnostic. */
#include "SDL_internal.h"
#include "video/SDL_sysvideo.h"
#include "video/aros/SDL_arosvideo.h"
#include <SDL3/SDL_opengl.h>
#include <intuition/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void snapshot(FILE *f, const char *phase, SDL_Window *w, SDL_Renderer *r) {
 int ww=0,hh=0,pw=0,ph=0,rw=0,rh=0,x=0,y=0;
 SDL_GetWindowSize(w,&ww,&hh); SDL_GetWindowSizeInPixels(w,&pw,&ph);SDL_GetWindowPosition(w,&x,&y);
 if(r) SDL_GetRenderOutputSize(r,&rw,&rh);
 SDL_WindowData *d=w->internal;SDL_VideoData *v=d->videodata;
 fprintf(f,"%s flags=%llu window=%dx%d pixels=%dx%d pos=%d,%d render=%dx%d native=%p backend_full=%d mon=%s custom=%p\n",phase,(unsigned long long)w->flags,ww,hh,pw,ph,x,y,rw,rh,(void*)d->win,v->FullScreen,v->ScrMonName?(const char*)v->ScrMonName:"(none)",(void*)v->CustomScreen);
 if(d->win) fprintf(f," native_outer=%dx%d borders=%d,%d,%d,%d screen=%dx%d native_screen=%p\n",d->win->Width,d->win->Height,d->win->BorderLeft,d->win->BorderTop,d->win->BorderRight,d->win->BorderBottom,d->win->WScreen->Width,d->win->WScreen->Height,(void*)d->win->WScreen);
 if(r && SDL_GL_GetCurrentContext()) {
  GLint vp[4]={0};typedef void (*GetInt)(GLenum,GLint*);GetInt get=(GetInt)SDL_GL_GetProcAddress("glGetIntegerv");if(get)get(GL_VIEWPORT,vp);
  fprintf(f," gl_viewport=%d,%d,%d,%d\n",vp[0],vp[1],vp[2],vp[3]);
 }
 fflush(f);
}
int main(int argc,char **argv) {
 if(argc!=5)return 20;
 FILE *f=fopen(argv[1],"w");if(!f)return 20;
 setvbuf(f,NULL,_IONBF,0);
 SDL_Window *w=NULL;SDL_Renderer *r=NULL;int rc=20;
 if(!strcmp(argv[2],"software"))SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION,"0");
 if(!SDL_Init(SDL_INIT_VIDEO))goto end;
 SDL_WindowFlags flags=SDL_WINDOW_RESIZABLE;
 if(atoi(argv[4]))flags|=SDL_WINDOW_OPENGL;
 if(!strcmp(argv[3],"fullscreen"))flags|=SDL_WINDOW_FULLSCREEN;
 w=SDL_CreateWindow("OpenSWOS geometry probe",736,544,flags);if(!w)goto end;
 snapshot(f,"created",w,NULL);
 if(strcmp(argv[3],"fullscreen")) SDL_SetWindowSize(w,736,544);
 snapshot(f,"post-fit",w,NULL);
 r=SDL_CreateRenderer(w,argv[2]);if(!r)goto end;
 fprintf(f,"renderer=%s\n",SDL_GetRendererName(r));snapshot(f,"renderer-created",w,r);
 if(!SDL_SetRenderLogicalPresentation(r,368,272,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE))goto end;
 Uint64 start=SDL_GetTicks();int resized=0,first=1;
 while(SDL_GetTicks()-start<18000) {
  SDL_Event e;while(SDL_PollEvent(&e))if(e.type==SDL_EVENT_QUIT)goto pass;
  if(!resized && !strcmp(argv[3],"resize") && SDL_GetTicks()-start>6000) {
   snapshot(f,"before-resize",w,r);SDL_SetWindowSize(w,920,640);resized=1;
  }
  SDL_SetRenderDrawColor(r,resized?20:200,resized?160:30,70,255);if(!SDL_RenderClear(r))goto end;
  SDL_SetRenderDrawColor(r,255,255,255,255);
  SDL_FRect a={0,0,24,24},b={344,248,24,24};SDL_RenderFillRect(r,&a);SDL_RenderFillRect(r,&b);
  if(!SDL_RenderPresent(r))goto end;
  if(first || resized==1){snapshot(f,first?"first-present":"after-resize-present",w,r);first=0;if(resized)resized=2;}
  SDL_Delay(40);
 }
pass:rc=0;snapshot(f,"finished",w,r);
end:fprintf(f,"result=%d error=%s\n",rc,SDL_GetError());fflush(f);
 if(r)SDL_DestroyRenderer(r);if(w)SDL_DestroyWindow(w);SDL_Quit();fclose(f);return rc;
}
