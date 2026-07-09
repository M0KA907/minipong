#include <tonc.h>
#include "iso.h"
#include "game.h"
#include "render.h"

/* ---- bg palette indices ---- */
#define CLR_BG		0
#define CLR_FLOOR	1
#define CLR_EDGE	2
#define CLR_MID		3
#define CLR_TEXT	4

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

/* ---- court: diamond with corners (80,20) (240,100) (160,140) (0,60) ---- */
static void draw_court(void)
{
	px_rect(0, 0, 239, 159, CLR_BG);
	for(int y = 20; y <= 140; y++){
		int xl = (y <= 60)  ? 80 - 2*(y - 20) : 2*(y - 60);
		int xr = (y <= 100) ? 80 + 2*(y - 20) : 240 - 2*(y - 100);
		if(xr > 239) xr = 239;
		if(xl > xr) continue;
		px_hline(xl, xr, y, CLR_FLOOR);
		px_hline(xl, (xl+1 < xr) ? xl+1 : xr, y, CLR_EDGE);
		px_hline((xr-1 > xl) ? xr-1 : xl, xr, y, CLR_EDGE);
	}
	/* center line: world x=80, y=0..80 -> (160,60)..(80,100) */
	for(int i = 0; i <= 80; i++)
		px_dot(160 - i, 60 + i/2, CLR_MID);
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

	/* paddle: 32x16 iso slab along +y direction (-1,+1/2) */
	for(int i = 0; i < 24; i++){
		int x = 27 - i, y = i / 2;
		for(int t = 0; t < 5; t++)
			spr_px(TID_PAD, 4, x, y + t, (t < 2) ? 4 : 5);
	}
}

void render_init(void)
{
	REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

	pal_bg_mem[CLR_BG]    = RGB15(2, 3, 5);
	pal_bg_mem[CLR_FLOOR] = RGB15(5, 12, 8);
	pal_bg_mem[CLR_EDGE]  = RGB15(24, 28, 24);
	pal_bg_mem[CLR_MID]   = RGB15(12, 20, 14);
	pal_bg_mem[CLR_TEXT]  = RGB15(31, 31, 31);

	/* obj palette bank 0: ball, shadow, player paddle (blue) */
	pal_obj_mem[1] = RGB15(31, 31, 31);
	pal_obj_mem[2] = RGB15(20, 20, 22);
	pal_obj_mem[3] = RGB15(1, 2, 3);
	pal_obj_mem[4] = RGB15(10, 18, 31);
	pal_obj_mem[5] = RGB15(4, 8, 18);
	/* bank 1: cpu paddle (red) */
	pal_obj_mem[16 + 4] = RGB15(31, 12, 10);
	pal_obj_mem[16 + 5] = RGB15(18, 5, 4);

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
	px_rect(8, 4, 16, 18, CLR_BG);
	px_rect(223, 4, 231, 18, CLR_BG);
	draw_glyph(8, 4, (char)('0' + cpu), 3, CLR_TEXT);
	draw_glyph(223, 4, (char)('0' + player), 3, CLR_TEXT);
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

void render_menu(const Config *cfg, int row)
{
	static const char *const diff[3]  = { "EASY", "NORMAL", "HARD" };
	static const char *const score[3] = { "5", "7", "11" };
	static const char *const ctrl[2]  = { "L-R", "U-D" };

	draw_court();
	px_rect(32, 20, 207, 122, CLR_BG);	/* panel so text reads over floor */
	draw_text(88, 26, "MINIPONG", 2, CLR_TEXT);
	draw_text(48,  56, "DIFFICULTY", 2, CLR_TEXT);
	draw_text(48,  72, "WIN SCORE", 2, CLR_TEXT);
	draw_text(48,  88, "CONTROLS", 2, CLR_TEXT);
	draw_text(48, 104, "START GAME", 2, CLR_TEXT);
	draw_text(140, 56, diff[cfg->difficulty], 2, CLR_TEXT);
	draw_text(140, 72, score[cfg->win_score_idx], 2, CLR_TEXT);
	draw_text(140, 88, ctrl[cfg->controls], 2, CLR_TEXT);
	draw_glyph(38, 56 + row*16, '>', 2, CLR_TEXT);
}
