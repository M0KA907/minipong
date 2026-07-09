#ifndef RENDER_H
#define RENDER_H

#include "game.h"

void render_init(void);			/* mode 4, palettes, court, tiles, OAM */
void render_game(const Game *g);	/* sprite positions; call during VBlank */
void render_score(int cpu, int player);	/* digits top-left / top-right */
void render_msg(const char *s);		/* centered bottom text; "" clears */

#endif
