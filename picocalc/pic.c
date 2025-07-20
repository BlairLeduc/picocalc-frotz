//
// pic.c - PicoCalc interface, picture outline functions
//

#include "picocalc_frotz.h"

bool pc_init_pictures(void)
{
	return FALSE;
}

bool os_picture_data(int num, int *height, int *width)
{
	return TRUE;
}

void os_draw_picture(int num, int row, int col)
{
	// No pictures in this port.
}

int os_peek_colour(void)
{
	return BLACK_COLOUR;
}
