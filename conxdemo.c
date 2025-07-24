/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "coniox.h"
#include "conxlib.h"




/* ----------------------------------------------------------------------------------------------------------------- */
void background (unsigned char msg[]);



/* ----------------------------------------------------------------------------------------------------------------- */
void main(void)
{
	int i;


	coniox_init(NULL);
	_setcursortype(_NOCURSOR);

	background("CONXLIB DEMO 2.0               (c) Copyright 1997-2025 Javier Gutierrez Chamorro");
	conioxlib_box(3, 3, 74, 19, 1 * 16 + 1, CONIOXLIB_BOX_A_DOUBLE, "Window", CONIOXLIB_BOX_SHADOW | CONIOXLIB_BOX_CLOSE);

	for (i = 0; i < 10000; i++)
	{
		window(3, 3, 74, 19);
		conioxlib_pollmouse();
		gotoxy(1, 1);
		cprintf("x: %03u\r\n", conioxlib_mousestatus.x);
		cprintf("y: %03u\r\n", conioxlib_mousestatus.y);
		cprintf("lclick: %01u\r\n", conioxlib_mousestatus.lclick);
		cprintf("rclick: %01u\r\n", conioxlib_mousestatus.rclick);
		cprintf("wheelx: %03u\r\n", conioxlib_mousestatus.wheelx);
		cprintf("wheely: %03u\r\n", conioxlib_mousestatus.wheely);
		window(1, 1, 80, 25);
		conioxlib_blocks();
		conioxlib_watch();
	}
}







/* --------------- Dibuja en pantalla la ventana del about ---------------- */
void background (unsigned char msg[])
{
	unsigned int i;
	unsigned short *oldb, *olda;

	oldb = malloc(4000);
	if (oldb == 0)
	{
		return;
	}
	olda = oldb;

	for (i=0; i!=2000-160; i++)
	{
		*olda = 0x13B0;
		olda++;
	}
	
	puttext(1, 2, 80, 24, oldb);
	
	if (msg[0])
	{
		textattr(7*16+0);
		gotoxy(1, 1);
		cputs(msg);
		olda=oldb;
		for (i=0; i!=80; i++)
		{
			*olda=0x7020;
			olda++;
		}
		puttext(1, 25, 80, 25, oldb);
	}
	free(oldb);
}


