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
void conioxlib_mouseinit(void)
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


	conioxlib_mouseinit();
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



#endif
