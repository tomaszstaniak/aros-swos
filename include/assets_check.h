/*
 * assets_check.h -- tell the player when their game data was made by an older OpenSWOS
 * Setup (src/assets_check.c, ASSETS.ini: include/assets_ini.h). SDL builds only.
 */
#ifndef ASSETS_CHECK_H
#define ASSETS_CHECK_H

#include "platform_sdl.h"   /* PlatformInitOptions */

/* After SDL_Init: log the parts of the assets folder an older setup made, and once a
 * set of data versions show the reminder (with "Don't remind me", saved to iniPath). */
/* A missing first-menu bank, or NULL. Uses the normal case-insensitive roots. */
const char *AssetsCheck_MissingRequired(void);
void AssetsCheck_FatalMissing(const char *filename);

void AssetsCheck_Run(PlatformInitOptions *opts, const char *iniPath);

#endif /* ASSETS_CHECK_H */
