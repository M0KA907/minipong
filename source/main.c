#include <tonc.h>

int main(void)
{
	REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
	m3_plot(120, 80, CLR_LIME);
	while(1);
}
