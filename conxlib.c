#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <time.h>
#include "conxlib.h"


#if defined(__WATCOMC__)
	#include <i86.h>
#endif


struct conioxlib_smousestatus conioxlib_mousestatus = {0};


/* ----------------------- Escribe un texto centrado ---------------------- */
void conioxlib_cputsc(unsigned int x1, unsigned int x2, unsigned int y, char* str)
{
	unsigned int x;

	x = (((x2 - x1 + 1) >> 1) - (strlen(str) >> 1) + x1);
	gotoxy(x, y);
	cputs(str);
}



/* -------------------------- Dibuja una ventana --------------------------- */
void conioxlib_box(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned int atr, char* tipo, char* titulo, unsigned int sombra)
{
	unsigned int x, y;
	unsigned int c1, c2;
	char l1[82], l2[82], l3[82], tmp[330];

	/* Shadow */
	if (sombra & 1)
	{
		gettext(x1 + 2, y2 + 1, x2 + 2, y2 + 1, tmp);
		c2 = (((x2 + 2) - (x1 + 2) + 1) * ((y2 + 1) - (y2 + 1) + 1)) << 1;
		for (c1 = 1; c1 <= c2; c1 += 2)
		{
			tmp[c1] = 8;
		}
		puttext(x1 + 2, y2 + 1, x2 + 2, y2 + 1, tmp);
		gettext(x2 + 1, y1 + 1, x2 + 2, y2 + 1, tmp);
		c2 = (((x2 + 2) - (x2 + 1) + 1) * ((y2 + 1) - (y1 + 1) + 1)) << 1;
		for (c1 = 1; c1 <= c2; c1 += 2)
		{
			tmp[c1] = 8;
		}
		puttext(x2 + 1, y1 + 1, x2 + 2, y2 + 1, tmp);
	}
	/* No window */
	if ((sombra & 4) == 0)
	{
		x = (x2 - x1);
		memset(&l1, 0, x + 2);
		memset(&l2, 0, x + 2);
		memset(&l3, 0, x + 2);
		l1[0] = tipo[0];
		memset(&l1[1], tipo[2], x - 1);
		l1[x] = tipo[1];
		l2[0] = tipo[4];
		memset(&l2[1], 32, x - 1);
		l2[x] = tipo[4];
		l3[0] = tipo[5];
		memset(&l3[1], tipo[3], x - 1);
		l3[x] = tipo[6];
		textattr(atr);
		gotoxy(x1, y1);
		cputs(l1);
		if (titulo[0] != 0)
		{
			sprintf(tmp, " %s ", titulo);
			conioxlib_cputsc(x1, x2, y1, tmp);
		}
		/* Close */
		if (sombra & 2)
		{
			gotoxy(x1 + 2, y1);
			cputs("[þ]");
		}
		for (y = y1 + 1; y != y2; y++)
		{
			gotoxy(x1, y);
			cputs(l2);
		}
		gotoxy(x1, y2);
		cputs(l3);
	}
}


/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void conioxlib_watch(void)
{
	unsigned int oldx, oldy;
	time_t t;
	struct tm* ti;

	oldx = wherex();
	oldy = wherey();

	time(&t);
	ti = localtime(&t);

	gotoxy(62, 25);
	textattr(7 * 16 + 0);
	cprintf("%2u:%02u", ti->tm_hour, ti->tm_min);
	gotoxy(oldx, oldy);
}




#if ((__WIN32__) || (__WINDOWS__) || (__NT__) || (_WIN32))


/* ----------------------------------------------------------------------------------------------------------------- */
void conioxlib_pollmouse(void)
{
	HANDLE hStdInput, hStdOutput;
	INPUT_RECORD ir[128];
	DWORD nRead;
	UINT i;
	short w;

	//memset(&conioxlib_mousestatus, 0, sizeof(conioxlib_mousestatus));
	hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	FlushConsoleInputBuffer(hStdInput);
	SetConsoleMode(hStdInput, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);


	ReadConsoleInput(hStdInput, ir, 128, &nRead);
	for (i = 0; i < nRead; i++)
	{
		if (ir[i].EventType == MOUSE_EVENT)
		{
			conioxlib_mousestatus.x = ir[i].Event.MouseEvent.dwMousePosition.X;
			conioxlib_mousestatus.y = ir[i].Event.MouseEvent.dwMousePosition.Y;

			if (ir[i].Event.MouseEvent.dwEventFlags & MOUSE_WHEELED)
			{
				w = HIWORD(ir[i].Event.MouseEvent.dwButtonState);
				w /= 120;
				w = -w;
				conioxlib_mousestatus.wheely = w;
			}
			else if (ir[i].Event.MouseEvent.dwEventFlags & MOUSE_HWHEELED)
			{
				w = HIWORD(ir[i].Event.MouseEvent.dwButtonState);
				w /= 120;
				w = -w;
				conioxlib_mousestatus.wheelx = w;
			}
			if (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
			{
				if (!ir[i].Event.MouseEvent.dwEventFlags & DOUBLE_CLICK)
				{
					conioxlib_mousestatus.lclick = 1;
				}
				else
				{
					conioxlib_mousestatus.lclick = 2;
				}
			}
			else if (ir[i].Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
			{
				if (!ir[i].Event.MouseEvent.dwEventFlags & DOUBLE_CLICK)
				{
					conioxlib_mousestatus.rclick = 1;
				}
				else
				{
					conioxlib_mousestatus.rclick = 2;
				}
			}
		}
	}
}


/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void conioxlib_blocks(void)
{
	SHORT fun;
	unsigned int oldx, oldy;
	char status[17];

	oldx = wherex();
	oldy = wherey();

	memset(status, 32, sizeof(status));
	status[sizeof(status) - 1] = 0;

	if (GetKeyState(VK_INSERT))
	{
		memcpy(&status[0], "INS", 3);
	}
	if (GetKeyState(VK_CAPITAL))
	{
		memcpy(&status[4], "CAP", 3);
	}
	if (GetKeyState(VK_NUMLOCK))

	{
		memcpy(&status[8], "NUM", 3);
	}
	if (GetKeyState(VK_SCROLL))
	{
		memcpy(&status[12], "SCR", 3);
	}

	textattr(7 * 16 + 0);
	gotoxy(64, 25);
	_wscroll = 0;
	cputs(status);
	gotoxy(oldx, oldy);
}


#endif


#if ((__DOS__) || (__MSDOS__))
int conioxlib_ismouse = -1;

/* ----------------------------------------------------------------------------------------------------------------- */
#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
	#define coniox_int86 int386
	#define coniox_int86x int386x
#else
	#define coniox_int86 int86
	#define coniox_int86x int86x
#endif


/* ----------------------------------------------------------------------------------------------------------------- */
void _conioxlib_mouseinit(void)
{
	union REGS r;

	if (conioxlib_ismouse == -1)
	{
		#if defined(__WATCOMC__)
			r.w.ax = 0;
			coniox_int86(0x33, &r, &r);
			if (r.w.ax == 0xFFFF)
			{
				conioxlib_ismouse = r.w.bx;
				/* Show mouse */
				r.w.ax = 1;
				coniox_int86(0x33, &r, &r);
			}
		#else
			r.w.ax = 0;
			coniox_int86(0x33, &r, &r);
			if (r.w.ax == 0xFFFF)
			{
				conioxlib_ismouse = r.w.bx;
				/* Show mouse */
				r.w.ax = 1;
				coniox_int86(0x33, &r, &r);
			}
		#endif
		else
		{
			conioxlib_ismouse = 0;
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void conioxlib_pollmouse(void)
{
	union REGS r;


	_conioxlib_mouseinit();
	if (conioxlib_ismouse > 0)
	{
		#if defined(__WATCOMC__)
			r.w.ax = 3;
			coniox_int86(0x33, &r, &r);
			conioxlib_mousestatus.x = (r.w.cx >> 3) + 1;
			conioxlib_mousestatus.y = (r.w.dx >> 3) + 1;
		#else	
			r.x.ax = 3;
			coniox_int86(0x33, &r, &r);
			conioxlib_mousestatus.x = (r.x.cx >> 3) + 1;
			conioxlib_mousestatus.y = (r.x.dx >> 3) + 1;
		#endif

		conioxlib_mousestatus.lclick = (unsigned int) (r.h.bl & 1);
		conioxlib_mousestatus.rclick = (unsigned int) (r.h.bl & 3) >> 1;
		conioxlib_mousestatus.wheely = (int) r.h.bh;

		if ((conioxlib_mousestatus.lclick) || (conioxlib_mousestatus.rclick))
		{
			#if defined(__WATCOMC__)
				r.w.ax = 5;
			#else
				r.x.ax = 5;
			#endif	
			if (conioxlib_mousestatus.lclick)
			{
				#if defined(__WATCOMC__)
					r.w.bx = 0;
					coniox_int86(0x33, &r, &r);
					conioxlib_mousestatus.lclick += r.w.bx;
				#else
					r.x.bx = 0;
					coniox_int86(0x33, &r, &r);
					conioxlib_mousestatus.lclick += r.x.bx;
				#endif	
			}
			else if (conioxlib_mousestatus.rclick)
			{
				#if defined(__WATCOMC__)
					r.w.bx = 1;
					coniox_int86(0x33, &r, &r);
					conioxlib_mousestatus.rclick += r.w.bx;
				#else
					r.x.bx = 1;
					coniox_int86(0x33, &r, &r);
					conioxlib_mousestatus.rclick += r.x.bx;
				#endif
			}
		}
	}
}



/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void conioxlib_blocks(void)
{
	unsigned char fun;
	unsigned int oldx, oldy;
	char status[17];

	oldx = wherex();
	oldy = wherey();

	memset(status, 32, sizeof(status));
	status[sizeof(status) - 1] = 0;

	fun = peekb(0x0040, 0x0017);

	if (fun & 128)
	{
		memcpy(&status[0], "INS", 3);
	}
	if (fun & 64)
	{
		memcpy(&status[4], "CAP", 3);
	}
	if (fun & 32)
	{
		memcpy(&status[8], "NUM", 3);
	}
	if (fun & 16)
	{
		memcpy(&status[12], "SCR", 3);
	}

	textattr(7 * 16 + 0);
	gotoxy(64, 25);
	_wscroll = 0;
	cputs(status);
	gotoxy(oldx, oldy);
}



#endif
