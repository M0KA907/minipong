#ifndef AUDIO_H
#define AUDIO_H

void audio_init(void);
void audio_update(void);

void audio_menu_move(void);
void audio_menu_cycle(void);
void audio_menu_confirm(void);
void audio_pause(void);

void audio_hit(int speed, int spin, int is_player);
void audio_bounce_floor(int speed);
void audio_bounce_wall(void);
void audio_point(void);
void audio_win(int player_won);

#endif
