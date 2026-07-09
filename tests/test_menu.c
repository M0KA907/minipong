#include <assert.h>
#include <stdio.h>
#include "menu.h"

static void test_defaults(void)
{
	Menu m; menu_init(&m);
	assert(m.cfg.difficulty == 1);		/* normal */
	assert(m.cfg.win_score_idx == 0);	/* 5 */
	assert(m.row == MROW_DIFF);
}

static void test_move_wraps(void)
{
	Menu m; menu_init(&m);
	menu_move(&m, -1);
	assert(m.row == MROW_START);
	menu_move(&m, 1);
	assert(m.row == MROW_DIFF);
	for(int i = 0; i < MROW_COUNT; i++) menu_move(&m, 1);
	assert(m.row == MROW_DIFF);
}

static void test_cycle_values(void)
{
	Menu m; menu_init(&m);
	m.row = MROW_DIFF;
	menu_cycle(&m, 1);  assert(m.cfg.difficulty == 2);
	menu_cycle(&m, 1);  assert(m.cfg.difficulty == 0);	/* wraps */
	menu_cycle(&m, -1); assert(m.cfg.difficulty == 2);
	m.row = MROW_SCORE;
	menu_cycle(&m, 1);  assert(m.cfg.win_score_idx == 1);
}

static void test_cycle_on_start_row_noop(void)
{
	Menu m; menu_init(&m);
	m.row = MROW_START;
	Config before = m.cfg;
	menu_cycle(&m, 1);
	assert(m.cfg.difficulty == before.difficulty);
	assert(m.cfg.win_score_idx == before.win_score_idx);
}

int main(void)
{
	test_defaults();
	test_move_wraps();
	test_cycle_values();
	test_cycle_on_start_row_noop();
	puts("test_menu OK");
	return 0;
}
