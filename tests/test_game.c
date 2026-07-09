#include <assert.h>
#include <stdio.h>
#include "iso.h"
#include "game.h"

static void test_init(void)
{
	Game g; game_init(&g);
	assert(g.score[0] == 0 && g.score[1] == 0);
	assert(g.winner == -1);
	assert(g.pads[0].y == COURT_D / 2 && g.pads[1].y == COURT_D / 2);
}

static void test_serve_places_ball(void)
{
	Game g; game_init(&g);
	g.server = 0; game_serve(&g);
	assert(g.ball.x == COURT_W - FX(8) && g.ball.vx == -SERVE_VX);
	assert(g.ball.z > 0 && g.ball.vz == SERVE_VZ);
	g.server = 1; game_serve(&g);
	assert(g.ball.x == FX(8) && g.ball.vx == SERVE_VX);
}

static void test_floor_bounce(void)
{
	Game g; game_init(&g); game_serve(&g);
	g.ball.z = FX(1); g.ball.vz = -FX(2);
	game_step(&g, 0);
	assert(g.ball.z >= 0);
	assert(g.ball.vz > 0);
}

static void test_wall_bounce(void)
{
	Game g; game_init(&g); game_serve(&g);
	g.ball.y = FX(1); g.ball.vy = -FX(2);
	game_step(&g, 0);
	assert(g.ball.y >= 0 && g.ball.vy > 0);
	g.ball.y = COURT_D - FX(1); g.ball.vy = FX(2);
	game_step(&g, 0);
	assert(g.ball.y <= COURT_D && g.ball.vy < 0);
}

static void test_paddle_hit_reflects_and_kicks(void)
{
	Game g; game_init(&g); game_serve(&g);
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y; g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_NONE);
	assert(g.ball.vx < 0);			/* reflected */
	assert(-g.ball.vx > SERVE_VX);		/* sped up */
	assert(g.ball.x <= COURT_W);		/* back in court */
	assert(g.ball.vz == KICK_VZ);		/* arcs up again */
}

static void test_miss_scores_for_other_side(void)
{
	Game g; game_init(&g); game_serve(&g);
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y + PAD_HALF + BALL_R + FX(2);
	g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_POINT);
	assert(g.score[1] == 1 && g.score[0] == 0);
	assert(g.server == 1);			/* scorer serves */
	assert(g.winner == -1);
}

static void test_win_at_five(void)
{
	Game g; game_init(&g); game_serve(&g);
	g.score[1] = WIN_SCORE - 1;
	g.ball.x = COURT_W - FX(1); g.ball.vx = SERVE_VX;
	g.ball.y = g.pads[0].y + PAD_HALF + BALL_R + FX(2);
	g.ball.vy = 0;
	int ev = game_step(&g, 0);
	assert(ev == GEV_WIN);
	assert(g.winner == 1 && g.score[1] == WIN_SCORE);
}

static void test_player_paddle_moves_and_clamps(void)
{
	Game g; game_init(&g); game_serve(&g);
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
	Game g; game_init(&g); game_serve(&g);
	g.ball.x = COURT_W / 2; g.ball.vx = 0; g.ball.vy = 0;
	g.ball.y = FX(70);
	int y0 = g.pads[1].y;
	game_step(&g, 0);
	assert(g.pads[1].y > y0);		/* moves toward ball */
	for(int i = 0; i < 500; i++) game_step(&g, 0);
	/* settles near ball y (within dead zone + one step) */
	int d = g.pads[1].y - g.ball.y;
	if(d < 0) d = -d;
	assert(d <= AI_DEADZONE + AI_SPEED);
}

int main(void)
{
	test_init();
	test_serve_places_ball();
	test_floor_bounce();
	test_wall_bounce();
	test_paddle_hit_reflects_and_kicks();
	test_miss_scores_for_other_side();
	test_win_at_five();
	test_player_paddle_moves_and_clamps();
	test_ai_tracks_ball();
	puts("test_game OK");
	return 0;
}
