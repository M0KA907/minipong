#include <tonc.h>
#include "game.h"
#include "render.h"

enum { ST_TITLE, ST_RALLY, ST_POINT, ST_PAUSE, ST_WIN };

int main(void)
{
	irq_init(NULL);
	irq_enable(II_VBLANK);
	render_init();

	Game g;
	game_init(&g);
	int st = ST_TITLE, timer = 0;
	render_score(0, 0);
	render_msg("MINIPONG  PRESS START");
	render_game(&g);

	while(1){
		VBlankIntrWait();
		key_poll();

		switch(st){
		case ST_TITLE:
			if(key_hit(KEY_START)){
				game_init(&g);
				render_score(0, 0);
				render_msg("");
				game_serve(&g);
				st = ST_RALLY;
			}
			break;

		case ST_RALLY: {
			if(key_hit(KEY_START)){
				render_msg("PAUSE");
				st = ST_PAUSE;
				break;
			}
			int dir = (key_held(KEY_DOWN) ? 1 : 0)
				- (key_held(KEY_UP) ? 1 : 0);
			int ev = game_step(&g, dir);
			render_game(&g);
			if(ev == GEV_POINT){
				render_score(g.score[1], g.score[0]);
				timer = 45;
				st = ST_POINT;
			}else if(ev == GEV_WIN){
				render_score(g.score[1], g.score[0]);
				render_msg(g.winner == 0 ? "YOU WIN" : "CPU WINS");
				st = ST_WIN;
			}
			break; }

		case ST_POINT:
			if(--timer <= 0){
				game_serve(&g);
				st = ST_RALLY;
			}
			break;

		case ST_PAUSE:
			if(key_hit(KEY_START)){
				render_msg("");
				st = ST_RALLY;
			}
			break;

		case ST_WIN:
			if(key_hit(KEY_START)){
				game_init(&g);
				render_score(0, 0);
				render_msg("MINIPONG  PRESS START");
				render_game(&g);
				st = ST_TITLE;
			}
			break;
		}
	}
}
