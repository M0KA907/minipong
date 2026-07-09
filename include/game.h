#ifndef GAME_H
#define GAME_H

#include "iso.h"

/* court */
#define COURT_W		FX(160)
#define COURT_D		FX(80)
#define PAD_HALF	FX(12)
#define BALL_R		FX(3)

/* tuning (all .8 fixed, per frame) */
#define PAD_SPEED	FX(2)
#define AI_SPEED	320
#define AI_DEADZONE	FX(3)
#define GRAVITY		10
#define SERVE_VX	320
#define SERVE_VY	150
#define SERVE_VZ	260
#define KICK_VZ		300
#define SPEEDUP_NUM	17
#define SPEEDUP_DEN	16
#define MAX_VX		640
#define MAX_VY		512
#define WIN_SCORE	5

enum { GEV_NONE, GEV_POINT, GEV_WIN };

typedef struct { int x, y, z, vx, vy, vz; } Ball;
typedef struct { int y, vy; } Paddle;

typedef struct {
	Ball ball;
	Paddle pads[2];		/* 0 = player (x=COURT_W), 1 = cpu (x=0) */
	int score[2];		/* indexed like pads */
	int server;		/* side serving next (= last scorer) */
	int winner;		/* -1 while playing */
} Game;

void game_init(Game *g);
void game_serve(Game *g);
/* dir: -1/0/+1 player paddle along y; returns GEV_* */
int game_step(Game *g, int dir);

#endif
