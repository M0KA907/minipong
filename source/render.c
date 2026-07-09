#include <tonc.h>
#include "iso.h"
#include "game.h"
#include "menu.h"
#include "render.h"

/* ---- bg palette indices ---- */
#define CLR_BG		0
#define CLR_FLOOR	1
#define CLR_FELT2	2
#define CLR_FRAME	3
#define CLR_LINE	4
#define CLR_NET		5
#define CLR_TEXT	6

/* ---- obj tiles (bitmap mode: ids must be >= 512) ---- */
#define TID_BALL	512
#define TID_SHADOW	513
#define TID_PAD		514	/* 8 tiles: 32x16, 1D mapping */

#define OBJ_TILES	((TILE*)tile_mem[4])

static OBJ_ATTR obj_buf[4];	/* 0 ball, 1 shadow, 2 player pad, 3 cpu pad */

/* ---- mode 4 pixel helpers (page 0 only; VRAM needs 16-bit writes) ---- */
static void px_dot(int x, int y, u8 c)
{
	u16 *dst = &vid_mem[(y * 240 + x) >> 1];
	if(x & 1) *dst = (*dst & 0x00FF) | ((u16)c << 8);
	else      *dst = (*dst & 0xFF00) | c;
}

static void px_hline(int x0, int x1, int y, u8 c)
{
	if(x0 > x1){ int t = x0; x0 = x1; x1 = t; }
	int x = x0;
	if(x & 1){ px_dot(x, y, c); x++; }
	u16 cc = c | ((u16)c << 8);
	for(; x + 1 <= x1; x += 2)
		vid_mem[(y * 240 + x) >> 1] = cc;
	if(x == x1) px_dot(x, y, c);
}

static void px_rect(int x0, int y0, int x1, int y1, u8 c)
{
	for(int y = y0; y <= y1; y++)
		px_hline(x0, x1, y, c);
}

/* ---- 3x5 font, scaled; row0 in bits 14..12 .. row4 in bits 2..0 ---- */
static const char s_font_chars[] = "0123456789ACEGIMNOPRSTUWY DFHL->";
static const u16 s_font[32] = {
	0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9,	/* 0-4 */
	0x79CF, 0x79EF, 0x7252, 0x7BEF, 0x7BCF,	/* 5-9 */
	0x2BED, 0x3923, 0x79E7, 0x396B, 0x7497,	/* A C E G I */
	0x5FED, 0x6B6D, 0x7B6F, 0x7BE4, 0x7BF5,	/* M N O P R */
	0x388E, 0x7492, 0x5B6F, 0x5BFD, 0x5A92,	/* S T U W Y */
	0x0000,					/* space */
	0x6B6E, 0x79A4, 0x5BED, 0x4927,		/* D F H L */
	0x01C0, 0x4454				/* - > */
};

static int glyph_index(char ch)
{
	for(int i = 0; s_font_chars[i]; i++)
		if(s_font_chars[i] == ch) return i;
	return 25;	/* unknown -> space */
}

static void draw_glyph(int x0, int y0, char ch, int scale, u8 c)
{
	u16 bits = s_font[glyph_index(ch)];
	for(int r = 0; r < 5; r++)
	for(int col = 0; col < 3; col++)
		if(bits & (1 << (14 - r*3 - col)))
			px_rect(x0 + col*scale, y0 + r*scale,
				x0 + col*scale + scale - 1,
				y0 + r*scale + scale - 1, c);
}

static void draw_text(int x, int y, const char *s, int scale, u8 c)
{
	for(int i = 0; s[i]; i++, x += 4*scale)
		draw_glyph(x, y, s[i], scale, c);
}

static void draw_number(int x, int y, int val, int scale, u8 c)
{
	if(val >= 10){
		draw_glyph(x, y, (char)('0' + val / 10), scale, c);
		draw_glyph(x + 4*scale, y, (char)('0' + val % 10), scale, c);
	}else
		draw_glyph(x, y, (char)('0' + val), scale, c);
}

static int score_right_x(int val)
{
	return 223 - ((val >= 10) ? 12 : 0);
}

/* diamond scanline bounds; corners (80,20) (240,100) (160,140) (0,60) */
static void court_span(int y, int *xl, int *xr)
{
	*xl = (y <= 60)  ? 80 - 2*(y - 20) : 2*(y - 60);
	*xr = (y <= 100) ? 80 + 2*(y - 20) : 240 - 2*(y - 100);
	if(*xr > 239) *xr = 239;
}

/* ---- court: ping-pong table in isometric view ---- */
static void draw_court(void)
{
	int xl, xr, fl, fr;

	px_rect(0, 0, 239, 159, CLR_BG);

	/* table frame / apron (expanded shell behind felt) */
	for(int y = 18; y <= 144; y++){
		court_span(y, &xl, &xr);
		if(xl > xr) continue;
		fl = xl - 3; if(fl < 0) fl = 0;
		fr = xr + 3; if(fr > 239) fr = 239;
		px_hline(fl, fr, y, CLR_FRAME);
	}

	/* front apron: thickness below near corner (160,140) */
	for(int d = 1; d <= 5; d++)
		px_hline(160 - d, 160 + d, 140 + d, CLR_FRAME);

	/* green felt playing surface with subtle stripe texture */
	for(int y = 20; y <= 140; y++){
		court_span(y, &xl, &xr);
		if(xl > xr) continue;
		u8 c = ((y >> 2) & 1) ? CLR_FLOOR : CLR_FELT2;
		px_hline(xl, xr, y, c);
	}

	/* white boundary lines along table edges (2 px) */
	for(int y = 20; y <= 140; y++){
		court_span(y, &xl, &xr);
		if(xl > xr) continue;
		px_hline(xl, (xl + 1 < xr) ? xl + 1 : xr, y, CLR_LINE);
		px_hline((xr - 1 > xl) ? xr - 1 : xl, xr, y, CLR_LINE);
	}

	/* center net: world x=80 -> (160,60)..(80,100), 2 px wide */
	for(int i = 0; i <= 80; i++){
		px_dot(160 - i, 60 + i/2, CLR_NET);
		px_dot(159 - i, 60 + i/2, CLR_NET);
	}

	/* net posts at table center ends */
	px_rect(158, 58, 161, 61, CLR_LINE);
	px_rect(78, 98, 81, 101, CLR_LINE);
}

/* ---- sprite tile data, generated in code ---- */
static void tile4_px(int tid, int x, int y, u8 c)
{
	u32 *d = &OBJ_TILES[tid].data[y];
	*d = (*d & ~(0xFu << (x * 4))) | ((u32)c << (x * 4));
}

/* tw = tiles per sprite row (1D mapping) */
static void spr_px(int tid, int tw, int x, int y, u8 c)
{
	tile4_px(tid + (y >> 3) * tw + (x >> 3), x & 7, y & 7, c);
}

static void build_tiles(void)
{
	/* VRAM ignores 8-bit writes; must clear with 32-bit stores */
	memset32(&OBJ_TILES[TID_BALL], 0, 10 * sizeof(TILE) / 4);

	/* ball: 8x8 shaded circle */
	for(int y = 0; y < 8; y++)
	for(int x = 0; x < 8; x++){
		int dx = 2*x - 7, dy = 2*y - 7;
		if(dx*dx + dy*dy <= 49)
			spr_px(TID_BALL, 1, x, y, (dx + dy < 0) ? 1 : 2);
	}

	/* shadow: flat ellipse */
	for(int y = 0; y < 8; y++)
	for(int x = 0; x < 8; x++){
		int dx = 2*x - 7, dy = (2*y - 7) * 2;
		if(dx*dx + dy*dy <= 49)
			spr_px(TID_SHADOW, 1, x, y, 3);
	}

	/* paddle: 32x16 ping-pong racket (oval head + wood handle) */
	for(int y = 0; y < 16; y++)
	for(int x = 0; x < 32; x++){
		u8 c = 0;
		int dx = x - 15, dy = y - 5;

		/* handle stub toward bottom of sprite */
		if(y >= 10 && x >= 14 && x <= 17)
			c = (x == 14 || x == 17 || y >= 14) ? 7 : 6;
		/* rubber face: wide oval */
		else if(dx*dx * 4 + dy*dy <= 11 * 11){
			c = (dx*dx * 4 + dy*dy <= 5 * 5) ? 5 : 4;
			if(dx*dx * 4 + dy*dy >= 9 * 9) c = 7;
		}
		if(c) spr_px(TID_PAD, 4, x, y, c);
	}
}

void render_init(void)
{
	REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

	pal_bg_mem[CLR_BG]    = RGB15(3, 5, 8);
	pal_bg_mem[CLR_FLOOR] = RGB15(8, 24, 10);
	pal_bg_mem[CLR_FELT2] = RGB15(6, 20, 8);
	pal_bg_mem[CLR_FRAME] = RGB15(10, 8, 6);
	pal_bg_mem[CLR_LINE]  = RGB15(28, 28, 26);
	pal_bg_mem[CLR_NET]   = RGB15(24, 24, 22);
	pal_bg_mem[CLR_TEXT]  = RGB15(31, 31, 31);

	/* obj palette bank 0: ball, shadow, player paddle (dark rubber) */
	pal_obj_mem[1] = RGB15(31, 31, 31);
	pal_obj_mem[2] = RGB15(20, 20, 22);
	pal_obj_mem[3] = RGB15(1, 2, 3);
	pal_obj_mem[4] = RGB15(6, 8, 12);
	pal_obj_mem[5] = RGB15(14, 16, 20);
	pal_obj_mem[6] = RGB15(22, 14, 6);
	pal_obj_mem[7] = RGB15(12, 7, 3);
	/* bank 1: cpu paddle (red rubber) */
	pal_obj_mem[16 + 4] = RGB15(26, 6, 4);
	pal_obj_mem[16 + 5] = RGB15(31, 14, 10);
	pal_obj_mem[16 + 6] = RGB15(22, 14, 6);
	pal_obj_mem[16 + 7] = RGB15(12, 7, 3);

	oam_init(oam_mem, 128);
	draw_court();
	build_tiles();

	/* 0: ball 8x8, 1: shadow 8x8 (behind ball by OAM order) */
	obj_buf[0].attr0 = 0;
	obj_buf[0].attr1 = 0;
	obj_buf[0].attr2 = TID_BALL;
	obj_buf[1] = obj_buf[0];
	obj_buf[1].attr2 = TID_SHADOW;
	/* 2: player paddle 32x16 (wide, size 2), 3: cpu paddle */
	obj_buf[2].attr0 = ATTR0_WIDE;
	obj_buf[2].attr1 = 2 << 14;
	obj_buf[2].attr2 = TID_PAD;
	obj_buf[3] = obj_buf[2];
	obj_buf[3].attr1 |= ATTR1_HFLIP;
	obj_buf[3].attr2 = TID_PAD | ATTR2_PALBANK(1);

	oam_copy(oam_mem, obj_buf, 4);
}

void render_game(const Game *g)
{
	int sx, sy;

	iso_project(g->ball.x, g->ball.y, g->ball.z, &sx, &sy);
	obj_set_pos(&obj_buf[0], sx - 4, sy - 4);

	iso_project(g->ball.x, g->ball.y, 0, &sx, &sy);
	obj_set_pos(&obj_buf[1], sx - 4, sy - 2);

	iso_project(COURT_W, g->pads[0].y, 0, &sx, &sy);
	obj_set_pos(&obj_buf[2], sx - 16, sy - 10);

	iso_project(0, g->pads[1].y, 0, &sx, &sy);
	obj_set_pos(&obj_buf[3], sx - 16, sy - 10);

	oam_copy(oam_mem, obj_buf, 4);
}

void render_score(int cpu, int player)
{
	px_rect(8, 4, 32, 18, CLR_BG);
	px_rect(score_right_x(player) - 1, 4, 231, 18, CLR_BG);
	draw_number(8, 4, cpu, 3, CLR_TEXT);
	draw_number(score_right_x(player), 4, player, 3, CLR_TEXT);
}

void render_msg(const char *s)
{
	px_rect(0, 144, 239, 159, CLR_BG);
	int len = 0;
	while(s[len]) len++;
	if(!len) return;
	int x = (240 - len * 8) / 2;
	if(x < 0) x = 0;
	for(int i = 0; i < len; i++, x += 8)
		draw_glyph(x, 146, s[i], 2, CLR_TEXT);
}

void render_court(void)
{
	draw_court();
}

static int menu_row_y(int row)
{
	return 56 + row * 16;
}

static void draw_menu_value(const Config *cfg, int row)
{
	static const char *const diff[3]  = { "EASY", "NORMAL", "HARD" };
	static const char *const score[3] = { "5", "7", "11" };
	int y = menu_row_y(row);

	if(row == MROW_START) return;
	px_rect(140, y, 207, y + 10, CLR_BG);
	switch(row){
	case MROW_DIFF:  draw_text(140, y, diff[cfg->difficulty], 2, CLR_TEXT); break;
	case MROW_SCORE: draw_text(140, y, score[cfg->win_score_idx], 2, CLR_TEXT); break;
	}
}

void render_menu(const Config *cfg, int row)
{
	draw_court();
	px_rect(32, 20, 207, 122, CLR_BG);	/* panel so text reads over floor */
	draw_text(88, 26, "MINIPONG", 2, CLR_TEXT);
	draw_text(48,  56, "DIFFICULTY", 2, CLR_TEXT);
	draw_text(48,  72, "WIN SCORE", 2, CLR_TEXT);
	draw_text(48,  88, "START GAME", 2, CLR_TEXT);
	draw_menu_value(cfg, MROW_DIFF);
	draw_menu_value(cfg, MROW_SCORE);
	draw_glyph(38, menu_row_y(row), '>', 2, CLR_TEXT);
}

void render_menu_cursor(int row, int show)
{
	int y = menu_row_y(row);
	px_rect(38, y, 45, y + 10, CLR_BG);
	if(show)
		draw_glyph(38, y, '>', 2, CLR_TEXT);
}

void render_menu_value(const Config *cfg, int row)
{
	draw_menu_value(cfg, row);
}
