#ifndef ISO_H
#define ISO_H

/* 24.8 fixed point in plain int (32-bit on ARM7TDMI and host) */
#define FX_SHIFT	8
#define FX(n)		((n) << FX_SHIFT)
#define FX2I(n)		((n) >> FX_SHIFT)

/* screen offsets so the 160x80 court fits 240x160 */
#define ISO_CX		80
#define ISO_CY		20

/* world (x,y,z) fixed-point -> screen pixels (2:1 isometric) */
void iso_project(int x, int y, int z, int *sx, int *sy);

#endif
