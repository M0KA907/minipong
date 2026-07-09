#include "menu.h"

void menu_init(Menu *m)
{
	m->cfg.difficulty = 1;		/* normal */
	m->cfg.win_score_idx = 0;	/* 5 */
	m->row = MROW_DIFF;
}

void menu_move(Menu *m, int dir)
{
	m->row = (m->row + dir + MROW_COUNT) % MROW_COUNT;
}

static int cycle(int v, int dir, int n)
{
	return (v + dir + n) % n;
}

void menu_cycle(Menu *m, int dir)
{
	switch(m->row){
	case MROW_DIFF:  m->cfg.difficulty    = cycle(m->cfg.difficulty, dir, 3); break;
	case MROW_SCORE: m->cfg.win_score_idx = cycle(m->cfg.win_score_idx, dir, 3); break;
	}
}
