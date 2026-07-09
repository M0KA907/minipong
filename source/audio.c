#include <tonc.h>
#include "audio.h"

#define MASTER_L	(SDMG_SQR1 | SDMG_SQR2 | SDMG_NOISE)
#define MASTER_R	(SDMG_SQR1 | SDMG_SQR2 | SDMG_NOISE)
#define MASTER_VOL	7

static int s_seq_step;
static int s_seq_timer;
static int s_seq_kind;	/* 0=idle, 1=player win, 2=cpu win */

static u16 hz_to_rate(int hz)
{
	if(hz < 64)   hz = 64;
	if(hz > 20000) hz = 20000;
	int rate = 2048 - (131072 / hz);
	if(rate < 0)    rate = 0;
	if(rate > 2047) rate = 2047;
	return (u16)rate;
}

static void master_pan(u16 lmask, u16 rmask)
{
	REG_SNDDMGCNT = SDMG_BUILD(lmask, rmask, MASTER_VOL, MASTER_VOL);
}

static void play_square2(int hz, int ivol, int len, int duty)
{
	master_pan(MASTER_L, MASTER_R);
	REG_SND2CNT = SSQR_BUILD(ivol, SSQR_DEC, 2, duty, len);
	REG_SND2FREQ = SFREQ_BUILD(hz_to_rate(hz), 1, 1);
}

#define PING_LEN_NOISE	62	/* ~8 ms cap; envelope sets perceived length */
#define PING_LEN_TONE	56	/* soft body, still brief */

/* Plastic click transient on noise channel 4. coarse=0 dull, 1 bright. */
static void play_ping_noise(int ivol, int coarse)
{
	REG_SND4CNT = SSQR_BUILD(ivol, SSQR_DEC, 1, coarse, PING_LEN_NOISE);
	REG_SND4FREQ = SFREQ_BUILD(coarse ? 5 : 9, 1, 1);
}

/* Hollow tone body: 25% duty square on channel 2. */
static void play_ping_tone(int hz, int ivol)
{
	REG_SND2CNT = SSQR_BUILD(ivol, SSQR_DEC, 1, 1, PING_LEN_TONE);
	REG_SND2FREQ = SFREQ_BUILD(hz_to_rate(hz), 1, 1);
}

/* Two-stage ping-pong hit: noise tick + soft tone. pan -1=left, 0=center, +1=right. */
static void play_ping(int hz, int ivol, int pan, int coarse)
{
	u16 lmask = MASTER_L, rmask = MASTER_R;
	if(pan < 0){
		lmask = SDMG_SQR2 | SDMG_NOISE;
		rmask = SDMG_NOISE;
	}else if(pan > 0){
		lmask = SDMG_NOISE;
		rmask = SDMG_SQR2 | SDMG_NOISE;
	}
	master_pan(lmask, rmask);

	int n_ivol = ivol + 1;
	if(n_ivol > 10) n_ivol = 10;
	int t_ivol = ivol - 1;
	if(t_ivol < 2) t_ivol = 2;

	play_ping_noise(n_ivol, coarse);
	play_ping_tone(hz, t_ivol);
}

void audio_init(void)
{
	REG_SNDSTAT = SSTAT_ENABLE;
	REG_SNDDSCNT = SDS_DMG100;
	REG_SND1SWEEP = SSW_OFF;
	master_pan(MASTER_L, MASTER_R);
	s_seq_kind = 0;
}

void audio_update(void)
{
	if(s_seq_kind == 0)
		return;
	if(++s_seq_timer < 10)
		return;
	s_seq_timer = 0;

	static const int win_notes[] = { 784, 988, 1175, 1568 };
	if(s_seq_step >= 4){
		s_seq_kind = 0;
		return;
	}
	play_square2(win_notes[s_seq_step], 10, 12, 2);
	s_seq_step++;
}

void audio_menu_move(void)
{
	play_square2(880, 6, 4, 1);
}

void audio_menu_cycle(void)
{
	play_square2(1320, 8, 6, 2);
}

void audio_menu_confirm(void)
{
	play_square2(660, 10, 8, 2);
}

void audio_pause(void)
{
	play_square2(440, 8, 10, 1);
}

void audio_hit(int speed, int spin, int is_player)
{
	(void)spin;
	int spd = speed < 0 ? -speed : speed;
	int hz = 600 + spd * 2 / 3;
	if(hz > 1020) hz = 1020;
	hz += is_player ? 36 : -36;

	int ivol = 3 + spd / 160;
	if(ivol > 6) ivol = 6;

	play_ping(hz, ivol, is_player ? 1 : -1, 1);
}

void audio_bounce_floor(int speed)
{
	int spd = speed < 0 ? -speed : speed;
	int hz = 220 + spd / 4;
	if(hz > 400) hz = 400;

	int ivol = 3 + spd / 280;
	if(ivol > 5) ivol = 5;

	play_ping(hz, ivol, 0, 0);
}

void audio_bounce_wall(void)
{
	play_ping(170, 3, 0, 0);
}

void audio_point(void)
{
	play_square2(220, 9, 16, 1);
}

void audio_win(int player_won)
{
	(void)player_won;
	s_seq_kind = 1;
	s_seq_step = 0;
	s_seq_timer = 0;
	play_square2(523, 12, 12, 2);
}
