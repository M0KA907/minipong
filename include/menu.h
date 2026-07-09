#ifndef MENU_H
#define MENU_H

#include "game.h"

enum { MROW_DIFF, MROW_SCORE, MROW_START, MROW_COUNT };

typedef struct {
	Config cfg;	/* persists across matches (RAM only) */
	int row;	/* cursor, MROW_* */
} Menu;

void menu_init(Menu *m);
void menu_move(Menu *m, int dir);	/* -1 up / +1 down, wraps */
void menu_cycle(Menu *m, int dir);	/* -1/+1 cycle value on current row */

#endif
