#ifndef GAME_H
#define GAME_H

#include "iso.h"

/* court */
#define COURT_W		FX(160)
#define COURT_D		FX(80)
#define PAD_HALF	FX(12)
#define BALL_R		FX(3)
#define PAD_HIT_MARGIN	FX(8)

/* tuning (all .8 fixed, per frame) */
#define PAD_SPEED	FX(2)
#define PAD_BOOST_SPEED	(PAD_SPEED * 3 / 2)	/* A/B held: 1.5x */
#define GRAVITY		10
#define SERVE_VX	320
#define SERVE_VY	150
#define SERVE_VZ	260
#define KICK_VZ		300
#define SPEEDUP_NUM	17
#define SPEEDUP_DEN	16
#define MAX_VX		640
#define MAX_VY		512

enum {
	GEV_NONE,
	GEV_POINT,
	GEV_WIN,
	GEV_HIT_PLAYER,
	GEV_HIT_CPU,
	GEV_FLOOR_BOUNCE,
	GEV_WALL_BOUNCE
};

/* settings chosen in the menu; copied into Game at init (match-immutable) */
typedef struct {
	int difficulty;		/* 0 easy, 1 normal, 2 hard */
	int win_score_idx;	/* index into GAME_WIN_SCORES */
} Config;

extern const int GAME_WIN_SCORES[3];	/* {5, 7, 11} */

typedef struct { int x, y, z, vx, vy, vz; } Ball;
typedef struct { int y, vy; } Paddle;

typedef struct {
	Ball ball;
	Paddle pads[2];		/* 0 = player (x=COURT_W), 1 = cpu (x=0) */
	int score[2];		/* indexed like pads */
	int server;		/* side serving next (= last scorer) */
	int winner;		/* -1 while playing */
	int win_score;		/* resolved from Config at init */
	int ai_speed;		/* per-frame cap, .8 fixed */
	int ai_deadzone;	/* .8 fixed */
} Game;

void game_init(Game *g, const Config *cfg);
void game_serve(Game *g);
/* -1/0/+1 for pads[side] to track ball y (dead zone applied) */
int game_ai_dir(const Game *g, int side);
/* dir: -1/0/+1 player paddle along y; boost: nonzero = 1.5x speed; returns GEV_* */
int game_step(Game *g, int dir, int boost);

#endif
