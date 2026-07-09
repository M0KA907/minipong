#include <tonc.h>
#include "game.h"
#include "menu.h"
#include "render.h"

enum { ST_MENU, ST_RALLY, ST_POINT, ST_PAUSE, ST_WIN };

static int player_dir(int controls)
{
	if(controls == 0)
		return (key_held(KEY_LEFT) ? 1 : 0)
		     - (key_held(KEY_RIGHT) ? 1 : 0);
	return (key_held(KEY_UP) ? 1 : 0)
	     - (key_held(KEY_DOWN) ? 1 : 0);
}

int main(void)
{
	irq_init(NULL);
	irq_enable(II_VBLANK);
	render_init();

	Menu mn;
	menu_init(&mn);
	Game g;
	game_init(&g, &mn.cfg);	/* attract game; any cfg works */
	game_serve(&g);
	int st = ST_MENU, timer = 0;
	render_menu(&mn.cfg, mn.row);

	while(1){
		VBlankIntrWait();
		key_poll();

		switch(st){
		case ST_MENU: {
			/* attract rally: both paddles AI, endless, no score */
			int ev = game_step(&g, game_ai_dir(&g, 0));
			if(ev != GEV_NONE){
				g.score[0] = 0;  g.score[1] = 0;  g.winner = -1;
				game_serve(&g);
			}
			render_game(&g);

			int dirty = 0;
			if(key_hit(KEY_UP))   { menu_move(&mn, -1);  dirty = 1; }
			if(key_hit(KEY_DOWN)) { menu_move(&mn,  1);  dirty = 1; }
			if(key_hit(KEY_LEFT)) { menu_cycle(&mn, -1); dirty = 1; }
			if(key_hit(KEY_RIGHT)){ menu_cycle(&mn,  1); dirty = 1; }
			if(dirty)
				render_menu(&mn.cfg, mn.row);

			if(key_hit(KEY_START) || key_hit(KEY_A)){
				game_init(&g, &mn.cfg);	/* cfg locked in here */
				render_court();
				render_score(0, 0);
				game_serve(&g);
				render_game(&g);
				st = ST_RALLY;
			}
			break; }

		case ST_RALLY: {
			if(key_hit(KEY_START)){
				render_msg("PAUSE");
				st = ST_PAUSE;
				break;
			}
			int ev = game_step(&g, player_dir(mn.cfg.controls));
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
				game_init(&g, &mn.cfg);
				game_serve(&g);
				render_menu(&mn.cfg, mn.row);
				st = ST_MENU;
			}
			break;
		}
	}
}
