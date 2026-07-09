#ifndef RENDER_H
#define RENDER_H

#include "game.h"

void render_init(void);			/* mode 4, palettes, court, tiles, OAM */
void render_game(const Game *g);	/* sprite positions; call during VBlank */
void render_score(int cpu, int player);	/* digits top-left / top-right */
void render_msg(const char *s);		/* centered bottom text; "" clears */
void render_court(void);		/* repaint bitmap backdrop (erases all text) */
void render_menu(const Config *cfg, int row);	/* menu panel over court */
void render_menu_cursor(int row, int show);	/* partial: toggle row cursor */
void render_menu_value(const Config *cfg, int row);	/* partial: redraw one value */

#endif
