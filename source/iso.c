#include "iso.h"

void iso_project(int x, int y, int z, int *sx, int *sy)
{
	*sx = FX2I(x - y) + ISO_CX;
	*sy = FX2I((x + y) / 2 - z) + ISO_CY;
}
