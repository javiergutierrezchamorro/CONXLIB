#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "conxlib.h"

#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	#include <windows.h>
	#include <wincon.h>
	#include <direct.h>
#endif

#if ((__DOS__) || (__MSDOS__))
	#include <dos.h>
	#if defined(__WATCOMC__)
		#include <i86.h>
		#include <direct.h>
	#else
		#include <dir.h>
	#endif
	#if defined(__DJGPP__)
		#include <go32.h>
		#include <dpmi.h>
		#include <sys/movedata.h>
	#endif
#endif

#if defined(_MSC_VER)
	#define stricmp	_stricmp
	#define strupr	_strupr
	#define strlwr	_strlwr
	#define strrev	_strrev
#endif

/* int86 and registers: Watcom uses r.w, the rest use r.x */
#if ((__DOS__) || (__MSDOS__))
	#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
		#define coniox_int86	int386
		#define coniox_int86x	int386x
	#else
		#define coniox_int86	int86
		#define coniox_int86x	int86x
	#endif
	#if defined(__WATCOMC__)
		#define conioxlib_rax(r)	(r).w.ax
		#define conioxlib_rbx(r)	(r).w.bx
		#define conioxlib_rcx(r)	(r).w.cx
		#define conioxlib_rdx(r)	(r).w.dx
		#define conioxlib_rdi(r)	(r).w.di
		#define conioxlib_rcflag(r)	(r).w.cflag
		#define conioxlib_getdisk()		(_getdrive() - 1)
		#define conioxlib_setdisk(d)	_chdrive((d) + 1)
		#define conioxlib_getcwd(p, n)	getcwd(p, n)
	#else
		#define conioxlib_rax(r)	(r).x.ax
		#define conioxlib_rbx(r)	(r).x.bx
		#define conioxlib_rcx(r)	(r).x.cx
		#define conioxlib_rdx(r)	(r).x.dx
		#define conioxlib_rdi(r)	(r).x.di
		#define conioxlib_rcflag(r)	(r).x.cflag
		#define conioxlib_getdisk()		getdisk()
		#define conioxlib_setdisk(d)	setdisk(d)
		#define conioxlib_getcwd(p, n)	getcwd(p, n)
	#endif
	#define conioxlib_chdir(p)	chdir(p)
	#if defined(__WATCOMC__)
		#define conioxlib_ffblk			struct find_t
		#define conioxlib_findfirst(n, b)	_dos_findfirst(n, _A_NORMAL | _A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_SUBDIR | _A_ARCH, b)
		#define conioxlib_findnext(b)		_dos_findnext(b)
		#define conioxlib_ffname(b)		((b).name)
		#define conioxlib_ffattrib(b)		((b).attrib)
		#define conioxlib_ffsize(b)		((b).size)
		#define conioxlib_ffdate(b)		((b).wr_date)
		#define conioxlib_fftime(b)		((b).wr_time)
	#else
		#define conioxlib_ffblk			struct ffblk
		#define conioxlib_findfirst(n, b)	findfirst(n, b, FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_DIREC | FA_ARCH)
		#define conioxlib_findnext(b)		findnext(b)
		#define conioxlib_ffname(b)		((b).ff_name)
		#define conioxlib_ffattrib(b)		((b).ff_attrib)
		#define conioxlib_ffsize(b)		((b).ff_fsize)
		#define conioxlib_ffdate(b)		((b).ff_fdate)
		#define conioxlib_fftime(b)		((b).ff_ftime)
	#endif
#endif

#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	#define conioxlib_getdisk()		(_getdrive() - 1)
	#define conioxlib_setdisk(d)	_chdrive((d) + 1)
	#define conioxlib_chdir(p)		_chdir(p)
	#define conioxlib_getcwd(p, n)	_getcwd(p, n)
#endif

struct conioxlib_smousestatus conioxlib_mousestatus = {0};

/* Active theme: kept near the top of the file because clock()/blocks()/status()
   and the conioxlib_hint() macro use it from the very first moment */
static struct conioxlib_stheme conioxlib_curtheme = {
	CONIOXLIB_ATTR(WHITE, BLUE),
	CONIOXLIB_ATTR(BLACK, GREEN),
	CONIOXLIB_ATTR(CYAN, LIGHTGRAY),
	CONIOXLIB_ATTR(BLACK, CYAN),
	CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
	CONIOXLIB_BOX_A_DOUBLE,
	"Borland C++ 3.1"
};

/* Pending key captured by conioxlib_pollmouse() under Windows */
static int conioxlib_pendingkey = 0;

static int conioxlib_filemultiselector_sort(const void *a, const void *b);
static void conioxlib_filemultiselector_format(char *res, long int n);

/* Reads a portable key: extended keys are returned as (scan << 8) and the rest as their ASCII code */
#define conioxlib_readkey(t)	{ if (conioxlib_pendingkey) { t = conioxlib_pendingkey; conioxlib_pendingkey = 0; } else { int conioxlib_k = getch(); if (conioxlib_k == 0) { t = ((int) getch()) << 8; } else { t = conioxlib_k; } } }
#define conioxlib_keyhit()		(conioxlib_pendingkey || kbhit())

/* Writes a help description on the status line (columns 1-59) with
   the color of the active theme; used by menu/menubar/buttons/option/property */
#define conioxlib_hint(d)	do { textattr(conioxlib_curtheme.atrstatus); _wscroll = 0; gotoxy(1, 25); cprintf("%-59.59s", (d)); } while (0)

/* ------------------------- Checks for a pending key --------------------- */
/* kbhit:
   Prototype:
   int conioxlib_kbhit(void);
   Returns nonzero when a key is waiting.
   Parameters: none.
   It sees both the keyboard buffer and the pending key kept
   by conioxlib_pollmouse() under Windows, so it is the safe
   replacement for the raw kbhit().
   */
int conioxlib_kbhit(void)
{
	return(conioxlib_keyhit());
}

/* ---------------------------- Reads a key ------------------------------ */
/* getkey:
   Prototype:
   unsigned int conioxlib_getkey(void);
   Reads the next key from the keyboard.
   Parameters: none.
   Plain keys arrive as their ASCII code and extended keys as
   (scan code << 8), so the KBD_ constants can be compared
   directly. It also drains the pending key captured by
   conioxlib_pollmouse() under Windows.
   */
unsigned int conioxlib_getkey(void)
{
	unsigned int t;
	conioxlib_readkey(t);
	return(t);
}


/* ------------------- Finds the accelerator key of a text --------------- */
/* Accelerator: an ampersand before X marks X as the accelerator key and is never
   drawn; with no ampersand, the first letter A-Z/a-z is the accelerator key. */
static char conioxlib_hotchar(const char *s)
{
	int i;

	if (s == 0)
	{
		return(0);
	}
	for (i = 0; s[i] != 0; i++)
	{
		if (s[i] == '&' && s[i + 1] != 0)
		{
			return(s[i + 1]);
		}
	}
	for (i = 0; s[i] != 0; i++)
	{
		if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))
		{
			return(s[i]);
		}
	}
	return(0);
}

/* ---------------- Writes text with the accelerator key highlighted ----------- */
/* cputs_hot:
   Prototype:
   void conioxlib_cputs_hot(unsigned short x,
       unsigned short y, unsigned char atr,
       unsigned char atrhot, const char *s);
   Writes s at (x,y) highlighting its accelerator key.
   Parameters: atr is the base color and atrhot the color of
   the highlighted letter. An ampersand before a letter
   marks it as the accelerator and is never drawn; with no
   ampersand the first letter is the accelerator.
   */
void conioxlib_cputs_hot(unsigned short x, unsigned short y, unsigned char atr, unsigned char atrhot, const char *s)
{
	int i;
	unsigned char hotatr;
	char h;

	if (s == 0)
	{
		return;
	}
	/* atrhot supplies only the FG of the accelerator; BG comes from the current item attr (normal or selected) */
	hotatr = (unsigned char) ((atr & 0xF0) | (atrhot & 0x0F));
	h = conioxlib_hotchar(s);
	gotoxy(x, y);
	for (i = 0; s[i] != 0; i++)
	{
		if (s[i] == '&')
		{
			continue;
		}
		if (h != 0 && s[i] == h)
		{
			textattr(hotatr);
			putch(s[i]);
			textattr(atr);
			h = 0;
		}
		else
		{
			textattr(atr);
			putch(s[i]);
		}
	}
}

/* ---------------- Checks whether a key triggers an option --------------- */
/* Accepts a plain letter and Alt+letter (BIOS scan << 8). The Alt A-Z map is
   the standard US/ES QWERTY keyboard layout. */
static int conioxlib_hotmatch(const char *s, unsigned int key)
{
	static const unsigned char altscan[26] = {
		0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
		0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
		0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
	};
	char h;
	unsigned int k;
	unsigned int scan;
	int i;

	h = conioxlib_hotchar(s);
	if (h == 0)
	{
		return(0);
	}
	if (h >= 'a' && h <= 'z')
	{
		h = (char) (h - 'a' + 'A');
	}
	if (key <= 127)
	{
		k = key;
		if (k >= 'a' && k <= 'z')
		{
			k = k - 'a' + 'A';
		}
		return(k == (unsigned int) (unsigned char) h);
	}
	/* Alt+letter: key = (scan << 8), ASCII 0 */
	if ((key & 0xFF) != 0)
	{
		return(0);
	}
	scan = (key >> 8) & 0xFF;
	for (i = 0; i < 26; i++)
	{
		if (altscan[i] == (unsigned char) scan)
		{
			return(('A' + i) == (unsigned int) (unsigned char) h);
		}
	}
	return(0);
}



/* ----------------------- Writes centered text ---------------------- */
/* cputsc:
   Prototype:
   void conioxlib_cputsc(unsigned short x1, unsigned short x2,
       unsigned short y, const char *str);
   Writes str centered between columns x1 and x2 on row y.
   Parameters: x1 and x2 define the centering range, y is the
   destination row and str the text to write.
   */
void conioxlib_cputsc (unsigned short x1, unsigned short x2, unsigned short y, const char *str)
{
	unsigned int x;

	if ((str == 0) || (str[0] == 0))
	{
		return;
	}
	x = (((x2 - x1 + 1) >> 1) - (strlen(str) >> 1) + x1);
	gotoxy(x, y);
	cputs((char *) str);
}


/* -------------------------- Draws a window --------------------------- */
/* box:
   Prototype:
   void conioxlib_box(unsigned short x1, unsigned short y1,
       unsigned short x2, unsigned short y2, unsigned int atr,
       const char *frame, const char *title,
       unsigned int flags);
   Draws a framed window from (x1,y1) to (x2,y2).
   Parameters: atr is the frame color; frame is one of A_NONE,
   A_BLOCK, A_SINGLE, A_DOUBLE, S_SINGLE, S_DOUBLE, U_SINGLE
   or U_DOUBLE; title is painted centered on the top border;
   flags combines SHADOW, CLOSE and NOWINDOW.
   */
void conioxlib_box (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned int atr, const char *frame, const char *title, unsigned int flags)
{
	unsigned int x, y;
	unsigned int c1, c2;
	unsigned short sx1, sx2, sy1, sy2;
	char l1[82], l2[82], l3[82], tmp[330];

	if ((frame == 0) || (x2 < x1) || (y2 < y1))
	{
		return;
	}
	/* Shadow: clamp to the visible 80x25 console so gettext/puttext stay valid */
	if (flags & 1)
	{
		if (y2 < 25)
		{
			sx1 = (unsigned short) (x1 + 2);
			sx2 = (unsigned short) (x2 + 2);
			if (sx1 < 1)
			{
				sx1 = 1;
			}
			if (sx2 > 80)
			{
				sx2 = 80;
			}
			if (sx1 <= sx2)
			{
				gettext(sx1, y2 + 1, sx2, y2 + 1, tmp);
				c2 = (unsigned int) ((sx2 - sx1 + 1) << 1);
				for (c1 = 1; c1 <= c2; c1 += 2)
				{
					tmp[c1] = 8;
				}
				puttext(sx1, y2 + 1, sx2, y2 + 1, tmp);
			}
		}
		if (x2 < 80)
		{
			sx1 = (unsigned short) (x2 + 1);
			sx2 = (unsigned short) (x2 + 2);
			sy1 = (unsigned short) (y1 + 1);
			sy2 = (unsigned short) (y2 + 1);
			if (sx2 > 80)
			{
				sx2 = 80;
			}
			if (sy2 > 25)
			{
				sy2 = 25;
			}
			if ((sx1 <= sx2) && (sy1 <= sy2))
			{
				gettext(sx1, sy1, sx2, sy2, tmp);
				c2 = (unsigned int) (((sx2 - sx1 + 1) * (sy2 - sy1 + 1)) << 1);
				for (c1 = 1; c1 <= c2; c1 += 2)
				{
					tmp[c1] = 8;
				}
				puttext(sx1, sy1, sx2, sy2, tmp);
			}
		}
	}
	/* No window */
	if ((flags & 4) == 0)
	{
		x = (x2 - x1);
		if (x + 2 > sizeof(l1))
		{
			x = (unsigned int) (sizeof(l1) - 2);
		}
		memset(&l1, 0, x + 2);
		memset(&l2, 0, x + 2);
		memset(&l3, 0, x + 2);
		l1[0] = frame[0];
		if (x > 0)
		{
			memset(&l1[1], frame[2], x - 1);
			memset(&l2[1], 32, x - 1);
			memset(&l3[1], frame[3], x - 1);
		}
		l1[x] = frame[1];
		l2[0] = frame[4];
		l2[x] = frame[4];
		l3[0] = frame[5];
		l3[x] = frame[6];
		textattr(atr);
		gotoxy(x1, y1);
		cputs(l1);
		if ((title != 0) && (title[0] != 0))
		{
			sprintf(tmp, " %s ", title);
			conioxlib_cputsc(x1, x2, y1, tmp);
		}
		/* Close */
		if (flags & 2)
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


/* ------------------- Paints the clock cells of line 25 ------------------- */
/* Writes the time in columns 76 to 80 with the status color of the active
   theme straight through puttext(), so neither the window nor the cursor
   move; conioxlib_clock() and conioxlib_background() share it. */
static void conioxlib_paintclock (struct tm *t)
{
	char clk[8];
	unsigned short cells[5];
	unsigned short cellattr;
	unsigned int ci;

	/* Clock in columns 76-80; the lock indicators occupy 60-74 and status() 1-59 */
	sprintf(clk, "%2u:%02u", (unsigned int) t->tm_hour, (unsigned int) t->tm_min);
	cellattr = (unsigned short) ((unsigned int) conioxlib_curtheme.atrstatus << 8);
	for (ci = 0; ci < 5; ci++)
	{
		cells[ci] = (unsigned short) (cellattr | (unsigned char) clk[ci]);
	}
	puttext(76, 25, 80, 25, cells);
}

/* ----------------------------- Draws the clock -------------------------- */
/* clock:
   Prototype:
   void conioxlib_clock(void);
   Refreshes the clock of line 25.
   Parameters: none.
   It shows the time in columns 76 to 80 with the color of
   the active theme and also refreshes the lock indicators.
   Call it inside every wait loop; the compatibility alias
   is conioxlib_watch().
   */
void conioxlib_clock (void)
{
	static unsigned int oldmin = 0xFFFF;
	time_t t;
	struct tm *ti;

	time(&t);
	ti = localtime(&t);
	/* Refresh the lock indicators on every call: blocks() repaints them unconditionally */
	conioxlib_blocks();
	if (oldmin != (unsigned int) ti->tm_min)
	{
		oldmin = (unsigned int) ti->tm_min;
		conioxlib_paintclock(ti);
	}
}


#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)

#ifndef MOUSE_HWHEELED
#define MOUSE_HWHEELED	0x0008
#endif
#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE	0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS	0x0080
#endif
#ifndef ENABLE_INSERT_MODE
#define ENABLE_INSERT_MODE	0x0020
#endif
#ifndef MOUSE_WHEELED
#define MOUSE_WHEELED	0x0004
#endif

/* Open Watcom / older SDKs lack prototypes; WINAPI (stdcall) so linker resolves GetConsoleWindow@0 not GetConsoleWindow_ */
HWND WINAPI GetConsoleWindow(void);
SHORT WINAPI GetAsyncKeyState(int vKey);
BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);

static int conioxlib_mousemode = 0;
/* Button edges for asynchronous polling (Windows Terminal does not deliver
   mouse events through ReadConsoleInput); GetDoubleClickTime sets the double click */
static int conioxlib_oldlbtn = 0;
static int conioxlib_oldrbtn = 0;
static DWORD conioxlib_lastlclick = 0;
static DWORD conioxlib_lastrclick = 0;

/* --------------------------- Reads the mouse state -------------------- */
/* pollmouse:
   Prototype:
   void conioxlib_pollmouse(void);
   Updates conioxlib_mousestatus with the mouse state.
   Parameters: none.
   The fields are x, y, lclick, rclick, wheelx and wheely.
   lclick and rclick are edges that stay set until polled;
   the wheel fields are deltas since the last call.
   */
void conioxlib_pollmouse (void)
{
	HANDLE hStdInput;
	INPUT_RECORD ir[64];
	DWORD evs;
	DWORD n, i;
	DWORD mode;
	int ch;
	int orgx, orgy;
	CONSOLE_SCREEN_BUFFER_INFO info;
	HWND hwnd;
	POINT pt;
	CONSOLE_FONT_INFO fi;
	COORD fsz;
	int col, row;

	conioxlib_mousestatus.lclick = 0;
	conioxlib_mousestatus.rclick = 0;
	conioxlib_mousestatus.wheelx = 0;
	conioxlib_mousestatus.wheely = 0;
	hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	/* Re-assert the mode on every call: other console readers (or previous reads)
	   may clear ENABLE_MOUSE_INPUT, and then lclick/rclick/wheelx/wheely would
	   always read 0. ENABLE_EXTENDED_FLAGS is required to be able to clear
	   ENABLE_QUICK_EDIT_MODE; otherwise the console steals every click for
	   text selection and the program never sees the mouse. */
	if (GetConsoleMode(hStdInput, &mode))
	{
		SetConsoleMode(hStdInput, (mode & ~(DWORD) (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE)) | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
	}
	conioxlib_mousemode = 1;
	orgx = 0;
	orgy = 0;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
	{
		orgx = info.srWindow.Left;
		orgy = info.srWindow.Top;
	}
	/* Always refresh position from the OS cursor so the demo updates every frame even without a new MOUSE_EVENT. */
	hwnd = GetConsoleWindow();
	if (hwnd != 0 && GetCursorPos(&pt) && ScreenToClient(hwnd, &pt))
	{
		fsz.X = 8;
		fsz.Y = 16;
		if (GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &fi))
		{
			if (fi.dwFontSize.X > 0)
			{
				fsz.X = fi.dwFontSize.X;
			}
			if (fi.dwFontSize.Y > 0)
			{
				fsz.Y = fi.dwFontSize.Y;
			}
		}
		col = (pt.x / fsz.X) + 1;
		row = (pt.y / fsz.Y) + 1;
		if (col < 1)
		{
			col = 1;
		}
		if (row < 1)
		{
			row = 1;
		}
		conioxlib_mousestatus.x = (unsigned int) col;
		conioxlib_mousestatus.y = (unsigned int) row;
	}
	evs = 0;
	if (GetNumberOfConsoleInputEvents(hStdInput, &evs) && (evs > 0))
	{
		if (evs > 64)
		{
			evs = 64;
		}
		if (ReadConsoleInputA(hStdInput, ir, evs, &n))
		{
			for (i = 0; i < n; i++)
			{
				if (ir[i].EventType == MOUSE_EVENT)
				{
					conioxlib_mousestatus.x = (unsigned int) (ir[i].Event.MouseEvent.dwMousePosition.X - orgx) + 1;
					conioxlib_mousestatus.y = (unsigned int) (ir[i].Event.MouseEvent.dwMousePosition.Y - orgy) + 1;
					if (ir[i].Event.MouseEvent.dwEventFlags & MOUSE_WHEELED)
					{
						conioxlib_mousestatus.wheely -= (int) (short) HIWORD(ir[i].Event.MouseEvent.dwButtonState) / 120;
					}
					else if (ir[i].Event.MouseEvent.dwEventFlags & MOUSE_HWHEELED)
					{
						conioxlib_mousestatus.wheelx -= (int) (short) HIWORD(ir[i].Event.MouseEvent.dwButtonState) / 120;
					}
					if (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
					{
						if (!(ir[i].Event.MouseEvent.dwEventFlags & DOUBLE_CLICK))
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
						if (!(ir[i].Event.MouseEvent.dwEventFlags & DOUBLE_CLICK))
						{
							conioxlib_mousestatus.rclick = 1;
						}
						else
						{
							conioxlib_mousestatus.rclick = 2;
						}
					}
				}
				else if ((ir[i].EventType == KEY_EVENT) && (ir[i].Event.KeyEvent.bKeyDown))
				{
					ch = (int) (unsigned char) ir[i].Event.KeyEvent.uChar.AsciiChar;
					if (ch == 0)
					{
						ch = ((int) ir[i].Event.KeyEvent.wVirtualScanCode) << 8;
					}
					if (conioxlib_pendingkey == 0)
					{
						conioxlib_pendingkey = ch;
					}
				}
			}
		}
	}
	/* Windows Terminal does not deliver mouse events: poll the buttons
	   asynchronously with edge detection. Clicks are only accepted when the console
	   has focus and the cursor is inside the client area; in the classic
	   console (conhost) the events were already read above. */
	hwnd = GetConsoleWindow();
	if ((conioxlib_mousestatus.lclick == 0) && (conioxlib_mousestatus.rclick == 0) && (hwnd != 0) && (GetForegroundWindow() == hwnd))
	{
		POINT apt;
		RECT arc;

		GetCursorPos(&apt);
		ScreenToClient(hwnd, &apt);
		GetClientRect(hwnd, &arc);
		if ((apt.x >= arc.left) && (apt.x < arc.right) && (apt.y >= arc.top) && (apt.y < arc.bottom))
		{
			if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
			{
				if (conioxlib_oldlbtn == 0)
				{
					conioxlib_mousestatus.lclick = ((GetTickCount() - conioxlib_lastlclick) <= GetDoubleClickTime()) ? 2 : 1;
					conioxlib_lastlclick = GetTickCount();
				}
				conioxlib_oldlbtn = 1;
			}
			else
			{
				conioxlib_oldlbtn = 0;
			}
			if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
			{
				if (conioxlib_oldrbtn == 0)
				{
					conioxlib_mousestatus.rclick = ((GetTickCount() - conioxlib_lastrclick) <= GetDoubleClickTime()) ? 2 : 1;
					conioxlib_lastrclick = GetTickCount();
				}
				conioxlib_oldrbtn = 1;
			}
			else
			{
				conioxlib_oldrbtn = 0;
			}
		}
	}
}


#endif


#if ((__DOS__) || (__MSDOS__))

static int conioxlib_ismouse = -1;

/* Library-drawn pointer under DOS: with conioxlib_vgaremap(1) the driver
   pointer is hidden and glyph 0x01 is drawn with its attribute nibbles swapped;
   conioxlib_ptrcell keeps the original cell to restore it on every move */
static int conioxlib_mouseptr = 0;
static int conioxlib_ptrx = -1;
static int conioxlib_ptry = -1;
static unsigned short conioxlib_ptrcell = 0;

/* --------------------------- Initializes the mouse ------------------------ */
static void _conioxlib_mouseinit (void)
{
	union REGS r;

	if (conioxlib_ismouse == -1)
	{
		conioxlib_rax(r) = 0;
		coniox_int86(0x33, &r, &r);
		if (conioxlib_rax(r) == 0xFFFF)
		{
			conioxlib_ismouse = conioxlib_rbx(r);
			/* Show mouse */
			conioxlib_rax(r) = 1;
			coniox_int86(0x33, &r, &r);
		}
		else
		{
			conioxlib_ismouse = 0;
		}
	}
}


/* --------------------------- Reads the mouse state -------------------- */
void conioxlib_pollmouse (void)
{
	union REGS r;
	static int conioxlib_haswheel = -1;
	unsigned int bx;
	signed char wheel;

	conioxlib_mousestatus.lclick = 0;
	conioxlib_mousestatus.rclick = 0;
	conioxlib_mousestatus.wheelx = 0;
	conioxlib_mousestatus.wheely = 0;
	_conioxlib_mouseinit();
	if (conioxlib_ismouse > 0)
	{
		/* CuteMouse and compatible: AX=0011h returns AX=574Dh ('MW') when the wheel API is present */
		if (conioxlib_haswheel < 0)
		{
			conioxlib_rax(r) = 0x11;
			coniox_int86(0x33, &r, &r);
			conioxlib_haswheel = (conioxlib_rax(r) == 0x574D) ? 1 : 0;
		}
		conioxlib_rax(r) = 3;
		coniox_int86(0x33, &r, &r);
		bx = conioxlib_rbx(r);
		conioxlib_mousestatus.x = (conioxlib_rcx(r) >> 3) + 1;
		conioxlib_mousestatus.y = (conioxlib_rdx(r) >> 3) + 1;
		conioxlib_mousestatus.lclick = (unsigned int) (bx & 1);
		conioxlib_mousestatus.rclick = (unsigned int) ((bx & 2) >> 1);
		if (conioxlib_haswheel)
		{
			/* BH = vertical wheel delta since last call (signed). Prefer BX high byte: portable on Watcom/Borland. */
			wheel = (signed char) ((bx >> 8) & 0xFF);
			conioxlib_mousestatus.wheely = (int) wheel;
			/* Horizontal wheel when present is returned by AX=0048h (CuteMouse extension) */
			conioxlib_rax(r) = 0x48;
			coniox_int86(0x33, &r, &r);
			if (conioxlib_rax(r) == 0x48)
			{
				conioxlib_mousestatus.wheelx = (int) (signed char) ((conioxlib_rbx(r) >> 8) & 0xFF);
				if (conioxlib_mousestatus.wheely == 0)
				{
					conioxlib_mousestatus.wheely = (int) (signed char) (conioxlib_rbx(r) & 0xFF);
				}
			}
		}
		/* Library pointer: erase the previous cell if nobody painted over it and
		   draw the glyph at the current position */
		if (conioxlib_mouseptr != 0)
		{
			unsigned short conioxlib_cells[2];
			unsigned char conioxlib_attr;
			unsigned char conioxlib_sattr;

			if ((conioxlib_ptrx >= 1) && (conioxlib_ptry >= 1))
			{
				if (gettext((unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, (unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, (void *) conioxlib_cells) != 0)
				{
					conioxlib_attr = (unsigned char) ((conioxlib_ptrcell >> 8) & 0xFF);
					conioxlib_sattr = (unsigned char) (((conioxlib_attr & 0x0F) << 4) | (conioxlib_attr >> 4));
					if (conioxlib_cells[0] == (unsigned short) (0x01 | ((unsigned short) conioxlib_sattr << 8)))
					{
						conioxlib_cells[0] = conioxlib_ptrcell;
						puttext((unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, (unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, conioxlib_cells);
					}
				}
			}
			if ((conioxlib_mousestatus.x >= 1) && (conioxlib_mousestatus.x <= 80) && (conioxlib_mousestatus.y >= 1) && (conioxlib_mousestatus.y <= 25))
			{
				conioxlib_ptrx = (int) conioxlib_mousestatus.x;
				conioxlib_ptry = (int) conioxlib_mousestatus.y;
				if (gettext((unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, (unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, (void *) conioxlib_cells) != 0)
				{
					conioxlib_ptrcell = conioxlib_cells[0];
					conioxlib_attr = (unsigned char) ((conioxlib_ptrcell >> 8) & 0xFF);
					conioxlib_sattr = (unsigned char) (((conioxlib_attr & 0x0F) << 4) | (conioxlib_attr >> 4));
					conioxlib_cells[0] = (unsigned short) (0x01 | ((unsigned short) conioxlib_sattr << 8));
					puttext((unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, (unsigned short) conioxlib_ptrx, (unsigned short) conioxlib_ptry, conioxlib_cells);
				}
			}
			else
			{
				conioxlib_ptrx = -1;
				conioxlib_ptry = -1;
			}
		}
	}
}

#endif


/* ------------ Draws the state of the lock indicators ------------- */
/* blocks:
   Prototype:
   void conioxlib_blocks(void);
   Draws the lock indicator panel of line 25.
   Parameters: none.
   It paints the CAPS, NUM and SCROLL mnemonics in columns
   60 to 74, upper case when the lock is active, and it is
   also refreshed by conioxlib_clock().
   */
void conioxlib_blocks (void)
{
	char status[16];
	unsigned short cells[15];
	unsigned int i;
	unsigned short cellattr;
	int on_ins, on_cap, on_num, on_scr;

	memset(status, 32, sizeof(status) - 1);
	status[sizeof(status) - 1] = 0;
	/* Always show the four mnemonics; uppercase when the lock is active */
	memcpy(&status[0], "ins", 3);
	memcpy(&status[4], "cap", 3);
	memcpy(&status[8], "num", 3);
	memcpy(&status[12], "scr", 3);
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	on_ins = (GetKeyState(VK_INSERT) & 1) != 0;
	on_cap = (GetKeyState(VK_CAPITAL) & 1) != 0;
	on_num = (GetKeyState(VK_NUMLOCK) & 1) != 0;
	on_scr = (GetKeyState(VK_SCROLL) & 1) != 0;
#else
	{
	unsigned int fun;

	fun = peekb(0x40, 0x17);
	on_ins = (fun & 128) != 0;
	on_cap = (fun & 64) != 0;
	on_num = (fun & 32) != 0;
	on_scr = (fun & 16) != 0;
	}
#endif
	if (on_ins)
	{
		memcpy(&status[0], "INS", 3);
	}
	if (on_cap)
	{
		memcpy(&status[4], "CAP", 3);
	}
	if (on_num)
	{
		memcpy(&status[8], "NUM", 3);
	}
	if (on_scr)
	{
		memcpy(&status[12], "SCR", 3);
	}
	/* Always repaint: background() clears line 25 and a stale cache would skip the redraw */
	/* puttext avoids window/gotoxy trouble on Win32 buffered consoles; the status
	   line is a fixed 80x25 area: status() writes 1-59, the lock indicators 60-74 */
	cellattr = (unsigned short) ((unsigned int) conioxlib_curtheme.atrstatus << 8);
	for (i = 0; i < 15; i++)
	{
		cells[i] = (unsigned short) (cellattr | (unsigned char) status[i]);
	}
	puttext(60, 25, 74, 25, cells);
}


/* ------------------- Gets the COUNTRY information -------------------- */
/* getcountry:
   Prototype:
   void conioxlib_getcountry(struct conioxlib_scountry *c);
   Fills c with the DOS country information.
   Parameters: c receives the date format, the currency
   symbol, the thousand, decimal, date and time separators,
   the currency style and digits and the time format.
   */
void conioxlib_getcountry (struct conioxlib_scountry *sc)
{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	char tmp[8];

	memset(sc, 0, sizeof(struct conioxlib_scountry));
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ILDATE, tmp, 8);
	sc->date = (unsigned int) atoi(tmp);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, sc->curr, 5);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sc->thsep, 2);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, sc->dsep, 2);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDATE, sc->dtsep, 2);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIME, sc->tmsep, 2);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICURRENCY, tmp, 8);
	sc->currstyle[0] = (unsigned char) atoi(tmp);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_INEGCURR, tmp, 8);
	sc->currstyle[1] = (unsigned char) atoi(tmp);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, tmp, 8);
	sc->digits = (unsigned char) atoi(tmp);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITIME, tmp, 8);
	sc->time = (unsigned char) atoi(tmp);
	sc->convcase = 0;
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SLIST, tmp, 8);
	sc->dasep = (unsigned char) tmp[0];
	sc->code = (unsigned int) GetConsoleOutputCP();
#else
	#if defined(__DJGPP__)
	__dpmi_regs r;

	memset(sc, 0, sizeof(struct conioxlib_scountry));
	memset(&r, 0, sizeof(r));
	r.x.ax = 0x3800;
	r.x.ds = (unsigned short) (__tb >> 4);
	r.x.dx = (unsigned short) (__tb & 0x0F);
	__dpmi_int(0x21, &r);
	dosmemget(__tb, 34, sc);
	sc->code = r.x.bx;
	#elif defined(__FLAT__)
	union REGS r;
	struct SREGS s;
	unsigned int sel;
	unsigned int seg;

	memset(&r, 0, sizeof(r));
	/* Allocate real-mode memory through DPMI (int 31h AX=0100) */
	conioxlib_rax(r) = 0x0100;
	conioxlib_rbx(r) = 4;
	int386(0x31, &r, &r);
	if (r.w.cflag)
	{
		return;
	}
	sel = r.w.bx;
	seg = r.w.dx;
	memset((void *) ((unsigned long) seg << 4), 0, 34);
	memset(&s, 0, sizeof(s));
	conioxlib_rax(r) = 0x3800;
	s.ds = (unsigned short) seg;
	conioxlib_rdx(r) = 0;
	int386x(0x21, &r, &r, &s);
	memcpy(sc, (void *) ((unsigned long) seg << 4), 34);
	sc->code = r.w.bx;
	/* Free the allocated memory */
	conioxlib_rax(r) = 0x0101;
	conioxlib_rdx(r) = sel;
	int386(0x31, &r, &r);
	#else
	union REGS r;
	struct SREGS s;

	memset(sc, 0, sizeof(struct conioxlib_scountry));
	conioxlib_rax(r) = 0x3800;
	conioxlib_rdx(r) = FP_OFF(sc);
	s.ds = FP_SEG(sc);
	coniox_int86x(0x21, &r, &r, &s);
	sc->code = conioxlib_rbx(r);
	#endif
#endif
}

/* -------- Free disk space: FAT32 through int 21h AX=7303h ------------- */
/* drive: 0 = current drive, 1 = A, 2 = B... Fills df with the cluster and
   sector sizes and the free/total cluster counts; returns 0 on success and 1
   on error. DOS tries extension 7303h first (FAT32 aware) and falls back to
   the classic AH=36h when it is not available. */
/* getdiskfree_ext:
   Prototype:
   unsigned char conioxlib_getdiskfree_ext(
       unsigned char drive, struct conioxlib_sdiskfree *df);
   Fills df with the free and total space of a drive.
   Parameters: drive is 0 for the current drive, 1 for A, 2
   for B and so on; df receives the sectors per cluster, the
   bytes per sector and the free and total clusters.
   Returns 0 on success and 1 on error. DOS uses the FAT32
   aware int 21h AX=7303h with the AH=36h fallback.
   */
unsigned char conioxlib_getdiskfree_ext (unsigned char drive, struct conioxlib_sdiskfree *df)
{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	char root[4];
	DWORD spc, bps, fc, tc;
	LPCSTR rp;

	if (df == 0)
	{
		return(1);
	}
	memset(df, 0, sizeof(struct conioxlib_sdiskfree));
	spc = 0;
	bps = 0;
	fc = 0;
	tc = 0;
	if (drive == 0)
	{
		rp = 0;
	}
	else
	{
		root[0] = (char) ('A' + drive - 1);
		root[1] = ':';
		root[2] = '\\';
		root[3] = 0;
		rp = root;
	}
	if ((GetDiskFreeSpaceA(rp, &spc, &bps, &fc, &tc) == 0) || (spc == 0) || (bps == 0))
	{
		return(1);
	}
	df->sectors_per_cluster = (unsigned long int) spc;
	df->bytes_per_sector = (unsigned long int) bps;
	df->avail_clusters = (unsigned long int) fc;
	df->total_clusters = (unsigned long int) tc;
	return(0);
#else
	unsigned char drv;
	char path[4];

	if (df == 0)
	{
		return(1);
	}
	memset(df, 0, sizeof(struct conioxlib_sdiskfree));
	if (drive == 0)
	{
		drv = (unsigned char) conioxlib_getdisk();
	}
	else
	{
		drv = (unsigned char) (drive - 1);
	}
	path[0] = (char) ('A' + drv);
	path[1] = ':';
	path[2] = '\\';
	path[3] = 0;
	df->size = (unsigned int) sizeof(struct conioxlib_sdiskfree);
	df->version = 0;
	#if defined(__DJGPP__)
	{
		__dpmi_regs r;

		memset(&r, 0, sizeof(r));
		r.x.ax = 0x7303;
		r.x.cx = (unsigned short) sizeof(struct conioxlib_sdiskfree);
		r.x.ds = (unsigned short) (__tb >> 4);
		r.x.dx = (unsigned short) (__tb & 0x0F);
		r.x.es = (unsigned short) ((__tb + 16) >> 4);
		r.x.di = (unsigned short) ((__tb + 16) & 0x0F);
		dosmemput(path, 4, __tb);
		dosmemput(df, sizeof(struct conioxlib_sdiskfree), __tb + 16);
		__dpmi_int(0x21, &r);
		dosmemget(__tb + 16, sizeof(struct conioxlib_sdiskfree), df);
		if ((r.x.cflag != 0) || (r.x.ax == 0x7300))
		{
			memset(&r, 0, sizeof(r));
			r.h.ah = 0x36;
			r.h.dl = (unsigned char) (drv + 1);
			__dpmi_int(0x21, &r);
			if (r.x.ax == 0xFFFF)
			{
				return(1);
			}
			df->sectors_per_cluster = (unsigned long int) r.x.ax;
			df->bytes_per_sector = (unsigned long int) r.x.cx;
			df->avail_clusters = (unsigned long int) r.x.bx;
			df->total_clusters = (unsigned long int) r.x.dx;
		}
		return(0);
	}
	#elif defined(__FLAT__)
	{
		union REGS r;
		struct SREGS s;
		unsigned int sel;
		unsigned int seg;
		unsigned char *low;

		memset(&r, 0, sizeof(r));
		/* Allocate real-mode memory through DPMI (int 31h AX=0100) as in getcountry():
		   "X:\" in paragraph 0 and the structure in paragraph 2 */
		conioxlib_rax(r) = 0x0100;
		conioxlib_rbx(r) = 4;
		int386(0x31, &r, &r);
		if (conioxlib_rcflag(r))
		{
			return(1);
		}
		sel = conioxlib_rbx(r);
		seg = conioxlib_rdx(r);
		low = (unsigned char *) ((unsigned long) seg << 4);
		memcpy(low, path, 4);
		memcpy(low + 32, df, sizeof(struct conioxlib_sdiskfree));
		memset(&s, 0, sizeof(s));
		conioxlib_rax(r) = 0x7303;
		conioxlib_rcx(r) = (unsigned short) sizeof(struct conioxlib_sdiskfree);
		s.ds = (unsigned short) seg;
		conioxlib_rdx(r) = 0;
		s.es = (unsigned short) (seg + 2);
		conioxlib_rdi(r) = 0;
		int386x(0x21, &r, &r, &s);
		memcpy(df, low + 32, sizeof(struct conioxlib_sdiskfree));
		/* Free the allocated memory */
		conioxlib_rax(r) = 0x0101;
		conioxlib_rdx(r) = sel;
		int386(0x31, &r, &r);
		if ((conioxlib_rcflag(r)) || (conioxlib_rax(r) == 0x7300))
		{
			memset(&r, 0, sizeof(r));
			conioxlib_rax(r) = 0x3600;
			conioxlib_rdx(r) = (unsigned short) (drv + 1);
			int386(0x21, &r, &r);
			if (conioxlib_rax(r) == 0xFFFF)
			{
				return(1);
			}
			df->sectors_per_cluster = (unsigned long int) conioxlib_rax(r);
			df->bytes_per_sector = (unsigned long int) conioxlib_rcx(r);
			df->avail_clusters = (unsigned long int) conioxlib_rbx(r);
			df->total_clusters = (unsigned long int) conioxlib_rdx(r);
		}
		return(0);
	}
	#else
	{
		union REGS r;
		struct SREGS s;

		memset(&r, 0, sizeof(r));
		memset(&s, 0, sizeof(s));
		conioxlib_rax(r) = 0x7303;
		conioxlib_rcx(r) = (unsigned short) sizeof(struct conioxlib_sdiskfree);
		s.ds = FP_SEG(path);
		conioxlib_rdx(r) = FP_OFF(path);
		s.es = FP_SEG(df);
		conioxlib_rdi(r) = FP_OFF(df);
		coniox_int86x(0x21, &r, &r, &s);
		if ((conioxlib_rcflag(r)) || (conioxlib_rax(r) == 0x7300))
		{
			memset(&r, 0, sizeof(r));
			conioxlib_rax(r) = 0x3600;
			conioxlib_rdx(r) = (unsigned short) (drv + 1);
			coniox_int86(0x21, &r, &r);
			if (conioxlib_rax(r) == 0xFFFF)
			{
				return(1);
			}
			df->sectors_per_cluster = (unsigned long int) conioxlib_rax(r);
			df->bytes_per_sector = (unsigned long int) conioxlib_rcx(r);
			df->avail_clusters = (unsigned long int) conioxlib_rbx(r);
			df->total_clusters = (unsigned long int) conioxlib_rdx(r);
		}
		return(0);
	}
	#endif
#endif
}


/* ------------------- File size in bytes ----------------------- */
/* Returns the file length or 0xFFFFFFFF when it cannot be opened or read. */
/* filesize:
   Prototype:
   unsigned long int conioxlib_filesize(const char *path);
   Returns the length of a file in bytes.
   Parameters: path is the file to measure.
   It returns 0xFFFFFFFFUL when the file cannot be opened or
   read.
   */
unsigned long int conioxlib_filesize (const char *path)
{
	FILE *f;
	unsigned long int n;

	if ((path == 0) || (path[0] == 0))
	{
		return(0xFFFFFFFFUL);
	}
	f = fopen(path, "rb");
	if (f == 0)
	{
		return(0xFFFFFFFFUL);
	}
	n = 0xFFFFFFFFUL;
	if (fseek(f, 0, SEEK_END) == 0)
	{
		n = (unsigned long int) ftell(f);
	}
	fclose(f);
	return(n);
}


/* --------------------- Changes the default palette --------------------- */
/* setpal:
   Prototype:
   void conioxlib_setpal(unsigned char e);
   Changes the text mode DAC palette.
   Parameters: e selects the palette: 0 restores the classic
   16 color palette and any other value applies the library
   palette used by the demo.
   It only works under DOS; on Windows it does nothing
   because the console has no DAC registers.
   */
void conioxlib_setpal (unsigned char e)
{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	(void) e;
	/* The DAC palette does not exist in the Windows console */
#else
	unsigned int i;
	unsigned char *auxpal;
	union REGS r;
	unsigned char dac[] = {	0, 1, 2, 3, 4, 5, 20, 7,
												56, 57, 58, 59, 60, 61, 62, 63};
	unsigned char defpal[] = {	0x00, 0x00, 0x00, /* 00 */
														0x00, 0x00, 0x2A,	/* 01 */
														0x00, 0x2A, 0x00, /* 02 */
														0x00, 0x2A, 0x2A, /* 03 */
														0x2A, 0x00, 0x00, /* 04 */
														0x2A, 0x00, 0x2A, /* 05 */
														0x2A, 0x15, 0x00, /* 06 */
														0x2A, 0x2A, 0x2A, /* 07 */
														0x15, 0x15, 0x15, /* 08 */
														0x00, 0x00, 0x3F,	/* 09 */
														0x00, 0x3F, 0x00, /* 10 */
														0x00, 0x3F, 0x3F, /* 11 */
														0x3F, 0x00, 0x00, /* 12 */
														0x3F, 0x00, 0x3F, /* 13 */
														0x3F, 0x3F, 0x00, /* 14 */
														0x3F, 0x3F, 0x3F};/* 15 */
	unsigned char newpal[] = {	0x00, 0x00, 0x00, /* 00 */
														0x00, 0x00, 0x24,	/* 01 */
														0x00, 0x24, 0x04, /* 02 */
														0x00, 0x22, 0x28, /* 03 */
														0x24, 0x00, 0x00, /* 04 */
														0x28, 0x08, 0x2A, /* 05 */
														0x30, 0x30, 0x08, /* 06 */
														0x2B, 0x2B, 0x2B, /* 07 */
														0x00, 0x00, 0x1D, /* 08 */
														0x04, 0x08, 0x3C,	/* 09 */
														0x00, 0x28, 0x0E, /* 10 */
														0x00, 0x1F, 0x3F, /* 11 */
														0x28, 0x04, 0x10, /* 12 */
														0x2C, 0x08, 0x3F, /* 13 */
														0x30, 0x2A, 0x10, /* 14 */
														0x30, 0x30, 0x3F};/* 15 */

	/* Keep bright backgrounds (bit7 = intensity), not blink */
	conioxlib_rax(r) = 0x1003;
	r.h.bl = 0;
	coniox_int86(0x10, &r, &r);
	if (e == 0)
	{
		auxpal = defpal;
	}
	else
	{
		auxpal = newpal;
	}
	/* Program the DAC register used by each text-mode color (the dac table), not registers 0-15 */
	for (i = 0; i <= 15; i++)
	{
		conioxlib_rax(r) = 0x1010;
		conioxlib_rbx(r) = dac[i];
		r.h.dh = *auxpal; /* Red */
		auxpal++;
		r.h.ch = *auxpal; /* Green */
		auxpal++;
		r.h.cl = *auxpal; /* Blue */
		auxpal++;
		coniox_int86(0x10, &r, &r);
	}
#endif
}
/* ---------------- Remaps VGA font glyphs (DOS only) ------------ */
/* conioxlib_vgaremap(1) reprograms glyphs 0x01 (mouse pointer), 0x10/
   0x11 (solid triangles) and 0x18/0x19 (chunky arrows) of the 8x16 font so
   the scroll bars and the library pointer look like Turbo Vision; it hides
   the driver pointer and enables the drawing done by pollmouse().
   With 0 it restores the driver pointer. Windows needs none of this: the
   console already ships the CP437 characters; with 32-bit Watcom the REGS
   union does not expose BP, so remapping is impossible there too. */
/* vgaremap:
   Prototype:
   void conioxlib_vgaremap(unsigned char on);
   Remaps glyphs of the 8x16 VGA font (DOS only).
   Parameters: on enables the library mouse pointer when
   nonzero and restores the driver pointer when zero.
   With 1 it reprograms glyphs 0x01 (mouse pointer), 0x10
   and 0x11 (solid triangles) and 0x18 and 0x19 (chunky
   arrows) so the scroll bars and the pointer look like
   Turbo Vision. It is a no-op on Windows and 32-bit Watcom.
   */
void conioxlib_vgaremap (unsigned char on)
{
#if ((__DOS__) || (__MSDOS__))
#if defined(__TURBOC__) || defined(__DJGPP__)
	static unsigned char conioxlib_glyphtab[5][16] = {
		{0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xBC, 0x98, 0x88, 0x80, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00}};
	static unsigned char conioxlib_glyphchr[5] = {0x01, 0x10, 0x11, 0x18, 0x19};
	unsigned int i;
#endif
	union REGS r;

	/* Enable the library pointer and hide the driver one (or restore it) */
	conioxlib_mouseptr = (on != 0);
	_conioxlib_mouseinit();
	if (conioxlib_ismouse > 0)
	{
		conioxlib_rax(r) = (on != 0) ? 2 : 1;
		coniox_int86(0x33, &r, &r);
	}
	if (on == 0)
	{
		return;
	}
	#if defined(__TURBOC__)
	{
		struct REGPACK rp;

		for (i = 0; i < 5; i++)
		{
			rp.r_ax = 0x1110;
			rp.r_bx = 0x0010;
			rp.r_cx = 1;
			rp.r_dx = conioxlib_glyphchr[i];
			rp.r_es = FP_SEG(conioxlib_glyphtab[i]);
			rp.r_bp = FP_OFF(conioxlib_glyphtab[i]);
			intr(0x10, &rp);
		}
	}
	#elif defined(__DJGPP__)
	{
		__dpmi_regs r2;

		for (i = 0; i < 5; i++)
		{
			dosmemput(conioxlib_glyphtab[i], 16, __tb);
			memset(&r2, 0, sizeof(r2));
			r2.x.ax = 0x1110;
			r2.x.bx = 0x0010;
			r2.x.cx = 1;
			r2.x.dx = conioxlib_glyphchr[i];
			r2.x.es = (unsigned short) (__tb >> 4);
			r2.x.bp = (unsigned short) (__tb & 0x0F);
			__dpmi_int(0x10, &r2);
		}
	}
	#endif
#else
	(void) on;
#endif
}


/* ------------------------ Waits for a key press -------------------------- */
/* ckey:
   Prototype:
   unsigned int conioxlib_ckey(void);
   Waits for a key and returns it.
   Parameters: none.
   While waiting it keeps the clock, the lock indicators and
   the mouse status up to date, so the dialogs stay alive.
   */
unsigned int conioxlib_ckey (void)
{
	unsigned int key;

	key = 0;
	do
	{
		conioxlib_clock();
		conioxlib_pollmouse();
		if (!conioxlib_keyhit())
		{
			delay(10);
		}
	}
	while (!conioxlib_keyhit());
	conioxlib_readkey(key);
	return(key);
}


/* ----------------- Clears the screen with a push effect ---------------- */
/* push:
   Prototype:
   void conioxlib_push(void);
   Clears the screen with a push effect.
   Parameters: none.
   The screen is folded away zone by zone in six passes and
   any key aborts the effect.
   */
void conioxlib_push (void)
{
	unsigned int i;
	char l[81];

	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
	for (i = 0; i != 13; i++)
	{
		/* Zone 1: rows 1-13, columns 1-14, down */
		movetext(1, 1, 14, 12, 1, 2);
		memset(l, 32, 14);
		l[14] = 0;
		gotoxy(1, 1);
		cputs(l);
		/* Zone 2: rows 1-13, columns 15-64, up */
		movetext(15, 2, 64, 13, 15, 1);
		memset(l, 32, 50);
		l[50] = 0;
		gotoxy(15, 13);
		cputs(l);
		/* Zone 3: rows 1-13, columns 65-80, down */
		movetext(65, 1, 80, 12, 65, 2);
		memset(l, 32, 16);
		l[16] = 0;
		gotoxy(65, 1);
		cputs(l);
		/* Zone 4: rows 14-25, columns 1-14, up */
		movetext(1, 15, 14, 25, 1, 14);
		memset(l, 32, 14);
		l[14] = 0;
		gotoxy(1, 25);
		cputs(l);
		/* Zone 5: rows 14-25, columns 15-64, down */
		movetext(15, 14, 64, 24, 15, 15);
		memset(l, 32, 50);
		l[50] = 0;
		gotoxy(15, 14);
		cputs(l);
		/* Zone 6: rows 14-25, columns 65-80, up */
		movetext(65, 15, 80, 25, 65, 14);
		memset(l, 32, 16);
		l[16] = 0;
		gotoxy(65, 25);
		cputs(l);
		delay(50);
		if (conioxlib_keyhit())
		{
			break;
		}
	}
}


/* -------------------- Reads a string from the keyboard ------------------- */
/* getstr:
   Prototype:
   void conioxlib_getstr(unsigned short x, unsigned short y,
       unsigned char atr, char *str, char len);
   Reads a string from the keyboard at (x,y).
   Parameters: atr is the color of the field, str receives the
   text and is also the initial value shown, and len is the
   maximum number of characters accepted.
   BACKSPACE erases and ENTER accepts; ESC restores the
   initial text.
   */
void conioxlib_getstr (unsigned short x, unsigned short y, unsigned char atr, char *str, char len)
{
	int i;
	unsigned int tec;
	char aux[80];

	if ((str == 0) || (len < 1))
	{
		return;
	}
	/* Keep one byte for the terminator inside the local field buffer */
	if (len > 78)
	{
		len = 78;
	}
	conioxlib_hint("Input a valid string");
	_setcursortype(_SOLIDCURSOR);
	memset(aux, 32, (size_t) len);
	aux[len] = 0;
	textattr(atr);
	gotoxy(x, y);
	cputs(aux);
	/* Bound the initial value: never strcpy past the field width */
	strncpy(aux, str, (size_t) len);
	aux[len] = 0;
	i = (int) strlen(aux);
	gotoxy(x, y);
	cputs(aux);
	do
	{
		blocks();
		conioxlib_pollmouse();
		if (!conioxlib_keyhit())
		{
			delay(10);
		}
		conioxlib_readkey(tec);
		textattr(atr);
		if (tec == KBD_BACK)
		{
			if (i > 0)
			{
				cputs(" ");
				i--;
				aux[i] = 0;
			}
		}
		else if (tec == KBD_ESC)
		{
		}
		else if (tec == KBD_ENTER)
		{
			strcpy(str, aux);
			i = (int) strlen(aux);
		}
		else if (tec == KBD_LEFT)
		{
			if (i > 0)
			{
				gotoxy(wherex() - 1, wherey());
				i--;
			}
		}
		else if (tec == KBD_RIGHT)
		{
			if (i < len)
			{
				gotoxy(wherex() + 1, wherey());
				i++;
			}
		}
		else
		{
			if ((i < len) && (tec >= 32) && (tec <= 255))
			{
				putch((char) tec);
				aux[i] = (char) tec;
				i++;
				aux[i] = 0;
			}
		}
	}
	while ((tec != KBD_ENTER) && (tec != KBD_ESC));
	if (tec != KBD_ESC)
	{
		str[i] = 0;
	}
	_setcursortype(_NOCURSOR);
}


/* ------------------- Reads a password from the keyboard ------------------- */
/* getpw:
   Prototype:
   void conioxlib_getpw(unsigned short x, unsigned short y,
       unsigned char atr, char *pass, char len);
   Reads a password without echoing it.
   Parameters: atr is the color of the field, pass receives
   the text and len is the maximum length.
   Every typed character is shown as an asterisk.
   */
void conioxlib_getpw (unsigned short x, unsigned short y, unsigned char atr, char *pass, char len)
{
	int i;
	unsigned int tec;
	char aux[80];
	char mask[80];

	if ((pass == 0) || (len < 1))
	{
		return;
	}
	if (len > 78)
	{
		len = 78;
	}
	conioxlib_hint("Type password");
	_setcursortype(_NOCURSOR);
	memset(mask, 32, (size_t) len);
	mask[len] = 0;
	textattr(atr);
	gotoxy(x, y);
	cputs(mask);
	aux[0] = 0;
	i = 0;
	gotoxy(x, y);
	do
	{
		blocks();
		conioxlib_pollmouse();
		if (!conioxlib_keyhit())
		{
			delay(10);
		}
		conioxlib_readkey(tec);
		textattr(atr);
		if (tec == KBD_BACK)
		{
			if (i > 0)
			{
				cputs(" ");
				i--;
				aux[i] = 0;
			}
		}
		else if ((tec == KBD_ENTER) || (tec == KBD_ESC))
		{
		}
		else
		{
			if ((i < len) && (tec >= 32) && (tec <= 255))
			{
				putch('*');
				aux[i] = (char) tec;
				i++;
				aux[i] = 0;
			}
		}
	}
	while ((tec != KBD_ENTER) && (tec != KBD_ESC));
	if (tec != KBD_ESC)
	{
		strcpy(pass, aux);
	}
	else
	{
		pass[0] = 0;
	}
	_setcursortype(_NOCURSOR);
}


/* --------------------- Draws Turbo Vision style buttons ---------------- */
/* Button copied from the Turbo Vision 2.0 TButton::drawState(): a face of
   colored spaces (in the gray Borland dialogs: green background, black text
   and yellow accelerator; white text when selected) with the centered title,
   top-right corner 0xDC and a bottom row of two spaces followed by the
   0xDF strip up to the last column, all with the shadow attribute 0x70 and
   no frame characters. The width of each button is the visible title
   length + 4; "separation" is the horizontal gap between buttons. Returns the
   index of the pressed button, 255 when F1 (help) was pressed or 0 when
   cancelled with ESC. */
#define conioxlib_drawbtn(b, c) \
	do { \
		int conioxlib_z; \
		\
		/* Face: row of spaces with the centered title */ \
		textattr(c); \
		gotoxy((unsigned short) pos[b], y1); \
		for (conioxlib_z = 0; conioxlib_z < wide[b]; conioxlib_z++) \
		{ \
			putch(' '); \
		} \
		conioxlib_cputs_hot((unsigned short) (pos[b] + 1 + ((wide[b] - vislen[b] - 2) / 2)), y1, c, atrhot, items[b]); \
		/* Top-right corner: half block with the shadow color */ \
		gotoxy((unsigned short) (pos[b] + wide[b] - 1), y1); \
		textattr(0x70); \
		putch(0xDC); \
		/* Bottom row: two spaces and the 0xDF strip with the shadow color */ \
		gotoxy((unsigned short) pos[b], (unsigned short) (y1 + 1)); \
		textattr(0x70); \
		putch(' '); \
		putch(' '); \
		for (conioxlib_z = 2; conioxlib_z < wide[b]; conioxlib_z++) \
		{ \
			putch(0xDF); \
		} \
	} while (0)

/* buttons:
   Prototype:
   unsigned char conioxlib_buttons(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       char items[][11], char desc[][71],
       unsigned char noptions, unsigned char separation);
   Draws a row of Turbo Vision style buttons.
   Parameters: (x1,y1) is the top left corner, atr0 and atr1
   are the normal and pressed colors, atrhot the accelerator
   color, items the labels, desc the status line hints,
   noptions their number and separation the gap between
   buttons. Returns the index of the pressed button, 255
   when F1 was pressed or 0 when cancelled with ESC.
   */
unsigned char conioxlib_buttons (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][11], char desc[][71], unsigned char noptions, unsigned char separation)
{
	int x;
	unsigned int key;
	int y;
	int i;
	int len;
	int pos[16];
	int wide[16];
	int vislen[16];
	unsigned char c;

	if (noptions > 16)
	{
		noptions = 16;
	}
	for (i = 0; i < (int) noptions; i++)
	{
		len = 0;
		for (y = 0; items[i][y] != 0; y++)
		{
			if (items[i][y] != '&')
			{
				len++;
			}
		}
		vislen[i] = len;
		wide[i] = len + 4;
		if (i == 0)
		{
			pos[i] = (int) x1;
		}
		else
		{
			pos[i] = pos[i - 1] + wide[i - 1] + (int) separation;
		}
	}
	_wscroll = 0;
	for (i = 0; i < (int) noptions; i++)
	{
		if (i == 0)
		{
			c = atr1;
		}
		else
		{
			c = atr0;
		}
		conioxlib_drawbtn(i, c);
	}
	conioxlib_hint(desc[0]);
	x = 0;
	do
	{
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
				if ((key >= 32) || ((key & 0xFF) == 0))
				{
					for (y = 0; y < (int) noptions; y++)
					{
						if (conioxlib_hotmatch(items[y], key))
						{
							if (y != x)
							{
								conioxlib_drawbtn(x, atr0);
								x = y;
								conioxlib_drawbtn(x, atr1);
								conioxlib_hint(desc[x]);
							}
							key = KBD_ENTER;
							break;
						}
					}
				}
			}
			/* Left button on the face or the shadow row of the button */
			if ((key == 0) && (conioxlib_mousestatus.lclick != 0) && ((conioxlib_mousestatus.y == y1) || (conioxlib_mousestatus.y == (unsigned int) y1 + 1)))
			{
				for (y = 0; y < (int) noptions; y++)
				{
					if (((int) conioxlib_mousestatus.x >= pos[y]) && ((int) conioxlib_mousestatus.x < pos[y] + wide[y]))
					{
						if (y != x)
						{
							conioxlib_drawbtn(x, atr0);
							x = y;
							conioxlib_drawbtn(x, atr1);
							conioxlib_hint(desc[x]);
						}
						key = KBD_ENTER;
						break;
					}
				}
			}
			/* Right button */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			/* Wheel: moves to the next or previous button */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_RIGHT;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_LEFT;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_LEFT) && (key != KBD_RIGHT) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
		if (key == KBD_LEFT)
		{
			conioxlib_drawbtn(x, atr0);
			x--;
		}
		else if (key == KBD_RIGHT)
		{
			conioxlib_drawbtn(x, atr0);
			x++;
		}
		if (x > noptions - 1)
		{
			x = 0;
		}
		else if (x < 0)
		{
			x = noptions - 1;
		}
		conioxlib_drawbtn(x, atr1);
		conioxlib_hint(desc[x]);
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1));
	if (key == KBD_ESC)
	{
		/* Align with menu(): ESC is not a valid button index */
		return(255);
	}
	else if (key == KBD_F1)
	{
		return(254);
	}
	else
	{
		/* Press feedback: flash the button inverted like the Turbo Vision dialogs */
		conioxlib_drawbtn(x, (unsigned char) (((atr1 & 0x0F) << 4) | (atr1 >> 4)));
		delay(120);
		conioxlib_drawbtn(x, atr1);
		return((unsigned char) x);
	}
}

/* -------------------------- Builds a menu ----------------------------- */
/* Items whose first character is '-' are non-selectable separators (Borland
   style). All rows share the same width l: the widest visible item length
   (the ampersand does not count), capped at 20; that is why the highlight is
   full width and leaves no debris when redrawing. When the items do not fit
   between y1 and row 23 a scroll bar appears on the right (0x18/0x19 arrows
   and a 0xB2 thumb over the 0xB0 track) and the list pages with PGUP/PGDN,
   the wheel or a click on the bar. sel sets the initial item; the chosen index is returned. */
#define conioxlib_menuline(idx, row, a) \
	do { \
		int conioxlib_c; \
		int conioxlib_v; \
		\
		if (items[idx][0] == '-') \
		{ \
			textattr(a); \
			gotoxy(x1, (unsigned short) (y1 + (row) * separation)); \
			for (conioxlib_c = 0; conioxlib_c < l; conioxlib_c++) \
			{ \
				putch(0xC4); \
			} \
		} \
		else \
		{ \
			conioxlib_v = 0; \
			for (conioxlib_c = 0; items[idx][conioxlib_c] != 0; conioxlib_c++) \
			{ \
				if (items[idx][conioxlib_c] != '&') \
				{ \
					conioxlib_v++; \
				} \
			} \
			conioxlib_cputs_hot(x1, (unsigned short) (y1 + (row) * separation), a, atrhot, items[idx]); \
			textattr(a); \
			gotoxy((unsigned short) (x1 + conioxlib_v), (unsigned short) (y1 + (row) * separation)); \
			for (conioxlib_c = conioxlib_v; conioxlib_c < l; conioxlib_c++) \
			{ \
				putch(' '); \
			} \
		} \
	} while (0)

/* menu:
   Prototype:
   unsigned char conioxlib_menu(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       char items[][21], char desc[][71],
       unsigned char noptions, unsigned char separation,
       unsigned char sel);
   Runs a vertical menu with status line hints.
   Parameters: (x1,y1) is the top left corner, atr0 and atr1
   the normal and selected colors, atrhot the accelerator
   color, items the entries, desc the hints, noptions their
   number, separation the distance between rows and sel the
   initial selection. Items starting with '-' are
   non-selectable separators. Returns the chosen index,
   CONIOXLIB_MENU_ESC, CONIOXLIB_MENU_HELP,
   CONIOXLIB_MENU_LEFT or CONIOXLIB_MENU_RIGHT. A scroll bar
   appears when the items do not fit in one page.
   */
unsigned char conioxlib_menu (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], unsigned char noptions, unsigned char separation, unsigned char sel)
{
	int y, prev;
	unsigned int key;
	int x, l, i, n;
	int top, nvis, row, redraw;

	l = 1;
	for (y = 0; y < (int) noptions; y++)
	{
		if (items[y][0] != '-')
		{
			n = 0;
			for (x = 0; items[y][x] != 0; x++)
			{
				if (items[y][x] != '&')
				{
					n++;
				}
			}
			if (n > l)
			{
				l = n;
			}
		}
	}
	if (l > 20)
	{
		l = 20;
	}
	/* Items visible between y1 and row 23; row 24 stays free and row 25
	   is the status line */
	nvis = (23 - (int) y1) / (int) separation;
	if (nvis < 1)
	{
		nvis = 1;
	}
	if ((int) noptions < nvis)
	{
		nvis = (int) noptions;
	}
	/* Initial selection: clamp it and skip separators */
	y = (int) sel;
	if ((y < 0) || (y >= (int) noptions))
	{
		y = 0;
	}
	while ((y < (int) noptions) && (items[y][0] == '-'))
	{
		y++;
	}
	if (y >= (int) noptions)
	{
		return(CONIOXLIB_MENU_ESC);
	}
	top = (nvis > 1) ? (y / nvis) * nvis : 0;
	if (top > ((int) noptions - nvis))
	{
		top = (int) noptions - nvis;
	}
	if (top < 0)
	{
		top = 0;
	}
	prev = y;
	redraw = 1;
	for (;;)
	{
		if (redraw != 0)
		{
			/* Full rows of width l: no debris from previous drawings */
			for (row = 0; row < nvis; row++)
			{
				i = top + row;
				if (i >= (int) noptions)
				{
					textattr(atr0);
					gotoxy(x1, (unsigned short) (y1 + row * separation));
					for (x = 0; x < l; x++)
					{
						putch(' ');
					}
				}
				else
				{
					conioxlib_menuline(i, row, (i == y) ? atr1 : atr0);
				}
			}
			/* Scroll bar in column x1+l */
			if ((int) noptions > nvis)
			{
				textattr(atr0);
				gotoxy((unsigned short) (x1 + l), y1);
				putch(0x18);
				for (row = 1; row < nvis - 1; row++)
				{
					gotoxy((unsigned short) (x1 + l), (unsigned short) (y1 + row * separation));
					putch(0xB0);
				}
				gotoxy((unsigned short) (x1 + l), (unsigned short) (y1 + (nvis - 1) * separation));
				putch(0x19);
				/* Thumb: offset according to top inside the range */
				row = 1 + (top * (nvis - 3)) / ((int) noptions - nvis);
				if (row > nvis - 2)
				{
					row = nvis - 2;
				}
				gotoxy((unsigned short) (x1 + l), (unsigned short) (y1 + row * separation));
				putch(0xB2);
			}
			redraw = 0;
		}
		conioxlib_hint(desc[y]);
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
				if ((key >= 32) || ((key & 0xFF) == 0))
				{
					for (x = 0; x < (int) noptions; x++)
					{
						if ((items[x][0] != '-') && conioxlib_hotmatch(items[x], key))
						{
							prev = y;
							y = x;
							key = KBD_ENTER;
							break;
						}
					}
				}
			}
			if (conioxlib_mousestatus.lclick != 0)
			{
				if ((conioxlib_mousestatus.y >= (unsigned int) y1) && (conioxlib_mousestatus.y <= (unsigned int) (y1 + (nvis - 1) * separation)))
				{
					row = ((int) conioxlib_mousestatus.y - (int) y1) / (int) separation;
					if ((int) conioxlib_mousestatus.x < (x1 + (unsigned int) l))
					{
						/* Click on an item: pick it */
						i = top + row;
						if ((i < (int) noptions) && (items[i][0] != '-'))
						{
							prev = y;
							y = i;
							key = KBD_ENTER;
						}
					}
					else if (((int) noptions > nvis) && (conioxlib_mousestatus.x == (x1 + (unsigned int) l)))
					{
						/* Click on the bar: jump to the matching page */
						if (row == 0)
						{
							top = 0;
						}
						else if (row >= nvis - 1)
						{
							top = (int) noptions - nvis;
						}
						else
						{
							top = ((row - 1) * ((int) noptions - nvis)) / (nvis - 3);
						}
						if (y < top)
						{
							y = top;
							while (items[y][0] == '-')
							{
								y++;
							}
						}
						else if (y >= top + nvis)
						{
							y = top + nvis - 1;
							while ((y > top) && (items[y][0] == '-'))
							{
								y--;
							}
						}
						prev = y;
						redraw = 1;
						/* Fake key 1: exit the poll loop and repaint */
						key = 1;
					}
				}
			}
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if ((key == 0) && (redraw == 0))
			{
				delay(10);
			}
		}
		while ((key != 1) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_LEFT) && (key != KBD_RIGHT) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
		if (key != 0)
		{
			if (key == KBD_UP)
			{
				prev = y;
				do
				{
					y--;
					if (y < 0)
					{
						y = noptions - 1;
					}
				}
				while (items[y][0] == '-');
			}
			else if (key == KBD_DOWN)
			{
				prev = y;
				do
				{
					y++;
					if (y > noptions - 1)
					{
						y = 0;
					}
				}
				while (items[y][0] == '-');
			}
			else if ((key == KBD_PGUP) || (key == KBD_PGDW))
			{
				prev = y;
				y += (key == KBD_PGDW) ? nvis : -nvis;
				if (y < 0)
				{
					y = 0;
				}
				if (y > noptions - 1)
				{
					y = noptions - 1;
				}
				while (items[y][0] == '-')
				{
					y--;
				}
			}
			else if (key == KBD_ORG)
			{
				prev = y;
				y = 0;
				while ((y < (int) noptions) && (items[y][0] == '-'))
				{
					y++;
				}
			}
			else if (key == KBD_END)
			{
				prev = y;
				y = noptions - 1;
				while ((y > 0) && (items[y][0] == '-'))
				{
					y--;
				}
			}
			/* Keep the item visible and clamp the page */
			if (y < top)
			{
				top = y;
				redraw = 1;
			}
			else if (y >= top + nvis)
			{
				top = y - nvis + 1;
				redraw = 1;
			}
		}
		if ((redraw == 0) && (key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1) && (key != KBD_LEFT) && (key != KBD_RIGHT))
		{
			/* Only the highlight changed: repaint the previous row and the new one */
			conioxlib_menuline(prev, prev - top, atr0);
			conioxlib_menuline(y, y - top, atr1);
		}
		if (key == KBD_ESC)
		{
			return(CONIOXLIB_MENU_ESC);
		}
		else if (key == KBD_F1)
		{
			return(CONIOXLIB_MENU_HELP);
		}
		else if (key == KBD_LEFT)
		{
			return(CONIOXLIB_MENU_LEFT);
		}
		else if (key == KBD_RIGHT)
		{
			return(CONIOXLIB_MENU_RIGHT);
		}
		else if (key == KBD_ENTER)
		{
			return((unsigned char) y);
		}
	}
}


/* ----------------------- Top menu bar (Borland style) ---------------- */
/* Returns the index of the chosen item or CONIOXLIB_MENU_ESC/HELP. The init
   parameter sets the item highlighted on entry (clamped to noptions-1) so the
   caller can come back to the same option; the horizontal wheel moves the
   selection and a direct click enters the item. */
/* menubar:
   Prototype:
   unsigned char conioxlib_menubar(unsigned short y,
       unsigned char atr0, unsigned char atr1,
       unsigned char atrhot, char items[][21],
       char desc[][71], unsigned char noptions,
       unsigned char init);
   Runs a Borland style top menu bar on row y.
   Parameters: atr0 and atr1 are the normal and selected
   colors, atrhot the accelerator color, items the labels,
   desc the hints, noptions their number and init the item
   highlighted on entry. The horizontal wheel and the arrows
   move; ENTER or a click picks an item.
   Returns the chosen index, CONIOXLIB_MENU_ESC or
   CONIOXLIB_MENU_HELP.
   */
unsigned char conioxlib_menubar (unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], unsigned char noptions, unsigned char init)
{
	int i, sel, prev, pos, key, cell;
	int start[32];
	int width[32];
	struct text_info tiinfo;
	unsigned int oldwl, oldwt, oldwr, oldwb;
	unsigned int oldx, oldy;

	if ((noptions == 0) || (noptions > 32))
	{
		return(255);
	}
	oldx = wherex();
	oldy = wherey();
	gettextinfo(&tiinfo);
	oldwl = tiinfo.winleft;
	oldwt = tiinfo.wintop;
	oldwr = tiinfo.winright;
	oldwb = tiinfo.winbottom;
	window(1, 1, (int) tiinfo.screenwidth, (int) tiinfo.screenheight);
	_wscroll = 0;
	for (i = 0; i < (int) noptions; i++)
	{
		/* The width counts visible characters only: the ampersand of the hot key
		   takes no cell on screen */
		width[i] = 0;
		for (cell = 0; items[i][cell] != 0; cell++)
		{
			if (items[i][cell] != '&')
			{
				width[i]++;
			}
		}
		if (width[i] < 1)
		{
			width[i] = 1;
		}
	}
	pos = 1;
	for (i = 0; i < (int) noptions; i++)
	{
		start[i] = pos;
		pos += width[i] + 2;
	}
	textattr(atr0);
	gotoxy(1, y);
	for (i = 1; i <= (int) tiinfo.screenwidth; i++)
	{
		putch(' ');
	}
	for (i = 0; i < (int) noptions; i++)
	{
		textattr(atr0);
		gotoxy(start[i], y);
		putch(' ');
		conioxlib_cputs_hot((unsigned short) (start[i] + 1), y, atr0, atrhot, items[i]);
		putch(' ');
	}
	sel = (int) init;
	if ((sel < 0) || (sel > (int) noptions - 1))
	{
		sel = 0;
	}
	prev = sel;
	textattr(atr1);
	gotoxy(start[sel], y);
	putch(' ');
	conioxlib_cputs_hot((unsigned short) (start[sel] + 1), y, atr1, atrhot, items[sel]);
	textattr(atr1);
	putch(' ');
	conioxlib_hint(desc[sel]);
	do
	{
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
				if ((key >= 32) || ((key & 0xFF) == 0))
				{
					for (i = 0; i < (int) noptions; i++)
					{
						if (conioxlib_hotmatch(items[i], key))
						{
							sel = i;
							key = KBD_ENTER;
							break;
						}
					}
				}
			}
			if (conioxlib_mousestatus.lclick != 0)
			{
				if (conioxlib_mousestatus.y == (unsigned int) y)
				{
					for (i = 0; i < (int) noptions; i++)
					{
						cell = width[i] + 2;
						if ((conioxlib_mousestatus.x >= (unsigned int) start[i]) && (conioxlib_mousestatus.x < (unsigned int) (start[i] + cell)))
						{
							sel = i;
							key = KBD_ENTER;
							break;
						}
					}
				}
			}
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			/* Horizontal wheel: jump to the previous or next menu */
			else if (conioxlib_mousestatus.wheelx < 0)
			{
				key = KBD_RIGHT;
			}
			else if (conioxlib_mousestatus.wheelx > 0)
			{
				key = KBD_LEFT;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_LEFT) && (key != KBD_RIGHT) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_DOWN) && (key != KBD_F1));
		/* Clear previous highlight over the full cell (padding + label). */
		textattr(atr0);
		gotoxy(start[prev], y);
		putch(' ');
		conioxlib_cputs_hot((unsigned short) (start[prev] + 1), y, atr0, atrhot, items[prev]);
		textattr(atr0);
		putch(' ');
		if (key == KBD_LEFT)
		{
			sel--;
			if (sel < 0)
			{
				sel = noptions - 1;
			}
		}
		else if (key == KBD_RIGHT)
		{
			sel++;
			if (sel > noptions - 1)
			{
				sel = 0;
			}
		}
		else if (key == KBD_ORG)
		{
			sel = 0;
		}
		else if (key == KBD_END)
		{
			sel = noptions - 1;
		}
		prev = sel;
		textattr(atr1);
		gotoxy(start[sel], y);
		putch(' ');
		conioxlib_cputs_hot((unsigned short) (start[sel] + 1), y, atr1, atrhot, items[sel]);
		textattr(atr1);
		putch(' ');
		conioxlib_hint(desc[sel]);
	}
	while ((key != KBD_ENTER) && (key != KBD_DOWN) && (key != KBD_ESC) && (key != KBD_F1));
	window((int) oldwl, (int) oldwt, (int) oldwr, (int) oldwb);
	gotoxy(oldx, oldy);
	if (key == KBD_ESC)
	{
		return(CONIOXLIB_MENU_ESC);
	}
	else if (key == KBD_F1)
	{
		return(CONIOXLIB_MENU_HELP);
	}
	else
	{
		return((unsigned char) sel);
	}
}


/* ---------------- Option list with check boxes ---------------- */
/* Shows [X] boxes that toggle with the space bar or the left mouse button
   Returns 255 for help (F1) or 0 when done. */
/* option:
   Prototype:
   unsigned char conioxlib_option(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       char items[][21], char desc[][71], char *sel,
       unsigned char noptions, unsigned char separation,
       unsigned char enabled);
   Runs a list of check boxes.
   Parameters: sel holds one mark byte per item, a space for
   unchecked and the character \\xFE for checked; enabled
   must be nonzero to allow interaction. The space bar and
   the left button toggle the entries. Returns 255 when F1
   was pressed or 0 when the dialog is closed.
   */
unsigned char conioxlib_option (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled)
{
	int y;
	unsigned int key;
	int x;

	textattr(atr0);
	for (y = 0; y != noptions; y++)
	{
		gotoxy(x1, y1 + y * separation);
		cprintf("[%c] ", sel[y]);
		conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr0, atrhot, items[y]);
	}
	if (enabled == 0)
	{
		return(0);
	}
	textattr(atr1);
	gotoxy(x1, y1);
	cprintf("[%c] ", sel[0]);
	conioxlib_cputs_hot((unsigned short) (x1 + 4), y1, atr1, atrhot, items[0]);
	conioxlib_hint(desc[0]);
	y = 0;
	do
	{
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
				if ((key >= 32) || ((key & 0xFF) == 0))
				{
					for (x = 0; x < (int) noptions; x++)
					{
						if (conioxlib_hotmatch(items[x], key))
						{
							y = x;
							key = KBD_INS;
							break;
						}
					}
				}
			}
			/* Left button */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 20)))
			{
				x = ((int) conioxlib_mousestatus.y - (int) y1) / separation;
				if ((x >= 0) && (x < noptions))
				{
					y = x;
					key = KBD_INS;
				}
			}
			/* Right button */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			/* Wheel: moves the selection up or down */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_STAB) && (key != KBD_TAB) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_SPACE) && (key != KBD_INS) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
		if ((key == KBD_STAB) || (key == KBD_UP))
		{
			y--;
		}
		else if ((key == KBD_TAB) || (key == KBD_DOWN))
		{
			y++;
		}
		else if ((key == KBD_SPACE) || (key == KBD_INS))
		{
			if (sel[y] == '\xFE')
			{
				sel[y] = ' ';
			}
			else
			{
				sel[y] = '\xFE';
			}
			y++;
		}
		if (y > noptions - 1)
		{
			y = 0;
		}
		else if (y < 0)
		{
			y = noptions - 1;
		}
		textattr(atr0);
		for (x = 0; x != noptions; x++)
		{
			gotoxy(x1, y1 + x * separation);
			cprintf("[%c] ", sel[x]);
			conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + x * separation), atr0, atrhot, items[x]);
		}
		textattr(atr1);
		gotoxy(x1, y1 + y * separation);
		cprintf("[%c] ", sel[y]);
		conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr1, atrhot, items[y]);
		conioxlib_hint(desc[y]);
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1));
	if (key == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


/* ---------------------- Property menu (buttons) --------------------- */
/* Shows the same check boxes of option() grouped as properties with a side
   mark; the behaviour is identical. */
/* property:
   Prototype:
   unsigned char conioxlib_property(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       char items[][21], char desc[][71], char *sel,
       unsigned char noptions, unsigned char separation,
       unsigned char enabled);
   Runs a radio button list: exactly one item is marked.
   Parameters: identical to conioxlib_option(), but only one
   entry holds the character \\xFE mark at a time.
   Returns 255 when F1 was pressed or 0 on close.
   */
unsigned char conioxlib_property (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled)
{
	int y;
	unsigned int key;
	int x;

	textattr(atr0);
	for (y = 0; y != noptions; y++)
	{
		gotoxy(x1, y1 + y * separation);
		cprintf("(%c) ", sel[y]);
		conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr0, atrhot, items[y]);
	}
	if (enabled == 0)
	{
		return(0);
	}
	textattr(atr1);
	gotoxy(x1, y1);
	cprintf("(%c) ", sel[0]);
	conioxlib_cputs_hot((unsigned short) (x1 + 4), y1, atr1, atrhot, items[0]);
	conioxlib_hint(desc[0]);
	y = 0;
	do
	{
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
				if ((key >= 32) || ((key & 0xFF) == 0))
				{
					for (x = 0; x < (int) noptions; x++)
					{
						if (conioxlib_hotmatch(items[x], key))
						{
							y = x;
							key = KBD_INS;
							break;
						}
					}
				}
			}
			/* Left button */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 20)))
			{
				x = ((int) conioxlib_mousestatus.y - (int) y1) / separation;
				if ((x >= 0) && (x < noptions))
				{
					y = x;
					key = KBD_INS;
				}
			}
			/* Right button */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			/* Wheel: moves the selection up or down */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_STAB) && (key != KBD_TAB) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_SPACE) && (key != KBD_INS) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
		if ((key == KBD_STAB) || (key == KBD_UP))
		{
			y--;
		}
		else if ((key == KBD_TAB) || (key == KBD_DOWN))
		{
			y++;
		}
		else if ((key == KBD_SPACE) || (key == KBD_INS))
		{
			memset(sel, ' ', noptions);
			sel[y] = '\xFE';
		}
		if (y > noptions - 1)
		{
			y = 0;
		}
		else if (y < 0)
		{
			y = noptions - 1;
		}
		textattr(atr0);
		for (x = 0; x != noptions; x++)
		{
			gotoxy(x1, y1 + x * separation);
			cprintf("(%c) ", sel[x]);
			conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + x * separation), atr0, atrhot, items[x]);
		}
		textattr(atr1);
		gotoxy(x1, y1 + y * separation);
		cprintf("(%c) ", sel[y]);
		conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr1, atrhot, items[y]);
		conioxlib_hint(desc[y]);
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1));
	if (key == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)

/* ------------------- Error control routine ----------------------- */
/* error:
   Prototype (Turbo C DOS builds only):
   void interrupt error(unsigned int, ...);
   Critical error handler installed by conioxlib_hello().
   Parameters: the interrupt frame registers.
   It keeps the CRITICAL ERROR flag set for conioxlib_shell().
   Do not call it directly; conioxlib_bye() restores
   the previous handler.
   */
void interrupt error (unsigned int bp, unsigned int di, unsigned int si, unsigned int ds, unsigned int es, unsigned int dx, unsigned int cx, unsigned int bx, unsigned int ax)
{
	unsigned int err;
	unsigned char oldx, oldy;
	char *oldb;
	char butt[][11] = {"Retry", "Fail", "Ignore", "Abort"};
	char desc[][71] = {"Retry the process that have caused the error                        ",
										"Fail the process that have caused the error                         ",
										"Ignore the ocurred error and continue execution                     ",
										"Terminate the execution of the program                              "};

	err = di & 0x00FF;
	oldb = malloc(2000);
	if (oldb == 0)
	{
		return;
	}
	oldx = wherex();
	oldy = wherey();
	gettext(8, 10, 70, 16, oldb);
	box(8, 10, 68, 15, CONIOXLIB_ATTR(WHITE, RED), A_DOUBLE, "Error", 1);
	cputsc(8, 68, 11, "A fatal error ocurred");
	if (err == 0x00)
	{
		cputsc(8, 68, 12, "Disk is write protected");
	}
	else if (err == 0x01)
	{
		cputsc(8, 68, 12, "Unknown unit");
	}
	else if (err == 0x02)
	{
		cputsc(8, 68, 12, "Drive is not ready");
	}
	else if (err == 0x03)
	{
		cputsc(8, 68, 12, "Unknown command");
	}
	else if (err == 0x04)
	{
		cputsc(8, 68, 12, "Data error (Bad CRC)");
	}
	else if (err == 0x05)
	{
		cputsc(8, 68, 12, "Bad request structure length");
	}
	else if (err == 0x06)
	{
		cputsc(8, 68, 12, "Seek error");
	}
	else if (err == 0x07)
	{
		cputsc(8, 68, 12, "Unkown media type");
	}
	else if (err == 0x08)
	{
		cputsc(8, 68, 12, "Sector not found");
	}
	else if (err == 0x09)
	{
		cputsc(8, 68, 12, "Printer is out of paper");
	}
	else if (err == 0x0A)
	{
		cputsc(8, 68, 12, "Write fault");
	}
	else if (err == 0x0B)
	{
		cputsc(8, 68, 12, "Read fault");
	}
	else if (err == 0x0C)
	{
		cputsc(8, 68, 12, "General failure");
	}
	err = conioxlib_buttons (19, 12, CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), butt, desc, 4, 2);
	gotoxy(oldx, oldy);
	puttext(8, 10, 70, 16, oldb);
	free(oldb);
	if (err == 0)
	{
		ax = 1;
	}
	else if (err == 1)
	{
		ax = 3;
	}
	else if (err == 2)
	{
		ax = 0;
	}
	else if (err == 3)
	{
		ax = 2;
	}
#pragma warn -par
}
#pragma warn .par

#endif


/* -------------------------- Displays a text --------------------------- */
/* view:
   Prototype:
   unsigned char conioxlib_view(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2, unsigned char atr0,
       unsigned char atr1, char buffer[][61],
       unsigned char last, const char *title);
   Displays a scrollable text viewer.
   Parameters: buffer holds the lines (60 usable columns),
   last is the number of lines minus one, atr0 the text
   color, atr1 the selected line color and title the frame
   title. The wheel, the arrows, PGUP, PGDN, HOME and END
   move; clicking a line jumps to it. Returns 255 when
   cancelled with ESC.
   */
unsigned char conioxlib_view (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, char buffer[][61], unsigned char last, const char *title)
{
	unsigned int z, c;
	unsigned char oldx, oldy;
	int f, ini;
	unsigned int key;
	char *oldb;
	char tmp[70];

	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return(0);
	}
	oldx = wherex();
	oldy = wherey();
	for (f = last; (f != last + y2 - y1) && (f < 256); f++)
	{
		buffer[f][0] = 0;
	}
	conioxlib_hint("ENTER / ESC close; arrows and pages move");
	box(x1, y1, x2, y2, atr0, A_DOUBLE, title, 3);
	gotoxy(x2, y1 + 1);
	putch('\x18');
	gotoxy(x2, y2 - 1);
	putch('\x19');
	textattr(atr0);
	for (z = y1 + 2; z != y2 - 1; z++)
	{
		gotoxy (x2, z);
		putch('\xB0');
	}
	z = 0;
	ini = 0;
	do
	{
		textattr(atr0);
		gotoxy (x2, z + y1 + 2);
		putch('\xB0');
		c = y1 + 1;
		textattr(atr1);
		for (f = ini; f != ini + y2 - y1 - 1; f++)
		{
			memset(tmp, 0, 61);
			memset(tmp, 32, 60);
			memcpy (tmp, buffer[f], strlen(buffer[f]));
			gotoxy(x1 + 2, c);
			cputs(tmp);
			c++;
		}
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
			}
			/* Left button */
			if (conioxlib_mousestatus.lclick != 0)
			{
				if (conioxlib_mousestatus.x == x2)
				{
					if (conioxlib_mousestatus.y == y1 + 1)
					{
						key = KBD_UP;
					}
					else if (conioxlib_mousestatus.y == y2 - 1)
					{
						key = KBD_DOWN;
					}
					else if ((conioxlib_mousestatus.y >= y1 + 2) && (conioxlib_mousestatus.y <= y2 - 2) && (y2 - y1 > 4) && (last != 0))
					{
						ini = (conioxlib_mousestatus.y - y1 - 2) * last / (y2 - y1 - 4);
						key = 0x1312;
					}
				}
				else if ((conioxlib_mousestatus.x == x1 + 3) && (conioxlib_mousestatus.y == y1))
				{
					key = KBD_ESC;
				}
			}
			/* Right button */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != 0x1312) && (key != KBD_F1));
		if (key == KBD_UP)
		{
			ini--;
		}
		else if (key == KBD_DOWN)
		{
			ini++;
		}
		else if (key == KBD_PGUP)
		{
			ini -= (y2 - y1) - 2;
		}
		else if (key == KBD_PGDW)
		{
			ini += (y2 - y1) - 2;
		}
		else if (key == KBD_ORG)
		{
			ini = 0;
		}
		else if (key == KBD_END)
		{
			ini = last;
		}
		if (ini < 0)
		{
			ini = 0;
		}
		else if (ini > last)
		{
			ini = last;
		}
		textattr(atr0);
		gotoxy(x2, z + y1 + 2);
		putch('\xB0');
		if (last != 0)
		{
			z = (ini * (y2 - y1 - 4)) / last;
		}
		gotoxy(x2, z + y1 + 2);
		putch('\xB2');
	}
	while ((key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	if (key == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


/* ------------------- Loads a whole file into a block -------------------- */
/* Reads the whole file with a single fread() and returns it as a zero
   terminated malloc block, size receiving its length; returns 0 when it
   cannot be opened or read, is empty or too big for one allocation.
   help() and helpindex() share it. */
static char *conioxlib_readblock (const char *name, unsigned long int *size)
{
	FILE *f;
	long int len;
	char *buf;

	f = fopen(name, "rb");
	if (f == 0)
	{
		return(0);
	}
	if ((fseek(f, 0, SEEK_END) != 0) || ((len = ftell(f)) <= 0) || (len > 65500L) || (fseek(f, 0, SEEK_SET) != 0))
	{
		fclose(f);
		return(0);
	}
	buf = (char *) malloc((size_t) len + 1);
	if (buf == 0)
	{
		fclose(f);
		return(0);
	}
	if (fread(buf, 1, (size_t) len, f) != (size_t) len)
	{
		free(buf);
		fclose(f);
		return(0);
	}
	fclose(f);
	buf[len] = 0;
	*size = (unsigned long int) len;
	return(buf);
}

/* ------------------ Walks the lines of a memory block ------------------- */
/* Terminates the next line of the block in place at \r or \n, swallowing the
   CRLF pair, and advances *pp past it; returns the start of the line or 0
   past the last one. help() and helpindex() share it. */
static char *conioxlib_nextline (char **pp, char *end)
{
	char *p;
	char *q;
	int c;

	p = *pp;
	if (p >= end)
	{
		return(0);
	}
	q = p;
	while ((q < end) && (*q != '\r') && (*q != '\n'))
	{
		q++;
	}
	c = *q;
	*q = 0;
	*pp = q + 1;
	if ((c == '\r') && (*pp < end) && (**pp == '\n'))
	{
		(*pp)++;
	}
	return(p);
}


/* ---------------------------- Loads the help ---------------------------- */
/* help:
   Prototype:
   unsigned char conioxlib_help(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2, unsigned char atr0,
       unsigned char atr1, char *name, char *topic,
       char buffer[][61]);
   Shows one topic of a help file.
   Parameters: name is the help file, topic the topic to
   find (its header line is @topic) and buffer the work area
   for the text. Returns 1 when the topic was found and 0
   when it was not or the file is missing. Lines are cut at
   60 characters and the next @ line ends the entry. The
   whole file is read in one block and parsed in memory.
   */
unsigned char conioxlib_help (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, char *name, char *topic, char buffer[][61])
{
	unsigned int last;
	unsigned long int size;
	char *buf;
	char *p;
	char *end;
	char *line;
	char key[50], title[80], tmp[256];
	unsigned int len;

	conioxlib_hint("Loading help...");
	strcpy(title, topic);
	sprintf(key, "@%s", topic);
	buf = conioxlib_readblock(name, &size);
	if (buf == 0)
	{
		strcpy(&buffer[0][0], "Help is not available.");
		conioxlib_view(x1, y1, x2, y2, atr0, atr1, buffer, 1, "Help: Not available");
		return(0);
	}
	end = buf + size;
	/* Find the topic */
	p = buf;
	line = 0;
	while ((line = conioxlib_nextline(&p, end)) != 0)
	{
		if (strcmp(key, line) == 0)
		{
			break;
		}
	}
	/* Load the topic */
	if (line != 0)
	{
		last = 0;
		while ((line = conioxlib_nextline(&p, end)) != 0)
		{
			if (line[0] == '@')
			{
				break;
			}
			/* conioxlib_view() touches buffer rows up to last + (y2 - y1): the limit keeps them inside buffer[256][61] */
			if (last < 255 - (y2 - y1))
			{
				memset(&buffer[last][0], 0, 61);
				len = strlen(line);
				if (len > 60)
				{
					len = 60;
				}
				memcpy(&buffer[last][0], line, len);
				last++;
			}
		}
		free(buf);
		sprintf(tmp, "Help: %s", title);
		last = conioxlib_view(x1, y1, x2, y2, atr0, atr1, buffer, (unsigned char) last, tmp);
		if (last == 255)
		{
			conioxlib_helpindex(x1, y1, atr1, CONIOXLIB_ATTR(WHITE, GREEN), name, (char (*)[16]) buffer);
		}
		return(1);
	}
	else
	{
		free(buf);
		strcpy(&buffer[0][0], "There is no help for this topic.");
		conioxlib_view(x1, y1, x2, y2, atr0, atr1, buffer, 1, "Help: Nothing");
		return(0);
	}
}


/* ------------------- Environment to pick up files ----------------------- */
/* filemultiselector:
   Prototype:
   unsigned char conioxlib_filemultiselector(unsigned char *sdisk,
       char *sdir, struct conioxlib_sfiles *files);
   Runs the file selector with multiple marks.
   Parameters: sdisk receives the chosen drive letter, sdir
   the chosen directory and files the table with the name,
   size, date, time, description and selected mark of every
   entry. F2 changes the mask, INS or SPACE mark a file, F3
   marks all, F4 unmarks all and ENTER enters a directory
   or accepts a drive. Returns 0 when accepted and 255 when
   cancelled.
   */
unsigned char conioxlib_filemultiselector (unsigned char *sdisk, char *sdir, struct conioxlib_sfiles *files)
{
	int i, j;
	unsigned char disk, ndisks, olddrive;
	unsigned char oldx, oldy;
	unsigned int last, key;
	int done;
#if ((__DOS__) || (__MSDOS__))
	struct
	{
		unsigned sec: 5;
		unsigned min: 6;
		unsigned hour: 5;
	} ff_ftime;
	struct
	{
		unsigned day: 5;
		unsigned mon: 4;
		unsigned year: 7;
	} ff_fdate;
	union REGS r;
	conioxlib_ffblk ffblk;
#endif
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	WIN32_FIND_DATA ffd;
	FILETIME ft;
	SYSTEMTIME st;
	unsigned int dr;
	unsigned int drmask;
	void *h;
#endif
	FILE *f;
	char *oldb;
	char *s;
	char mask[15];
	char tmp1[80], tmp2[80], dir[80], olddir[80];

	_setcursortype(_NOCURSOR);
	oldb = conioxlib_savescr(1, 2, 80, 24);
	if (oldb == 0)
	{
		return(0);
	}
	oldx = wherex();
	oldy = wherey();
	/* Enumerate drives */
	i = 0;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	drmask = GetLogicalDrives();
	for (dr = 0; dr != 26; dr++)
	{
		if (drmask & (1u << dr))
		{
			tmp1[0] = (char) ('A' + dr);
			tmp1[1] = ':';
			tmp1[2] = '\\';
			tmp1[3] = 0;
			switch (GetDriveTypeA(tmp1))
			{
				case DRIVE_REMOVABLE:
					strcpy(files[i].comment, "Removable Disk");
					break;
				case DRIVE_FIXED:
					strcpy(files[i].comment, "Hard Disk");
					break;
				case DRIVE_REMOTE:
					strcpy(files[i].comment, "Network Disk");
					break;
				case DRIVE_CDROM:
					strcpy(files[i].comment, "CD-ROM Disk");
					break;
				case DRIVE_RAMDISK:
					strcpy(files[i].comment, "Virtual Floppy Disk");
					break;
				default:
					strcpy(files[i].comment, "Disk Drive");
					break;
			}
			sprintf(files[i].name, "\x10%c\x11", 'A' + dr);
			strcpy(files[i].size, "\xF0   DRIVE   \xF0");
			files[i].date[0] = 0;
			files[i].time[0] = 0;
			i++;
		}
	}
#else
	key = ((peekb(0x40, 0x10) & 192) >> 6) + 1;
	for (j = 0; j != key; j++)
	{
		r.h.ah = 8;
		r.h.dl = j;
		coniox_int86(0x13, &r, &r);
		sprintf(files[i].name, "\x10%c\x11", i + 'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0] = 0;
		files[i].time[0] = 0;
		if (r.h.bl <= 2)
		{
			strcpy(files[i].comment, "5\xAC Floppy Disk");
		}
		else if ((r.h.bl >= 3) && (r.h.bl <= 5))
		{
			strcpy(files[i].comment, "3\xAB Floppy Disk");
		}
		else
		{
			strcpy(files[i].comment, "Disk Drive");
		}
		i++;
	}
	if (key == 1)
	{
		sprintf(files[i].name, "\x10%c\x11", i + 'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0] = 0;
		files[i].time[0] = 0;
		strcpy(files[i].comment, "Virtual Floppy Disk");
		i++;
	}
	key = peekb(0x40, 0x75);
	for (j = 0; j != key; j++)
	{
		sprintf(files[i].name, "\x10%c\x11", i + 'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0] = 0;
		files[i].time[0] = 0;
		strcpy(files[i].comment, "Hard Disk");
		i++;
	}
#endif
	olddrive = conioxlib_getdisk();
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
	conioxlib_getcwd(olddir, 79);
#else
	getcurdir(0, tmp2);
	sprintf(olddir, "\\%s", tmp2);
#endif
	ndisks = i - 1;
	textattr(CONIOXLIB_ATTR(BLACK, CYAN));
	strcpy(mask, "*.*");
	conioxlib_hint("ESC close; ENTER accept; SPACE/INS mark; arrows move");
	select_1:
	for (i = 0; i != 220; i++)
	{
		files[i].selected = 0;
	}
	disk = conioxlib_getdisk() + 'A';
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
	conioxlib_getcwd(tmp2, 79);
	if (tmp2[3] == 0)
	{
		sprintf(dir, "\\");
	}
	else
	{
		sprintf(dir, "\\%s\\", &tmp2[3]);
	}
#else
	getcurdir(disk - 'A' + 1, dir);
	if (dir[0] == 0)
	{
		sprintf(tmp2, "\\");
	}
	else
	{
		sprintf(tmp2, "\\%s\\", dir);
	}
	strcpy(dir, tmp2);
#endif
	/* Ensure we are on the selected drive/dir so relative find and DESCRIPT.ION work on DOS and Windows */
	conioxlib_setdisk(disk - 'A');
	conioxlib_chdir(dir[0] == '\\' && dir[1] == 0 ? "\\" : dir);
	sprintf(tmp2, "%c:%sDESCRIPT.ION", disk, dir);
	f = fopen(tmp2, "rb");
	if (f == 0)
	{
		/* Also try lowercase name used by some tools */
		sprintf(tmp2, "%c:%sdescript.ion", disk, dir);
		f = fopen(tmp2, "rb");
	}
	i = ndisks + 1;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	/* FindFirstFileA("*.*") already returns "." and ".." in the enumeration: they must not
	   be added by hand (manual insertion duplicated them in the list) */
	h = (void *) FindFirstFileA(mask, &ffd);
	done = (h == INVALID_HANDLE_VALUE);
#else
	/* Relative mask after chdir is more reliable on DOS (Watcom/Borland/DJGPP) */
	done = conioxlib_findfirst(mask, &ffblk);
#endif
	while (!done)
	{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		memset(files[i].name, 0, 13);
		strncpy(files[i].name, ffd.cFileName, 12);
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
		memcpy(files[i].name, conioxlib_ffname(ffblk), 12);
		files[i].name[12] = 0;
		if (conioxlib_ffattrib(ffblk) & 0x10)
#endif
		{
			if (strcmp(files[i].name, ".") == 0)
			{
				strcpy(files[i].comment, "Current directory");
			}
			else if (strcmp(files[i].name, "..") == 0)
			{
				strcpy(files[i].comment, "Previous directory");
			}
			else
			{
				memset(files[i].comment, 0, 34);
			}
			strcpy(files[i].size, "\xF0 DIRECTORY \xF0");
			strupr(files[i].name);
		}
		else
		{
			memset(files[i].comment, 0, 34);
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
			conioxlib_filemultiselector_format(files[i].size, (long) ffd.nFileSizeLow);
#else
			conioxlib_filemultiselector_format(files[i].size, conioxlib_ffsize(ffblk));
#endif
			strlwr(files[i].name);
		}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		FileTimeToLocalFileTime(&ffd.ftLastWriteTime, &ft);
		FileTimeToSystemTime(&ft, &st);
		sprintf(files[i].date, "%02u-%02u-%02u", st.wDay, st.wMonth, st.wYear % 100);
		sprintf(files[i].time, "%02u:%02u", st.wHour, st.wMinute);
#else
		memcpy(&ff_fdate, &conioxlib_ffdate(ffblk), 2);
		if (ff_fdate.year < 20)
		{
			sprintf(files[i].date, "%02u-%02u-%02u", ff_fdate.day, ff_fdate.mon, ff_fdate.year + 80);
		}
		else if (ff_fdate.year < 120)
		{
			sprintf(files[i].date, "%02u-%02u-%02u", ff_fdate.day, ff_fdate.mon, ff_fdate.year - 20);
		}
		else
		{
			sprintf(files[i].date, "%02u-%02u-XX", ff_fdate.day, ff_fdate.mon);
		}
		memcpy(&ff_ftime, &conioxlib_fftime(ffblk), 2);
		sprintf(files[i].time, "%02u:%02u", ff_ftime.hour, ff_ftime.min);
#endif
		if (f != 0)
		{
			/* 4DOS / Take Command DESCRIPT.ION: "name" desc  or  name desc  (case-insensitive) */
			rewind(f);
			while (fgets(tmp2, 79, f) != 0)
			{
				s = tmp2;
				while (*s == ' ' || *s == '\t')
				{
					s++;
				}
				if (*s == 0 || *s == '\r' || *s == '\n' || *s == ';')
				{
					continue;
				}
				if (*s == '"')
				{
					s++;
					strcpy(tmp1, s);
					s = strchr(tmp1, '"');
					if (s == 0)
					{
						continue;
					}
					*s = 0;
					s++;
				}
				else
				{
					strcpy(tmp1, s);
					s = tmp1;
					while (*s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n')
					{
						s++;
					}
					if (*s)
					{
						*s = 0;
						s++;
					}
				}
				while (*s == ' ' || *s == '\t')
				{
					s++;
				}
				if (strpbrk(s, "\r\n") != 0)
				{
					*strpbrk(s, "\r\n") = 0;
				}
				if (stricmp(tmp1, files[i].name) == 0)
				{
					memset(files[i].comment, 0, 34);
					strncpy(files[i].comment, s, 33);
					break;
				}
			}
		}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		if (!FindNextFileA(h, &ffd))
		{
			done = 1;
		}
#else
		done = conioxlib_findnext(&ffblk);
#endif
		i++;
		if (i > 200)
		{
			break;
		}
	}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	if (h != INVALID_HANDLE_VALUE)
	{
		FindClose(h);
	}
#endif
	if (f != 0)
	{
		fclose(f);
	}
	last = i - 1;
	for (i = last + 1; i != 220; i++)
	{
		files[i].selected = 0;
		files[i].name[0] = 0;
		files[i].size[0] = 0;
		files[i].date[0] = 0;
		files[i].time[0] = 0;
		files[i].comment[0] = 0;
	}
	qsort((void *) files, last + 1, sizeof(files[0]), conioxlib_filemultiselector_sort);
	box(2, 2, 78, 23, 0, A_NONE, "", 5);
	textattr(CONIOXLIB_ATTR(BLACK, CYAN));
	memset(tmp1, 0xCD, 77);
	tmp1[0] = '\xC9';
	tmp1[77] = '\xBB';
	tmp1[78] = 0;
	gotoxy (1, 2);
	cputs(tmp1);
	textattr(CONIOXLIB_ATTR(WHITE, CYAN));
	sprintf(tmp1, "[%c:%s]", disk, dir);
	cputsc(1, 78, 2, tmp1);
	gotoxy(3, 2);
	cputs("[\xFE]");
	textattr(CONIOXLIB_ATTR(BLACK, CYAN));
	gotoxy (1, 3);
	cputs("\xBA    Name    \xBA     Size     \xBA  Date  \xBATime \xBA             Comment             \xBA");
	memset(tmp1, 0xCD, 78);
	tmp1[0] = '\xB6';
	tmp1[77] = '\xB5';
	tmp1[78] = 0;
	gotoxy (1, 4);
	cputs(tmp1);
	for (j = 5; j != 23; j++)
	{
		memset(tmp1, 32, 78);
		tmp1[0] = '\xBA';
		tmp1[13] = '\xBA';
		tmp1[28] = '\xBA';
		tmp1[37] = '\xBA';
		tmp1[43] = '\xBA';
		if (j == 5)
		{
			tmp1[77] = '\x18';
		}
		else if (j == 22)
		{
			tmp1[77] = '\x19';
		}
		else
		{
			tmp1[77] = '\xBA';
		}
		tmp1[78] = 0;
		gotoxy (1, j);
		cputs(tmp1);
	}
	memset(tmp1, 0xCD, 77);
	tmp1[0] = '\xC8';
	tmp1[77] = '\xBC';
	tmp1[78] = 0;
	gotoxy (1, 23);
	cputs(tmp1);
	if (last >= 18)
	{
		for (i = 0; i < 18; i++)
		{
			gotoxy (2, 5 + i);
			cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[i].name, files[i].size, files[i].date, files[i].time, files[i].comment);
		}
	}
	else
	{
		for (i = 0; i <= last; i++)
		{
			gotoxy (2, 5 + i);
			cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[i].name, files[i].size, files[i].date, files[i].time, files[i].comment);
		}
	}
	textattr(CONIOXLIB_ATTR(WHITE, GREEN));
	gotoxy(2, 5);
	cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[0].name, files[0].size, files[0].date, files[0].time, files[0].comment);
	i = 0;
	j = 0;
	do
	{
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
			}
			/* Left button */
			if (conioxlib_mousestatus.lclick != 0)
			{
				if ((conioxlib_mousestatus.x == 4) && (conioxlib_mousestatus.y == 2))
				{
					key = KBD_ESC;
				}
				else if ((conioxlib_mousestatus.x == 78) && (conioxlib_mousestatus.y == 22))
				{
					key = KBD_DOWN;
				}
				else if ((conioxlib_mousestatus.x == 78) && (conioxlib_mousestatus.y == 5))
				{
					key = KBD_UP;
				}
				else if ((conioxlib_mousestatus.x > 1) && (conioxlib_mousestatus.x < 77) && (conioxlib_mousestatus.y > 4) && (conioxlib_mousestatus.y < 23))
				{
					if (files[j].selected)
					{
						textattr(CONIOXLIB_ATTR(WHITE, CYAN));
					}
					else
					{
						textattr(CONIOXLIB_ATTR(BLACK, CYAN));
					}
					gotoxy(2, 5 + i);
					cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);
					j -= i;
					i = 0;
					i = conioxlib_mousestatus.y - 5;
					j += i;
					if (j < 0)
					{
						j = 0;
					}
					else if (j > last)
					{
						j = last;
					}
					key = KBD_SPACE;
				}
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_DOWN) && (key != KBD_UP) && (key != KBD_PGDW) && (key != KBD_PGUP) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_INS) && (key != KBD_SPACE) && (key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F4) && (key != KBD_F3) && (key != KBD_F2) && (key != KBD_F1));
		if (files[j].selected)
		{
			textattr(CONIOXLIB_ATTR(WHITE, CYAN));
		}
		else
		{
			textattr(CONIOXLIB_ATTR(BLACK, CYAN));
		}
		gotoxy(2, 5 + i);
		cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);

		if (key == KBD_ENTER)
		{
			if (strncmp(files[j].size, "\xF0   DRIVE   \xF0", 15) == 0)
			{
				conioxlib_setdisk(files[j].name[1] - 'A');
				goto select_1;
			}
			else if (strncmp(files[j].size, "\xF0 DIRECTORY \xF0", 15) == 0)
			{
				conioxlib_chdir(files[j].name);
				goto select_1;
			}
		}
		else if (key == KBD_F2)
		{
			box (23, 12, 56, 14, CONIOXLIB_ATTR(WHITE, LIGHTGRAY), A_DOUBLE, "Mask", 1);
			textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
			gotoxy(24, 13);
			cputs("Enter file mask:");
			conioxlib_getstr(42, 13, CONIOXLIB_ATTR(WHITE, BLUE), mask, 13);
			goto select_1;
		}
		else if (key == KBD_F3)
		{
			/* Select every plain file, skipping the directory and drive pseudo-entries */
			for (done = 0; done <= (int) last; done++)
			{
				if ((strcmp(files[done].size, "\xF0 DIRECTORY \xF0") != 0) && (strcmp(files[done].size, "\xF0   DRIVE   \xF0") != 0))
				{
					files[done].selected = 1;
				}
			}
		}
		else if (key == KBD_F4)
		{
			/* Deselect every entry */
			for (done = 0; done <= (int) last; done++)
			{
				files[done].selected = 0;
			}
		}
		else if (key == KBD_DOWN)
		{
			if (j != last)
			{
				i++;
				j++;
			}
		}
		else if (key == KBD_UP)
		{
			if (j != 0)
			{
				i--;
				j--;
			}
		}
		else if (key == KBD_PGDW)
		{
			if (j < last - 17)
			{
				j += 17 - i;
			}
		}
		else if (key == KBD_PGUP)
		{
			if (j >= 17)
			{
				j -= 17 - i;
			}
		}
		else if (key == KBD_ORG)
		{
			j = 0;
		}
		else if (key == KBD_END)
		{
			j = last - 17;
			if (j < 0)
			{
				j = 0;
			}
		}
		else if ((key == KBD_SPACE) || (key == KBD_INS))
		{
			if ((strcmp(files[j].size, "\xF0 DIRECTORY \xF0") != 0) && (strcmp(files[j].size, "\xF0   DRIVE   \xF0") != 0) && (j <= last))
			{
				files[j].selected++;
				files[j].selected = files[j].selected & 1;
				if (files[j].selected)
				{
					textattr(CONIOXLIB_ATTR(WHITE, CYAN));
				}
				else
				{
					textattr(CONIOXLIB_ATTR(BLACK, CYAN));
				}
				gotoxy(2, 5 + i);
				cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);
			}
			if (j < last)
			{
				i++;
				j++;
			}
		}
		if (i < 0)
		{
			movetext(2, 5, 76, 21, 2, 6);
			textattr(CONIOXLIB_ATTR(BLACK, CYAN));
			memset(tmp1, 32, 75);
			tmp1[75] = 0;
			gotoxy(2, 5);
			cputs(tmp1);
			i = 0;
		}
		else if (i > 17)
		{
			movetext(2, 6, 76, 22, 2, 5);
			textattr(CONIOXLIB_ATTR(BLACK, CYAN));
			memset(tmp1, 32, 75);
			tmp1[75] = 0;
			gotoxy(2, 22);
			cputs(tmp1);
			i = 17;
		}
		if ((key == KBD_PGDW) || (key == KBD_PGUP) || (key == KBD_ORG) || (key == KBD_END))
		{
			for (i = 0; i < 18; i++)
			{
				if (files[i + j].selected)
				{
					textattr(CONIOXLIB_ATTR(WHITE, CYAN));
				}
				else
				{
					textattr(CONIOXLIB_ATTR(BLACK, CYAN));
				}
				gotoxy (2, 5 + i);
				cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[i + j].name, files[i + j].size, files[i + j].date, files[i + j].time, files[i + j].comment);
			}
			if (key == KBD_END)
			{
				i = last - j;
				if (i > 17)
				{
					i = 17;
				}
				j = last;
			}
			else
			{
				i = 0;
			}
		}
		textattr(CONIOXLIB_ATTR(WHITE, GREEN));
		gotoxy(2, 5 + i);
		cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1));
	if (key == KBD_ESC)
	{
		for (i = 0; i != 220; i++)
		{
			files[i].selected = 0;
		}
	}
	else
	{
		for (i = 0; i <= last; i++)
		{
			if (files[i].selected)
			{
				i = 0x0AAA;
				break;
			}
		}
		if (i != 0x0AAA)
		{
			files[j].selected = 1;
		}
	}
	*sdisk = disk;
	memcpy (sdir, dir, strlen(dir) - 1);
	sdir[strlen(dir) - 1] = 0;
	conioxlib_setdisk(olddrive);
	conioxlib_chdir(olddir);
	conioxlib_restorescr(1, 2, 80, 24, oldb);
	gotoxy(oldx, oldy);
	if (key == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


/* -------------------- Sorts the select ---------------------------------- */
static int conioxlib_filemultiselector_sort (const void *a, const void *b)
{
	/* Sort by name: the selected byte of the structure must not alter the order */
	return(strcmp(((struct conioxlib_sfiles *) a)->name, ((struct conioxlib_sfiles *) b)->name));
}


/* ------------------------- Formats a long with thousand separators ------------------------------- */
static void conioxlib_filemultiselector_format (char *res, long int n)
{
	unsigned int i, j, k;
	char tmp[15];

	j = 0;
	k = 0;
	memset(tmp, 0, 15);
	memset(res, 0, 15);
	sprintf(tmp, "%ld", n);
	if (n < 1000)
	{
		strcpy(res, tmp);
		return;
	}
	strrev(tmp);
	for (i = 0; i != strlen(tmp); i++)
	{
		if (j == 3)
		{
			j = 0;
			res[k] = ',';
			k++;
		}
		res[k] = tmp[i];
		j++;
		k++;
	}
	strrev(res);
}


/* ------------------------- Compact file selector ------------------------- */
/* fileselector:
   Prototype:
   unsigned char conioxlib_fileselector(unsigned char *sdisk, char *sdir, char *name);
   Runs a compact picker for a single file: drives, directories and names
   only, without sizes, dates or descriptions, so it fits the Open and
   Save as dialogs of an editor. Parameters: sdisk receives the chosen
   drive letter, sdir the chosen directory and name the chosen file name.
   ENTER on a drive or a directory changes the path and ENTER on a plain
   file accepts it. Returns 1 when a file was chosen and 0 when cancelled.
   */
unsigned char conioxlib_fileselector (unsigned char *sdisk, char *sdir, char *name)
{
	static char CONIOXLIB_FARDATA items[200][41];
	int i;
	int done;
	unsigned char disk, olddrive, res;
#if ((__DOS__) || (__MSDOS__))
	int j;
	unsigned int key;
	conioxlib_ffblk ffblk;
#endif
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	WIN32_FIND_DATA ffd;
	unsigned int dr, drmask;
	void *h;
#endif
	char tmp[80], dir[80], olddir[80];
	char *oldb;

	_setcursortype(_NOCURSOR);
	oldb = conioxlib_savescr(1, 1, 80, 25);
	if (oldb == 0)
	{
		return(0);
	}
	olddrive = conioxlib_getdisk();
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
	conioxlib_getcwd(olddir, 79);
#else
	getcurdir(0, tmp);
	sprintf(olddir, "\\%s", tmp);
#endif
	fileselector_1:
	disk = conioxlib_getdisk() + 'A';
	/* Build the current directory in the "\sub\dir\" form used by the rest of the library */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
	conioxlib_getcwd(tmp, 79);
	if (tmp[3] == 0)
	{
		sprintf(dir, "\\");
	}
	else
	{
		sprintf(dir, "\\%s\\", &tmp[3]);
	}
#else
	getcurdir(disk - 'A' + 1, dir);
	if (dir[0] == 0)
	{
		sprintf(tmp, "\\");
	}
	else
	{
		sprintf(tmp, "\\%s\\", dir);
	}
	strcpy(dir, tmp);
#endif
	/* Ensure we are on the selected drive and directory so the relative scan works everywhere */
	conioxlib_setdisk(disk - 'A');
	conioxlib_chdir(dir[0] == '\\' && dir[1] == 0 ? "\\" : dir);
	/* Enumerate the drives the same way the filemultiselector does */
	i = 0;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	drmask = GetLogicalDrives();
	for (dr = 0; dr != 26; dr++)
	{
		if (drmask & (1u << dr))
		{
			sprintf(items[i], "\x10%c\x11", 'A' + dr);
			i++;
		}
	}
#else
	key = ((peekb(0x40, 0x10) & 192) >> 6) + 1;
	for (j = 0; j != (int) key; j++)
	{
		sprintf(items[i], "\x10%c\x11", i + 'A');
		i++;
	}
	if (key == 1)
	{
		sprintf(items[i], "\x10%c\x11", i + 'A');
		i++;
	}
	key = peekb(0x40, 0x75);
	for (j = 0; j != (int) key; j++)
	{
		sprintf(items[i], "\x10%c\x11", i + 'A');
		i++;
	}
#endif
	strcpy(items[i], "..");
	i++;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	h = (void *) FindFirstFileA("*.*", &ffd);
	done = (h == INVALID_HANDLE_VALUE);
#else
	done = conioxlib_findfirst("*.*", &ffblk);
#endif
	while (!done)
	{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		strcpy(tmp, ffd.cFileName);
#else
		strcpy(tmp, conioxlib_ffname(ffblk));
#endif
		if ((strcmp(tmp, ".") != 0) && (strcmp(tmp, "..") != 0) && (i < 200))
		{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
			if (conioxlib_ffattrib(ffblk) & 0x10)
#endif
			{
				strncpy(items[i], "[", 40);
				strncat(items[i], tmp, 38);
				strcat(items[i], "]");
				items[i][40] = 0;
				strupr(items[i]);
			}
			else
			{
				strncpy(items[i], tmp, 40);
				items[i][40] = 0;
				strlwr(items[i]);
			}
			i++;
		}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		done = (FindNextFileA(h, &ffd) == 0);
#else
		done = conioxlib_findnext(&ffblk);
#endif
	}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	if (h != INVALID_HANDLE_VALUE)
	{
		FindClose(h);
	}
#endif
	sprintf(tmp, "%c:%s", disk, dir);
	if (strlen(tmp) > 56)
	{
		sprintf(tmp, "...%s", &tmp[strlen(tmp) - 53]);
	}
	res = conioxlib_listbox(10, 4, 68, 21, conioxlib_curtheme.atrdialog, conioxlib_curtheme.atr1, items, (unsigned char) i, tmp);
	if (res == 255)
	{
		conioxlib_setdisk(olddrive);
		conioxlib_chdir(olddir);
		conioxlib_restorescr(1, 1, 80, 25, oldb);
		return(0);
	}
	if (items[res][0] == '\x10')
	{
		/* Drive: switch to it and rescan */
		conioxlib_setdisk(items[res][1] - 'A');
		goto fileselector_1;
	}
	if (strcmp(items[res], "..") == 0)
	{
		/* Directory: go one level up and rescan */
		conioxlib_chdir("..");
		goto fileselector_1;
	}
	if (items[res][0] == '[')
	{
		/* Directory: enter it and rescan */
		strcpy(tmp, &items[res][1]);
		tmp[strlen(tmp) - 1] = 0;
		conioxlib_chdir(tmp);
		goto fileselector_1;
	}
	/* Plain file: accept it */
	strcpy(name, items[res]);
	*sdisk = disk;
	strcpy(sdir, dir);
	conioxlib_setdisk(olddrive);
	conioxlib_chdir(olddir);
	conioxlib_restorescr(1, 1, 80, 25, oldb);
	return(1);
}


/* --------------------------- Startup function ---------------------------- */
/* hello:
   Prototype:
   void conioxlib_hello(char *name, unsigned char commercial);
   Saves the screen to a file and shows the about box.
   Parameters: name is the file used to save the screen that
   conioxlib_bye() will restore; commercial selects between
   the shareware and the personal about text.
   Under Turbo C it also installs the critical error
   handler restored by conioxlib_bye().
   */
void conioxlib_hello (char *name, unsigned char commercial)
{
	unsigned char oldx, oldy;
	FILE *f;
	char *oldb;
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	void interrupt (*oldvect)();
#endif
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	HANDLE hIn;
	DWORD mode;
#endif

	oldb = malloc(4000);
	if (oldb == 0)
	{
		return;
	}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	/* Drop any queued keys/mouse from process startup so About/menu do not auto-close */
	hIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hIn != INVALID_HANDLE_VALUE)
	{
		FlushConsoleInputBuffer(hIn);
		if (GetConsoleMode(hIn, &mode))
		{
			SetConsoleMode(hIn, (mode & ~(DWORD) (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)) | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT);
		}
	}
	conioxlib_pendingkey = 0;
#endif
	gettext(1, 1, 80, 25, oldb);
	oldx = wherex();
	oldy = wherey();
	_setcursortype(_NOCURSOR);
	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
	conioxlib_setpal(1);
	conioxlib_push();
	conioxlib_background((unsigned char *) "");
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	oldvect = getvect(0x24);
#endif
	f = fopen(name, "wb");
	if (f == 0)
	{
		free(oldb);
		return;
	}
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	setvect(0x24, error);
#endif
	fwrite(oldb, 1, 4000, f);
	fwrite(&oldx, 1, 1, f);
	fwrite(&oldy, 1, 1, f);
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	fwrite(&oldvect, 1, sizeof(oldvect), f);
#endif
	fclose(f);
	free(oldb);
	conioxlib_about(commercial);
}


/* --------------------------- Final function ------------------------------ */
/* bye:
   Prototype:
   void conioxlib_bye(char *name, unsigned char e,
       unsigned char commercial);
   Restores the screen saved by conioxlib_hello() and ends
   the program.
   Parameters: name is the same file given to hello(), e is
   the exit code and commercial selects the farewell text.
   */
void conioxlib_bye (char *name, unsigned char e, unsigned char commercial)
{
	unsigned char oldx, oldy;
	FILE *f;
	char *oldb;
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	void interrupt (*oldvect)();
#endif

	oldb = malloc(4000);
	if (oldb == 0)
	{
		exit(e);
	}
	f = fopen(name, "rb");
	if (f == 0)
	{
		free(oldb);
		exit(e);
	}
	fread(oldb, 1, 4000, f);
	fread(&oldx, 1, 1, f);
	fread(&oldy, 1, 1, f);
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	fread(&oldvect, 1, sizeof(oldvect), f);
#endif
	fclose(f);
	remove(name);
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	setvect(0x24, oldvect);
#endif
	puttext(1, 1, 80, 25, oldb);
	if (oldy < 25 - 12)
	{
		box(1, 1, 78, 11, CONIOXLIB_ATTR(YELLOW, BLACK), A_SINGLE, "Thank you for using", 1);
		textattr(CONIOXLIB_ATTR(CYAN, BLACK));
		window(3, 2, 76, 10);
	}
	else
	{
		box(1, 2, 78, 12, CONIOXLIB_ATTR(YELLOW, BLACK), A_SINGLE, "Thank you for using", 1);
		textattr(CONIOXLIB_ATTR(CYAN, BLACK));
		window(3, 3, 76, 11);
	}
	gotoxy(1, 1);
	cputs("CONXLIB 2.0 for DOS and Windows\r\n");
	cputs("(c) Copyright 1997-2026 Javier Gutierrez Chamorro\r\n");
	cputs("https://nikkhokkho.sourceforge.io\r\n");
	cputs("Public Domain source code - use, copy and modify it freely\r\n");
	textattr(CONIOXLIB_ATTR(LIGHTRED, BLACK));
	if (commercial)
	{
		cputs("Registered version. Do not distribute it.");
	}
	else
	{
		cputs("Free version. Please visit the web page.");
	}
	window(1, 1, 80, 25);
	if (oldy < 12)
	{
		gotoxy(oldx, 12);
	}
	else
	{
		gotoxy(oldx, oldy);
	}
	_setcursortype(_NORMALCURSOR);
	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
	free(oldb);
	exit(e);
}


/* ------------------- Draws the application background -------------------- */
/* background:
   Prototype:
   void conioxlib_background(unsigned char msg[]);
   Paints the application background with the colors of the
   active theme and writes msg centered on line 25.
   Parameters: msg is the message for the status area; an
   empty string leaves the status area blank.
   */
void conioxlib_background (unsigned char msg[])
{
	unsigned int i;
	unsigned short *oldb, *olda;
	unsigned short fill;
	unsigned short status;
	time_t t;
	struct tm *ti;

	/* Cell is always 2 bytes (char + attr). Never use unsigned int here: on Win32/DOS32 that is 4 bytes and overflows. */
	oldb = (unsigned short *) malloc(80 * 25 * sizeof(unsigned short));
	if (oldb == 0)
	{
		return;
	}
	fill = (unsigned short) ((CONIOXLIB_ATTR(CYAN, BLUE) << 8) | 0xB0);
	olda = oldb;
	for (i = 0; i != (unsigned int) (80 * 23); i++)
	{
		*olda = fill;
		olda++;
	}
	puttext(1, 2, 80, 24, oldb);
	status = (unsigned short) (((unsigned int) conioxlib_curtheme.atrstatus << 8) | 0x20);
	olda = oldb;
	for (i = 0; i != 80; i++)
	{
		*olda = status;
		olda++;
	}
	puttext(1, 25, 80, 25, oldb);
	if (msg[0] != 0)
	{
		textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
		gotoxy(1, 1);
		cputs((char *) msg);
	}
	time(&t);
	ti = localtime(&t);
	conioxlib_paintclock(ti);
	/* Invalidate blocks cache: line 25 was just cleared to spaces */
	conioxlib_blocks();
	free(oldb);
}


/* --------------- Draws the about window on screen ---------------- */
/* about:
   Prototype:
   void conioxlib_about(unsigned char commercial);
   Shows the credits box of the library.
   Parameters: commercial selects between the shareware and
   the personal text.
   */
void conioxlib_about (unsigned char commercial)
{
	unsigned int key;
	char x1 = 9, y1 = 5, x2 = 68, y2 = 18;
	unsigned char oldx, oldy;
	char *oldb;

	oldb = malloc(gettextsize(x1, y1, x2 + 2, y2 + 1));
	if (oldb == 0)
	{
		return;
	}
	oldx = wherex();
	oldy = wherey();
	gettext(x1, y1, x2 + 2, y2 + 1, oldb);
	_setcursortype(_NOCURSOR);
	window(1, 1, 80, 25);
	_wscroll = 0;
	conioxlib_hint("ENTER / ESC close about");
	box (x1, y1, x2, y2, CONIOXLIB_ATTR(WHITE, LIGHTGRAY), A_DOUBLE, "About", 3);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	cputsc(x1, x2, y1 + 1, "CONXLIB for DOS and Windows");
	cputsc(x1, x2, y1 + 2, "Version  2.0");
	cputsc(x1, x2, y1 + 4, "(c) Copyright 1997-2026 Javier Gutierrez Chamorro");
	cputsc(x1, x2, y1 + 5, "https://nikkhokkho.sourceforge.io");
	cputsc(x1, x2, y1 + 6, "CONIOX 5.0 based - Public Domain source code");
	textattr(CONIOXLIB_ATTR(RED, LIGHTGRAY));
	if (commercial)
	{
		cputsc(x1, x2, y1 + 12, "Registered version");
	}
	else
	{
		cputsc(x1, x2, y1 + 12, "Free distribution");
	}
	key = 0;
	do
	{
		conioxlib_clock();
		conioxlib_pollmouse();
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(key);
		}
		/* Left button on the close button */
		if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.y == (unsigned int) y1) && (conioxlib_mousestatus.x == (unsigned int) x1 + 3))
		{
			key = KBD_ESC;
		}
		if (key == 0)
		{
			delay(10);
		}
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC));
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	{
		HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
		if (hIn != INVALID_HANDLE_VALUE)
		{
			FlushConsoleInputBuffer(hIn);
		}
		conioxlib_pendingkey = 0;
	}
#endif
	gotoxy(oldx, oldy);
	puttext(x1, y1, x2 + 2, y2 + 1, oldb);
	free(oldb);
}


/* -------------------- Block-by-block screen saver ------------------------- */
/* screensaver:
   Prototype:
   void conioxlib_screensaver(void);
   Runs the block screen saver until a key or mouse event.
   Parameters: none.
   The screen is removed and put back block by block; any
   key, click or wheel movement wakes it up.
   */
void conioxlib_screensaver (void)
{
	unsigned char i, j;
	unsigned char k;
	unsigned char oldx, oldy;
	unsigned char rep[8][5];
	char oldb[8][5][10 * 5 * 2];
	unsigned int key;

	oldx = wherex();
	oldy = wherey();
	srand((unsigned int) time(0));
	/* Save the original screen */
	for (j = 0; j != 5; j++)
	{
		for (i = 0; i != 8; i++)
		{
			gettext(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5, oldb[i][j]);
		}
	}
	ini:
	/* Start removing */
	memset(&rep, 0, sizeof(rep));
	for (k = 0; k != 40; k++)
	{
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(key);
			goto done;
		}
		do
		{
			i = rand() % 8;
			j = rand() % 5;
		}
		while (rep[i][j]);
		rep[i][j] = 1;
		window(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5);
		textattr(CONIOXLIB_ATTR(BLACK, BLACK));
		clrscr();
		window(1, 1, 80, 25);
		delay(200);
	}
	/* Start putting back */
	memset(&rep, 0, sizeof(rep));
	for (k = 0; k != 40; k++)
	{
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(key);
			goto done;
		}
		do
		{
			i = rand() % 8;
			j = rand() % 5;
		}
		while (rep[i][j]);
		rep[i][j] = 1;
		puttext(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5, oldb[i][j]);
		delay(200);
	}
	goto ini;
	done:
	/* Restore the original screen */
	for (j = 0; j != 5; j++)
	{
		for (i = 0; i != 8; i++)
		{
			puttext(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5, oldb[i][j]);
		}
	}
	gotoxy(oldx, oldy);
}


/* ------------------------- Bounces the screen ---------------------------- */
/* bounce:
   Prototype:
   void conioxlib_bounce(void);
   Clears the screen with a bounce effect.
   Parameters: none.
   The screen is thrown down and bounces back up until it
   leaves the display; any key aborts the effect.
   */
void conioxlib_bounce (void)
{
	unsigned int i;
	unsigned int sw, sh, height;
	unsigned int done;
	unsigned short *fb;
	struct text_info ti;

	gettextinfo(&ti);
	sw = ti.screenwidth;
	sh = ti.screenheight;
	fb = (unsigned short *) malloc(sw * sh * sizeof(unsigned short));
	if (fb == 0)
	{
		return;
	}
	window(1, 1, (int) sw, (int) sh);
	gettext(1, 1, (int) sw, (int) sh, fb);
	/* The image falls one row at a time leaving a blank gap on top and bounces
	   back up half of the previous height, until it leaves the display erased;
	   any key aborts the effect */
	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
	done = 0;
	height = sh >> 1;
	while ((height > 0) && (!done))
	{
		/* Fall: the image drops height rows */
		for (i = 1; i <= height; i++)
		{
			gotoxy(1, (int) i);
			clreol();
			if (i < sh)
			{
				puttext(1, (int) (i + 1), (int) sw, (int) sh, fb + (i * sw));
			}
			if (conioxlib_keyhit())
			{
				done = 1;
				break;
			}
			delay(25);
		}
		/* Bounce: the image jumps back to the top */
		if (!done)
		{
			for (i = height; i != 0; i--)
			{
				puttext(1, (int) i, (int) sw, (int) sh, fb + ((i - 1) * sw));
				if (conioxlib_keyhit())
				{
					done = 1;
					break;
				}
				delay(25);
			}
		}
		height >>= 1;
	}
	/* Final fall: the image leaves the display for good */
	if (!done)
	{
		for (i = 1; i <= sh; i++)
		{
			gotoxy(1, (int) i);
			clreol();
			if (i < sh)
			{
				puttext(1, (int) (i + 1), (int) sw, (int) sh, fb + (i * sw));
			}
			if (conioxlib_keyhit())
			{
				break;
			}
			delay(25);
		}
	}
	free(fb);
}

/* -------------------- Diagonal wipe without globals ---------------------- */
/* wipe:
   Prototype:
   void conioxlib_wipe(void);
   Clears the screen with a diagonal wipe.
   Parameters: none.
   The screen is erased from one corner to the opposite one
   without using global buffers; any key aborts it.
   */
void conioxlib_wipe (void)
{
	unsigned int phase, x, y;
	unsigned int sw, sh, cells;
	unsigned short *fb;
	struct text_info ti;

	gettextinfo(&ti);
	sw = ti.screenwidth;
	sh = ti.screenheight;
	cells = sw * sh;
	fb = (unsigned short *) malloc(cells * sizeof(unsigned short));
	if (fb == 0)
	{
		return;
	}
	gettext(1, 1, (int) sw, (int) sh, fb);
	for (phase = 0; phase < 8; phase++)
	{
		for (y = 0; y < sh; y++)
		{
			for (x = 0; x < sw; x++)
			{
				if (((x + y) & 7) <= phase)
				{
					fb[(y * sw) + x] = (unsigned short) ((CONIOXLIB_ATTR(LIGHTGRAY, BLACK) << 8) | 0x20);
				}
			}
		}
		puttext(1, 1, (int) sw, (int) sh, fb);
		if (conioxlib_keyhit())
		{
			break;
		}
		delay(40);
	}
	free(fb);
}


/* -------------------- Asks before exiting to DOS ---------------------- */
/* exittodos:
   Prototype:
   unsigned char conioxlib_exittodos(char *name);
   Asks for confirmation before leaving to DOS.
   Parameters: name is the help file shown by its F1 button.
   Returns 1 when the user confirms the exit and 0 when he
   stays. The Shell answer runs conioxlib_shell() and also
   returns 0.
   */
unsigned char conioxlib_exittodos (char *name)
{
	unsigned int i;
	unsigned char oldx, oldy;
	char *oldb, *buf;
	char but1[][11] = {"No", "Yes", "Shell"};
	char dbut1[][71] = {"Continue running program                                            ",
						"Exit to DOS and finish program                                      ",
						"Exit to DOS and continue running program                            "};

	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(20, 10, 62, 15);
	if (oldb == 0)
	{
		return(255);
	}
	box(20, 10, 60, 14, CONIOXLIB_ATTR(WHITE, LIGHTGRAY), A_DOUBLE, "Exit", SHADOW);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	cputsc(20, 60, 11, "Do you want to exit to DOS?");
	do
	{
		i = conioxlib_buttons(28, 12, CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), but1, dbut1, 3, 2);
		if (i == 255)
		{
			buf = malloc(16384);
			if (buf == 0)
			{
				conioxlib_restorescr(20, 10, 62, 15, oldb);
				return(255);
			}
			conioxlib_help (7, 3, 69, 21, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), name, "Exit", (char (*)[61]) buf);
			free(buf);
		}
	}
	while (i == 255);
	conioxlib_restorescr(20, 10, 62, 15, oldb);
	gotoxy(oldx, oldy);
	if (i == 2)
	{
		conioxlib_shell(3, 2, 75, 22, "");
		i = 0;
	}
	return(i);
}


/* ----------------------------- Shell to DOS ------------------------------- */
/* shell:
   Prototype:
   void conioxlib_shell(unsigned short x1, unsigned short y1,
       unsigned short x2, unsigned short y2, char *command);
   Runs a shell of the command interpreter.
   Parameters: the rectangle frames the shell output under
   DOS and is ignored on Windows, where the shell runs on
   the full screen; command is the program to run and an
   empty command starts the interactive interpreter named
   by COMSPEC (COMMAND.COM or CMD.EXE). Type EXIT inside
   the shell to come back; the screen is restored on return.
   */
void conioxlib_shell (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *command)
{
	unsigned char oldx, oldy;
	char *oldb;
	int i;

	oldx = wherex();
	oldy = wherey();
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	/* On Windows the interpreter needs no frame: run it on the whole screen and
	   restore the previous one when it exits */
	(void) x1;
	(void) y1;
	(void) x2;
	(void) y2;
	oldb = conioxlib_savescr(1, 1, 80, 25);
	if (oldb == 0)
	{
		return;
	}
	window(1, 1, 80, 25);
	textattr(CONIOXLIB_ATTR(WHITE, BLACK));
	clrscr();
	cputs("Type 'EXIT' to return\r\n\r\n");
#else
	/* Under DOS the shell runs inside the window of the rectangle, as in the
	   Borland IDEs */
	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return;
	}
	if (command[0] == 0)
	{
		box(x1, y1, x2, y2, CONIOXLIB_ATTR(YELLOW, BLACK), A_DOUBLE, "DOS Shell", SHADOW);
	}
	else
	{
		box(x1, y1, x2, y2, CONIOXLIB_ATTR(YELLOW, BLACK), A_DOUBLE, command, SHADOW);
	}
	window(x1 + 1, y1 + 1, x2 - 1, y2 - 1);
	textattr(CONIOXLIB_ATTR(WHITE, BLACK));
	clrscr();
	cputs("Type 'EXIT' to return\r\n\r\n");
#endif
	/* Flush pending keystrokes so the shell does not take them as its first command line */
	while (conioxlib_kbhit())
	{
		conioxlib_getkey();
	}
	/* The command interpreter needs a visible cursor: the caller may have hidden it */
	_setcursortype(_NORMALCURSOR);
	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	{
		void interrupt (*oldvect)();
		char *comspec;

		/* DOS keeps the real interpreter path in COMSPEC; do not rely on command.com being in the current directory or PATH */
		comspec = getenv("COMSPEC");
		oldvect = getvect(0x29);
		setvect(0x29, conioxlib_shell_patch);
		i = system((command[0] != 0) ? command : ((comspec != 0) ? comspec : "command.com"));
		setvect(0x29, oldvect);
	}
#else
	{
		char *comspec;

		comspec = getenv("COMSPEC");
#if ((__DOS__) || (__MSDOS__))
		i = system((command[0] != 0) ? command : ((comspec != 0) ? comspec : "command.com"));
#else
		i = system((command[0] != 0) ? command : ((comspec != 0) ? comspec : "cmd"));
#endif
	}
#endif
	_setcursortype(_NOCURSOR);
	window(1, 1, 80, 25);
	if (i != 0)
	{
		/* Wait for a key: restorescr would erase the message before it can be read */
		textattr(CONIOXLIB_ATTR(WHITE, BLACK));
		cputs("Cannot execute program.\r\nPress any key...");
		while (conioxlib_kbhit())
		{
			conioxlib_getkey();
		}
		conioxlib_getkey();
	}
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	conioxlib_restorescr(1, 1, 80, 25, oldb);
#else
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
#endif
	gotoxy(oldx, oldy);
}


#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)

/* ------------------- Exit routine of conioxlib_shell -------------------------- */
/* conioxlib_shell_patch:
   Prototype (Turbo C DOS builds only):
   void interrupt conioxlib_shell_patch(unsigned int, ...);
   Int 29h filter installed by conioxlib_shell() under Turbo C.
   Parameters: the interrupt frame registers.
   It redirects the CON output of the child process into the
   shell window. Do not call it directly.
   */
void interrupt conioxlib_shell_patch (unsigned int bp, unsigned int di, unsigned int si, unsigned int ds, unsigned int es, unsigned int dx, unsigned int cx, unsigned int bx, unsigned int ax)
{
	unsigned char al;

	al = ax & 255;
	putch(al);
#pragma warn -par
}
#pragma warn .par

#endif


/* ----------------- Interactive month calendar ------------------- */
/* calendar:
   Prototype:
   unsigned char conioxlib_calendar(unsigned short x,
       unsigned short y, unsigned char atr0,
       unsigned char atr1, unsigned char atr2,
       unsigned char *day, unsigned char *month,
       unsigned int *year);
   Shows an interactive month calendar at (x,y).
   Parameters: atr0 is the color of the normal days, atr1
   the color of the selected day and atr2 the color of the
   Sundays; day, month and year receive the date chosen and
   0 starts on the current date. The arrows move the day,
   PGUP and PGDN change the month, HOME and END change the
   year and clicking a day selects it. Returns 1 when a
   date was chosen with ENTER and 0 when cancelled with ESC
   or the close button.
   */
unsigned char conioxlib_calendar (unsigned short x, unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atr2, unsigned char *day, unsigned char *month, unsigned int *year)
{
	unsigned char i, days, start, cday, cmon, prevdays;
	unsigned char oldx, oldy;
	unsigned int key, cyear;
	char *oldb;
	struct tm *now;
	struct tm t;
	time_t curr_time;
	unsigned char monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	char *monthnameuk[12] = {	"January", "February", "March", "April",
								"May", "June", "July", "August",
								"September", "October", "November",
								"December" };

	_setcursortype(_NOCURSOR);
	oldb = conioxlib_savescr(x, y, x + 40, y + 11);
	if (oldb == 0)
	{
		return(0);
	}
	oldx = wherex();
	oldy = wherey();
	/* Get the current date */
	curr_time = time(0);
	now = localtime(&curr_time);
	cday = *day;
	cmon = *month;
	cyear = *year;
	/* When a parameter is zero it gets the current one; month goes from 1 = January to 12 = December */
	if (cday == 0)
	{
		cday = (unsigned char) now->tm_mday;
	}
	if (cmon == 0)
	{
		cmon = (unsigned char) now->tm_mon + 1;
	}
	if (cmon > 12)
	{
		cmon = 12;
	}
	if (cyear < 100)
	{
		cyear = (unsigned int) now->tm_year + 1900;
	}
	_wscroll = 0;
	for (;;)
	{
		/* Work out the weekday of day 1 */
		memset(&t, 0, sizeof(t));
		t.tm_mday = 1;
		t.tm_mon = cmon - 1;
		t.tm_year = cyear - 1900;
		if (mktime(&t) == (time_t) -1)
		{
			start = 1;
		}
		else
		{
			start = (unsigned char) t.tm_wday + 1;
		}
		days = monthdays[cmon - 1];
		if ((cmon == 2) && ((((cyear % 4) == 0) && ((cyear % 100) != 0)) || ((cyear % 400) == 0)))
		{
			days = 29;
		}
		/* Days of the previous month, used when the day wraps over a month edge */
		prevdays = monthdays[(cmon == 1) ? 11 : (cmon - 2)];
		if ((cmon == 3) && ((((cyear % 4) == 0) && ((cyear % 100) != 0)) || ((cyear % 400) == 0)))
		{
			prevdays = 29;
		}
		/* Keep status text in columns 1..59 so clock (76-80) and locks (60-74) never collide */
		conioxlib_hint("ENTER accepts; ESC closes; arrows, PGUP/PGDN, HOME/END");
		/* atr0 = normal day cells, atr1 = selected day, atr2 = Sundays */
		box(x, y, x + 38, y + 10, atr0, A_DOUBLE, "Calendar", SHADOW | CLOSE);
		textattr(atr0);
		gotoxy(x + 2, y + 1);
		cprintf("%-30s %u", monthnameuk[cmon - 1], cyear);
		gotoxy(x + 5, y + 2);
		cputs("Sun Mon Tue Wed Thu Fri Sat");
		gotoxy(x + 5, y + 4);
		for (i = 1; i <= days; i++)
		{
			gotoxy(x + 2 + (start << 2), wherey());
			if (i == cday)
			{
				textattr(atr1);
			}
			else if (start == 1)
			{
				textattr(atr2);
			}
			else
			{
				textattr(atr0);
			}
			cprintf("%2u", i);
			if (start == 7)
			{
				gotoxy(x + 5, wherey() + 1);
				start = 1;
			}
			else
			{
				start++;
			}
		}
		key = 0;
		i = 0;
		do
		{
			conioxlib_pollmouse();
			conioxlib_clock();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
			}
			/* Left button on the close button */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.y == y) && (conioxlib_mousestatus.x == (unsigned int) x + 3))
			{
				key = KBD_ESC;
			}
			/* Left button on a day cell picks that day */
			else if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= (unsigned int) x + 6) && (conioxlib_mousestatus.x <= (unsigned int) x + 33) && (conioxlib_mousestatus.y >= (unsigned int) y + 4) && (conioxlib_mousestatus.y <= (unsigned int) y + 9))
			{
				i = (unsigned char) ((((unsigned int) (conioxlib_mousestatus.y - y) - 4) * 7) + (((unsigned int) (conioxlib_mousestatus.x - x) - 6) >> 2) - (start - 1) + 1);
				if ((i >= 1) && (i <= days))
				{
					cday = i;
					key = 1;
				}
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != 1) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_LEFT) && (key != KBD_RIGHT) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER));
		if (key == KBD_ESC)
		{
			break;
		}
		if (key == KBD_ENTER)
		{
			*day = cday;
			*month = cmon;
			*year = cyear;
			conioxlib_restorescr(x, y, x + 40, y + 11, oldb);
			gotoxy(oldx, oldy);
			return(1);
		}
		if (key == KBD_LEFT)
		{
			cday--;
			if (cday == 0)
			{
				cmon--;
				if (cmon == 0)
				{
					cmon = 12;
					cyear--;
				}
				cday = prevdays;
			}
		}
		else if (key == KBD_RIGHT)
		{
			cday++;
			if (cday > days)
			{
				cday = 1;
				cmon++;
				if (cmon > 12)
				{
					cmon = 1;
					cyear++;
				}
			}
		}
		else if (key == KBD_UP)
		{
			if (cday > 7)
			{
				cday -= 7;
			}
			else
			{
				cmon--;
				if (cmon == 0)
				{
					cmon = 12;
					cyear--;
				}
				cday = (unsigned char) (cday + prevdays - 7);
			}
		}
		else if (key == KBD_DOWN)
		{
			cday += 7;
			if (cday > days)
			{
				cday = (unsigned char) (cday - days);
				cmon++;
				if (cmon > 12)
				{
					cmon = 1;
					cyear++;
				}
			}
		}
		else if (key == KBD_PGUP)
		{
			cmon--;
			if (cmon == 0)
			{
				cmon = 12;
				cyear--;
			}
		}
		else if (key == KBD_PGDW)
		{
			cmon++;
			if (cmon > 12)
			{
				cmon = 1;
				cyear++;
			}
		}
		else if (key == KBD_ORG)
		{
			cyear--;
		}
		else if (key == KBD_END)
		{
			cyear++;
		}
		/* Clamp the day to the new month after a month or year jump: February keeps 29 days in leap years */
		if (cday > monthdays[cmon - 1])
		{
			cday = monthdays[cmon - 1];
			if ((cmon == 2) && ((((cyear % 4) == 0) && ((cyear % 100) != 0)) || ((cyear % 400) == 0)))
			{
				cday = 29;
			}
		}
	}
	conioxlib_restorescr(x, y, x + 40, y + 11, oldb);
	gotoxy(oldx, oldy);
	return(0);
}


/* ----------------------- Options grid drawn by grid() ---------------------- */
/* Draws the options in 4 columns per row; the left mouse button selects the
   cell and the wheel moves the selection. */
/* grid:
   Prototype:
   unsigned char conioxlib_grid(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, char items[][16],
       unsigned char noptions);
   Picks an option from a grid of four columns per row.
   Parameters: atr0 is the normal color and atr1 the selected
   one, items holds the texts and noptions their number.
   The arrow keys, the mouse wheel and the left button move
   the selection. Returns the index of the chosen item or 255
   when cancelled with ESC.
   */
unsigned char conioxlib_grid (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, char items[][16], unsigned char noptions)
{
	int y;
	unsigned int key;
	int x;

	if (noptions == 0)
	{
		return(255);
	}
	textattr(atr0);
	gotoxy(x1, y1 - 1);
	for (x = 0; x != noptions; x++)
	{
		if ((x & 3) == 0)
		{
			gotoxy(x1, wherey() + 1);
		}
		cprintf("%-15s", items[x]);
	}
	textattr(atr1);
	gotoxy(x1, y1);
	cputs(items[0]);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	_wscroll = 0;
	y = 0;
	do
	{
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
			}
			/* Left button */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 60)) && (conioxlib_mousestatus.y >= y1) && (conioxlib_mousestatus.y <= (y1 + ((noptions + 3) >> 2) - 1)))
			{
				textattr(atr0);
				gotoxy(((y & 3) * 15) + x1, (y >> 2) + y1);
				cputs(items[y]);
				y = ((conioxlib_mousestatus.y - y1) << 2) + (((conioxlib_mousestatus.x - x1) / 15) & 3);
				key = 0xFFFF;
			}
			/* Right button */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				key = KBD_F1;
			}
			/* Wheel: moves the selection up or down */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1) && (key != KBD_LEFT) && (key != KBD_RIGHT) && (key != 0xFFFF));
		textattr(atr0);
		gotoxy(((y & 3) * 15) + x1, (y >> 2) + y1);
		cputs(items[y]);
		if (key == KBD_UP)
		{
			y -= 4;
		}
		else if (key == KBD_DOWN)
		{
			y += 4;
		}
		else if (key == KBD_ORG)
		{
			y = 0;
		}
		else if (key == KBD_END)
		{
			y = noptions - 1;
		}
		else if (key == KBD_LEFT)
		{
			y--;
		}
		else if (key == KBD_RIGHT)
		{
			y++;
		}
		if (y > noptions - 1)
		{
			y = 0;
		}
		else if (y < 0)
		{
			y = noptions - 1;
		}
		textattr(atr1);
		gotoxy(((y & 3) * 15) + x1, (y >> 2) + y1);
		cputs(items[y]);
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1));
	if (key == KBD_ESC)
	{
		return(255);
	}
	else if (key == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(y);
	}
}


/* ----------------------- Loads the help index ------------------------ */
/* helpindex:
   Prototype:
   unsigned char conioxlib_helpindex(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, char *name, char buffer[][16]);
   Lists every topic of a help file and opens the picked
   one.
   Parameters: name is the help file and buffer a work area
   that receives the topic names. Returns the index of the
   topic shown or 255 when cancelled. The whole file is
   read in one block and parsed in memory.
   */
unsigned char conioxlib_helpindex (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, char *name, char buffer[][16])
{
	unsigned int i;
	unsigned int last;
	unsigned char oldx, oldy;
	unsigned long int size;
	char *oldb;
	char *buf;
	char *p;
	char *end;
	char *line;
	char (*helpbuf)[61];
	unsigned int len;

	helpbuf = (char (*)[61]) buffer;
	oldb = conioxlib_savescr(1, 1, 80, 25);
	if (oldb == 0)
	{
		return(255);
	}
	oldx = wherex();
	oldy = wherey();
	conioxlib_hint("Loading help...");
	buf = conioxlib_readblock(name, &size);
	if (buf == 0)
	{
		strcpy(&helpbuf[0][0], "Help is not available.");
		conioxlib_view(x1, y1, x1 + 65, y1 + 20, atr0, atr0, helpbuf, 1, "Help: Not available");
		conioxlib_restorescr(1, 1, 80, 25, oldb);
		gotoxy(oldx, oldy);
		return(255);
	}
	end = buf + size;
	last = 0;
	p = buf;
	while ((line = conioxlib_nextline(&p, end)) != 0)
	{
		if ((line[0] == '@') && (strlen(line) > 3))
		{
			if (last < 255)
			{
				memset(&buffer[last][0], 0, 16);
				len = strlen(&line[1]);
				if (len > 15)
				{
					len = 15;
				}
				memcpy(&buffer[last][0], &line[1], len);
				last++;
			}
		}
	}
	free(buf);
	if (last == 0)
	{
		strcpy(&helpbuf[0][0], "Help is not available.");
		conioxlib_view(x1, y1, x1 + 65, y1 + 20, atr0, atr0, helpbuf, 1, "Help: Not available");
		conioxlib_restorescr(1, 1, 80, 25, oldb);
		gotoxy(oldx, oldy);
		return(255);
	}
	box (x1, y1, x1 + 62, y1 + 19, atr0, A_DOUBLE, "Help: Index", 1);
	i = conioxlib_grid(x1 + 2, y1 + 1, atr0, atr1, buffer, (unsigned char) last);
	conioxlib_restorescr(1, 1, 80, 25, oldb);
	gotoxy(oldx, oldy);
	if (i == 255)
	{
		return(255);
	}
	conioxlib_help (x1, y1, x1 + 62, y1 + 18, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), name, &buffer[i][0], helpbuf);
	return(i);
}

/* ----------------------------- Apply theme ------------------------------- */
/* theme:
   Prototype:
   void conioxlib_theme(unsigned char id);
   Applies one of the nine visual themes.
   Parameters: id is 0 Borland C++ 3.1, 1 Turbo C 2.0, 2 Turbo
   C 1.x, 3 Norton Utilities 9, 4 PC Tools, 5 Turbo Pascal
   7.0, 6 Norton Commander, 7 MS-DOS Edit and 8 Lotus 1-2-3.
   Themes 0 and 5 share the real Turbo Vision 2.0 palette
   used by both Borland IDEs.
   */
void conioxlib_theme (unsigned char id)
{
	switch (id)
	{
		case 1: /* Turbo C 2.0: blue desktop, yellow text and cyan menus */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(YELLOW, BLUE);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, BLUE);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Turbo C 2.0");
			break;
		case 2: /* Turbo C 1.x */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(LIGHTGRAY, BLACK);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, BLACK);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(WHITE, BLACK);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_SINGLE;
			strcpy(conioxlib_curtheme.name, "Turbo C 1.x");
			break;
		case 3: /* Norton Utilities 9: blue with cyan frames and yellow accents */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, BLUE);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(CYAN, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Norton Utilities 9");
			break;
		case 4: /* PC Tools */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(WHITE, CYAN);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, CYAN);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "PC Tools");
			break;
		case 5: /* Turbo Pascal 7.0: Turbo Vision 2.0 desktop (blue), gray dialogs,
			   green selection and the black-on-cyan status line of the real IDE;
			   identical to the Borland C++ 3.1 theme (default): both IDEs use
			   the same Turbo Vision 2.0 palette */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, GREEN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(CYAN, LIGHTGRAY);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Turbo Pascal 7.0");
			break;
		case 6: /* Norton Commander: blue panels and cyan menus with black focus */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(WHITE, BLACK);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, CYAN);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(CYAN, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Norton Commander");
			break;
		case 7: /* MS-DOS Edit: blue screen with single-line cyan frames */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, BLUE);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(CYAN, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_SINGLE;
			strcpy(conioxlib_curtheme.name, "MS-DOS Edit");
			break;
		case 8: /* Lotus 1-2-3: black and white with inverse selection */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(WHITE, BLACK);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, WHITE);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(YELLOW, BLACK);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(WHITE, BLACK);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(WHITE, BLACK);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_SINGLE;
			strcpy(conioxlib_curtheme.name, "Lotus 1-2-3");
			break;
		default: /* Borland C++ 3.1: same Turbo Vision 2.0 palette as Turbo
			      Pascal 7.0 (case 5): blue desktop, gray dialogs, green
			      selection and black-on-cyan status line */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, GREEN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(CYAN, LIGHTGRAY);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Borland C++ 3.1");
			break;
	}
}

/* --------------------------- Read current theme -------------------------- */
/* gettheme:
   Prototype:
   void conioxlib_gettheme(struct conioxlib_stheme *t);
   Copies the active theme into t.
   Parameters: t receives atr0, atr1, atrhot, atrstatus,
   atrdialog, the frame string and the theme name.
   */
void conioxlib_gettheme (struct conioxlib_stheme *t)
{
	if (t != 0)
	{
		*t = conioxlib_curtheme;
	}
}

/* -------------------- Save screen region (malloc) ------------------------ */
/* savescr:
   Prototype:
   char *conioxlib_savescr(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2);
   Saves a screen region with malloc.
   Parameters: the rectangle to save. Returns the buffer or 0
   when there is no memory. Free it with
   conioxlib_restorescr().
   */
char *conioxlib_savescr (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
	char *buf;
	unsigned int sz;

	sz = (unsigned int) gettextsize(x1, y1, x2, y2);
	buf = (char *) malloc(sz);
	if (buf == 0)
	{
		return(0);
	}
	gettext(x1, y1, x2, y2, buf);
	return(buf);
}

/* ------------------- Restore screen region and free ---------------------- */
/* restorescr:
   Prototype:
   void conioxlib_restorescr(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2, char *buf);
   Restores a region saved by conioxlib_savescr() and frees
   the buffer.
   Parameters: the same rectangle given on save and the
   buffer returned by conioxlib_savescr().
   */
void conioxlib_restorescr (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *buf)
{
	if (buf != 0)
	{
		puttext(x1, y1, x2, y2, buf);
		free(buf);
	}
}

/* ---------------------- Status line columns 1..59 ------------------------ */
/* status:
   Prototype:
   void conioxlib_status(const char *msg);
   Writes msg on the status line.
   Parameters: msg is the text to show.
   The status line lives in columns 1 to 59 of line 25; the
   lock indicators use 60 to 74 and the clock 76 to 80.
   */
void conioxlib_status (const char *msg)
{
	char line[60];
	unsigned short cells[59];
	unsigned short cellattr;
	unsigned int i;
	unsigned int len;

	memset(line, 32, 59);
	line[59] = 0;
	if (msg != 0)
	{
		len = (unsigned int) strlen(msg);
		if (len > 59)
		{
			len = 59;
		}
		memcpy(line, msg, len);
	}
	cellattr = (unsigned short) ((unsigned int) conioxlib_curtheme.atrstatus << 8);
	for (i = 0; i < 59; i++)
	{
		cells[i] = (unsigned short) (cellattr | (unsigned char) line[i]);
	}
	puttext(1, 25, 59, 25, cells);
}

/* ------------------------- Temporary tooltip hint ------------------------ */
/* tooltip:
   Prototype:
   void conioxlib_tooltip(unsigned short x, unsigned short y,
       const char *msg, unsigned int ms);
   Shows a tooltip near (x,y) during ms milliseconds.
   Parameters: msg is the text of the tip.
   */
void conioxlib_tooltip (unsigned short x, unsigned short y, const char *msg, unsigned int ms)
{
	char *oldb;
	unsigned int len;
	unsigned short x2;

	if ((msg == 0) || (msg[0] == 0))
	{
		return;
	}
	len = (unsigned int) strlen(msg);
	if (len > 40)
	{
		len = 40;
	}
	x2 = (unsigned short) (x + len + 1);
	if (x2 > 80)
	{
		x2 = 80;
		if (x + 2 > x2)
		{
			x = (unsigned short) (x2 - 2);
		}
	}
	oldb = conioxlib_savescr(x, y, x2, y);
	if (oldb == 0)
	{
		return;
	}
	textattr(CONIOXLIB_ATTR(BLACK, YELLOW));
	gotoxy(x, y);
	cprintf(" %.*s", (int) len, msg);
	delay(ms ? ms : 1200);
	conioxlib_restorescr(x, y, x2, y, oldb);
}

/* ---------------------- Progress bar (place anywhere) -------------------- */
/* Progress ruler in the style of classic installers: the ruler spans width - 5
   cells as a low-intensity block grid that fills up with solid blocks and the
   percentage number follows it on the right, so the whole gauge always fits
   in exactly width cells. value/maxv sets the proportion. */
/* progressbar:
   Prototype:
   void conioxlib_progressbar(unsigned short x,
       unsigned short y, unsigned short width,
       unsigned char atr, unsigned int value,
       unsigned int maxv);
   Draws a Turbo Vision style progress gauge.
   Parameters: (x,y) is the position, width the length in
   cells, atr the color, and value and maxv the proportion
   shown: the ruler spans width - 5 cells as a 0xB0 grid
   that fills up with solid 0xDB cells like the Turbo
   Vision gauges and the percentage number follows it
   on the right.
   */
void conioxlib_progressbar (unsigned short x, unsigned short y, unsigned short width, unsigned char atr, unsigned int value, unsigned int maxv)
{
	unsigned int filled;
	unsigned int i;
	unsigned int pct;
	unsigned int barw;

	if (width < 8)
	{
		width = 8;
	}
	if (maxv == 0)
	{
		maxv = 1;
	}
	if (value > maxv)
	{
		value = maxv;
	}
	pct = (value * 100) / maxv;
	barw = (unsigned int) width - 5;
	textattr(atr);
	gotoxy(x, y);
	filled = (value * barw) / maxv;
	for (i = 0; i < barw; i++)
	{
		putch((i < filled) ? '\xDB' : '\xB0');
	}
	cprintf(" %3u%%", pct);
}

/* ------------------- Progress in the status line -------------------- */
/* Writes the title in columns 1-28, the ruler in columns 30-44 and the
   percentage number in columns 45-49, all with the status line color of the
   active theme, as the Turbo Vision installers did. */
/* progress:
   Prototype:
   void conioxlib_progress(const char *title,
       unsigned int value, unsigned int maxv);
   Shows the progress on the status line.
   Parameters: title is the label shown in the first 28
   columns and value and maxv the proportion. The ruler is
   drawn in columns 30 to 44, the percentage number
   follows it and both use the status line color of the
   active theme.
   */
void conioxlib_progress (const char *title, unsigned int value, unsigned int maxv)
{
	if (maxv == 0)
	{
		maxv = 1;
	}
	if (value > maxv)
	{
		value = maxv;
	}
	_wscroll = 0;
	textattr(conioxlib_curtheme.atrstatus);
	gotoxy(1, 25);
	if ((title != 0) && (title[0] != 0))
	{
		cprintf("%-28.28s", title);
	}
	else
	{
		cprintf("%*s", 28, "");
	}
	conioxlib_progressbar(30, 25, 20, conioxlib_curtheme.atrstatus, value, maxv);
}

/* --------------------------- Message box dialog -------------------------- */
/* msgbox:
   Prototype:
   unsigned char conioxlib_msgbox(const char *title,
       const char *msg, char buttons[][11],
       unsigned char nbuttons);
   Shows a message box with up to three buttons.
   Parameters: title is the caption, msg the text, buttons
   the labels and nbuttons their number (1 to 3); passing 0
   as buttons uses the defaults OK and Cancel. Returns as
   conioxlib_buttons() does: the index of the pressed
   button, 255 when F1 was pressed or 0 when cancelled.
   */
unsigned char conioxlib_msgbox (const char *title, const char *msg, char buttons[][11], unsigned char nbuttons)
{
	unsigned char oldx, oldy;
	char *oldb;
	unsigned int i;
	unsigned char r;
	char defbtn[3][11];
	char defdesc[3][71];
	char (*btns)[11];
	unsigned short x1, y1, x2, y2;
	unsigned int len;

	if ((nbuttons == 0) || (nbuttons > 3) || (buttons == 0))
	{
		nbuttons = 1;
		strcpy(defbtn[0], "OK");
		strcpy(defdesc[0], "Accept and close                                             ");
		btns = defbtn;
	}
	else
	{
		btns = buttons;
		for (i = 0; i < nbuttons; i++)
		{
			memset(defdesc[i], 32, 70);
			defdesc[i][70] = 0;
		}
	}
	len = (msg != 0) ? (unsigned int) strlen(msg) : 0;
	if (len < 24)
	{
		len = 24;
	}
	if (len > 60)
	{
		len = 60;
	}
	x1 = (unsigned short) ((80 - (len + 4)) / 2);
	x2 = (unsigned short) (x1 + len + 3);
	y1 = 9;
	y2 = 14;
	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return(255);
	}
	box(x1, y1, x2, y2, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, (char *) ((title != 0) ? title : "Message"), SHADOW);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	if (msg != 0)
	{
		cputsc(x1, x2, y1 + 1, (char *) msg);
	}
	r = conioxlib_buttons((unsigned short) (x1 + 2), (unsigned short) (y1 + 3), CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), btns, defdesc, nbuttons, 2);
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	return(r);
}

/* ------------------------ Input box (label + field) ---------------------- */
/* inputbox:
   Prototype:
   unsigned char conioxlib_inputbox(const char *title,
       const char *label, char *buf, char len);
   Asks for a string in a dialog box.
   Parameters: title is the caption, label the text next to
   the field, buf receives the answer and is also the
   initial value, and len the maximum length. Returns 0
   when accepted with ENTER, 1 when cancelled and 255 when
   F1 was pressed.
   */
unsigned char conioxlib_inputbox (const char *title, const char *label, char *buf, char len)
{
	unsigned char oldx, oldy;
	char *oldb;
	unsigned char r;
	char btns[2][11];
	char desc[2][71];
	unsigned short x1, y1, x2, y2;

	if ((buf == 0) || (len < 1))
	{
		return(255);
	}
	x1 = 18;
	x2 = 62;
	y1 = 9;
	y2 = 15;
	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return(255);
	}
	box(x1, y1, x2, y2, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, (char *) ((title != 0) ? title : "Input"), SHADOW);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	gotoxy(x1 + 2, y1 + 1);
	cputs((label != 0) ? (char *) label : "Value:");
	conioxlib_getstr((unsigned short) (x1 + 2), (unsigned short) (y1 + 2), CONIOXLIB_ATTR(WHITE, BLUE), buf, len);
	strcpy(btns[0], "OK");
	strcpy(btns[1], "Cancel");
	memset(desc[0], 32, 70);
	memset(desc[1], 32, 70);
	desc[0][70] = 0;
	desc[1][70] = 0;
	r = conioxlib_buttons((unsigned short) (x1 + 14), (unsigned short) (y1 + 4), CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), btns, desc, 2, 2);
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	return(r);
}

/* -------------- Input box digits-only with minimum length ---------------- */
/* conioxlib_inputmask(): numeric field that refuses invalid characters as they
   are typed: only digits are accepted (BACK erases, ENTER accepts, ESC
   cancels, F1 asks for help) and after OK it checks that at least mindigits
   digits were entered. Returns 0 (OK), 1 (Cancel) or 255 (help). */
/* inputmask:
   Prototype:
   unsigned char conioxlib_inputmask(const char *title,
       const char *label, char *buf, char len,
       char mindigits);
   Asks for a digits-only string in a dialog box.
   Parameters: like conioxlib_inputbox(), plus mindigits:
   the OK button only accepts the answer when at least that
   many digits were typed. Invalid characters are ignored
   as they are pressed. Returns 0 when accepted, 1 when
   cancelled and 255 when F1 was pressed.
   */
unsigned char conioxlib_inputmask (const char *title, const char *label, char *buf, char len, char mindigits)
{
	unsigned char oldx, oldy;
	char *oldb;
	unsigned char r;
	unsigned int key;
	char btns[2][11];
	char desc[2][71];
	unsigned short x1, y1, x2, y2;
	unsigned int digits;

	if ((buf == 0) || (len < 1))
	{
		return(255);
	}
	x1 = 18;
	x2 = 62;
	y1 = 9;
	y2 = 15;
	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return(255);
	}
	if (len > 38)
	{
		len = 38;
	}
	buf[0] = 0;
	digits = 0;
	do
	{
		box(x1, y1, x2, y2, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, (char *) ((title != 0) ? title : "Numeric"), SHADOW);
		textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
		gotoxy(x1 + 2, y1 + 1);
		cputs((label != 0) ? (char *) label : "Number:");
		conioxlib_hint("Only digits; BACK deletes, ENTER accepts");
		_setcursortype(_SOLIDCURSOR);
		/* Input: any key that is not a digit is ignored the very moment it is
		   pressed, so invalid characters can never get in */
		for (;;)
		{
			textattr(CONIOXLIB_ATTR(WHITE, BLUE));
			gotoxy((unsigned short) (x1 + 2), (unsigned short) (y1 + 2));
			cprintf("%-*.*s", (int) len, (int) len, buf);
			gotoxy((unsigned short) (x1 + 2 + (int) digits), (unsigned short) (y1 + 2));
			key = 0;
			do
			{
				conioxlib_clock();
				conioxlib_pollmouse();
				if (conioxlib_keyhit())
				{
					conioxlib_readkey(key);
				}
				if (key == 0)
				{
					delay(10);
				}
			}
			while (key == 0);
			if ((key >= '0') && (key <= '9') && ((int) digits < (int) len))
			{
				textattr(CONIOXLIB_ATTR(WHITE, BLUE));
				putch((char) key);
				buf[digits] = (char) key;
				digits++;
				buf[digits] = 0;
			}
			else if ((key == KBD_BACK) && (digits > 0))
			{
				textattr(CONIOXLIB_ATTR(WHITE, BLUE));
				cputs("\b \b");
				digits--;
				buf[digits] = 0;
			}
			else if ((key == KBD_ENTER) || (key == KBD_ESC) || (key == KBD_F1))
			{
				break;
			}
			/* Letters, symbols and other keys: ignored */
		}
		_setcursortype(_NOCURSOR);
		if (key == KBD_ESC)
		{
			r = 1;
		}
		else if (key == KBD_F1)
		{
			r = 255;
		}
		else
		{
			strcpy(btns[0], "OK");
			strcpy(btns[1], "Cancel");
			memset(desc[0], 32, 70);
			memset(desc[1], 32, 70);
			desc[0][70] = 0;
			desc[1][70] = 0;
			r = conioxlib_buttons((unsigned short) (x1 + 14), (unsigned short) (y1 + 4), CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), btns, desc, 2, 2);
			if (r == 255)
			{
				continue;
			}
			if (r != 0)
			{
				break;
			}
			if (digits < (unsigned int) mindigits)
			{
				conioxlib_msgbox("Validation", "Minimum number of digits not reached", 0, 0);
				r = 255;
			}
		}
	}
	while (r == 255);
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	return(r);
}

/* ------------------------ Scrollable list box ---------------------------- */
/* listbox:
   Prototype:
   unsigned char conioxlib_listbox(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2, unsigned char atr0,
       unsigned char atr1, char items[][41],
       unsigned char nitems, const char *title);
   Runs a scrollable list box with a scroll bar.
   Parameters: atr0 is the normal color, atr1 the selected
   one, items the texts (up to 40 characters), nitems their
   number and title the caption. The arrows, PGUP, PGDN,
   HOME, END and the wheel move; ENTER or a click chooses a
   row and a click on the bar jumps to that position.
   Returns the index of the chosen item or 255 when
   cancelled with ESC.
   */
unsigned char conioxlib_listbox (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, char items[][41], unsigned char nitems, const char *title)
{
	unsigned char oldx, oldy;
	char *oldb;
	int top, sel, vis, key, row, maxrow, repaint;

	if ((nitems == 0) || (items == 0))
	{
		return(255);
	}
	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return(255);
	}
	vis = (int) (y2 - y1 - 1);
	if (vis < 1)
	{
		vis = 1;
	}
	top = 0;
	sel = 0;
	repaint = 1;
	box(x1, y1, x2, y2, atr0, A_DOUBLE, (title != 0) ? title : "List", SHADOW | CLOSE);
	do
	{
		if (repaint != 0)
		{
			/* Item rows leave the last interior column free for the bar */
			textattr(atr0);
			for (row = 0; row < vis; row++)
			{
				gotoxy(x1 + 1, y1 + 1 + row);
				if (top + row < (int) nitems)
				{
					if (top + row == sel)
					{
						textattr(atr1);
					}
					else
					{
						textattr(atr0);
					}
					cprintf("%-*.*s", (int) (x2 - x1 - 2), (int) (x2 - x1 - 2), items[top + row]);
				}
				else
				{
					textattr(atr0);
					cprintf("%*s", (int) (x2 - x1 - 2), "");
				}
			}
			/* Scroll bar in the last interior column when the list overflows */
			if (((int) nitems > vis) && (vis >= 3))
			{
				textattr(atr0);
				gotoxy((unsigned short) (x2 - 1), (unsigned short) (y1 + 1));
				putch(0x18);
				for (row = 2; row < vis; row++)
				{
					gotoxy((unsigned short) (x2 - 1), (unsigned short) (y1 + row));
					putch(0xB0);
				}
				gotoxy((unsigned short) (x2 - 1), (unsigned short) (y1 + vis));
				putch(0x19);
				/* Thumb at the position of top inside the bar */
				row = 1 + (top * (vis - 3)) / ((int) nitems - vis);
				if (row > vis - 2)
				{
					row = vis - 2;
				}
				gotoxy((unsigned short) (x2 - 1), (unsigned short) (y1 + 1 + row));
				putch(0xB2);
			}
			repaint = 0;
		}
		key = 0;
		do
		{
			conioxlib_clock();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
			}
			if (conioxlib_mousestatus.lclick != 0)
			{
				if ((conioxlib_mousestatus.y == y1) && (conioxlib_mousestatus.x == (unsigned int) (x1 + 3)))
				{
					key = KBD_ESC;
				}
				else if ((conioxlib_mousestatus.y > y1) && (conioxlib_mousestatus.y < y2) && (conioxlib_mousestatus.x > x1) && (conioxlib_mousestatus.x < x2))
				{
					row = (int) conioxlib_mousestatus.y - (int) y1 - 1;
					if ((((int) nitems > vis) && (vis >= 3)) && (conioxlib_mousestatus.x == (unsigned int) (x2 - 1)))
					{
						/* Click on the scroll bar: jump to that position */
						if (row == 0)
						{
							top = 0;
						}
						else if (row >= vis - 1)
						{
							top = (int) nitems - vis;
						}
						else
						{
							top = ((row - 1) * ((int) nitems - vis)) / (vis - 3);
						}
						if (sel < top)
						{
							sel = top;
						}
						else if (sel >= top + vis)
						{
							sel = top + vis - 1;
						}
						/* Fake key 1: exit the poll loop and repaint */
						key = 1;
						repaint = 1;
					}
					else if ((row >= 0) && (top + row < (int) nitems))
					{
						sel = top + row;
						key = KBD_ENTER;
					}
				}
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(10);
			}
		}
		while ((key != 1) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER));
		if (key == KBD_UP)
		{
			if (sel > 0)
			{
				sel--;
			}
		}
		else if (key == KBD_DOWN)
		{
			if (sel < (int) nitems - 1)
			{
				sel++;
			}
		}
		else if (key == KBD_PGUP)
		{
			sel -= vis;
			if (sel < 0)
			{
				sel = 0;
			}
		}
		else if (key == KBD_PGDW)
		{
			sel += vis;
			if (sel >= (int) nitems)
			{
				sel = (int) nitems - 1;
			}
		}
		else if (key == KBD_ORG)
		{
			sel = 0;
		}
		else if (key == KBD_END)
		{
			sel = (int) nitems - 1;
		}
		if (sel < top)
		{
			top = sel;
		}
		maxrow = top + vis - 1;
		if (sel > maxrow)
		{
			top = sel - vis + 1;
		}
		/* Every key that reaches this point moved the selection or the page: the list must be repainted */
		repaint = 1;
	}
	while ((key != KBD_ENTER) && (key != KBD_ESC));
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	if (key == KBD_ESC)
	{
		return(255);
	}
	return((unsigned char) sel);
}

/* Returns whether a shift key is held: Windows polls the async key state and DOS reads the BIOS shift flags */
static int conioxlib_edit_shift (void)
{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	return ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
#else
	return ((peekb(0x40, 0x17) & 3) != 0);
#endif
}

/* Editor undo stacks: conioxlib_pushundo() pushes a full copy of the text onto
   the stack (dst) and conioxlib_clearundo() empties the opposite stack after a
   change; they only expand inside conioxlib_edit(). */
#define conioxlib_pushundo(dst, ndst) \
	do { \
		if ((ndst) == 4) \
		{ \
			free((dst)[0]); \
			(dst)[0] = (dst)[1]; \
			(dst)[1] = (dst)[2]; \
			(dst)[2] = (dst)[3]; \
			(ndst) = 3; \
		} \
		(dst)[(ndst)] = (struct conioxlib_seditundo *) malloc(sizeof(struct conioxlib_seditundo)); \
		if ((dst)[(ndst)] != 0) \
		{ \
			memcpy((dst)[(ndst)]->lines, lines, sizeof(lines)); \
			(dst)[(ndst)]->nlines = nlines; \
			(dst)[(ndst)]->cur = cur; \
			(dst)[(ndst)]->col = col; \
			(ndst)++; \
		} \
	} while (0)

#define conioxlib_clearundo(dst, ndst) \
	do { \
		while ((ndst) > 0) \
		{ \
			(ndst)--; \
			free((dst)[(ndst)]); \
		} \
	} while (0)

/* ------------------ Borland style text editor ------------------------ */
/* Editor that embeds in a rectangle (x1,y1)-(x2,y2) with a File/Edit/Search
   menu bar (F10 or click), atr0/atr1 colors and an optional title. It
   redraws only what is needed, highlights the active line with atr1, expands
   tabs to 4 spaces when painting, loads up to 250 lines, supports mouse/wheel,
   block selection with Shift, paging with PGUP/PGDW and undo/redo (4 levels).
   Returns 0. */
/* edit:
   Prototype:
   unsigned char conioxlib_edit(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2, unsigned char atr0,
       unsigned char atr1, const char *title,
       const char *filename);
   Embeds a text editor in a rectangle.
   Parameters: atr0 is the text color, atr1 the color of the
   active line, title the caption and filename the file
   loaded at startup (an empty string starts empty). The
   File, Edit and Search menus open with F10 or a click;
   the mouse and the wheel work, PGUP/PGDW page and HOME/
   END jump, and Shift plus those keys marks a block that
   Cut, Copy, Paste and typing replace. File > Open picks
   the file with conioxlib_fileselector() and the undo
   and redo stacks keep four levels. Returns 0.
   */
unsigned char conioxlib_edit (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char *title, const char *filename)
{
#define CONIOXLIB_EDIT_MAXL	250
#define CONIOXLIB_EDIT_MAXC	76
	static char CONIOXLIB_FARDATA lines[CONIOXLIB_EDIT_MAXL][CONIOXLIB_EDIT_MAXC];
	static char CONIOXLIB_FARDATA clip[CONIOXLIB_EDIT_MAXL][CONIOXLIB_EDIT_MAXC];
	char fname[80];
	char fdir[80];
	char fname2[41];
	unsigned char fdrive;
	char findstr[40];
	char repstr[40];
	char gotobuf[8];
	char outln[160];
	unsigned char oldx, oldy;
	char *oldb;
	int nlines, cur, col, top, key, i, j, vis;
	int off, lenfind, lenrep, lenrest;
	int l, width, height;
	int drawfull, drawline, oldtop, oldcur, oldnlines;
	char *s;
	FILE *f;
	char baritems[3][21] = {"&File", "&Edit", "&Search"};
	char bardesc[3][71];
	char fileitems[5][21] = {"&Open...", "&Save", "Save &as...", "-", "E&xit"};
	char filedesc[5][71];
	char edititems[7][21] = {"Cu&t", "&Copy", "&Paste", "-", "C&lear line", "&Undo", "&Redo"};
	char editdesc[7][71];
	char searchitems[3][21] = {"&Find...", "&Replace...", "&Goto line..."};
	char searchdesc[3][71];
	int barpos[3];
	int mbsel;
	int nclip;
	int selrow, selcol, selactive, selpaint, shiftpressed;
	int clr, ln, r1, c1, r2, c2, a, b, vs, ve, k;
	unsigned int selatr;
	unsigned char bar, sub;
	struct conioxlib_stheme th;
	struct conioxlib_seditundo
	{
		char lines[CONIOXLIB_EDIT_MAXL][CONIOXLIB_EDIT_MAXC];
		int nlines;
		int cur;
		int col;
	};
	struct conioxlib_seditundo *undo[4];
	struct conioxlib_seditundo *redo[4];
	int nundo, nredo;

	/* Expand tabs (stop every 4) into outln; returns visual length */
#define conioxlib_edit_expand(src, dst, maxw) \
	do { \
		int _si, _di, _ts; \
		_di = 0; \
		for (_si = 0; (src)[_si] != 0 && _di < (maxw); _si++) \
		{ \
			if ((src)[_si] == '\t') \
			{ \
				_ts = 4 - (_di % 4); \
				while (_ts-- > 0 && _di < (maxw)) \
				{ \
					(dst)[_di++] = ' '; \
				} \
			} \
			else \
			{ \
				(dst)[_di++] = (src)[_si]; \
			} \
		} \
		(dst)[_di] = 0; \
	} while (0)

	/* Compares two block selection corners: negative when the first comes before the second */
#define conioxlib_edit_selcmp(r1, c1, r2, c2)	(((r1) != (r2)) ? ((r1) - (r2)) : ((c1) - (c2)))

	/* Copies the active block selection, or the current line when none, to the multiline clipboard */
#define conioxlib_edit_selcopy() \
	do { \
		int _r1, _c1, _r2, _c2, _k, _from, _len; \
		if (selactive != 0) \
		{ \
			if (conioxlib_edit_selcmp(selrow, selcol, cur, col) <= 0) \
			{ \
				_r1 = selrow; _c1 = selcol; _r2 = cur; _c2 = col; \
			} \
			else \
			{ \
				_r1 = cur; _c1 = col; _r2 = selrow; _c2 = selcol; \
			} \
			if (_c2 > (int) strlen(lines[_r2])) \
			{ \
				_c2 = (int) strlen(lines[_r2]); \
			} \
			for (_k = _r1; _k <= _r2; _k++) \
			{ \
				_from = (_k == _r1) ? _c1 : 0; \
				_len = ((_k == _r2) ? _c2 : (int) strlen(lines[_k])) - _from; \
				if (_len < 0) \
				{ \
					_len = 0; \
				} \
				if (_len > CONIOXLIB_EDIT_MAXC - 1) \
				{ \
					_len = CONIOXLIB_EDIT_MAXC - 1; \
				} \
				memcpy(clip[_k - _r1], &lines[_k][_from], (size_t) _len); \
				clip[_k - _r1][_len] = 0; \
			} \
			nclip = _r2 - _r1 + 1; \
		} \
		else \
		{ \
			strcpy(clip[0], lines[cur]); \
			nclip = 1; \
		} \
	} while (0)

	/* Deletes the active block selection with undo; typing, ENTER and BACK call it before editing */
#define conioxlib_edit_delsel() \
	do { \
		int _r1, _c1, _r2, _c2, _k; \
		if (selactive != 0) \
		{ \
			if (conioxlib_edit_selcmp(selrow, selcol, cur, col) <= 0) \
			{ \
				_r1 = selrow; _c1 = selcol; _r2 = cur; _c2 = col; \
			} \
			else \
			{ \
				_r1 = cur; _c1 = col; _r2 = selrow; _c2 = selcol; \
			} \
			if (_c2 > (int) strlen(lines[_r2])) \
			{ \
				_c2 = (int) strlen(lines[_r2]); \
			} \
			if (_c1 > (int) strlen(lines[_r1])) \
			{ \
				_c1 = (int) strlen(lines[_r1]); \
			} \
			conioxlib_pushundo(undo, nundo); \
			conioxlib_clearundo(redo, nredo); \
			if (_r1 == _r2) \
			{ \
				memmove(&lines[_r1][_c1], &lines[_r1][_c2], (size_t) (strlen(&lines[_r1][_c2]) + 1)); \
			} \
			else \
			{ \
				strncpy(&lines[_r1][_c1], &lines[_r2][_c2], (size_t) (CONIOXLIB_EDIT_MAXC - 2 - _c1)); \
				lines[_r1][CONIOXLIB_EDIT_MAXC - 2] = 0; \
				for (_k = _r2 + 1; _k < nlines; _k++) \
				{ \
					strcpy(lines[_k - (_r2 - _r1)], lines[_k]); \
				} \
				nlines -= (_r2 - _r1); \
			} \
			cur = _r1; \
			col = _c1; \
			selactive = 0; \
			selpaint = 1; \
			drawline = 1; \
		} \
	} while (0)

	/* Computes in res the widest visible item of a drop-down, skipping separators: the width every row shares */
#define conioxlib_edit_menuw(items, nitems, res) \
	do { \
		int _w, _c, _n; \
		(res) = 1; \
		for (_w = 0; _w < (nitems); _w++) \
		{ \
			if ((items)[_w][0] != '-') \
			{ \
				_n = 0; \
				for (_c = 0; (items)[_w][_c] != 0; _c++) \
				{ \
					if ((items)[_w][_c] != '&') \
					{ \
						_n++; \
					} \
				} \
				if (_n > (res)) \
				{ \
					(res) = _n; \
				} \
			} \
		} \
	} while (0)

	/* Loads the whole file f into the lines buffer: nl receives the line count, never zero */
#define conioxlib_edit_load(f, nl) \
	do { \
		int _ll; \
		(nl) = 0; \
		while (((nl) < CONIOXLIB_EDIT_MAXL - 1) && (fgets(lines[(nl)], CONIOXLIB_EDIT_MAXC - 1, (f)) != 0)) \
		{ \
			for (_ll = 0; lines[(nl)][_ll] != 0; _ll++) \
			{ \
				if ((lines[(nl)][_ll] == '\r') || (lines[(nl)][_ll] == '\n')) \
				{ \
					lines[(nl)][_ll] = 0; \
					break; \
				} \
			} \
			(nl)++; \
		} \
		if ((nl) == 0) \
		{ \
			(nl) = 1; \
		} \
	} while (0)

	if ((x2 <= x1 + 4) || (y2 <= y1 + 3))
	{
		return(255);
	}
	width = (int) (x2 - x1 + 1);
	height = (int) (y2 - y1 + 1);
	vis = height - 3;
	if (vis < 1)
	{
		vis = 1;
	}
	if (width > CONIOXLIB_EDIT_MAXC - 1)
	{
		width = CONIOXLIB_EDIT_MAXC - 1;
	}
	memset(bardesc, 32, sizeof(bardesc));
	memset(filedesc, 32, sizeof(filedesc));
	memset(editdesc, 32, sizeof(editdesc));
	memset(searchdesc, 32, sizeof(searchdesc));
	for (i = 0; i < 4; i++)
	{
		undo[i] = 0;
		redo[i] = 0;
	}
	nundo = 0;
	nredo = 0;
	conioxlib_gettheme(&th);
	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(x1, y1, x2, y2);
	if (oldb == 0)
	{
		return(255);
	}
	memset(lines, 0, sizeof(lines));
	nclip = 0;
	findstr[0] = 0;
	repstr[0] = 0;
	nlines = 1;
	cur = 0;
	col = 0;
	top = 0;
	fname[0] = 0;
	barpos[0] = (int) x1;
	barpos[1] = (int) x1 + 6;
	barpos[2] = (int) x1 + 12;
	if ((filename != 0) && (filename[0] != 0))
	{
		strncpy(fname, filename, 79);
		fname[79] = 0;
		f = fopen(fname, "rb");
		if (f != 0)
		{
			conioxlib_edit_load(f, nlines);
			fclose(f);
		}
	}
	_setcursortype(_NORMALCURSOR);
	drawfull = 1;
	drawline = 0;
	oldtop = -1;
	oldcur = -1;
	oldnlines = -1;
	selactive = 0;
	selpaint = 0;
	mbsel = 0;
	for (;;)
	{
		selatr = (unsigned int) (((atr0 & 0x0F) << 4) | (atr0 >> 4));
		clr = ((top != oldtop) || (nlines != oldnlines));
		/* Repaint the menu bar row and the caption only on full redraws */
		if (drawfull != 0)
		{
			textattr(atr0);
			gotoxy(x1, y1);
			for (i = 0; i < (int) (x2 - x1 + 1); i++)
			{
				putch(' ');
			}
			for (i = 0; i < 3; i++)
			{
				gotoxy((unsigned short) barpos[i], y1);
				textattr(atr0);
				putch(' ');
				conioxlib_cputs_hot((unsigned short) (barpos[i] + 1), y1, atr0, th.atrhot, baritems[i]);
				textattr(atr0);
				putch(' ');
			}
			if ((title != 0) && (title[0] != 0))
			{
				textattr(atr0);
				gotoxy((unsigned short) (x2 - (unsigned short) strlen(title) - 1), y1);
				cputs((char *) title);
			}
		}
		/* Clear the text area when the page or the line count moved to wipe stale rows */
		if ((clr != 0) || (drawfull != 0))
		{
			textattr(atr0);
			window((int) x1, (int) (y1 + 1), (int) x2, (int) (y2 - 1));
			clrscr();
			window(1, 1, 80, 25);
		}
		/* Repaint the visible lines: each line is fully covered, so repainting needs no clrscr() */
		if ((drawfull != 0) || (clr != 0) || (cur != oldcur) || (drawline != 0) || (selpaint != 0))
		{
			window((int) x1, (int) (y1 + 1), (int) x2, (int) (y2 - 1));
			if (selactive != 0)
			{
				if (conioxlib_edit_selcmp(selrow, selcol, cur, col) <= 0)
				{
					r1 = selrow; c1 = selcol; r2 = cur; c2 = col;
				}
				else
				{
					r1 = cur; c1 = col; r2 = selrow; c2 = selcol;
				}
				if (c1 > (int) strlen(lines[r1]))
				{
					c1 = (int) strlen(lines[r1]);
				}
				if (c2 > (int) strlen(lines[r2]))
				{
					c2 = (int) strlen(lines[r2]);
				}
			}
			else
			{
				r1 = 0; c1 = 0; r2 = -1; c2 = 0;
			}
			for (i = 0; i < vis; i++)
			{
				if (top + i >= nlines)
				{
					break;
				}
				ln = top + i;
				gotoxy(1, i + 1);
				conioxlib_edit_expand(lines[ln], outln, width);
				textattr((ln == cur) ? atr1 : atr0);
				cprintf("%-*.*s", width, width, outln);
				/* Overlay the selected segment of the line with inverted video */
				if ((ln >= r1) && (ln <= r2))
				{
					a = (ln == r1) ? c1 : 0;
					b = ((ln == r2) ? c2 : (int) strlen(lines[ln]));
					if (b > (int) strlen(lines[ln]))
					{
						b = (int) strlen(lines[ln]);
					}
					if (b < a)
					{
						b = a;
					}
					vs = 0;
					for (k = 0; (k < a) && (lines[ln][k] != 0); k++)
					{
						vs += (lines[ln][k] == '\t') ? (4 - (vs % 4)) : 1;
					}
					ve = vs;
					for (k = a; (k < b) && (lines[ln][k] != 0); k++)
					{
						ve += (lines[ln][k] == '\t') ? (4 - (ve % 4)) : 1;
					}
					if ((ve > vs) && (vs < width))
					{
						if (ve > width)
						{
							ve = width;
						}
						textattr((unsigned char) selatr);
						gotoxy((unsigned short) (vs + 1), (unsigned short) (i + 1));
						cprintf("%-.*s", ve - vs, &outln[vs]);
					}
				}
			}
			window(1, 1, 80, 25);
			oldtop = top;
			oldcur = cur;
			oldnlines = nlines;
			drawfull = 0;
			drawline = 0;
			selpaint = 0;
		}
		/* Status bar at bottom of region */
		textattr(th.atrstatus);
		gotoxy(x1, y2);
		cprintf(" %-12.12s Ln %3d Col %3d F10=menu ESC=quit", fname[0] ? fname : "(untitled)", cur + 1, col + 1);
		/* Never putch() the bottom-right console cell: with the window
		   covering (80,25) and scrolling disabled the cursor would not
		   advance and this loop would hang the editor */
		while (wherex() <= (int) x2)
		{
			if ((wherex() >= 80) && ((int) y2 >= 25))
			{
				break;
			}
			putch(' ');
		}
		if (((int) x2 >= 80) && ((int) y2 >= 25))
		{
			unsigned short cell;

			cell = (unsigned short) ((th.atrstatus << 8) | ' ');
			puttext(80, 25, 80, 25, &cell);
		}
		/* Cursor visual: column after tab expansion up to col */
		{
			int vcol = 0;
			for (i = 0; i < col && lines[cur][i] != 0; i++)
			{
				if (lines[cur][i] == '\t')
				{
					vcol += 4 - (vcol % 4);
				}
				else
				{
					vcol++;
				}
			}
			if (vcol >= width)
			{
				vcol = width - 1;
			}
			gotoxy((unsigned short) (x1 + vcol), (unsigned short) (y1 + 1 + (cur - top)));
		}
		key = 0;
		shiftpressed = 0;
		do
		{
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(key);
				shiftpressed = conioxlib_edit_shift();
			}
			else if (conioxlib_mousestatus.lclick != 0)
			{
				if (conioxlib_mousestatus.y == (unsigned int) y1)
				{
					mbsel = (int) ((conioxlib_mousestatus.x > (unsigned int) barpos[2]) ? 2 : ((conioxlib_mousestatus.x > (unsigned int) barpos[1]) ? 1 : 0));
					key = KBD_F10;
				}
				else if ((conioxlib_mousestatus.y > (unsigned int) y1) && (conioxlib_mousestatus.y < (unsigned int) y2) && (conioxlib_mousestatus.x >= (unsigned int) x1) && (conioxlib_mousestatus.x <= (unsigned int) x2))
				{
					i = top + (int) conioxlib_mousestatus.y - (int) y1 - 1;
					if (i < nlines)
					{
						cur = i;
					}
					i = (int) conioxlib_mousestatus.x - (int) x1;
					j = (int) strlen(lines[cur]);
					if (i > j)
					{
						i = j;
					}
					if (i > CONIOXLIB_EDIT_MAXC - 2)
					{
						i = CONIOXLIB_EDIT_MAXC - 2;
					}
					if (i < 0)
					{
						i = 0;
					}
					col = i;
					if (selactive != 0)
					{
						selactive = 0;
						selpaint = 1;
					}
					key = 0x1312;
				}
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				key = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				key = KBD_UP;
			}
			if (key == 0)
			{
				delay(15);
			}
		}
		while (key == 0);
		/* Shift plus a movement key starts or extends the block selection; a plain movement clears it */
		if ((key == KBD_UP) || (key == KBD_DOWN) || (key == KBD_LEFT) || (key == KBD_RIGHT) || (key == KBD_PGUP) || (key == KBD_PGDW) || (key == KBD_ORG) || (key == KBD_END))
		{
			if (shiftpressed != 0)
			{
				if (selactive == 0)
				{
					selrow = cur;
					selcol = col;
					selactive = 1;
				}
				selpaint = 1;
			}
			else if (selactive != 0)
			{
				selactive = 0;
				selpaint = 1;
			}
		}
		if (key == KBD_ESC)
		{
			break;
		}
		else if (key == KBD_F10)
		{
			mbsel = (mbsel < 0) ? 0 : mbsel;
			bar = conioxlib_menubar(y1, atr0, atr1, th.atrhot, baritems, bardesc, 3, (unsigned char) mbsel);
			/* Keep the last visited menu so the next F10 comes back to it */
			if (bar < 3)
			{
				mbsel = (int) bar;
				if (bar == 0)
				{
					conioxlib_edit_menuw(fileitems, 5, l);
					box(x1, (unsigned short) (y1 + 1), (unsigned short) (x1 + l + 2), (unsigned short) (y1 + 7), th.atrdialog, th.frame, "", SHADOW);
					sub = conioxlib_menu((unsigned short) (x1 + 1), (unsigned short) (y1 + 2), atr0, atr1, th.atrhot, fileitems, filedesc, 5, 1, 0);
					if (sub == 0)
					{
						fdrive = 0;
						fdir[0] = 0;
						if (conioxlib_fileselector(&fdrive, fdir, fname2) != 0)
						{
							sprintf(outln, "%c:%s%s", fdrive, fdir, fname2);
						}
						else
						{
							outln[0] = 0;
						}
						f = fopen(outln, "rb");
						if (f != 0)
						{
							strcpy(fname, outln);
							memset(lines, 0, sizeof(lines));
							conioxlib_edit_load(f, nlines);
							fclose(f);
							cur = 0;
							col = 0;
							selactive = 0;
							top = 0;
						}
					}
					else if ((sub == 1) || (sub == 2))
					{
						if ((sub == 2) || (fname[0] == 0))
						{
							conioxlib_inputbox("Save as", "File:", fname, 40);
						}
						if (fname[0] != 0)
						{
							f = fopen(fname, "wb");
							if (f != 0)
							{
								for (i = 0; i < nlines; i++)
								{
									fprintf(f, "%s\r\n", lines[i]);
								}
								fclose(f);
							}
						}
					}
					else if (sub == 4)
					{
						break;
					}
				}
				else if (bar == 1)
				{
					conioxlib_edit_menuw(edititems, 7, l);
					box((unsigned short) (x1 + 6), (unsigned short) (y1 + 1), (unsigned short) (x1 + l + 8), (unsigned short) (y1 + 9), th.atrdialog, th.frame, "", SHADOW);
					sub = conioxlib_menu((unsigned short) (x1 + 7), (unsigned short) (y1 + 2), atr0, atr1, th.atrhot, edititems, editdesc, 7, 1, 0);
					if (sub == 0)
					{
						if (selactive != 0)
						{
							conioxlib_edit_selcopy();
							conioxlib_edit_delsel();
						}
						else
						{
							conioxlib_pushundo(undo, nundo);
							conioxlib_clearundo(redo, nredo);
							strcpy(clip[0], lines[cur]);
							nclip = 1;
							lines[cur][0] = 0;
							col = 0;
							drawline = 1;
						}
					}
					else if (sub == 1)
					{
						conioxlib_edit_selcopy();
					}
					else if (sub == 2)
					{
						if (nclip > 0)
						{
							if (selactive != 0)
							{
								conioxlib_edit_delsel();
							}
							else
							{
								conioxlib_pushundo(undo, nundo);
								conioxlib_clearundo(redo, nredo);
							}
							if ((nclip == 1) && ((int) strlen(lines[cur]) + (int) strlen(clip[0]) < CONIOXLIB_EDIT_MAXC - 1))
							{
								memmove(&lines[cur][col + (int) strlen(clip[0])], &lines[cur][col], (size_t) (strlen(&lines[cur][col]) + 1));
								memcpy(&lines[cur][col], clip[0], (size_t) strlen(clip[0]));
								col += (int) strlen(clip[0]);
								drawline = 1;
							}
							else if ((nclip > 1) && (nlines + nclip - 1 < CONIOXLIB_EDIT_MAXL) && (col + (int) strlen(clip[0]) < CONIOXLIB_EDIT_MAXC - 1) && ((int) strlen(clip[nclip - 1]) + (int) strlen(&lines[cur][col]) < CONIOXLIB_EDIT_MAXC - 1))
							{
								strcpy(outln, &lines[cur][col]);
								strcpy(&lines[cur][col], clip[0]);
								for (j = nlines - 1; j > cur; j--)
								{
									strcpy(lines[j + nclip - 1], lines[j]);
								}
								for (j = 1; j < nclip - 1; j++)
								{
									strcpy(lines[cur + j], clip[j]);
								}
								strcpy(lines[cur + nclip - 1], clip[nclip - 1]);
								strcat(lines[cur + nclip - 1], outln);
								nlines += nclip - 1;
								cur += nclip - 1;
								col = (int) strlen(clip[nclip - 1]);
								drawline = 1;
							}
						}
					}
					else if (sub == 4)
					{
						if (selactive != 0)
						{
							conioxlib_edit_delsel();
						}
						else
						{
							conioxlib_pushundo(undo, nundo);
							conioxlib_clearundo(redo, nredo);
							lines[cur][0] = 0;
							col = 0;
							drawline = 1;
						}
					}
					else if (sub == 5)
					{
						if (nundo > 0)
						{
							conioxlib_pushundo(redo, nredo);
							nundo--;
							memcpy(lines, undo[nundo]->lines, sizeof(lines));
							nlines = undo[nundo]->nlines;
							cur = undo[nundo]->cur;
							col = undo[nundo]->col;
							free(undo[nundo]);
							selactive = 0;
						}
					}
					else if (sub == 6)
					{
						if (nredo > 0)
						{
							conioxlib_pushundo(undo, nundo);
							nredo--;
							memcpy(lines, redo[nredo]->lines, sizeof(lines));
							nlines = redo[nredo]->nlines;
							cur = redo[nredo]->cur;
							col = redo[nredo]->col;
							free(redo[nredo]);
							selactive = 0;
						}
					}
				}
				else if (bar == 2)
				{
					conioxlib_edit_menuw(searchitems, 3, l);
					box((unsigned short) (x1 + 12), (unsigned short) (y1 + 1), (unsigned short) (x1 + l + 14), (unsigned short) (y1 + 5), th.atrdialog, th.frame, "", SHADOW);
					sub = conioxlib_menu((unsigned short) (x1 + 13), (unsigned short) (y1 + 2), atr0, atr1, th.atrhot, searchitems, searchdesc, 3, 1, 0);
					if (sub == 0)
					{
						conioxlib_inputbox("Find", "Text:", findstr, 30);
						if (findstr[0] != 0)
						{
							for (i = cur; i < nlines; i++)
							{
							s = strstr(&lines[i][(i == cur) ? col : 0], findstr);
							if (s != 0)
							{
								cur = i;
								col = (int) (s - lines[i]);
								break;
							}
							}
						}
					}
					else if (sub == 1)
					{
						conioxlib_inputbox("Replace", "Find:", findstr, 30);
						if (findstr[0] != 0)
						{
							conioxlib_inputbox("Replace", "With:", repstr, 30);
							lenfind = (int) strlen(findstr);
							lenrep = (int) strlen(repstr);
							s = 0;
							for (i = cur; (i < nlines) && (s == 0); i++)
							{
								s = strstr(&lines[i][(i == cur) ? col : 0], findstr);
							}
							if (s != 0)
							{
								i--;
								off = (int) (s - lines[i]);
								lenrest = (int) strlen(&lines[i][off + lenfind]);
								if (off + lenrep + lenrest < CONIOXLIB_EDIT_MAXC - 1)
								{
									conioxlib_pushundo(undo, nundo);
									conioxlib_clearundo(redo, nredo);
									memmove(&lines[i][off + lenrep], &lines[i][off + lenfind], (size_t) lenrest + 1);
									memcpy(&lines[i][off], repstr, (size_t) lenrep);
									cur = i;
									col = off + lenrep;
								}
							}
						}
					}
					else if (sub == 2)
					{
						if (conioxlib_inputmask("Goto", "Line:", gotobuf, 4, 1) == 0)
						{
							i = atoi(gotobuf);
							if ((i >= 1) && (i <= nlines))
							{
								cur = i - 1;
								col = 0;
							}
						}
					}
				}
			}
			drawfull = 1;
		}
		/* Vertical moves drop the cursor on the line end when the column sits past it, so typing always lands on screen */
		else if (key == KBD_UP)
		{
			if (cur > 0)
			{
				cur--;
				if (col > (int) strlen(lines[cur]))
				{
					col = (int) strlen(lines[cur]);
				}
			}
		}
		else if (key == KBD_DOWN)
		{
			if (cur < nlines - 1)
			{
				cur++;
				if (col > (int) strlen(lines[cur]))
				{
					col = (int) strlen(lines[cur]);
				}
			}
		}
		else if (key == KBD_LEFT)
		{
			if (col > 0)
			{
				col--;
			}
		}
		else if (key == KBD_RIGHT)
		{
			if ((col < (int) strlen(lines[cur])) && (col < CONIOXLIB_EDIT_MAXC - 2))
			{
				col++;
			}
		}
		else if (key == KBD_PGUP)
		{
			cur -= vis;
			if (cur < 0)
			{
				cur = 0;
			}
			if (col > (int) strlen(lines[cur]))
			{
				col = (int) strlen(lines[cur]);
			}
		}
		else if (key == KBD_PGDW)
		{
			cur += vis;
			if (cur > nlines - 1)
			{
				cur = nlines - 1;
			}
			if (col > (int) strlen(lines[cur]))
			{
				col = (int) strlen(lines[cur]);
			}
		}
		else if (key == KBD_ORG)
		{
			col = 0;
		}
		else if (key == KBD_END)
		{
			col = (int) strlen(lines[cur]);
			if (col > CONIOXLIB_EDIT_MAXC - 2)
			{
				col = CONIOXLIB_EDIT_MAXC - 2;
			}
		}
		else if (key == KBD_ENTER)
		{
			j = (selactive != 0) ? 1 : 0;
			if (selactive != 0)
			{
				conioxlib_edit_delsel();
			}
			if (nlines < CONIOXLIB_EDIT_MAXL - 1)
			{
				if (j == 0)
				{
					conioxlib_pushundo(undo, nundo);
					conioxlib_clearundo(redo, nredo);
				}
				for (j = nlines; j > cur + 1; j--)
				{
					strcpy(lines[j], lines[j - 1]);
				}
				strcpy(lines[cur + 1], &lines[cur][col]);
				lines[cur][col] = 0;
				nlines++;
				cur++;
				col = 0;
			}
		}
		else if (key == KBD_BACK)
		{
			if (selactive != 0)
			{
				conioxlib_edit_delsel();
			}
			else if (col > 0)
			{
				conioxlib_pushundo(undo, nundo);
				conioxlib_clearundo(redo, nredo);
				for (i = col - 1; lines[cur][i] != 0; i++)
				{
					lines[cur][i] = lines[cur][i + 1];
				}
				col--;
				drawline = 1;
			}
		}
		else if (key == '\t')
		{
			j = (selactive != 0) ? 1 : 0;
			if (selactive != 0)
			{
				conioxlib_edit_delsel();
			}
			i = (int) strlen(lines[cur]);
			if (i < CONIOXLIB_EDIT_MAXC - 2)
			{
				if (j == 0)
				{
					conioxlib_pushundo(undo, nundo);
					conioxlib_clearundo(redo, nredo);
				}
				for (j = i; j >= col; j--)
				{
					lines[cur][j + 1] = lines[cur][j];
				}
				lines[cur][col] = '\t';
				col++;
				drawline = 1;
			}
		}
		else if ((key >= 32) && (key < 127))
		{
			j = (selactive != 0) ? 1 : 0;
			if (selactive != 0)
			{
				conioxlib_edit_delsel();
			}
			i = (int) strlen(lines[cur]);
			if (i < CONIOXLIB_EDIT_MAXC - 2)
			{
				if (j == 0)
				{
					conioxlib_pushundo(undo, nundo);
					conioxlib_clearundo(redo, nredo);
				}
				for (j = i; j >= col; j--)
				{
					lines[cur][j + 1] = lines[cur][j];
				}
				lines[cur][col] = (char) key;
				col++;
				drawline = 1;
			}
		}
		if (cur < top)
		{
			top = cur;
		}
		if (cur >= top + vis)
		{
			top = cur - vis + 1;
		}
	}
	_setcursortype(_NOCURSOR);
	conioxlib_clearundo(undo, nundo);
	conioxlib_clearundo(redo, nredo);
	conioxlib_restorescr(x1, y1, x2, y2, oldb);
	gotoxy(oldx, oldy);
	return(0);
#undef CONIOXLIB_EDIT_MAXL
#undef CONIOXLIB_EDIT_MAXC
#undef conioxlib_edit_expand
#undef conioxlib_edit_load
#undef conioxlib_edit_menuw
#undef conioxlib_edit_selcmp
#undef conioxlib_edit_selcopy
#undef conioxlib_edit_delsel
}
