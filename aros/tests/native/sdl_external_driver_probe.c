/* SDL backend selection must import explicit native AmigaDOS driver variables. */
#include "swos_sdl.h"
#include <proto/dos.h>
#include <dos/var.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    if (!SetVar((CONST_STRPTR)"SDL_AUDIODRIVER", (CONST_STRPTR)"dummy", -1,
                LV_VAR | GVF_LOCAL_ONLY)) return 20;
    if (SDL_Init(0) != 0) return 20;
    const char *value = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "SDL_AUDIODRIVER");
    int ok = value && !strcmp(value, "dummy");
    printf("external SDL_AUDIODRIVER: %s; %s\n", value ? value : "missing", ok ? "PASS" : "FAIL");
    SDL_Quit();
    return ok ? 0 : 1;
}
