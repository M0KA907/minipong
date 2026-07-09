#include "iso.h"
#include "game.h"

static void clamp_pad(Paddle *p)
{
	if(p->y < PAD_HALF)           p->y = PAD_HALF;
	if(p->y > COURT_D - PAD_HALF) p->y = COURT_D - PAD_HALF;
}

void game_init(Game *g)
{
	g->score[0] = 0;  g->score[1] = 0;
	g->server = 0;    g->winner = -1;
	g->pads[0].y = COURT_D / 2;  g->pads[0].vy = 0;
	g->pads[1].y = COURT_D / 2;  g->pads[1].vy = 0;
	g->ball.x = COURT_W / 2;  g->ball.y = COURT_D / 2;  g->ball.z = 0;
	g->ball.vx = 0;  g->ball.vy = 0;  g->ball.vz = 0;
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
	if(g->score[side] >= WIN_SCORE){ g->winner = side; return GEV_WIN; }
	return GEV_POINT;
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
	g->pads[0].vy = dir * PAD_SPEED;
	g->pads[0].y += g->pads[0].vy;
	clamp_pad(&g->pads[0]);

	/* cpu paddle: track ball y, capped speed, dead zone (beatable) */
	{
		int d = b->y - g->pads[1].y;
		int v = 0;
		if(d >  AI_DEADZONE) v =  AI_SPEED;
		if(d < -AI_DEADZONE) v = -AI_SPEED;
		g->pads[1].vy = v;
		g->pads[1].y += v;
		clamp_pad(&g->pads[1]);
	}

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
		if(off <= PAD_HALF + BALL_R){
			b->x = 2*COURT_W - b->x;
			reflect_off_pad(b, &g->pads[0]);
		}else
			return point_for(g, 1);
	}

	/* cpu plane (x = 0) */
	if(b->vx < 0 && b->x <= 0){
		int off = b->y - g->pads[1].y;
		if(off < 0) off = -off;
		if(off <= PAD_HALF + BALL_R){
			b->x = -b->x;
			reflect_off_pad(b, &g->pads[1]);
		}else
			return point_for(g, 0);
	}

	return GEV_NONE;
}
