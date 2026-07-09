#include <assert.h>
#include <stdio.h>
#include "iso.h"

int main(void)
{
	int sx, sy;

	/* four court corners (world 160x80, z=0) */
	iso_project(0, 0, 0, &sx, &sy);
	assert(sx == 80 && sy == 20);
	iso_project(FX(160), 0, 0, &sx, &sy);
	assert(sx == 240 && sy == 100);
	iso_project(0, FX(80), 0, &sx, &sy);
	assert(sx == 0 && sy == 60);
	iso_project(FX(160), FX(80), 0, &sx, &sy);
	assert(sx == 160 && sy == 140);

	/* height moves the point straight up on screen */
	iso_project(FX(80), FX(40), FX(10), &sx, &sy);
	assert(sx == 120 && sy == 70);

	/* FX round-trip */
	assert(FX2I(FX(37)) == 37);

	puts("test_iso OK");
	return 0;
}
