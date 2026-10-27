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

/* int86 y registros: Watcom usa r.w, el resto usa r.x */
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

/* Tema activo: al principio del fichero porque clock()/blocks()/status() y la
   macro conioxlib_hint() lo usan desde el primer momento */
static struct conioxlib_stheme conioxlib_curtheme = {
	CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
	CONIOXLIB_ATTR(BLACK, GREEN),
	CONIOXLIB_ATTR(RED, LIGHTGRAY),
	CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
	CONIOXLIB_ATTR(WHITE, BLUE),
	CONIOXLIB_BOX_A_DOUBLE,
	"Borland C++ 3.1"
};

/* Tecla pendiente capturada por conioxlib_pollmouse() bajo Windows */
static int conioxlib_pendingkey = 0;

static int conioxlib_select_sort(const void *a, const void *b);
static void conioxlib_select_format(char *res, long int n);

/* Lee una tecla portable: las extendidas se devuelven como (scan << 8) y las demas como su codigo ASCII */
#define conioxlib_readkey(t)	{ if (conioxlib_pendingkey) { t = conioxlib_pendingkey; conioxlib_pendingkey = 0; } else { int conioxlib_k = getch(); if (conioxlib_k == 0) { t = ((int) getch()) << 8; } else { t = conioxlib_k; } } }
#define conioxlib_keyhit()		(conioxlib_pendingkey || kbhit())

/* Escribe una descripcion de ayuda en la linea de estado (columnas 1-59) con
   el color del tema activo; lo usan menu/menubar/buttons/option/property */
#define conioxlib_hint(d)	do { textattr(conioxlib_curtheme.atrstatus); _wscroll = 0; gotoxy(1, 25); cprintf("%-59.59s", (d)); } while (0)

/* ------------------------- Comprueba tecla pendiente --------------------- */
int conioxlib_kbhit(void)
{
	return(conioxlib_keyhit());
}

/* ---------------------------- Lee una tecla ------------------------------ */
unsigned int conioxlib_getkey(void)
{
	unsigned int t;
	conioxlib_readkey(t);
	return(t);
}


/* ------------------- Localiza la tecla caliente de un texto --------------- */
/* Acelerador: '&X' marca X como tecla caliente (el '&' no se dibuja); si no hay
   '&', la primera letra A-Z/a-z es la tecla caliente. */
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

/* ---------------- Escribe un texto con tecla caliente resaltada ----------- */
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

/* ---------------- Comprueba si una tecla activa una opcion --------------- */
static int conioxlib_hotmatch(const char *s, unsigned int tecla)
{
	char h;
	unsigned int k;

	if (tecla > 127)
	{
		return(0);
	}
	h = conioxlib_hotchar(s);
	if (h == 0)
	{
		return(0);
	}
	k = tecla;
	if (k >= 'a' && k <= 'z')
	{
		k = k - 'a' + 'A';
	}
	if (h >= 'a' && h <= 'z')
	{
		h = (char) (h - 'a' + 'A');
	}
	return(k == (unsigned int) (unsigned char) h);
}



/* ----------------------- Escribe un texto centrado ---------------------- */
void conioxlib_cputsc (unsigned short x1, unsigned short x2, unsigned short y, char *str)
{
	unsigned int x;

	x = (((x2 - x1 + 1) >> 1) - (strlen(str) >> 1) + x1);
	gotoxy(x, y);
	cputs(str);
}


/* -------------------------- Dibuja una ventana --------------------------- */
void conioxlib_box (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned int atr, char *frame, char *title, unsigned int flags)
{
	unsigned int x, y;
	unsigned int c1, c2;
	char l1[82], l2[82], l3[82], tmp[330];

	/* Shadow */
	if (flags & 1)
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
	/* Sin ventana */
	if ((flags & 4) == 0)
	{
		x = (x2 - x1);
		memset(&l1, 0, x + 2);
		memset(&l2, 0, x + 2);
		memset(&l3, 0, x + 2);
		l1[0] = frame[0];
		memset(&l1[1], frame[2], x - 1);
		l1[x] = frame[1];
		l2[0] = frame[4];
		memset(&l2[1], 32, x - 1);
		l2[x] = frame[4];
		l3[0] = frame[5];
		memset(&l3[1], frame[3], x - 1);
		l3[x] = frame[6];
		textattr(atr);
		gotoxy(x1, y1);
		cputs(l1);
		if (title[0] != 0)
		{
			sprintf(tmp, " %s ", title);
			conioxlib_cputsc(x1, x2, y1, tmp);
		}
		/* Cierre */
		if (flags & 2)
		{
			gotoxy(x1 + 2, y1);
			cputs("[\xFE]");
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


/* ----------------------------- Escribe el reloj -------------------------- */
void conioxlib_clock (void)
{
	static unsigned int oldmin = 0xFFFF;
	unsigned int oldx, oldy;
	unsigned int oldwl, oldwt, oldwr, oldwb;
	time_t t;
	struct tm *ti;
	struct text_info tiinfo;

	time(&t);
	ti = localtime(&t);
	/* Refresh lock indicators every tick path; blocks() early-outs if unchanged */
	conioxlib_blocks();
	if (oldmin != (unsigned int) ti->tm_min)
	{
		oldmin = (unsigned int) ti->tm_min;
		oldx = wherex();
		oldy = wherey();
		gettextinfo(&tiinfo);
		oldwl = tiinfo.winleft;
		oldwt = tiinfo.wintop;
		oldwr = tiinfo.winright;
		oldwb = tiinfo.winbottom;
		{
			char clk[8];
			unsigned short cells[5];
			unsigned short cellattr;
			unsigned int ci;

			/* Reloj en columnas 76-80; las cerraduras ocupan 60-74 y status() 1-59 */
			sprintf(clk, "%2u:%02u", (unsigned int) ti->tm_hour, (unsigned int) ti->tm_min);
			cellattr = (unsigned short) ((unsigned int) conioxlib_curtheme.atrstatus << 8);
			for (ci = 0; ci < 5; ci++)
			{
				cells[ci] = (unsigned short) (cellattr | (unsigned char) clk[ci]);
			}
			puttext(76, 25, 80, 25, cells);
		}
		window((int) oldwl, (int) oldwt, (int) oldwr, (int) oldwb);
		gotoxy(oldx, oldy);
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
BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);

static int conioxlib_mousemode = 0;
/* Flancos de los botones para el sondeo asincrono (Windows Terminal no entrega
   eventos de raton por ReadConsoleInput); GetDoubleClickTime marca el doble clic */
static int conioxlib_oldlbtn = 0;
static int conioxlib_oldrbtn = 0;
static DWORD conioxlib_lastlclick = 0;
static DWORD conioxlib_lastrclick = 0;

/* --------------------------- Lee el estado del raton -------------------- */
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
	/* Re-afirmar el modo en cada llamada: otros lectores de la consola (o lecturas
	   previas) pueden limpiar ENABLE_MOUSE_INPUT, y entonces lclick/rclick/wheelx/
	   wheely saldrian siempre a 0. ENABLE_EXTENDED_FLAGS es necesario para poder
	   quitar ENABLE_QUICK_EDIT_MODE; si no, la consola roba cada clic para
	   seleccionar texto y el programa nunca ve el raton. */
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
	/* Windows Terminal no entrega eventos de raton: sondeo asincrono de los
	   botones con deteccion de flancos. Solo se aceptan clics cuando la consola
	   tiene el foco y el cursor esta dentro del area cliente; en la consola
	   clasica (conhost) los eventos ya se han leido arriba. */
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

/* Puntero propio bajo DOS: con conioxlib_vgaremap(1) el puntero del driver se
   oculta y se dibuja el glifo 0x01 con el atributo intercambiado por nibbles;
   conioxlib_ptrcell guarda la celda original para restaurarla al moverse */
static int conioxlib_mouseptr = 0;
static int conioxlib_ptrx = -1;
static int conioxlib_ptry = -1;
static unsigned short conioxlib_ptrcell = 0;

/* --------------------------- Inicializa el raton ------------------------ */
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


/* --------------------------- Lee el estado del raton -------------------- */
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
		/* Puntero propio: borrar el anterior si nadie pinto encima y dibujar
		   el glifo en la posicion actual */
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


/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void conioxlib_blocks (void)
{
	static char oldstatus[16];
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
	/* Always repaint: background() clears line 25 and the static cache would skip the redraw */
	(void) oldstatus;
	memcpy(oldstatus, status, sizeof(status));
	/* puttext evita problemas de window/gotoxy en consolas Win32 con buffer; la linea de
	   estado es fija 80x25: status() escribe en 1-59, las cerraduras en 60-74 */
	cellattr = (unsigned short) ((unsigned int) conioxlib_curtheme.atrstatus << 8);
	for (i = 0; i < 15; i++)
	{
		cells[i] = (unsigned short) (cellattr | (unsigned char) status[i]);
	}
	puttext(60, 25, 74, 25, cells);
}


/* ------------------- Obtiene informacion del COUNTRY -------------------- */
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
	/* Reservar memoria real mode via DPMI (int 31h AX=0100) */
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
	/* Liberar la memoria reservada */
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

/* -------- Espacio libre en disco: FAT32 con int 21h AX=7303h ------------- */
/* drive: 0 = unidad actual, 1 = A, 2 = B... Rellena df con los tamanos de
   cluster/sector y los clusters libres/totales; devuelve 0 si todo fue bien y
   1 si fallo. En DOS se prueba primero la extension 7303h (soporta FAT32) y
   si no esta disponible se usa el clasico AH=36h. */
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
		/* Reservar memoria real mode via DPMI (int 31h AX=0100), como en getcountry():
		   "X:\" en el parrafo 0 y la estructura en el parrafo 2 */
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
		/* Liberar la memoria reservada */
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


/* ------------------- Tamano de un fichero en bytes ----------------------- */
/* Devuelve la longitud del fichero o 0xFFFFFFFF si no se puede abrir/leer. */
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


/* --------------------- Cambia la paleta por defecto --------------------- */
void conioxlib_setpal (unsigned char e)
{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	(void) e;
	/* La paleta DAC no existe en la consola de Windows */
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
	/* Programar el DAC del registro que usa cada color en modo texto (la tabla dac), no el registro 0-15 */
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
/* ---------------- Remapea glifos de la fuente VGA (solo DOS) ------------ */
/* conioxlib_vgaremap(1) reprograma los glifos 0x01 (puntero de raton), 0x10/
   0x11 (triangulos solidos) y 0x18/0x19 (flechas gruesas de la fuente 8x16
   para que los scrollbars y el puntero propio tengan estilo Turbo Vision;
   oculta el puntero del driver y activa el dibujo propio de pollmouse().
   Con 0 restaura el puntero del driver. En Windows no hace falta: la consola
   ya trae los caracteres de CP437; con Watcom de 32 bits la union REGS no
   expone BP y tampoco se puede remapear. */
void conioxlib_vgaremap (unsigned char on)
{
#if ((__DOS__) || (__MSDOS__))
	static unsigned char conioxlib_glyphtab[5][16] = {
		{0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xBC, 0x98, 0x88, 0x80, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00}};
	static unsigned char conioxlib_glyphchr[5] = {0x01, 0x10, 0x11, 0x18, 0x19};
	unsigned int i;
	union REGS r;

	/* Activar el puntero propio y ocultar el del driver (o restaurarlo) */
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


/* ------------------------ Espera una pulsacion -------------------------- */
unsigned int conioxlib_ckey (void)
{
	unsigned int tecla;

	tecla = 0;
	do
	{
		watch();
		conioxlib_pollmouse();
		if (!conioxlib_keyhit())
		{
			delay(10);
		}
	}
	while (!conioxlib_keyhit());
	conioxlib_readkey(tecla);
	return(tecla);
}


/* ----------------- Borra la pantalla con efecto de empuje ---------------- */
void conioxlib_push (void)
{
	unsigned int i;
	char l[81];

	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
	for (i = 0; i != 13; i++)
	{
		/* Zona 1: filas 1-13, columnas 1-14, abajo */
		movetext(1, 1, 14, 12, 1, 2);
		memset(l, 32, 14);
		l[14] = 0;
		gotoxy(1, 1);
		cputs(l);
		/* Zona 2: filas 1-13, columnas 15-64, arriba */
		movetext(15, 2, 64, 13, 15, 1);
		memset(l, 32, 50);
		l[50] = 0;
		gotoxy(15, 13);
		cputs(l);
		/* Zona 3: filas 1-13, columnas 65-80, abajo */
		movetext(65, 1, 80, 12, 65, 2);
		memset(l, 32, 16);
		l[16] = 0;
		gotoxy(65, 1);
		cputs(l);
		/* Zona 4: filas 14-25, columnas 1-14, arriba */
		movetext(1, 15, 14, 25, 1, 14);
		memset(l, 32, 14);
		l[14] = 0;
		gotoxy(1, 25);
		cputs(l);
		/* Zona 5: filas 14-25, columnas 15-64, abajo */
		movetext(15, 14, 64, 24, 15, 15);
		memset(l, 32, 50);
		l[50] = 0;
		gotoxy(15, 14);
		cputs(l);
		/* Zona 6: filas 14-25, columnas 65-80, arriba */
		movetext(65, 15, 80, 25, 65, 14);
		memset(l, 32, 16);
		l[16] = 0;
		gotoxy(65, 25);
		cputs(l);
		delay(50);
	}
}


/* -------------------- Consigue una cadena por teclado ------------------- */
void conioxlib_getstr (unsigned short x, unsigned short y, unsigned char atr, char *str, char len)
{
	int i;
	unsigned int tec;
	char aux[80];

	conioxlib_hint("Input a valid string");
	_setcursortype(_SOLIDCURSOR);
	memset(aux, 0, 80);
	memset(aux, 32, len + 1);
	textattr(atr);
	gotoxy(x, y);
	cputs(aux);
	strcpy(aux, str);
	i = strlen(aux);
	/* Recorta la cadena inicial a la longitud del campo para no desbordar aux */
	if (i > len)
	{
		i = len;
		aux[len] = 0;
	}
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
				cputs("\b \b");
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
			i = strlen(aux);
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


/* ------------------- Consigue un password por teclado ------------------- */
void conioxlib_getpw (unsigned short x, unsigned short y, unsigned char atr, char *pass, char len)
{
	int i;
	unsigned int tec;

	conioxlib_hint("Type password");
	_setcursortype(_NOCURSOR);
	memset(pass, 0, len + 1);
	memset(pass, 32, len);
	textattr(atr);
	gotoxy(x, y);
	cputs(pass);
	pass[0] = 0;
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
				cputs("\b \b");
				i--;
				pass[i] = 0;
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
				pass[i] = (char) tec;
				i++;
				pass[i] = 0;
			}
		}
	}
	while ((tec != KBD_ENTER) && (tec != KBD_ESC));
	pass[i] = 0;
	_setcursortype(_NOCURSOR);
}


/* --------------------- Dibuja botones estilo Turbo Vision ---------------- */
/* Boton estilo Turbo Vision 2.0: cara de espacios coloreados con el titulo
   centrado (tecla caliente en color atrhot), esquina superior derecha 0xDC y
   fila inferior de sombra 0xDF, ambas con el atributo de sombra 0x08, sin
   caracteres de marco. El ancho de cada boton es la longitud visible del
   titulo + 4; "separation" es el hueco horizontal entre botones. Devuelve el
   indice del boton pulsado, 255 si se pulso F1 (ayuda) o 0 si se cancelo
   con ESC. */
#define conioxlib_drawbtn(b, c) \
	do { \
		int conioxlib_z; \
		\
		/* Cara: fila de espacios con el titulo centrado */ \
		textattr(c); \
		gotoxy((unsigned short) pos[b], y1); \
		for (conioxlib_z = 0; conioxlib_z < wide[b]; conioxlib_z++) \
		{ \
			putch(' '); \
		} \
		conioxlib_cputs_hot((unsigned short) (pos[b] + 1 + ((wide[b] - vislen[b] - 2) / 2)), y1, c, atrhot, items[b]); \
		/* Esquina superior derecha: medio bloque con el color de la sombra */ \
		gotoxy((unsigned short) (pos[b] + wide[b] - 1), y1); \
		textattr(0x08); \
		putch(0xDC); \
		/* Fila inferior: espacio + sombra completa con el color de la sombra */ \
		gotoxy((unsigned short) pos[b], (unsigned short) (y1 + 1)); \
		textattr(0x08); \
		putch(' '); \
		for (conioxlib_z = 1; conioxlib_z < wide[b]; conioxlib_z++) \
		{ \
			putch(0xDF); \
		} \
	} while (0)

unsigned char conioxlib_buttons (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][11], char desc[][71], unsigned char noptions, unsigned char separation)
{
	int x;
	unsigned int tecla;
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
				if ((tecla >= 32) && (tecla < 127))
				{
					for (y = 0; y < (int) noptions; y++)
					{
						if (conioxlib_hotmatch(items[y], tecla))
						{
							if (y != x)
							{
								conioxlib_drawbtn(x, atr0);
								x = y;
								conioxlib_drawbtn(x, atr1);
								conioxlib_hint(desc[x]);
							}
							tecla = KBD_ENTER;
							break;
						}
					}
				}
			}
			/* Boton izquierdo sobre la cara o la fila de sombra del boton */
			if ((tecla == 0) && (conioxlib_mousestatus.lclick != 0) && ((conioxlib_mousestatus.y == y1) || (conioxlib_mousestatus.y == (unsigned int) y1 + 1)))
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
						tecla = KBD_ENTER;
						break;
					}
				}
			}
			/* Boton derecho */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			/* Rueda: mueve al boton siguiente o anterior */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_RIGHT;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_LEFT;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_LEFT) && (tecla != KBD_RIGHT) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_F1));
		if (tecla == KBD_LEFT)
		{
			conioxlib_drawbtn(x, atr0);
			x--;
		}
		else if (tecla == KBD_RIGHT)
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
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F1));
	if (tecla == KBD_ESC)
	{
		return(0);
	}
	else if (tecla == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(x);
	}
}

/* -------------------------- Construye un menu ----------------------------- */
/* Los items cuyo primer caracter es '-' son separadores no seleccionables (estilo
   Borland). Todas las filas ocupan la misma anchura l: la maxima longitud visible
   de un item (el '&' no cuenta), limitada a 20; por eso el resaltado es de ancho
   completo y no quedan restos al redibujar. Si no caben todos los items entre y1
   y la fila 23 aparece una barra de desplazamiento a la derecha (0x18/0x19 y
   cursor 0xB2 sobre 0xB0) y se pagina con PGUP/PGDN, la rueda o el clic en la
   barra. sel fija el item inicial; se devuelve el indice elegido. */
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

unsigned char conioxlib_menu (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], unsigned char noptions, unsigned char separation, unsigned char sel)
{
	int y, prev;
	unsigned int tecla;
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
	/* Items visibles entre y1 y la fila 23; la fila 24 queda libre y la 25
	   es la linea de estado */
	nvis = (23 - (int) y1) / (int) separation;
	if (nvis < 1)
	{
		nvis = 1;
	}
	if ((int) noptions < nvis)
	{
		nvis = (int) noptions;
	}
	/* Seleccion inicial: acotar y saltar separadores */
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
			/* Filas completas de ancho l: sin restos de dibujos anteriores */
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
			/* Barra de desplazamiento en la columna x1+l */
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
				/* Cursor: desplazado segun top dentro del recorrido */
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
				if ((tecla >= 32) && (tecla < 127))
				{
					for (x = 0; x < (int) noptions; x++)
					{
						if ((items[x][0] != '-') && conioxlib_hotmatch(items[x], tecla))
						{
							prev = y;
							y = x;
							tecla = KBD_ENTER;
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
						/* Clic sobre un item: elegirlo */
						i = top + row;
						if ((i < (int) noptions) && (items[i][0] != '-'))
						{
							prev = y;
							y = i;
							tecla = KBD_ENTER;
						}
					}
					else if (((int) noptions > nvis) && (conioxlib_mousestatus.x == (x1 + (unsigned int) l)))
					{
						/* Clic en la barra: saltar a la pagina correspondiente */
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
					}
				}
			}
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if ((tecla == 0) && (redraw == 0))
			{
				delay(10);
			}
		}
		while ((tecla != KBD_UP) && (tecla != KBD_DOWN) && (tecla != KBD_LEFT) && (tecla != KBD_RIGHT) && (tecla != KBD_PGUP) && (tecla != KBD_PGDW) && (tecla != KBD_ORG) && (tecla != KBD_END) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_F1));
		if (tecla != 0)
		{
			if (tecla == KBD_UP)
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
			else if (tecla == KBD_DOWN)
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
			else if ((tecla == KBD_PGUP) || (tecla == KBD_PGDW))
			{
				prev = y;
				y += (tecla == KBD_PGDW) ? nvis : -nvis;
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
			else if (tecla == KBD_ORG)
			{
				prev = y;
				y = 0;
				while ((y < (int) noptions) && (items[y][0] == '-'))
				{
					y++;
				}
			}
			else if (tecla == KBD_END)
			{
				prev = y;
				y = noptions - 1;
				while ((y > 0) && (items[y][0] == '-'))
				{
					y--;
				}
			}
			/* Mantener el item visible y acotar la pagina */
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
		if ((redraw == 0) && (tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F1) && (tecla != KBD_LEFT) && (tecla != KBD_RIGHT))
		{
			/* Solo cambio el resaltado: repintar la fila anterior y la nueva */
			conioxlib_menuline(prev, prev - top, atr0);
			conioxlib_menuline(y, y - top, atr1);
		}
		if (tecla == KBD_ESC)
		{
			return(CONIOXLIB_MENU_ESC);
		}
		else if (tecla == KBD_F1)
		{
			return(CONIOXLIB_MENU_HELP);
		}
		else if (tecla == KBD_LEFT)
		{
			return(CONIOXLIB_MENU_LEFT);
		}
		else if (tecla == KBD_RIGHT)
		{
			return(CONIOXLIB_MENU_RIGHT);
		}
		else if (tecla == KBD_ENTER)
		{
			return((unsigned char) y);
		}
	}
}


/* ----------------------- Barra de menus superior (Borland) ---------------- */
/* Devuelve el indice del item elegido o CONIOXLIB_MENU_ESC/HELP. El parametro
   init fija el item resaltado al entrar (se acota a noptions-1), para que el
   llamador pueda volver a la misma opcion; la rueda horizontal mueve la
   seleccion y el clic directo entra en el item. */
unsigned char conioxlib_menubar (unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], unsigned char noptions, unsigned char init)
{
	int i, sel, prev, pos, w, tecla, cell;
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
	w = 0;
	for (i = 0; i < (int) noptions; i++)
	{
		/* La anchura cuenta solo caracteres visibles: el '&' de la tecla
		   caliente no ocupa celda en pantalla */
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
		/* cell = label + one space of padding on each side for the highlight bar */
		w += width[i] + 2;
	}
	(void) w;
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
				if ((tecla >= 32) && (tecla < 127))
				{
					for (i = 0; i < (int) noptions; i++)
					{
						if (conioxlib_hotmatch(items[i], tecla))
						{
							sel = i;
							tecla = KBD_ENTER;
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
							tecla = KBD_ENTER;
							break;
						}
					}
				}
			}
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			/* Rueda horizontal: salta al menu anterior o siguiente */
			else if (conioxlib_mousestatus.wheelx < 0)
			{
				tecla = KBD_RIGHT;
			}
			else if (conioxlib_mousestatus.wheelx > 0)
			{
				tecla = KBD_LEFT;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_LEFT) && (tecla != KBD_RIGHT) && (tecla != KBD_ORG) && (tecla != KBD_END) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_DOWN) && (tecla != KBD_F1));
		/* Clear previous highlight over the full cell (padding + label). */
		textattr(atr0);
		gotoxy(start[prev], y);
		putch(' ');
		conioxlib_cputs_hot((unsigned short) (start[prev] + 1), y, atr0, atrhot, items[prev]);
		textattr(atr0);
		putch(' ');
		if (tecla == KBD_LEFT)
		{
			sel--;
			if (sel < 0)
			{
				sel = noptions - 1;
			}
		}
		else if (tecla == KBD_RIGHT)
		{
			sel++;
			if (sel > noptions - 1)
			{
				sel = 0;
			}
		}
		else if (tecla == KBD_ORG)
		{
			sel = 0;
		}
		else if (tecla == KBD_END)
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
	while ((tecla != KBD_ENTER) && (tecla != KBD_DOWN) && (tecla != KBD_ESC) && (tecla != KBD_F1));
	window((int) oldwl, (int) oldwt, (int) oldwr, (int) oldwb);
	gotoxy(oldx, oldy);
	if (tecla == KBD_ESC)
	{
		return(CONIOXLIB_MENU_ESC);
	}
	else if (tecla == KBD_F1)
	{
		return(CONIOXLIB_MENU_HELP);
	}
	else
	{
		return((unsigned char) sel);
	}
}


/* ---------------- Lista de opciones con casillas de marcar ---------------- */
/* Muestra casillas [X] que se activan o desactivan con espacio o con el boton
   izquierdo del raton. Devuelve 255 para ayuda (F1) o 0 al terminar. */
unsigned char conioxlib_option (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], char *sel, unsigned char noptions, unsigned char separation, unsigned char activado)
{
	int y;
	unsigned int tecla;
	int x;

	textattr(atr0);
	for (y = 0; y != noptions; y++)
	{
		gotoxy(x1, y1 + y * separation);
		cprintf("[%c] ", sel[y]);
		conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr0, atrhot, items[y]);
	}
	if (activado == 0)
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
				if ((tecla >= 32) && (tecla < 127))
				{
					for (x = 0; x < (int) noptions; x++)
					{
						if (conioxlib_hotmatch(items[x], tecla))
						{
							y = x;
							tecla = KBD_INS;
							break;
						}
					}
				}
			}
			/* Boton izquierdo */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 20)))
			{
				x = ((int) conioxlib_mousestatus.y - (int) y1) / separation;
				if ((x >= 0) && (x < noptions))
				{
					y = x;
					tecla = KBD_INS;
				}
			}
			/* Boton derecho */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			/* Rueda: desplaza la seleccion arriba o abajo */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_STAB) && (tecla != KBD_TAB) && (tecla != KBD_UP) && (tecla != KBD_DOWN) && (tecla != KBD_SPACE) && (tecla != KBD_INS) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_F1));
		if ((tecla == KBD_STAB) || (tecla == KBD_UP))
		{
			y--;
		}
		else if ((tecla == KBD_TAB) || (tecla == KBD_DOWN))
		{
			y++;
		}
		else if ((tecla == KBD_SPACE) || (tecla == KBD_INS))
		{
			if (sel[y] == 'X')
			{
				sel[y] = ' ';
			}
			else
			{
				sel[y] = 'X';
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
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F1));
	if (tecla == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


/* ---------------------- Menu de propiedades (botones) --------------------- */
/* Muestra una lista de las mismas casillas de option() pero agrupadas como
   propiedades con marca lateral; el funcionamiento es identico. */
unsigned char conioxlib_property (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, char items[][21], char desc[][71], char *sel, unsigned char noptions, unsigned char separation, unsigned char activado)
{
	int y;
	unsigned int tecla;
	int x;

	textattr(atr0);
	for (y = 0; y != noptions; y++)
	{
		gotoxy(x1, y1 + y * separation);
		cprintf("(%c) ", sel[y]);
		conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr0, atrhot, items[y]);
	}
	if (activado == 0)
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
				if ((tecla >= 32) && (tecla < 127))
				{
					for (x = 0; x < (int) noptions; x++)
					{
						if (conioxlib_hotmatch(items[x], tecla))
						{
							y = x;
							tecla = KBD_INS;
							break;
						}
					}
				}
			}
			/* Boton izquierdo */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 20)))
			{
				x = ((int) conioxlib_mousestatus.y - (int) y1) / separation;
				if ((x >= 0) && (x < noptions))
				{
					y = x;
					tecla = KBD_INS;
				}
			}
			/* Boton derecho */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			/* Rueda: desplaza la seleccion arriba o abajo */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_STAB) && (tecla != KBD_TAB) && (tecla != KBD_UP) && (tecla != KBD_DOWN) && (tecla != KBD_SPACE) && (tecla != KBD_INS) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_F1));
		if ((tecla == KBD_STAB) || (tecla == KBD_UP))
		{
			y--;
		}
		else if ((tecla == KBD_TAB) || (tecla == KBD_DOWN))
		{
			y++;
		}
		else if ((tecla == KBD_SPACE) || (tecla == KBD_INS))
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
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F1));
	if (tecla == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)

/* ------------------- Rutina de control de errores ----------------------- */
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
	err = conioxlib_buttons (19, 12, CONIOXLIB_ATTR(WHITE, RED), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, RED), butt, desc, 4, 2);
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


/* -------------------------- Visualiza un texto --------------------------- */
unsigned char conioxlib_view (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, char buffer[][61], unsigned char ultimo, char *title)
{
	unsigned int z, c;
	unsigned char oldx, oldy;
	int f, ini;
	unsigned int tecla;
	char *oldb;
	char tmp[70];

	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return(0);
	}
	oldx = wherex();
	oldy = wherey();
	for (f = ultimo; (f != ultimo + y2 - y1) && (f < 256); f++)
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
			}
			/* Boton izquierdo */
			if (conioxlib_mousestatus.lclick != 0)
			{
				if (conioxlib_mousestatus.x == x2)
				{
					if (conioxlib_mousestatus.y == y1 + 1)
					{
						tecla = KBD_UP;
					}
					else if (conioxlib_mousestatus.y == y2 - 1)
					{
						tecla = KBD_DOWN;
					}
					else if ((conioxlib_mousestatus.y >= y1 + 2) && (conioxlib_mousestatus.y <= y2 - 2) && (y2 - y1 > 4) && (ultimo != 0))
					{
						ini = (conioxlib_mousestatus.y - y1 - 2) * ultimo / (y2 - y1 - 4);
						tecla = 0x1312;
					}
				}
				else if ((conioxlib_mousestatus.x == x1 + 3) && (conioxlib_mousestatus.y == y1))
				{
					tecla = KBD_ESC;
				}
			}
			/* Boton derecho */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_UP) && (tecla != KBD_DOWN) && (tecla != KBD_PGUP) && (tecla != KBD_PGDW) && (tecla != KBD_ORG) && (tecla != KBD_END) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != 0x1312) && (tecla != KBD_F1));
		if (tecla == KBD_UP)
		{
			ini--;
		}
		else if (tecla == KBD_DOWN)
		{
			ini++;
		}
		else if (tecla == KBD_PGUP)
		{
			ini -= (y2 - y1) - 2;
		}
		else if (tecla == KBD_PGDW)
		{
			ini += (y2 - y1) - 2;
		}
		else if (tecla == KBD_ORG)
		{
			ini = 0;
		}
		else if (tecla == KBD_END)
		{
			ini = ultimo;
		}
		if (ini < 0)
		{
			ini = 0;
		}
		else if (ini > ultimo)
		{
			ini = ultimo;
		}
		textattr(atr0);
		gotoxy(x2, z + y1 + 2);
		putch('\xB0');
		if (ultimo != 0)
		{
			z = (ini * (y2 - y1 - 4)) / ultimo;
		}
		gotoxy(x2, z + y1 + 2);
		putch('\xB2');
	}
	while ((tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_F1));
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	if (tecla == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


/* ---------------------------- Cargar la ayuda ---------------------------- */
unsigned char conioxlib_help (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, char *nombre, char *tema, char buffer[][61])
{
	unsigned int ultimo;
	FILE *f;
	char key[50], title[80], tmp[256];
	char *s;
	unsigned int len;

	conioxlib_hint("Loading help...");
	strcpy(title, tema);
	f = fopen(nombre, "rb");
	if (f == 0)
	{
		strcpy(&buffer[0][0], "Help is not available.");
		conioxlib_view(x1, y1, x2, y2, atr0, atr1, buffer, 1, "Help: Not available");
		return(0);
	}
	sprintf(key, "@%s", tema);
	tmp[0] = 0;
	/* Buscar el tema */
	do
	{
		if (fgets(tmp, 255, f) == 0)
		{
			break;
		}
		s = strpbrk(tmp, "\r\n");
		if (s != 0)
		{
			*s = 0;
		}
	}
	while ((strcmp(key, tmp) != 0) && (!feof(f)));
	/* Cargar el tema */
	if (strcmp(key, tmp) == 0)
	{
		ultimo = 0;
		do
		{
			if (fgets(tmp, 255, f) == 0)
			{
				break;
			}
			s = strpbrk(tmp, "\r\n");
			if (s != 0)
			{
				*s = 0;
			}
			/* conioxlib_view() toca filas de buffer hasta ultimo + (y2 - y1): el limite las mantiene dentro de buffer[256][61] */
			if (ultimo < 255 - (y2 - y1))
			{
				memset(&buffer[ultimo][0], 0, 61);
				len = strlen(tmp);
				if (len > 60)
				{
					len = 60;
				}
				memcpy(&buffer[ultimo][0], tmp, len);
				ultimo++;
			}
		}
		while ((tmp[0] != '@') && (!feof(f)));
		fclose(f);
		if ((tmp[0] == '@') && (ultimo != 0))
		{
			ultimo--;
		}
		sprintf(tmp, "Help: %s", title);
		ultimo = conioxlib_view(x1, y1, x2, y2, atr0, atr1, buffer, (unsigned char) ultimo, tmp);
		if (ultimo == 255)
		{
			conioxlib_helpindex(x1, y1, atr1, CONIOXLIB_ATTR(WHITE, GREEN), nombre, (char (*)[16]) buffer);
		}
		return(1);
	}
	else
	{
		fclose(f);
		strcpy(&buffer[0][0], "There is no help for this topic.");
		conioxlib_view(x1, y1, x2, y2, atr0, atr1, buffer, 1, "Help: Nothing");
		return(0);
	}
}


/* ------------------- Entorno para pillar ficheros ----------------------- */
unsigned char conioxlib_select (unsigned char *sdisk, char *sdir, struct conioxlib_sfiles *files)
{
	int i, j;
	unsigned char disk, ndisks, olddrive;
	unsigned char oldx, oldy;
	unsigned int ultimo, tecla;
	int fin;
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
	/* Enumerar unidades */
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
	tecla = ((peekb(0x40, 0x10) & 192) >> 6) + 1;
	for (j = 0; j != tecla; j++)
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
	if (tecla == 1)
	{
		sprintf(files[i].name, "\x10%c\x11", i + 'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0] = 0;
		files[i].time[0] = 0;
		strcpy(files[i].comment, "Virtual Floppy Disk");
		i++;
	}
	tecla = peekb(0x40, 0x75);
	for (j = 0; j != tecla; j++)
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
	/* FindFirstFileA("*.*") ya devuelve "." y ".." en la enumeracion: no hay que
	   anadirlos a mano (la insercion manual los duplicaba en la lista) */
	h = (void *) FindFirstFileA(mask, &ffd);
	fin = (h == INVALID_HANDLE_VALUE);
#else
	/* Relative mask after chdir is more reliable on DOS (Watcom/Borland/DJGPP) */
	fin = conioxlib_findfirst(mask, &ffblk);
#endif
	while (!fin)
	{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		memset(files[i].name, 0, 13);
		strncpy(files[i].name, ffd.cFileName, 12);
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
		memcpy(files[i].name, conioxlib_ffname(ffblk), 13);
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
			conioxlib_select_format(files[i].size, (long) ffd.nFileSizeLow);
#else
			conioxlib_select_format(files[i].size, conioxlib_ffsize(ffblk));
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
			fin = 1;
		}
#else
		fin = conioxlib_findnext(&ffblk);
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
	ultimo = i - 1;
	for (i = ultimo + 1; i != 220; i++)
	{
		files[i].selected = 0;
		files[i].name[0] = 0;
		files[i].size[0] = 0;
		files[i].date[0] = 0;
		files[i].time[0] = 0;
		files[i].comment[0] = 0;
	}
	qsort((void *) files, ultimo + 1, sizeof(files[0]), conioxlib_select_sort);
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
	if (ultimo >= 18)
	{
		for (i = 0; i < 18; i++)
		{
			gotoxy (2, 5 + i);
			cprintf("%-12s\xBA%14s\xBA%-8s\xBA%-5s\xBA%-33s", files[i].name, files[i].size, files[i].date, files[i].time, files[i].comment);
		}
	}
	else
	{
		for (i = 0; i <= ultimo; i++)
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
			}
			/* Boton izquierdo */
			if (conioxlib_mousestatus.lclick != 0)
			{
				if ((conioxlib_mousestatus.x == 4) && (conioxlib_mousestatus.y == 2))
				{
					tecla = KBD_ESC;
				}
				else if ((conioxlib_mousestatus.x == 78) && (conioxlib_mousestatus.y == 22))
				{
					tecla = KBD_DOWN;
				}
				else if ((conioxlib_mousestatus.x == 78) && (conioxlib_mousestatus.y == 5))
				{
					tecla = KBD_UP;
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
					else if (j > ultimo)
					{
						j = ultimo;
					}
					tecla = KBD_SPACE;
				}
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_DOWN) && (tecla != KBD_UP) && (tecla != KBD_PGDW) && (tecla != KBD_PGUP) && (tecla != KBD_ORG) && (tecla != KBD_END) && (tecla != KBD_INS) && (tecla != KBD_SPACE) && (tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F4) && (tecla != KBD_F3) && (tecla != KBD_F2) && (tecla != KBD_F1));
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

		if (tecla == KBD_ENTER)
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
		else if (tecla == KBD_F2)
		{
			box (23, 12, 56, 14, CONIOXLIB_ATTR(WHITE, LIGHTGRAY), A_DOUBLE, "Mask", 1);
			textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
			gotoxy(24, 13);
			cputs("Enter file mask:");
			conioxlib_getstr(42, 13, CONIOXLIB_ATTR(WHITE, BLUE), mask, 13);
			goto select_1;
		}
		else if (tecla == KBD_F3)
		{
/*			for (fin = 0; fin != ultimo; fin++)
				files[fin].selected = 1;
			tecla = KBD_ORG; */
		}
		else if (tecla == KBD_F4)
		{
/*			for (fin = 0; fin != ultimo; fin++)
				files[fin].selected = 0;
			tecla = KBD_ORG; */
		}
		else if (tecla == KBD_DOWN)
		{
			if (j != ultimo)
			{
				i++;
				j++;
			}
		}
		else if (tecla == KBD_UP)
		{
			if (j != 0)
			{
				i--;
				j--;
			}
		}
		else if (tecla == KBD_PGDW)
		{
			if (j < ultimo - 17)
			{
				j += 17 - i;
			}
		}
		else if (tecla == KBD_PGUP)
		{
			if (j >= 17)
			{
				j -= 17 - i;
			}
		}
		else if (tecla == KBD_ORG)
		{
			j = 0;
		}
		else if (tecla == KBD_END)
		{
			j = ultimo - 17;
			if (j < 0)
			{
				j = 0;
			}
		}
		else if ((tecla == KBD_SPACE) || (tecla == KBD_INS))
		{
			if ((strcmp(files[j].size, "\xF0 DIRECTORY \xF0") != 0) && (strcmp(files[j].size, "\xF0   DRIVE   \xF0") != 0) && (j <= ultimo))
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
			if (j < ultimo)
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
		if ((tecla == KBD_PGDW) || (tecla == KBD_PGUP) || (tecla == KBD_ORG) || (tecla == KBD_END))
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
			if (tecla == KBD_END)
			{
				i = ultimo - j;
				if (i > 17)
				{
					i = 17;
				}
				j = ultimo;
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
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F1));
	if (tecla == KBD_ESC)
	{
		for (i = 0; i != 200; i++)
		{
			files[i].selected = 0;
		}
	}
	else
	{
		for (i = 0; i != ultimo; i++)
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
	if (tecla == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(0);
	}
}


/* -------------------- Ordena el select ---------------------------------- */
static int conioxlib_select_sort (const void *a, const void *b)
{
	/* Ordenar por nombre: el byte selected de la estructura no debe alterar el orden */
	return(strcmp(((struct conioxlib_sfiles *) a)->name, ((struct conioxlib_sfiles *) b)->name));
}


/* ------------------------- Puntua un long ------------------------------- */
static void conioxlib_select_format (char *res, long int n)
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


/* --------------------------- Funcion inicial ---------------------------- */
void conioxlib_hello (char *nombre, unsigned char comercial)
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
	f = fopen(nombre, "wb");
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
	conioxlib_about(comercial);
}


/* --------------------------- Funcion final ------------------------------ */
void conioxlib_bye (char *nombre, unsigned char e, unsigned char comercial)
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
	f = fopen(nombre, "rb");
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
	remove(nombre);
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
	if (comercial)
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


/* ------------------- Dibuja el fondo de la aplicacion -------------------- */
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
	{
		char clk[8];
		unsigned short cells[5];
		unsigned short cellattr;
		unsigned int ci;

		time(&t);
		ti = localtime(&t);
		/* Reloj en columnas 76-80; las cerraduras ocupan 60-74 y status() 1-59 */
		sprintf(clk, "%2u:%02u", (unsigned int) ti->tm_hour, (unsigned int) ti->tm_min);
		cellattr = (unsigned short) ((unsigned int) conioxlib_curtheme.atrstatus << 8);
		for (ci = 0; ci < 5; ci++)
		{
			cells[ci] = (unsigned short) (cellattr | (unsigned char) clk[ci]);
		}
		puttext(76, 25, 80, 25, cells);
	}
	/* Invalidate blocks cache: line 25 was just cleared to spaces */
	conioxlib_blocks();
	free(oldb);
}


/* --------------- Dibuja en pantalla la about window ---------------- */
void conioxlib_about (unsigned char comercial)
{
	unsigned int tecla;
	char x1 = 9, y1 = 5, x2 = 68, y2 = 18;
	unsigned char oldx, oldy;
	char *oldb;

	oldb = malloc(textsize(x1, y1, x2 + 2, y2 + 1));
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
	if (comercial)
	{
		cputsc(x1, x2, y1 + 12, "Registered version");
	}
	else
	{
		cputsc(x1, x2, y1 + 12, "Free distribution");
	}
	tecla = 0;
	do
	{
		watch();
		conioxlib_pollmouse();
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(tecla);
		}
		/* Boton izquierdo sobre el boton de cierre */
		if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.y == (unsigned int) y1) && (conioxlib_mousestatus.x == (unsigned int) x1 + 3))
		{
			tecla = KBD_ESC;
		}
		if (tecla == 0)
		{
			delay(10);
		}
	}
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC));
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


/* -------------------- Salvapantallas por bloques ------------------------- */
void conioxlib_screensaver (void)
{
	unsigned char i, j;
	unsigned char k;
	unsigned char oldx, oldy;
	unsigned char rep[8][5];
	char oldb[8][5][10 * 5 * 2];
	unsigned int tecla;

	oldx = wherex();
	oldy = wherey();
	srand((unsigned int) time(0));
	/* Guardar pantalla original */
	for (j = 0; j != 5; j++)
	{
		for (i = 0; i != 8; i++)
		{
			gettext(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5, oldb[i][j]);
		}
	}
	ini:
	/* Empezar a quitar */
	memset(&rep, 0, sizeof(rep));
	for (k = 0; k != 40; k++)
	{
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(tecla);
			goto fin;
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
	/* Empezar a poner */
	memset(&rep, 0, sizeof(rep));
	for (k = 0; k != 40; k++)
	{
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(tecla);
			goto fin;
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
	fin:
	/* Restaurar pantalla original */
	for (j = 0; j != 5; j++)
	{
		for (i = 0; i != 8; i++)
		{
			puttext(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5, oldb[i][j]);
		}
	}
	gotoxy(oldx, oldy);
}


/* ------------------------- Rebota la pantalla ---------------------------- */
void conioxlib_bounce (void)
{
	unsigned int i, j;
	unsigned int sw, sh;
	char *oldb;
	struct text_info ti;

	gettextinfo(&ti);
	sw = ti.screenwidth;
	sh = ti.screenheight;
	oldb = (char *) malloc(sw * sh * 2);
	if (oldb == 0)
	{
		return;
	}
	window(1, 1, (int) sw, (int) sh);
	gettext(1, 1, (int) sw, (int) sh, oldb);
	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
	clrscr();
	for (j = sh + 2; j != 0; j--)
	{
		for (i = 1; i != j; i++)
		{
			if (i < 2)
			{
				puttext(1, (int) i, (int) sw, (int) (i + sh - 1), oldb);
			}
			if (i <= sh)
			{
				gotoxy(1, (int) i);
				clreol();
			}
			delay(20);
		}
		for (i = j; i != 0; i--)
		{
			if (i < 2)
			{
				puttext(1, (int) i, (int) sw, (int) (i + sh - 1), oldb);
			}
			if (i <= sh)
			{
				gotoxy(1, (int) i);
				clreol();
			}
			delay(20);
		}
	}
	puttext(1, 1, (int) sw, (int) sh, oldb);
	free(oldb);
}

/* -------------------- Diagonal wipe without globals ---------------------- */
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


/* -------------------- Pregunta antes de salir a DOS ---------------------- */
unsigned char conioxlib_exittodos (char *nombre)
{
	unsigned int i;
	unsigned char oldx, oldy;
	char *oldb, *buf;
	char but1[][11] = {"Yes", "No", "Shell"};
	char dbut1[][71] = {"Exit to DOS and finish program                                      ",
						"Continue running program                                            ",
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
		i = conioxlib_buttons(28, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(RED, LIGHTGRAY), but1, dbut1, 3, 2);
		if (i == 255)
		{
			buf = malloc(16384);
			if (buf == 0)
			{
				conioxlib_restorescr(20, 10, 62, 15, oldb);
				return(255);
			}
			conioxlib_help (7, 3, 69, 21, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), nombre, "Exit", (char (*)[61]) buf);
			free(buf);
		}
	}
	while (i == 255);
	conioxlib_restorescr(20, 10, 62, 15, oldb);
	gotoxy(oldx, oldy);
	if (i == 2)
	{
		conioxlib_shell(3, 2, 75, 22, "");
		i = 1;
	}
	return(i);
}


/* ----------------------------- Shell a DOS ------------------------------- */
void conioxlib_shell (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *comando)
{
	unsigned char oldx, oldy;
	char *oldb;
	int i;

	oldx = wherex();
	oldy = wherey();
	oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
	if (oldb == 0)
	{
		return;
	}
	if (comando[0] == 0)
	{
		box(x1, y1, x2, y2, CONIOXLIB_ATTR(YELLOW, BLACK), A_DOUBLE, "DOS Shell", SHADOW);
	}
	else
	{
		box(x1, y1, x2, y2, CONIOXLIB_ATTR(YELLOW, BLACK), A_DOUBLE, comando, SHADOW);
	}
	window(x1 + 1, y1 + 1, x2 - 1, y2 - 1);
	textattr(CONIOXLIB_ATTR(WHITE, BLACK));
	clrscr();
	cputs("Type 'EXIT' to return\r\n\r\n");
	textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
	{
		void interrupt (*oldvect)();

		oldvect = getvect(0x29);
		setvect(0x29, conioxlib_shell_patch);
		i = system((comando[0] == 0) ? "command.com" : comando);
		setvect(0x29, oldvect);
	}
#else
	#if ((__DOS__) || (__MSDOS__))
	i = system((comando[0] == 0) ? "command.com" : comando);
	#else
	i = system((comando[0] == 0) ? "cmd" : comando);
	#endif
#endif
	if (i != 0)
	{
		textattr(CONIOXLIB_ATTR(WHITE, BLACK));
		cputs("Cannot execute program.");
	}
	window(1, 1, 80, 25);
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
}


#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)

/* ------------------- Rutina de salida del conioxlib_shell -------------------------- */
void interrupt conioxlib_shell_patch (unsigned int bp, unsigned int di, unsigned int si, unsigned int ds, unsigned int es, unsigned int dx, unsigned int cx, unsigned int bx, unsigned int ax)
{
	unsigned char al;

	al = ax & 255;
	putch(al);
#pragma warn -par
}
#pragma warn .par

#endif


/* ------------------- Dibuja en pantalla un calendario ------------------- */
void conioxlib_calendar (unsigned short x, unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atr2, unsigned char day, unsigned char month, unsigned int year)
{
	unsigned char i, days, start;
	unsigned char oldx, oldy;
	unsigned int tecla;
	char *oldb;
	struct tm *tyme;
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
		return;
	}
	oldx = wherex();
	oldy = wherey();
	/* Conseguir fecha actual */
	curr_time = time(0);
	tyme = localtime(&curr_time);
	/* Si algun parametro es nulo, obtiene el actual. month va de 1 = enero a 12 = diciembre */
	if (day == 0)
	{
		day = tyme->tm_mday;
	}
	if (month == 0)
	{
		month = (unsigned char) tyme->tm_mon + 1;
	}
	if (month > 12)
	{
		month = 12;
	}
	if (year < 100)
	{
		year = (unsigned int) tyme->tm_year + 1900;
	}
	/* Calcular el dia de la semana del dia 1 */
	memset(&t, 0, sizeof(t));
	t.tm_mday = 1;
	t.tm_mon = month - 1;
	t.tm_year = year - 1900;
	if (mktime(&t) == (time_t) -1)
	{
		start = 1;
	}
	else
	{
		start = (unsigned char) t.tm_wday + 1;
	}
	days = monthdays[month - 1];
	if ((month == 2) && ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0)))
	{
		days = 29;
	}
	_wscroll = 0;
	/* Keep status text in columns 1..59 so clock (76-80) and locks (60-74) never collide */
	conioxlib_hint("ENTER / ESC close calendar");
	/* atr0 = normal day cells, atr1 = today highlight, atr2 = weekend */
	box(x, y, x + 38, y + 10, atr0, A_DOUBLE, "Calendar", SHADOW | CLOSE);
	textattr(atr0);
	gotoxy(x + 2, y + 1);
	cprintf("%-30s %u", monthnameuk[month - 1], year);
	gotoxy(x + 5, y + 2);
	cputs("Sun Mon Tue Wed Thu Fri Sat");
	gotoxy(x + 5, y + 4);
	for (i = 1; i <= days; i++)
	{
		gotoxy(x + 2 + (start << 2), wherey());
		if (i == day)
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
	tecla = 0;
	do
	{
		conioxlib_pollmouse();
		if (conioxlib_keyhit())
		{
			conioxlib_readkey(tecla);
		}
		/* Boton izquierdo sobre el boton de cierre */
		if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.y == y) && (conioxlib_mousestatus.x == (unsigned int) x + 3))
		{
			tecla = KBD_ESC;
		}
		if (tecla == 0)
		{
			delay(10);
		}
	}
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC));
	conioxlib_restorescr(x, y, x + 40, y + 11, oldb);
	gotoxy(oldx, oldy);
}


/* ----------------------- Rejilla de opciones (grid) ---------------------- */
/* Dibuja las opciones en columnas de 4 por fila; el boton izquierdo del raton
   selecciona la celda y la rueda desplaza la seleccion. */
unsigned char conioxlib_grid (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, char items[][16], unsigned char noptions)
{
	int y;
	unsigned int tecla;
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
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
			}
			/* Boton izquierdo */
			if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 60)) && (conioxlib_mousestatus.y >= y1) && (conioxlib_mousestatus.y <= (y1 + ((noptions + 3) >> 2) - 1)))
			{
				textattr(atr0);
				gotoxy(((y & 3) * 15) + x1, (y >> 2) + y1);
				cputs(items[y]);
				y = ((conioxlib_mousestatus.y - y1) << 2) + (((conioxlib_mousestatus.x - x1) / 15) & 3);
				tecla = 0xFFFF;
			}
			/* Boton derecho */
			else if (conioxlib_mousestatus.rclick != 0)
			{
				tecla = KBD_F1;
			}
			/* Rueda: desplaza la seleccion arriba o abajo */
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_UP) && (tecla != KBD_DOWN) && (tecla != KBD_ORG) && (tecla != KBD_END) && (tecla != KBD_ESC) && (tecla != KBD_ENTER) && (tecla != KBD_F1) && (tecla != KBD_LEFT) && (tecla != KBD_RIGHT) && (tecla != 0xFFFF));
		textattr(atr0);
		gotoxy(((y & 3) * 15) + x1, (y >> 2) + y1);
		cputs(items[y]);
		if (tecla == KBD_UP)
		{
			y -= 4;
		}
		else if (tecla == KBD_DOWN)
		{
			y += 4;
		}
		else if (tecla == KBD_ORG)
		{
			y = 0;
		}
		else if (tecla == KBD_END)
		{
			y = noptions - 1;
		}
		else if (tecla == KBD_LEFT)
		{
			y--;
		}
		else if (tecla == KBD_RIGHT)
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
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC) && (tecla != KBD_F1));
	if (tecla == KBD_ESC)
	{
		return(0);
	}
	else if (tecla == KBD_F1)
	{
		return(255);
	}
	else
	{
		return(y);
	}
}


/* ----------------------- Carga el indice de ayuda ------------------------ */
unsigned char conioxlib_helpindex (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, char *nombre, char buffer[][16])
{
	unsigned int i;
	unsigned int ultimo;
	unsigned char oldx, oldy;
	FILE *f;
	char *oldb;
	char tmp[256];
	char (*helpbuf)[61];
	char *s;
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
	f = fopen(nombre, "rb");
	if (f == 0)
	{
		strcpy(&helpbuf[0][0], "Help is not available.");
		conioxlib_view(x1, y1, x1 + 65, y1 + 20, atr0, atr0, helpbuf, 1, "Help: Not available");
		conioxlib_restorescr(1, 1, 80, 25, oldb);
		gotoxy(oldx, oldy);
		return(255);
	}
	ultimo = 0;
	do
	{
		if (fgets(tmp, 255, f) == 0)
		{
			break;
		}
		if ((tmp[0] == '@') && (strlen(tmp) > 3))
		{
			s = strpbrk(tmp, "\r\n");
			if (s != 0)
			{
				*s = 0;
			}
			if (ultimo < 255)
			{
				memset(&buffer[ultimo][0], 0, 16);
				len = strlen(&tmp[1]);
				if (len > 15)
				{
					len = 15;
				}
				memcpy(&buffer[ultimo][0], &tmp[1], len);
				ultimo++;
			}
		}
	}
	while (!feof(f));
	fclose(f);
	if (ultimo == 0)
	{
		strcpy(&helpbuf[0][0], "Help is not available.");
		conioxlib_view(x1, y1, x1 + 65, y1 + 20, atr0, atr0, helpbuf, 1, "Help: Not available");
		conioxlib_restorescr(1, 1, 80, 25, oldb);
		gotoxy(oldx, oldy);
		return(255);
	}
	box (x1, y1, x1 + 62, y1 + 19, atr0, A_DOUBLE, "Help: Index", 1);
	i = conioxlib_grid(x1 + 2, y1 + 1, atr0, atr1, buffer, (unsigned char) ultimo);
	conioxlib_restorescr(1, 1, 80, 25, oldb);
	gotoxy(oldx, oldy);
	if (i == 255)
	{
		return(255);
	}
	conioxlib_help (x1, y1, x1 + 62, y1 + 18, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), nombre, &buffer[i][0], helpbuf);
	return(i);
}

/* ----------------------------- Apply theme ------------------------------- */
void conioxlib_theme (unsigned char id)
{
	switch (id)
	{
		case 1: /* Turbo C 2.0 */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, GREEN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(RED, LIGHTGRAY);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_SINGLE;
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
		case 3: /* Norton Utilities */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(YELLOW, BLUE);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, CYAN);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(YELLOW, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Norton Utilities");
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
		default: /* Borland C++ 3.1 */
			conioxlib_curtheme.atr0 = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atr1 = CONIOXLIB_ATTR(BLACK, GREEN);
			conioxlib_curtheme.atrhot = CONIOXLIB_ATTR(RED, LIGHTGRAY);
			conioxlib_curtheme.atrstatus = CONIOXLIB_ATTR(BLACK, LIGHTGRAY);
			conioxlib_curtheme.atrdialog = CONIOXLIB_ATTR(WHITE, BLUE);
			conioxlib_curtheme.frame = CONIOXLIB_BOX_A_DOUBLE;
			strcpy(conioxlib_curtheme.name, "Borland C++ 3.1");
			break;
	}
}

/* --------------------------- Read current theme -------------------------- */
void conioxlib_gettheme (struct conioxlib_stheme *t)
{
	if (t != 0)
	{
		*t = conioxlib_curtheme;
	}
}

/* -------------------- Save screen region (malloc) ------------------------ */
char *conioxlib_savescr (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
	char *buf;
	unsigned int sz;

	sz = (unsigned int) conioxlib_textsize(x1, y1, x2, y2);
	buf = (char *) malloc(sz);
	if (buf == 0)
	{
		return(0);
	}
	gettext(x1, y1, x2, y2, buf);
	return(buf);
}

/* ------------------- Restore screen region and free ---------------------- */
void conioxlib_restorescr (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *buf)
{
	if (buf != 0)
	{
		puttext(x1, y1, x2, y2, buf);
		free(buf);
	}
}

/* ---------------------- Status line columns 1..59 ------------------------ */
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
/* Regla de progreso estilo instaladores clasicos: segmentos '=' con la punta
   '>' en la ultima celda llena y espacios vacios en lo restante, de modo que no
   parezca una barra solida de bloques. value/maxv fija la proporcion. */
void conioxlib_progressbar (unsigned short x, unsigned short y, unsigned short width, unsigned char atr, unsigned int value, unsigned int maxv)
{
	unsigned int filled;
	unsigned int i;

	if (width < 3)
	{
		width = 3;
	}
	if (maxv == 0)
	{
		maxv = 1;
	}
	if (value > maxv)
	{
		value = maxv;
	}
	textattr(atr);
	gotoxy(x, y);
	filled = (value * (unsigned int) (width - 2)) / maxv;
	for (i = 0; i < (unsigned int) (width - 2); i++)
	{
		if (i < filled)
		{
			putch(((i == filled - 1) && (value < maxv)) ? '>' : '=');
		}
		else
		{
			putch(' ');
		}
	}
}

/* ------------------- Progress in status or titled box -------------------- */
/* Escribe "titulo NN%" en la linea de estado y dibuja la regla a partir de la
   columna 50 con el color de la linea de estado del tema activo. */
void conioxlib_progress (const char *title, unsigned int value, unsigned int maxv)
{
	char tmp[60];
	unsigned int pct;

	if (maxv == 0)
	{
		maxv = 1;
	}
	if (value > maxv)
	{
		value = maxv;
	}
	pct = (value * 100) / maxv;
	if ((title != 0) && (title[0] != 0))
	{
		sprintf(tmp, "%.40s %u%%", title, pct);
	}
	else
	{
		sprintf(tmp, "%u%%", pct);
	}
	conioxlib_status(tmp);
	conioxlib_progressbar(50, 25, 10, conioxlib_curtheme.atrstatus, value, maxv);
}

/* --------------------------- Message box dialog -------------------------- */
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
	r = conioxlib_buttons((unsigned short) (x1 + 2), (unsigned short) (y1 + 3), CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(RED, LIGHTGRAY), btns, defdesc, nbuttons, 2);
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	return(r);
}

/* ------------------------ Input box (label + field) ---------------------- */
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
	r = conioxlib_buttons((unsigned short) (x1 + 14), (unsigned short) (y1 + 4), CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(RED, LIGHTGRAY), btns, desc, 2, 2);
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	return(r);
}

/* -------------- Input box digits-only with minimum length ---------------- */
/* conioxlib_inputmask(): campo numerico que no admite teclear caracteres no
   validos: solo acepta digitos (BACK borra, ENTER acepta, ESC cancela, F1 pide
   ayuda) y tras OK comprueba que haya al menos mindigits digitos. Devuelve 0
   (OK), 1 (Cancel) o 255 (ayuda). */
unsigned char conioxlib_inputmask (const char *title, const char *label, char *buf, char len, char mindigits)
{
	unsigned char oldx, oldy;
	char *oldb;
	unsigned char r;
	unsigned int tecla;
	char btns[2][11];
	char desc[2][71];
	unsigned short x1, y1, x2, y2;
	unsigned int i;
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
	(void) i;
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
		/* Entrada: cualquier tecla que no sea un digito se ignora en el mismo
		   momento de pulsarla, asi no pueden entrar caracteres invalidos */
		for (;;)
		{
			textattr(CONIOXLIB_ATTR(WHITE, BLUE));
			gotoxy((unsigned short) (x1 + 2), (unsigned short) (y1 + 2));
			cprintf("%-*.*s", (int) len, (int) len, buf);
			gotoxy((unsigned short) (x1 + 2 + (int) digits), (unsigned short) (y1 + 2));
			tecla = 0;
			do
			{
				watch();
				conioxlib_pollmouse();
				if (conioxlib_keyhit())
				{
					conioxlib_readkey(tecla);
				}
				if (tecla == 0)
				{
					delay(10);
				}
			}
			while (tecla == 0);
			if ((tecla >= '0') && (tecla <= '9') && ((int) digits < (int) len))
			{
				textattr(CONIOXLIB_ATTR(WHITE, BLUE));
				putch((char) tecla);
				buf[digits] = (char) tecla;
				digits++;
				buf[digits] = 0;
			}
			else if ((tecla == KBD_BACK) && (digits > 0))
			{
				textattr(CONIOXLIB_ATTR(WHITE, BLUE));
				cputs("\b \b");
				digits--;
				buf[digits] = 0;
			}
			else if ((tecla == KBD_ENTER) || (tecla == KBD_ESC) || (tecla == KBD_F1))
			{
				break;
			}
			/* Letras, simbolos y demas teclas: ignoradas */
		}
		_setcursortype(_NOCURSOR);
		if (tecla == KBD_ESC)
		{
			r = 1;
		}
		else if (tecla == KBD_F1)
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
			r = conioxlib_buttons((unsigned short) (x1 + 14), (unsigned short) (y1 + 4), CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(RED, LIGHTGRAY), btns, desc, 2, 2);
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
unsigned char conioxlib_listbox (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, char items[][41], unsigned char nitems, char *title)
{
	unsigned char oldx, oldy;
	char *oldb;
	int top, sel, vis, tecla, row, maxrow;

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
	box(x1, y1, x2, y2, atr0, A_DOUBLE, (title != 0) ? title : "List", SHADOW | CLOSE);
	do
	{
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
				cprintf("%-*.*s", (int) (x2 - x1 - 1), (int) (x2 - x1 - 1), items[top + row]);
			}
			else
			{
				textattr(atr0);
				cprintf("%*s", (int) (x2 - x1 - 1), "");
			}
		}
		tecla = 0;
		do
		{
			watch();
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
			}
			if (conioxlib_mousestatus.lclick != 0)
			{
				if ((conioxlib_mousestatus.y == y1) && (conioxlib_mousestatus.x == (unsigned int) (x1 + 3)))
				{
					tecla = KBD_ESC;
				}
				else if ((conioxlib_mousestatus.y > y1) && (conioxlib_mousestatus.y < y2) && (conioxlib_mousestatus.x > x1) && (conioxlib_mousestatus.x < x2))
				{
					row = (int) conioxlib_mousestatus.y - (int) y1 - 1;
					if ((row >= 0) && (top + row < (int) nitems))
					{
						sel = top + row;
						tecla = KBD_ENTER;
					}
				}
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(10);
			}
		}
		while ((tecla != KBD_UP) && (tecla != KBD_DOWN) && (tecla != KBD_PGUP) && (tecla != KBD_PGDW) && (tecla != KBD_ORG) && (tecla != KBD_END) && (tecla != KBD_ESC) && (tecla != KBD_ENTER));
		if (tecla == KBD_UP)
		{
			if (sel > 0)
			{
				sel--;
			}
		}
		else if (tecla == KBD_DOWN)
		{
			if (sel < (int) nitems - 1)
			{
				sel++;
			}
		}
		else if (tecla == KBD_PGUP)
		{
			sel -= vis;
			if (sel < 0)
			{
				sel = 0;
			}
		}
		else if (tecla == KBD_PGDW)
		{
			sel += vis;
			if (sel >= (int) nitems)
			{
				sel = (int) nitems - 1;
			}
		}
		else if (tecla == KBD_ORG)
		{
			sel = 0;
		}
		else if (tecla == KBD_END)
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
	}
	while ((tecla != KBD_ENTER) && (tecla != KBD_ESC));
	conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
	gotoxy(oldx, oldy);
	if (tecla == KBD_ESC)
	{
		return(255);
	}
	return((unsigned char) sel);
}

/* Pilas de deshacer del editor: conioxlib_pushundo() mete una copia completa del
   texto en la pila (dst) y conioxlib_clearundo() vacia la pila contraria tras un
   cambio; solo se expanden dentro de conioxlib_edit(). */
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

/* ------------------ Editor de texto estilo Borland ------------------------ */
/* Editor a pantalla completa con barra de menus File/Edit/Search/Quit (F10 o clic
   en la barra) como la de menubar(): los desplegables usan menu() con filas de
   ancho completo y separadores. Toda la salida usa los colores del tema activo.
   Redibujo por banderas (solo se repinta lo que cambia), soporte de raton (clic en
   el texto mueve el cursor, clic en la barra abre el menu, rueda desplaza) y
   deshacer/rehacer con pilas de 4 niveles. Devuelve 0. */
unsigned char conioxlib_edit (char *filename)
{
	char lines[120][76];
	char clip[76];
	char fname[80];
	char findstr[40];
	char repstr[40];
	char gotobuf[8];
	unsigned char oldx, oldy;
	char *oldb;
	int nlines, cur, col, top, tecla, i, j, vis;
	int off, lenfind, lenrep, lenrest;
	int l, n;
	int drawfull, drawline, oldtop, oldcur, oldnlines;
	char *s;
	FILE *f;
	char baritems[4][21] = {"&File", "&Edit", "&Search", "&Quit"};
	char bardesc[4][71];
	char fileitems[5][21] = {"&Open...", "&Save", "Save &as...", "-", "E&xit"};
	char filedesc[5][71];
	char edititems[7][21] = {"Cu&t", "&Copy", "&Paste", "-", "C&lear line", "&Undo", "&Redo"};
	char editdesc[7][71];
	char searchitems[3][21] = {"&Find...", "&Replace...", "&Goto line..."};
	char searchdesc[3][71];
	int barpos[4] = {1, 7, 13, 21};
	int mbsel;
	unsigned char bar, sub;
	struct conioxlib_stheme th;
	/* Pilas de deshacer/rehacer: cada entrada guarda una copia completa del
	   texto (lineas, posicion incluida); 4 niveles por pila */
	struct conioxlib_seditundo
	{
		char lines[120][76];
		int nlines;
		int cur;
		int col;
	};
	struct conioxlib_seditundo *undo[4];
	struct conioxlib_seditundo *redo[4];
	int nundo, nredo;

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
	oldb = conioxlib_savescr(1, 1, 80, 25);
	if (oldb == 0)
	{
		return(255);
	}
	memset(lines, 0, sizeof(lines));
	clip[0] = 0;
	findstr[0] = 0;
	repstr[0] = 0;
	nlines = 1;
	cur = 0;
	col = 0;
	top = 0;
	vis = 21;
	fname[0] = 0;
	if ((filename != 0) && (filename[0] != 0))
	{
		strncpy(fname, filename, 79);
		fname[79] = 0;
		f = fopen(fname, "rb");
		if (f != 0)
		{
			nlines = 0;
			while ((nlines < 119) && (fgets(lines[nlines], 75, f) != 0))
			{
				for (i = 0; lines[nlines][i] != 0; i++)
				{
					if ((lines[nlines][i] == '\r') || (lines[nlines][i] == '\n'))
					{
						lines[nlines][i] = 0;
						break;
					}
				}
				nlines++;
			}
			fclose(f);
			if (nlines == 0)
			{
				nlines = 1;
			}
		}
	}
	_setcursortype(_NORMALCURSOR);
	drawfull = 1;
	drawline = 0;
	oldtop = -1;
	oldcur = -1;
	oldnlines = -1;
	mbsel = 0;
	for (;;)
	{
		if (drawfull != 0)
		{
			/* Barra de menus fija con los colores del tema */
			textattr(th.atr0);
			gotoxy(1, 1);
			for (i = 0; i < 80; i++)
			{
				putch(' ');
			}
			for (i = 0; i < 4; i++)
			{
				gotoxy((unsigned short) barpos[i], 1);
				textattr(th.atr0);
				putch(' ');
				conioxlib_cputs_hot((unsigned short) (barpos[i] + 1), 1, th.atr0, th.atrhot, baritems[i]);
				textattr(th.atr0);
				putch(' ');
			}
			/* Area de texto */
			textattr(th.atrdialog);
			window(1, 2, 80, 23);
			clrscr();
			for (i = 0; i < vis; i++)
			{
				if (top + i < nlines)
				{
					gotoxy(1, i + 1);
					cprintf("%-75.75s", lines[top + i]);
				}
			}
			window(1, 1, 80, 25);
			oldtop = top;
			oldcur = cur;
			oldnlines = nlines;
			drawfull = 0;
			drawline = 0;
		}
		else
		{
			if ((top != oldtop) || (nlines != oldnlines))
			{
				/* Desplazamiento o linea insertada/borrada: redibujar todas */
				textattr(th.atrdialog);
				window(1, 2, 80, 23);
				clrscr();
				for (i = 0; i < vis; i++)
				{
					if (top + i < nlines)
					{
						gotoxy(1, i + 1);
						cprintf("%-75.75s", lines[top + i]);
					}
				}
				window(1, 1, 80, 25);
				oldtop = top;
				oldcur = cur;
				oldnlines = nlines;
			}
			else
			{
				if ((nlines != oldnlines) || (cur != oldcur))
				{
					/* Linea nueva o anterior: repintar solo esas dos */
					textattr(th.atrdialog);
					window(1, 2, 80, 23);
					if ((oldcur >= top) && (oldcur < top + vis) && (oldcur < nlines) && (oldcur != cur))
					{
						gotoxy(1, oldcur - top + 1);
						cprintf("%-75.75s", lines[oldcur]);
					}
					if ((cur >= top) && (cur < top + vis))
					{
						gotoxy(1, cur - top + 1);
						cprintf("%-75.75s", lines[cur]);
					}
					window(1, 1, 80, 25);
					oldcur = cur;
					oldnlines = nlines;
				}
				if (drawline != 0)
				{
					/* La linea del cursor ha cambiado su contenido */
					textattr(th.atrdialog);
					window(1, 2, 80, 23);
					gotoxy(1, cur - top + 1);
					cprintf("%-75.75s", lines[cur]);
					window(1, 1, 80, 25);
					drawline = 0;
				}
			}
		}
		/* Linea de estado con el color del tema */
		textattr(th.atrstatus);
		gotoxy(1, 24);
		cprintf(" %-20.20s Ln %3d Col %3d F10=menu ESC=quit", fname[0] ? fname : "(untitled)", cur + 1, col + 1);
		while (wherex() < 80)
		{
			putch(' ');
		}
		gotoxy((unsigned short) (col + 1), (unsigned short) ((cur - top) + 2));
		tecla = 0;
		do
		{
			conioxlib_pollmouse();
			if (conioxlib_keyhit())
			{
				conioxlib_readkey(tecla);
			}
			else if (conioxlib_mousestatus.lclick != 0)
			{
				if (conioxlib_mousestatus.y == 1)
				{
					/* Clic en la barra: invoca el menu correspondiente */
					mbsel = (int) ((conioxlib_mousestatus.x > 21) ? 3 : ((conioxlib_mousestatus.x > 13) ? 2 : ((conioxlib_mousestatus.x > 7) ? 1 : 0)));
					tecla = KBD_F10;
				}
				else if ((conioxlib_mousestatus.y >= 2) && (conioxlib_mousestatus.y <= 23))
				{
					/* Clic en el texto: mueve el cursor */
					i = top + (int) conioxlib_mousestatus.y - 2;
					if (i < nlines)
					{
						cur = i;
					}
					i = (int) conioxlib_mousestatus.x - 1;
					j = (int) strlen(lines[cur]);
					if (i > j)
					{
						i = j;
					}
					if (i > 74)
					{
						i = 74;
					}
					if (i < 0)
					{
						i = 0;
					}
					col = i;
					tecla = 0x1312;
				}
			}
			else if (conioxlib_mousestatus.wheely < 0)
			{
				tecla = KBD_DOWN;
			}
			else if (conioxlib_mousestatus.wheely > 0)
			{
				tecla = KBD_UP;
			}
			if (tecla == 0)
			{
				delay(15);
			}
		}
		while (tecla == 0);
		if (tecla == KBD_ESC)
		{
			break;
		}
		else if (tecla == KBD_F10)
		{
			mbsel = (mbsel < 0) ? 0 : mbsel;
			for (;;)
			{
				bar = conioxlib_menubar(1, th.atr0, th.atr1, th.atrhot, baritems, bardesc, 4, (unsigned char) mbsel);
				if (bar == CONIOXLIB_MENU_RIGHT)
				{
					mbsel = (mbsel + 1) & 3;
					continue;
				}
				else if (bar == CONIOXLIB_MENU_LEFT)
				{
					mbsel = (mbsel + 3) & 3;
					continue;
				}
				break;
			}
			if (bar < 4)
			{
				if (bar == 0)
				{
					/* Anchura del desplegable: la misma que calculara menu() */
					l = 1;
					for (i = 0; i < 5; i++)
					{
						if (fileitems[i][0] != '-')
						{
							n = 0;
							for (j = 0; fileitems[i][j] != 0; j++)
							{
								if (fileitems[i][j] != '&')
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
					box(1, 2, (unsigned short) (l + 3), 8, th.atrdialog, th.frame, "", SHADOW);
					sub = conioxlib_menu(2, 3, th.atr0, th.atr1, th.atrhot, fileitems, filedesc, 5, 1, 0);
					if (sub == 0)
					{
						conioxlib_inputbox("Open", "File:", fname, 40);
						f = fopen(fname, "rb");
						if (f != 0)
						{
							memset(lines, 0, sizeof(lines));
							nlines = 0;
							while ((nlines < 119) && (fgets(lines[nlines], 75, f) != 0))
							{
								for (i = 0; lines[nlines][i] != 0; i++)
								{
									if ((lines[nlines][i] == '\r') || (lines[nlines][i] == '\n'))
									{
										lines[nlines][i] = 0;
										break;
									}
								}
								nlines++;
							}
							fclose(f);
							if (nlines == 0)
							{
								nlines = 1;
							}
							cur = 0;
							col = 0;
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
					l = 1;
					for (i = 0; i < 7; i++)
					{
						if (edititems[i][0] != '-')
						{
							n = 0;
							for (j = 0; edititems[i][j] != 0; j++)
							{
								if (edititems[i][j] != '&')
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
					box(7, 2, (unsigned short) (l + 9), 10, th.atrdialog, th.frame, "", SHADOW);
					sub = conioxlib_menu(8, 3, th.atr0, th.atr1, th.atrhot, edititems, editdesc, 7, 1, 0);
					if (sub == 0)
					{
						/* Cortar: guardar copia en deshacer antes de vaciar la linea */
						conioxlib_pushundo(undo, nundo);
						conioxlib_clearundo(redo, nredo);
						strncpy(clip, lines[cur], 75);
						clip[75] = 0;
						lines[cur][0] = 0;
						col = 0;
						drawline = 1;
					}
					else if (sub == 1)
					{
						strncpy(clip, lines[cur], 75);
						clip[75] = 0;
					}
					else if (sub == 2)
					{
						/* Pegar: guardar copia en deshacer antes de sobrescribir */
						conioxlib_pushundo(undo, nundo);
						conioxlib_clearundo(redo, nredo);
						strncpy(lines[cur], clip, 75);
						lines[cur][75] = 0;
						drawline = 1;
					}
					else if (sub == 4)
					{
						/* Limpiar linea: guardar copia en deshacer antes */
						conioxlib_pushundo(undo, nundo);
						conioxlib_clearundo(redo, nredo);
						lines[cur][0] = 0;
						col = 0;
						drawline = 1;
					}
					else if (sub == 5)
					{
						/* Deshacer: mover el estado actual a la pila de rehacer */
						if (nundo > 0)
						{
							conioxlib_pushundo(redo, nredo);
							nundo--;
							memcpy(lines, undo[nundo]->lines, sizeof(lines));
							nlines = undo[nundo]->nlines;
							cur = undo[nundo]->cur;
							col = undo[nundo]->col;
							free(undo[nundo]);
						}
					}
					else if (sub == 6)
					{
						/* Rehacer: mover el estado actual a la pila de deshacer */
						if (nredo > 0)
						{
							conioxlib_pushundo(undo, nundo);
							nredo--;
							memcpy(lines, redo[nredo]->lines, sizeof(lines));
							nlines = redo[nredo]->nlines;
							cur = redo[nredo]->cur;
							col = redo[nredo]->col;
							free(redo[nredo]);
						}
					}
				}
				else if (bar == 2)
				{
					l = 1;
					for (i = 0; i < 3; i++)
					{
						n = 0;
						for (j = 0; searchitems[i][j] != 0; j++)
						{
							if (searchitems[i][j] != '&')
							{
								n++;
							}
						}
						if (n > l)
						{
							l = n;
						}
					}
					box(13, 2, (unsigned short) (l + 15), 6, th.atrdialog, th.frame, "", SHADOW);
					sub = conioxlib_menu(14, 3, th.atr0, th.atr1, th.atrhot, searchitems, searchdesc, 3, 1, 0);
					if (sub == 0)
					{
						conioxlib_inputbox("Find", "Text:", findstr, 30);
						if (findstr[0] != 0)
						{
							for (i = cur; i < nlines; i++)
							{
								if (strstr(lines[i], findstr) != 0)
								{
									cur = i;
									col = 0;
									break;
								}
							}
						}
					}
					else if (sub == 1)
					{
						/* Reemplazar la primera coincidencia desde el cursor */
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
								/* Comprobar que la linea no desborda el limite de 75 */
								if (off + lenrep + lenrest < 75)
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
						/* Ir a linea: entrada numerica con inputmask */
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
				else if (bar == 3)
				{
					break;
				}
			}
			drawfull = 1;
		}
		else if (tecla == KBD_UP)
		{
			if (cur > 0)
			{
				cur--;
			}
		}
		else if (tecla == KBD_DOWN)
		{
			if (cur < nlines - 1)
			{
				cur++;
			}
		}
		else if (tecla == KBD_LEFT)
		{
			if (col > 0)
			{
				col--;
			}
		}
		else if (tecla == KBD_RIGHT)
		{
			if (col < 74)
			{
				col++;
			}
		}
		else if (tecla == KBD_ORG)
		{
			col = 0;
		}
		else if (tecla == KBD_END)
		{
			col = (int) strlen(lines[cur]);
			if (col > 74)
			{
				col = 74;
			}
		}
		else if (tecla == KBD_ENTER)
		{
			if (nlines < 119)
			{
				/* Partir la linea: guardar copia en deshacer antes */
				conioxlib_pushundo(undo, nundo);
				conioxlib_clearundo(redo, nredo);
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
		else if (tecla == KBD_BACK)
		{
			if (col > 0)
			{
				/* Borrar caracter: guardar copia en deshacer antes */
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
		else if ((tecla >= 32) && (tecla < 127))
		{
			i = (int) strlen(lines[cur]);
			if (i < 74)
			{
				/* Insertar caracter: guardar copia en deshacer antes */
				conioxlib_pushundo(undo, nundo);
				conioxlib_clearundo(redo, nredo);
				for (j = i; j >= col; j--)
				{
					lines[cur][j + 1] = lines[cur][j];
				}
				lines[cur][col] = (char) tecla;
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
	/* Liberar las pilas de deshacer/rehacer al salir */
	conioxlib_clearundo(undo, nundo);
	conioxlib_clearundo(redo, nredo);
	conioxlib_restorescr(1, 1, 80, 25, oldb);
	gotoxy(oldx, oldy);
	return(0);
}
