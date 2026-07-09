#include "iso.h"
#include "game.h"

const int GAME_WIN_SCORES[3] = { 5, 7, 11 };

/* per-difficulty AI tuning; index = Config.difficulty */
static const struct { int speed, deadzone; } s_ai[3] = {
	{ 224, FX(6) },		/* easy: slow, blind spot */
	{ 320, FX(3) },		/* normal: previous fixed tuning */
	{ 416, FX(1) },		/* hard: fast, tight */
};

static void move_pad(Paddle *p, int vy)
{
	int y0 = p->y;
	p->y += vy;
	if(p->y < PAD_HALF)           p->y = PAD_HALF;
	if(p->y > COURT_D - PAD_HALF) p->y = COURT_D - PAD_HALF;
	p->vy = p->y - y0;	/* actual movement, so spin transfer is honest */
}

void game_init(Game *g, const Config *cfg)
{
	g->score[0] = 0;  g->score[1] = 0;
	g->server = 0;    g->winner = -1;
	g->pads[0].y = COURT_D / 2;  g->pads[0].vy = 0;
	g->pads[1].y = COURT_D / 2;  g->pads[1].vy = 0;
	g->ball.x = COURT_W / 2;  g->ball.y = COURT_D / 2;  g->ball.z = 0;
	g->ball.vx = 0;  g->ball.vy = 0;  g->ball.vz = 0;
	g->win_score = GAME_WIN_SCORES[cfg->win_score_idx];
	g->ai_speed = s_ai[cfg->difficulty].speed;
	g->ai_deadzone = s_ai[cfg->difficulty].deadzone;
}

void game_serve(Game *g)
{
	Ball *b = &g->ball;
	b->y = COURT_D / 2;
	b->z = FX(8);
	b->vy = ((g->score[0] + g->score[1]) & 1) ? SERVE_VY : -SERVE_VY;
	b->vz = SERVE_VZ;
	if(g->server == 0){ b->x = COURT_W - FX(8);  b->vx = -SERVE_VX; }
	else              { b->x = FX(8);            b->vx =  SERVE_VX; }
}

static int point_for(Game *g, int side)
{
	g->score[side]++;
	g->server = side;
	g->ball.vx = 0;  g->ball.vy = 0;  g->ball.vz = 0;
	if(g->score[side] >= g->win_score){ g->winner = side; return GEV_WIN; }
	return GEV_POINT;
}

int game_ai_dir(const Game *g, int side)
{
	int d = g->ball.y - g->pads[side].y;
	if(d >  g->ai_deadzone) return  1;
	if(d < -g->ai_deadzone) return -1;
	return 0;
}

static void reflect_off_pad(Ball *b, const Paddle *p)
{
	b->vx = -b->vx;
	b->vx = b->vx * SPEEDUP_NUM / SPEEDUP_DEN;
	if(b->vx >  MAX_VX) b->vx =  MAX_VX;
	if(b->vx < -MAX_VX) b->vx = -MAX_VX;
	b->vy += p->vy / 2;
	if(b->vy >  MAX_VY) b->vy =  MAX_VY;
	if(b->vy < -MAX_VY) b->vy = -MAX_VY;
	b->vz = KICK_VZ;
}

int game_step(Game *g, int dir)
{
	Ball *b = &g->ball;

	/* player paddle */
	move_pad(&g->pads[0], dir * PAD_SPEED);

	/* cpu paddle: track ball y, capped speed, dead zone (beatable) */
	move_pad(&g->pads[1], game_ai_dir(g, 1) * g->ai_speed);

	/* ball: gravity, integrate, bounce floor and side walls */
	b->vz -= GRAVITY;
	b->x += b->vx;  b->y += b->vy;  b->z += b->vz;
	if(b->z < 0)      { b->z = -b->z;              b->vz = -b->vz; }
	if(b->y < 0)      { b->y = -b->y;              b->vy = -b->vy; }
	if(b->y > COURT_D){ b->y = 2*COURT_D - b->y;   b->vy = -b->vy; }

	/* player plane (x = COURT_W) */
	if(b->vx > 0 && b->x >= COURT_W){
		int off = b->y - g->pads[0].y;
		if(off < 0) off = -off;
		if(off <= PAD_HALF + BALL_R + PAD_HIT_MARGIN){
			b->x = 2*COURT_W - b->x;
			reflect_off_pad(b, &g->pads[0]);
		}else
			return point_for(g, 1);
	}

	/* cpu plane (x = 0) */
	if(b->vx < 0 && b->x <= 0){
		int off = b->y - g->pads[1].y;
		if(off < 0) off = -off;
		if(off <= PAD_HALF + BALL_R + PAD_HIT_MARGIN){
			b->x = -b->x;
			reflect_off_pad(b, &g->pads[1]);
		}else
			return point_for(g, 0);
	}

	return GEV_NONE;
}
