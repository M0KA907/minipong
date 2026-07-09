#include <tonc.h>
#include "game.h"
#include "menu.h"
#include "render.h"
#include "audio.h"

enum { ST_MENU, ST_RALLY, ST_POINT, ST_PAUSE, ST_WIN };

static void play_game_event(const Game *g, int ev)
{
	switch(ev){
	case GEV_HIT_PLAYER:
		audio_hit(g->ball.vx, g->ball.vy, 1);
		break;
	case GEV_HIT_CPU:
		audio_hit(g->ball.vx, g->ball.vy, 0);
		break;
	case GEV_FLOOR_BOUNCE:
		audio_bounce_floor(g->ball.vz);
		break;
	case GEV_WALL_BOUNCE:
		audio_bounce_wall();
		break;
	default:
		break;
	}
}

/* L / Left / Down = left; R / Right / Up = right (iso: left = +y); opposites cancel */
static int player_dir(void)
{
	int left  = key_held(KEY_L) || key_held(KEY_LEFT)  || key_held(KEY_DOWN);
	int right = key_held(KEY_R) || key_held(KEY_RIGHT) || key_held(KEY_UP);
	return (left ? 1 : 0) - (right ? 1 : 0);
}

static int player_boost(void)
{
	return key_held(KEY_A) || key_held(KEY_B);
}

int main(void)
{
	irq_init(NULL);
	irq_enable(II_VBLANK);
	render_init();
	audio_init();

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
		audio_update();

		switch(st){
		case ST_MENU: {
			/* attract rally: both paddles AI, endless, no score */
			int ev = game_step(&g, game_ai_dir(&g, 0), 0);
			if(ev == GEV_POINT || ev == GEV_WIN){
				g.score[0] = 0;  g.score[1] = 0;  g.winner = -1;
				game_serve(&g);
			}else
				play_game_event(&g, ev);
			render_game(&g);

			if(key_hit(KEY_UP)){
				audio_menu_move();
				int prev = mn.row;
				menu_move(&mn, -1);
				render_menu_cursor(prev, 0);
				render_menu_cursor(mn.row, 1);
			}
			if(key_hit(KEY_DOWN)){
				audio_menu_move();
				int prev = mn.row;
				menu_move(&mn,  1);
				render_menu_cursor(prev, 0);
				render_menu_cursor(mn.row, 1);
			}
			if(key_hit(KEY_LEFT)){
				audio_menu_cycle();
				menu_cycle(&mn, -1);
				render_menu_value(&mn.cfg, mn.row);
			}
			if(key_hit(KEY_RIGHT)){
				audio_menu_cycle();
				menu_cycle(&mn,  1);
				render_menu_value(&mn.cfg, mn.row);
			}

			if(key_hit(KEY_START) || key_hit(KEY_A)){
				audio_menu_confirm();
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
				audio_pause();
				render_msg("PAUSE");
				st = ST_PAUSE;
				break;
			}
			int ev = game_step(&g, player_dir(), player_boost());
			render_game(&g);
			if(ev == GEV_POINT){
				audio_point();
				render_score(g.score[1], g.score[0]);
				timer = 45;
				st = ST_POINT;
			}else if(ev == GEV_WIN){
				audio_win(g.winner == 0);
				render_score(g.score[1], g.score[0]);
				render_msg(g.winner == 0 ? "YOU WIN" : "CPU WINS");
				st = ST_WIN;
			}else
				play_game_event(&g, ev);
			break; }

		case ST_POINT:
			if(--timer <= 0){
				game_serve(&g);
				st = ST_RALLY;
			}
			break;

		case ST_PAUSE:
			if(key_hit(KEY_START)){
				audio_menu_confirm();
				render_msg("");
				st = ST_RALLY;
			}
			break;

		case ST_WIN:
			if(key_hit(KEY_START)){
				audio_menu_confirm();
				game_init(&g, &mn.cfg);
				game_serve(&g);
				render_menu(&mn.cfg, mn.row);
				st = ST_MENU;
			}
			break;
		}
	}
}
