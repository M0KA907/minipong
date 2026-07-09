#include <assert.h>
#include <stdio.h>
#include "iso.h"
#include "game.h"

static const Config CFG_NORMAL = { 1, 0, 0 };	/* normal, to 5, L-R */

static void test_init(void)
{
	Game g; game_init(&g, &CFG_NORMAL);
	assert(g.score[0] == 0 && g.score[1] == 0);
	assert(g.winner == -1);
	assert(g.pads[0].y == COURT_D / 2 && g.pads[1].y == COURT_D / 2);
}

static void test_serve_places_ball(void)
{
	Game g; game_init(&g, &CFG_NORMAL);
	g.server = 0; game_serve(&g);
	assert(g.ball.x == COURT_W - FX(8) && g.ball.vx == -SERVE_VX);
	assert(g.ball.z > 0 && g.ball.vz == SERVE_VZ);
	g.server = 1; game_serve(&g);
	assert(g.ball.x == FX(8) && g.ball.vx == SERVE_VX);
}

static void test_floor_bounce(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.z = FX(1); g.ball.vz = -FX(2);
	game_step(&g, 0);
	assert(g.ball.z >= 0);
	assert(g.ball.vz > 0);
}

static void test_wall_bounce(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.y = FX(1); g.ball.vy = -FX(2);
	game_step(&g, 0);
	assert(g.ball.y >= 0 && g.ball.vy > 0);
	g.ball.y = COURT_D - FX(1); g.ball.vy = FX(2);
	game_step(&g, 0);
	assert(g.ball.y <= COURT_D && g.ball.vy < 0);
}

static void test_paddle_hit_reflects_and_kicks(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y; g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_NONE);
	assert(g.ball.vx < 0);			/* reflected */
	assert(-g.ball.vx > SERVE_VX);		/* sped up */
	assert(g.ball.x <= COURT_W);		/* back in court */
	assert(g.ball.vz == KICK_VZ);		/* arcs up again */
}

static void test_paddle_hit_has_forgiving_margin(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y + PAD_HALF + BALL_R + PAD_HIT_MARGIN - FX(1);
	g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_NONE);
	assert(g.ball.vx < 0);
}

static void test_miss_scores_for_other_side(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y + PAD_HALF + BALL_R + PAD_HIT_MARGIN + FX(2);
	g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_POINT);
	assert(g.score[1] == 1 && g.score[0] == 0);
	assert(g.server == 1);			/* scorer serves */
	assert(g.winner == -1);
}

static void test_win_at_five(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.score[1] = g.win_score - 1;
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y + PAD_HALF + BALL_R + PAD_HIT_MARGIN + FX(2);
	g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_WIN);
	assert(g.winner == 1 && g.score[1] == g.win_score);
}

static void test_player_paddle_moves_and_clamps(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	/* park the ball mid-court so no points happen */
	g.ball.x = COURT_W / 2; g.ball.vx = 0; g.ball.vy = 0;
	int y0 = g.pads[0].y;
	game_step(&g, 1);
	assert(g.pads[0].y == y0 + PAD_SPEED);
	for(int i = 0; i < 200; i++) game_step(&g, 1);
	assert(g.pads[0].y == COURT_D - PAD_HALF);
	for(int i = 0; i < 200; i++) game_step(&g, -1);
	assert(g.pads[0].y == PAD_HALF);
}

static void test_ai_tracks_ball(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.x = COURT_W / 2; g.ball.vx = 0; g.ball.vy = 0;
	g.ball.y = FX(70);
	int y0 = g.pads[1].y;
	game_step(&g, 0);
	assert(g.pads[1].y > y0);		/* moves toward ball */
	for(int i = 0; i < 500; i++) game_step(&g, 0);
	/* settles near ball y (within dead zone + one step) */
	int d = g.pads[1].y - g.ball.y;
	if(d < 0) d = -d;
	assert(d <= g.ai_deadzone + g.ai_speed);
}

static void test_clamped_paddle_gives_no_spin(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	/* pin player paddle at bottom clamp */
	g.ball.x = COURT_W / 2; g.ball.vx = 0; g.ball.vy = 0;
	for(int i = 0; i < 200; i++) game_step(&g, 1);
	assert(g.pads[0].y == COURT_D - PAD_HALF);
	/* paddle pinned: one more push must record zero actual movement */
	game_step(&g, 1);
	assert(g.pads[0].vy == 0);
	/* ball hits pinned paddle while input held: no spin transfer */
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y; g.ball.vy = 0;
	game_step(&g, 1);
	assert(g.ball.vy == 0);
}

static void test_ai_dir(void)
{
	Game g; game_init(&g, &CFG_NORMAL); game_serve(&g);
	g.ball.y = g.pads[1].y + g.ai_deadzone + FX(1);
	assert(game_ai_dir(&g, 1) == 1);
	g.ball.y = g.pads[1].y - g.ai_deadzone - FX(1);
	assert(game_ai_dir(&g, 1) == -1);
	g.ball.y = g.pads[1].y;	/* inside dead zone */
	assert(game_ai_dir(&g, 1) == 0);
}

static void test_difficulty_table(void)
{
	Game e, n, h;
	Config c = CFG_NORMAL;
	c.difficulty = 0; game_init(&e, &c);
	c.difficulty = 1; game_init(&n, &c);
	c.difficulty = 2; game_init(&h, &c);
	assert(e.ai_speed < n.ai_speed && n.ai_speed < h.ai_speed);
	assert(e.ai_deadzone > n.ai_deadzone && n.ai_deadzone > h.ai_deadzone);
	assert(n.ai_speed == 320 && n.ai_deadzone == FX(3));	/* legacy tuning */
}

static void test_win_score_honored(void)
{
	for(int i = 0; i < 3; i++){
		Config c = CFG_NORMAL; c.win_score_idx = i;
		Game g; game_init(&g, &c); game_serve(&g);
		assert(g.win_score == GAME_WIN_SCORES[i]);
		g.score[1] = g.win_score - 1;
		g.ball.x = COURT_W - FX(1); g.ball.vx = MAX_VX;
		g.ball.y = 0; g.pads[0].y = COURT_D - PAD_HALF;	/* miss */
		int ev = 0;
		for(int t = 0; t < 60 && ev == 0; t++) ev = game_step(&g, 0);
		assert(ev == GEV_WIN && g.winner == 1);
	}
}

static void test_config_immutable_after_init(void)
{
	Config c = CFG_NORMAL;
	Game g; game_init(&g, &c);
	c.difficulty = 2; c.win_score_idx = 2;	/* mutate after init */
	assert(g.ai_speed == 320 && g.ai_deadzone == FX(3));
	assert(g.win_score == 5);
}

int main(void)
{
	test_init();
	test_serve_places_ball();
	test_floor_bounce();
	test_wall_bounce();
	test_paddle_hit_reflects_and_kicks();
	test_paddle_hit_has_forgiving_margin();
	test_miss_scores_for_other_side();
	test_win_at_five();
	test_player_paddle_moves_and_clamps();
	test_ai_tracks_ball();
	test_clamped_paddle_gives_no_spin();
	test_ai_dir();
	test_difficulty_table();
	test_win_score_honored();
	test_config_immutable_after_init();
	puts("test_game OK");
	return 0;
}
