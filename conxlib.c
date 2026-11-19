#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "conxlib.h"

#if defined(__WATCOMC__) || defined(__DJGPP__) || defined(_MSC_VER)
        #include <sys/stat.h>
#endif

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
        #define stricmp _stricmp
        #define strupr  _strupr
        #define strlwr  _strlwr
        #define strrev  _strrev
#endif


/* Portable inline for small private helpers (same policy as coniox). */
#if defined(__WATCOMC__)
        #define conioxlib_inline __inline
#elif (defined(__TURBOC__) && defined(__BORLANDC__) && (__BORLANDC__ >= 0x450))
        #define conioxlib_inline __inline
#elif defined(__TURBOC__)
        #define conioxlib_inline
#elif defined(_MSC_VER)
        #define conioxlib_inline __inline
#else
        #define conioxlib_inline inline
#endif

/* Vector set/get: Watcom names them _dos_setvect/_dos_getvect and Borland
   C has both names; Turbo C only has setvect/getvect, so alias them there.
   _conioxlib_intvec declares interrupt handler pointer variables with the
   qualifiers of each compiler */
#if ((__DOS__) || (__MSDOS__))
        #if defined(__WATCOMC__)
                #define _conioxlib_intvec       void __interrupt __far
        #elif defined(__TURBOC__)
                #define _conioxlib_intvec       void interrupt
        #endif
        #if defined(__TURBOC__) && !defined(__BORLANDC__)
                #define _dos_setvect(v, h)      setvect(v, h)
                #define _dos_getvect(v)         getvect(v)
        #endif
        /* Forward declaration of the INT 29h filter of conioxlib_shell(): it is defined after conioxlib_shell() and Watcom needs the symbol declared at the _dos_setvect() call, Borland accepted the implicit int declaration there */
        #if defined(__WATCOMC__) || defined(__TURBOC__)
                _conioxlib_intvec conioxlib_shell_patch();
        #endif
#endif

/* int86 and registers: Watcom uses r.w, the rest use r.x */
#if ((__DOS__) || (__MSDOS__))
        #if ((defined(__FLAT__)) || (defined(__DJGPP__)))
                #define _coniox_int86   int386
                #define _coniox_int86x  int386x
        #else
                #define _coniox_int86   int86
                #define _coniox_int86x  int86x
        #endif
        #if defined(__WATCOMC__)
                #define _conioxlib_rax(r)       (r).w.ax
                #define _conioxlib_rbx(r)       (r).w.bx
                #define _conioxlib_rcx(r)       (r).w.cx
                #define _conioxlib_rdx(r)       (r).w.dx
                #define _conioxlib_rdi(r)       (r).w.di
                #define _conioxlib_rcflag(r)    (r).w.cflag
                #define _conioxlib_getdisk()            (_getdrive() - 1)
                #define _conioxlib_setdisk(d)   _chdrive((d) + 1)
                #define _conioxlib_getcwd(p, n) getcwd(p, n)
        #else
                #define _conioxlib_rax(r)       (r).x.ax
                #define _conioxlib_rbx(r)       (r).x.bx
                #define _conioxlib_rcx(r)       (r).x.cx
                #define _conioxlib_rdx(r)       (r).x.dx
                #define _conioxlib_rdi(r)       (r).x.di
                #define _conioxlib_rcflag(r)    (r).x.cflag
                #define _conioxlib_getdisk()            getdisk()
                #define _conioxlib_setdisk(d)   setdisk(d)
                #define _conioxlib_getcwd(p, n) getcwd(p, n)
        #endif
        #define _conioxlib_chdir(p)     chdir(p)
        #if defined(__WATCOMC__)
                #define _conioxlib_ffblk                        struct find_t
                #define _conioxlib_findfirst(n, b)      _dos_findfirst(n, _A_NORMAL | _A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_SUBDIR | _A_ARCH, b)
                #define _conioxlib_findnext(b)          _dos_findnext(b)
                #define _conioxlib_ffname(b)            ((b).name)
                #define _conioxlib_ffattrib(b)          ((b).attrib)
                #define _conioxlib_ffsize(b)            ((b).size)
                #define _conioxlib_ffdate(b)            ((b).wr_date)
                #define _conioxlib_fftime(b)            ((b).wr_time)
        #else
                #define _conioxlib_ffblk                        struct ffblk
                #define _conioxlib_findfirst(n, b)      findfirst(n, b, FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_DIREC | FA_ARCH)
                #define _conioxlib_findnext(b)          findnext(b)
                #define _conioxlib_ffname(b)            ((b).ff_name)
                #define _conioxlib_ffattrib(b)          ((b).ff_attrib)
                #define _conioxlib_ffsize(b)            ((b).ff_fsize)
                #define _conioxlib_ffdate(b)            ((b).ff_fdate)
                #define _conioxlib_fftime(b)            ((b).ff_ftime)
        #endif
#endif

#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        #define _conioxlib_getdisk()            (_getdrive() - 1)
        #define _conioxlib_setdisk(d)   _chdrive((d) + 1)
        #define _conioxlib_chdir(p)             _chdir(p)
        #define _conioxlib_getcwd(p, n) _getcwd(p, n)
#endif

struct conioxlib_smousestatus conioxlib_mousestatus = {0};

/* Default TV-style button colors; dialogs may still override via parameters.
   Prefer conioxlib_theme_btnattrs() when building new dialogs. */
/* CONIOXLIB_BTN_ATR* macros removed: were dead code with a bug
   (BTN_ATR1 pointed to atr1 instead of btnatr1). Callers now use
   _conioxlib_curtheme.btnatr0/1/hot directly. */

/* Active theme: kept near the top of the file because clock()/blocks()/status()
   and the _conioxlib_hint() macro use it from the very first moment */
static struct conioxlib_stheme _conioxlib_curtheme = {
        CONIOXLIB_ATTR(BLACK, LIGHTGRAY),   /* atr0: menu bar normal (TV 0x70: Black on LightGray) */
        CONIOXLIB_ATTR(BLACK, GREEN),       /* atr1: menu bar selected (TV 0x20: Black on Green) */
        CONIOXLIB_ATTR(RED, LIGHTGRAY),     /* atrhot: menu hotkey (TV 0x74: Red on LightGray) */
        CONIOXLIB_ATTR(BLACK, LIGHTGRAY),   /* atrstatus: status line (TV 0x70) */
        CONIOXLIB_ATTR(BLACK, LIGHTGRAY),   /* atrdialog: dialog frame passive (TV 0x70) */
        CONIOXLIB_ATTR(BLACK, GREEN),       /* btnatr0: button normal (TV 0x20: Black on Green) */
        CONIOXLIB_ATTR(WHITE, GREEN),       /* btnatr1: button focused (TV 0x2F: White on Green) */
        CONIOXLIB_ATTR(YELLOW, GREEN),      /* btnatrhot: button accelerator (TV 0x2E: Yellow on Green) */
        CONIOXLIB_ATTR(WHITE, BLUE),        /* atrinput: input line passive (TV 0x1F: White on Blue) */
        CONIOXLIB_ATTR(WHITE, GREEN),       /* atrinputsel: input line selected (TV 0x2F) */
        CONIOXLIB_ATTR(BLACK, LIGHTGRAY),   /* atrlabel: label normal (TV 0x70) */
        CONIOXLIB_ATTR(YELLOW, LIGHTGRAY),  /* atrlabelhot: label accelerator (TV 0x7E) */
        CONIOXLIB_ATTR(LIGHTGREEN, LIGHTGRAY), /* atrinterior: dialog interior (TV 0x7A: LightGreen on LightGray) */
        CONIOXLIB_ATTR(BLACK, LIGHTGRAY),   /* atrframe: dialog frame active (TV 0x70) */
        CONIOXLIB_BOX_A_DOUBLE,
        "Borland C++ 3.1"
};

/* Keys captured by conioxlib_pollmouse() under Windows: a small FIFO keeps
   every key of a batch read; the old single slot dropped all but the first.
   Layout constants CONIOXLIB_SCR_W/H, STATUS_*, KEYQUEUE live in conxlib.h. */
static int _conioxlib_keyqueue[CONIOXLIB_KEYQUEUE];
static unsigned char _conioxlib_keyqhead = 0;
static unsigned char _conioxlib_keyqtail = 0;

/* Set whenever a library routine wipes line 25 without restoring it: blocks() and
   clock() repaint the lock indicators and the clock the next time they run */
static int _conioxlib_blocksdirty = 1;

/* Status double buffer: the last text painted on the status strip (columns
   1..st_end of the status row, space-padded) plus a dirty flag. status()
   compares the new text against this buffer and skips the puttext when it is
   unchanged and nothing has wiped the strip, so the ~100 Hz hint refresh in
   the menu/menubar loop does not repaint line 25 every frame. The dirty flag
   is raised by theme() (new status color), background() (full line 25 wipe)
   and restorescr() when the restored region overlaps the status row. */
static char _conioxlib_status_last[256];
static int _conioxlib_status_dirty = 1;

/* "Open from the menu" zoom origin: the screen cell of the item that the last
   menu()/menubar() pick activated. box() with CONIOXLIB_BOX_ZOOM grows the
   frame from this cell when valid, otherwise from the rectangle center. It
   stays valid until the next pick, so a menu bar item followed by a dialog
   zooms the dialog out of the bar item ("combo zoom + item position"). */
static int _conioxlib_zoom_ox = 0;
static int _conioxlib_zoom_oy = 0;
static int _conioxlib_zoom_valid = 0;

static int _conioxlib_filemultiselector_sort(const struct conioxlib_sfiles *fa, const char *la, const struct conioxlib_sfiles *fb, const char *lb, unsigned char column, int desc);
static void _conioxlib_filemultiselector_format(char *res, unsigned long int n);
static void _conioxlib_filemultiselector_row(unsigned short y, int idx, unsigned char atr, int nw, int cw, const char lnames[][CONIOXLIB_LISTITEM_W], const struct conioxlib_sfiles *files);
static unsigned char _conioxlib_enum_drives(char names[][13], char comments[][34], unsigned char max);
/* Prototypes required by Watcom W131 before first use (definitions are later).
   No conioxlib_inline on prototypes: Watcom E1035 rejects __inline on a
   forward declaration without a body (same rule as _conioxlib_enum_drives_set_name). */
static void _conioxlib_fileselector_format_size(char *res, unsigned long int n);
static void _conioxlib_fmt_u2(char *dst, unsigned int v);
static void _conioxlib_fmt_date(char *dest, unsigned int day, unsigned int mon, unsigned int yy);
static void _conioxlib_fmt_time(char *dest, unsigned int hour, unsigned int min);
static unsigned int _conioxlib_dos_yy(unsigned int year);
static unsigned char _conioxlib_build_curdir(char *out);
/* Far scratch for drive letters/comments: defined with enum_drives, declared
   here so filemultiselector/fileselector (earlier in the file) can use them. */
static char CONIOXLIB_FARDATA _conioxlib_drivenames[26][13];
static char CONIOXLIB_FARDATA _conioxlib_drivecomments[26][34];
static void _conioxlib_pollkey_refresh(void);
static conioxlib_inline void _conioxlib_fillch(unsigned short x, unsigned short y, int n, unsigned char ch, unsigned char atr);
/* Forward declaration so _conioxlib_enum_drives sees the helper before its
   definition below. Watcom (W131/W308) requires a prototype for every called
   function when warnings are enabled. Note: no conioxlib_inline here because
   Watcom E1035 fires when __inline appears on a forward declaration without
   the body; the qualifier on the definition below is sufficient. */
static void _conioxlib_enum_drives_set_name(char *out, char letter);
/* Helper for blank-padded button hints: fills CONIOXLIB_HINT_W-1 columns
   with spaces and adds the NUL terminator. Used by msgbox/inputbox/inputmask. */
static conioxlib_inline void _conioxlib_blank_hint(char *hint);
/* Windows: cached standard handles. Defined at file scope so _conioxlib_pollkey
   and _conioxlib_box_zoom (which run before conioxlib_pollmouse in the file)
   can reuse them without calling GetStdHandle per poll cycle (~100 Hz). The
   values are constant for the process lifetime. */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
static HANDLE _conioxlib_hStdIn_cache = 0;
#endif

/* Reusable cell scratch (malloc): avoids large stack arrays on DOS 16-bit.
   Grow-only; never shrinks. Callers must not free the pointer. */
static unsigned short *_conioxlib_cellpool = 0;
static unsigned int _conioxlib_cellpool_cap = 0;

static unsigned short *_conioxlib_need_cells(unsigned int n)
{
        unsigned short *p;
        if (n == 0)
        {
                return 0;
        }
        if (_conioxlib_cellpool_cap >= n)
        {
                return _conioxlib_cellpool;
        }
        /* realloc() can grow in-place when the allocator has room, avoiding a copy */
        p = (unsigned short *) realloc(_conioxlib_cellpool, (size_t) n * sizeof(unsigned short));
        if (p == 0)
        {
                return 0;
        }
        _conioxlib_cellpool = p;
        _conioxlib_cellpool_cap = n;
        return _conioxlib_cellpool;
}

/* Fill n unsigned short cells with a constant value.
   On DOS16 Watcom the compiler generates rep stosw for this pattern with -3s/-4s.
   On DOS32/Win32 Watcom the #pragma aux coniox_fmemsetw in coniox.c uses rep stosd
   (2 cells/iteration via dword alignment + REP STOSD). We delegate to it because
   coniox_far is empty (== near pointer) in FLAT/Win32/Win64 builds.
   On non-Watcom compilers (Borland/Turbo C/DJGPP) coniox_fmemsetw is a C function
   with 4-way unroll, still faster than a plain loop.
   The pool from realloc is always 8-byte aligned, matching coniox_fmemsetw's
   alignment requirements. */
static void _conioxlib_memsetw(unsigned short *dst, unsigned short val, unsigned int n)
{
        /* C fallback with 4-way unroll. On Watcom DOS16 the compiler generates
           rep stosw for this pattern with -3s/-4s. On DOS32/Win32/Win64 it can
           vectorize to rep stosd/stosq. This matches coniox_fmemsetw's C fallback. */
        while (n >= 4)
        {
                dst[0] = val;
                dst[1] = val;
                dst[2] = val;
                dst[3] = val;
                dst += 4;
                n -= 4;
        }
        if (n & 2)
        {
                dst[0] = val;
                dst[1] = val;
                dst += 2;
        }
        if (n & 1)
        {
                *dst = val;
        }
}

/* Combined screen size: one gettextinfo call instead of two. */
static void _conioxlib_scr_size(unsigned short *w, unsigned short *h)
{
        struct text_info ti;
        gettextinfo(&ti);
        *w = (ti.screenwidth >= 40 && ti.screenwidth <= 255) ? (unsigned short) ti.screenwidth : (unsigned short) CONIOXLIB_SCR_W;
        *h = (ti.screenheight >= 14 && ti.screenheight <= 255) ? (unsigned short) ti.screenheight : (unsigned short) CONIOXLIB_SCR_H;
}



/* Live console geometry: prefer gettextinfo() when available so Windows buffers
   wider/taller than 80x25 still clamp correctly. Falls back to SCR_W/H. */
static conioxlib_inline unsigned short _conioxlib_scr_w(void)
{
        struct text_info ti;
        gettextinfo(&ti);
        if (ti.screenwidth >= 40 && ti.screenwidth <= 255)
        {
                return ti.screenwidth;
        }
        return (unsigned short) CONIOXLIB_SCR_W;
}

static conioxlib_inline unsigned short _conioxlib_scr_h(void)
{
        struct text_info ti;
        gettextinfo(&ti);
        if (ti.screenheight >= 14 && ti.screenheight <= 255)
        {
                return ti.screenheight;
        }
        return (unsigned short) CONIOXLIB_SCR_H;
}

/* Shared vertical-list navigation: updates sel and top given a key, vis rows and nitems.
   Returns 1 when the key was a navigation key (caller may need repaint). */
static int _conioxlib_listbox_nav(unsigned int key, int *sel, int *top, int nitems, int vis)
{
        if (key == KBD_UP)
        {
                if (*sel > 0) { (*sel)--; }
        }
        else if (key == KBD_DOWN)
        {
                if (*sel < nitems - 1) { (*sel)++; }
        }
        else if (key == KBD_PGUP)
        {
                *sel -= vis;
                if (*sel < 0) { *sel = 0; }
        }
        else if (key == KBD_PGDW)
        {
                *sel += vis;
                if (*sel >= nitems) { *sel = nitems - 1; }
        }
        else if (key == KBD_ORG)
        {
                *sel = 0;
        }
        else if (key == KBD_END)
        {
                *sel = nitems - 1;
        }
        else
        {
                return 0;
        }
        if (*sel < *top)
        {
                *top = *sel;
        }
        else if (*sel >= *top + vis)
        {
                *top = *sel - vis + 1;
        }
        if (*top < 0)
        {
                *top = 0;
        }
        return 1;
}


/* Pure window scroll (no selection): adjusts *top for UP/DOWN/PGUP/PGDN/HOME/END. */
static int _conioxlib_scroll_nav(unsigned int key, int *top, int nitems, int vis)
{
        if (top == 0)
        {
                return 0;
        }
        if (key == KBD_UP)
        {
                (*top)--;
        }
        else if (key == KBD_DOWN)
        {
                (*top)++;
        }
        else if (key == KBD_PGUP)
        {
                *top -= vis;
        }
        else if (key == KBD_PGDW)
        {
                *top += vis;
        }
        else if (key == KBD_ORG)
        {
                *top = 0;
        }
        else if (key == KBD_END)
        {
                *top = (nitems > vis) ? (nitems - vis) : 0;
        }
        else
        {
                return 0;
        }
        if (*top < 0)
        {
                *top = 0;
        }
        if (nitems <= vis)
        {
                *top = 0;
        }
        else if (*top > nitems - vis)
        {
                *top = nitems - vis;
        }
        return 1;
}

/* Draw a classic Turbo Vision scrollbar in column x.
   vis visible slots starting at row y, spaced by `step` rows (1 for listbox). */
static void _conioxlib_draw_scrollbar(unsigned short x, unsigned short y, int vis, int top, int nitems, unsigned char atr, int step)
{
        int row, thumb;
        unsigned short *cells;
        unsigned short ca;
        if (step < 1)
                step = 1;
        if ((nitems <= vis) || (vis < 3))
                return;
        if (step == 1)
        {
                cells = _conioxlib_need_cells((unsigned int) vis);
                if (cells == 0)
                        return;
                ca = (unsigned short) ((unsigned int) atr << 8);
                cells[0] = (unsigned short) (ca | 0x18);
                for (row = 1; row < vis - 1; row++)
                        cells[row] = (unsigned short) (ca | 0xB0);
                cells[vis - 1] = (unsigned short) (ca | 0x19);
                thumb = 1 + (top * (vis - 3)) / (nitems - vis);
                if (thumb > vis - 2) thumb = vis - 2;
                if (thumb < 1) thumb = 1;
                cells[thumb] = (unsigned short) (ca | 0xB2);
                puttext(x, y, x, (unsigned short) (y + vis - 1), cells);
                return;
        }
        _conioxlib_fillch(x, y, 1, 0x18, atr);
        for (row = 1; row < vis - 1; row++)
                _conioxlib_fillch(x, (unsigned short) (y + row * step), 1, 0xB0, atr);
        _conioxlib_fillch(x, (unsigned short) (y + (vis - 1) * step), 1, 0x19, atr);
        thumb = 1 + (top * (vis - 3)) / (nitems - vis);
        if (thumb > vis - 2) thumb = vis - 2;
        if (thumb < 1) thumb = 1;
        _conioxlib_fillch(x, (unsigned short) (y + thumb * step), 1, 0xB2, atr);
}

/* Button colors of the active theme (btnatr0/btnatr1/btnatrhot). Each theme
   owns its triple, so a Norton Commander dialog paints cyan buttons, a Lotus
   dialog black-on-white ones, and so on, instead of the fixed TV green. */
void conioxlib_theme_btnattrs(unsigned char *atr0, unsigned char *atr1, unsigned char *atrhot)
{
        if (atr0 != 0)
        {
                *atr0 = _conioxlib_curtheme.btnatr0;
        }
        if (atr1 != 0)
        {
                *atr1 = _conioxlib_curtheme.btnatr1;
        }
        if (atrhot != 0)
        {
                *atrhot = _conioxlib_curtheme.btnatrhot;
        }
}


/* Darkens every attribute byte of a rectangle to 8 (dark gray) via gettext/puttext.
   Used by box() shadows so the two shadow arms share one loop and one temp buffer. */
static void _conioxlib_box_darken(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *tmp)
{
        unsigned int n;
        unsigned short *cells;

        if ((x1 > x2) || (y1 > y2))
        {
                return;
        }
        gettext(x1, y1, x2, y2, tmp);
        /* Each cell is a word: low byte char, high byte attr; force attr to dark gray.
           Process 2 cells per iteration via unsigned long (4 bytes) on DOS32/Win32,
           and 4 cells via unsigned long long (8 bytes) on Win64. The buffer from
           box()'s malloc is always 8-byte aligned. */
        n = (unsigned int) (x2 - x1 + 1) * (unsigned int) (y2 - y1 + 1);
        cells = (unsigned short *) tmp;
        /* Process pairs via unsigned long: mask 0x00FF00FF keeps chars, 0x08000800 sets attr */
        while (n >= 2)
        {
                unsigned long pair = *((unsigned long *) cells);
                pair = (pair & 0x00FF00FFUL) | 0x08000800UL;
                *((unsigned long *) cells) = pair;
                cells += 2;
                n -= 2;
        }
        while (n--)
        {
                *cells = (unsigned short) ((*cells & 0x00FF) | 0x0800);
                cells++;
        }
        puttext(x1, y1, x2, y2, tmp);
}

/* Checks for a pending key: the FIFO drained first holds the keys read by
   conioxlib_pollmouse(). Functions instead of macros: identical code, but with
   real type checking, arguments evaluated exactly once and no risk of the old
   declaration-embedding macro breaking unbraced if/else statements. */
static conioxlib_inline int _conioxlib_keyhit(void)
{
        return((_conioxlib_keyqhead != _conioxlib_keyqtail) || kbhit());
}

/* Reads a portable key: extended keys are returned as (scan << 8) and the rest
   as their ASCII code; the FIFO drained first holds the keys read by
   conioxlib_pollmouse(). */
static void _conioxlib_readkey(unsigned int *t)
{
        if (_conioxlib_keyqhead != _conioxlib_keyqtail)
        {
                *t = (unsigned int) _conioxlib_keyqueue[_conioxlib_keyqhead];
                _conioxlib_keyqhead = (unsigned char) ((_conioxlib_keyqhead + 1) & (CONIOXLIB_KEYQUEUE - 1));
        }
        else
        {
                int _conioxlib_k = getch();

                if (_conioxlib_k == 0)
                {
                        *t = ((unsigned int) getch()) << 8;
                }
                else
                {
                        *t = (unsigned int) _conioxlib_k;
                }
        }
}

/* One dialog wait-cycle: refresh clock/locks/mouse, then return a pending key
   or 0 after a short delay. Dialogs still interpret mouse clicks themselves. */
static unsigned int _conioxlib_pollkey(void)
{
        unsigned int key = 0;

        _conioxlib_pollkey_refresh();
        if (_conioxlib_keyhit())
        {
                _conioxlib_readkey(&key);
        }
        else
        {
                /* Prefer a real wait over a fixed delay(10): lower CPU and faster wake on input. */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                {
                        /* Reuse the cached handle from conioxlib_pollmouse: GetStdHandle
                           is a kernel32 call that returns the same value for the whole
                           process lifetime, so polling it ~100 times per second is wasted. */
                        HANDLE h = _conioxlib_hStdIn_cache;
                        if (h == 0)
                        {
                                h = GetStdHandle(STD_INPUT_HANDLE);
                                _conioxlib_hStdIn_cache = h;
                        }
                        if ((h != INVALID_HANDLE_VALUE) && (h != 0))
                        {
                                WaitForSingleObject(h, 10);
                        }
                        else
                        {
                                delay(10);
                        }
                }
#elif ((__DOS__) || (__MSDOS__))
                {
                        /* Sleep until the next BDA timer tick (~55 ms) or a key arrives.
                           A guard counter caps the spin so a TSR that has frozen the
                           timer interrupt cannot hang the whole UI. */
                        unsigned long t0 = peekl(0x40, 0x6C);
                        unsigned long guard = 0;
                        while (!_conioxlib_keyhit() && (peekl(0x40, 0x6C) == t0))
                        {
                                if (++guard > 200000UL)
                                {
                                        delay(1);
                                        break;
                                }
                        }
                }
#else
                delay(10);
#endif
        }
        return(key);
}

/* Status-line hint used by menu/menubar/buttons/option/property/about. */
#define _conioxlib_hint(d)      conioxlib_status(d)

/* Map common mouse gestures when *key is still 0: right-click → F1,
   vertical wheel → upkey/downkey. Horizontal wheel is left to the caller.
   Returns nonzero when *key was written. */
static conioxlib_inline int _conioxlib_mouse_nav(unsigned int *key, unsigned int upkey, unsigned int downkey)
{
        if ((key == 0) || (*key != 0))
        {
                return(0);
        }
        if (conioxlib_mousestatus.rclick != 0)
        {
                *key = KBD_F1;
                return(1);
        }
        if (conioxlib_mousestatus.wheely < 0)
        {
                *key = downkey;
                return(1);
        }
        if (conioxlib_mousestatus.wheely > 0)
        {
                *key = upkey;
                return(1);
        }
        return(0);
}

/* ------------------------- Checks for a pending key --------------------- */
/* kbhit:
   Prototype:
   int conioxlib_kbhit(void);
   Returns nonzero when a key is waiting.
   Parameters: none.
   It sees both the keyboard buffer and the pending keys kept
   by conioxlib_pollmouse() under Windows, so it is the safe
   replacement for the raw kbhit().
   */
int conioxlib_kbhit(void)
{
        return(_conioxlib_keyhit());
}

/* ---------------------------- Reads a key ------------------------------ */
/* getkey:
   Prototype:
   unsigned int conioxlib_getkey(void);
   Reads the next key from the keyboard.
   Parameters: none.
   Plain keys arrive as their ASCII code and extended keys as
   (scan code << 8), so the KBD_ constants can be compared
   directly. It also drains the pending keys captured by
   conioxlib_pollmouse() under Windows.
   */
unsigned int conioxlib_getkey(void)
{
        unsigned int t;
        _conioxlib_readkey(&t);
        return(t);
}


/* ------------------- Finds the accelerator key of a text --------------- */
/* Accelerator: an ampersand before X marks X as the accelerator key and is never
   drawn; with no ampersand, the first letter A-Z/a-z is the accelerator key. */
static conioxlib_inline char _conioxlib_hotmatch_char(const char *s)
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
        int hp;
        int out;
        unsigned char hotatr;
        unsigned short *cells;
        unsigned short cellattr;
        unsigned short hotcell;

        if (s == 0)
        {
                return;
        }
        cells = _conioxlib_need_cells(40);
        if (cells == 0)
        {
                return;
        }
        /* atrhot supplies only the FG of the accelerator; BG comes from the current item attr (normal or selected) */
        hotatr = (unsigned char) ((atr & 0xF0) | (atrhot & 0x0F));
        cellattr = (unsigned short) ((unsigned int) atr << 8);
        hotcell = (unsigned short) ((unsigned int) hotatr << 8);
        /* Position of the accelerator in the source string: char after first '&', else first letter */
        hp = -1;
        for (i = 0; s[i] != 0; i++)
        {
                if ((s[i] == '&') && (s[i + 1] != 0))
                {
                        hp = i + 1;
                        break;
                }
        }
        if (hp < 0)
        {
                for (i = 0; s[i] != 0; i++)
                {
                        if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))
                        {
                                hp = i;
                                break;
                        }
                }
        }
        /* One puttext of the visible glyphs (ampersands are never drawn); caps at 40 cells */
        out = 0;
        for (i = 0; s[i] != 0 && out < 40; i++)
        {
                if (s[i] == '&')
                {
                        continue;
                }
                cells[out++] = (unsigned short) (((i == hp) ? hotcell : cellattr) | (unsigned char) s[i]);
        }
        if (out > 0)
        {
                puttext(x, y, (unsigned short) (x + out - 1), y, cells);
        }
}

/* ---------------- Checks whether a key triggers an option --------------- */
/* Accepts a plain letter and Alt+letter (BIOS scan << 8). The Alt A-Z map is
   the standard US/ES QWERTY keyboard layout. */
static conioxlib_inline int _conioxlib_hotmatch(const char *s, unsigned int key)
{
        /* Alt+A..Z BIOS scan codes (US/ES QWERTY), indexed by letter - 'A'. */
        static const unsigned char altscan[26] = {
                0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
                0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
                0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
        };
        char h;
        unsigned int k;
        unsigned int scan;
        int i;

        h = _conioxlib_hotmatch_char(s);
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
        /* Direct compare: only one letter can match, index is h-'A' */
        i = (int) ((unsigned char) h - 'A');
        if ((i >= 0) && (i < 26) && (altscan[i] == (unsigned char) scan))
        {
                return(1);
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
        unsigned int x, len, i, width;
        unsigned short *cells;
        unsigned short cellattr;
        struct text_info ti;

        if ((str == 0) || (str[0] == 0) || (x2 < x1))
        {
                return;
        }
        /* Compute width first so the cellpool request matches the actual need */
        width = (unsigned int) (x2 - x1 + 1);
        cells = _conioxlib_need_cells((width > 255) ? 256 : (width + 1));
        if (cells == 0)
        {
                return;
        }
        /* Clamp over-long strings: without the guard the centering underflows */
        len = (unsigned int) strlen(str);
        if (len > width)
        {
                len = width;
                x = x1;
        }
        else
        {
                x = x1 + ((width - len) >> 1);
        }
        if (len == 0)
        {
                return;
        }
        if (len > 255)
        {
                len = 255;
        }
        /* textattr() was called by the caller; one puttext replaces gotoxy+cputs */
        gettextinfo(&ti);
        cellattr = (unsigned short) ((unsigned int) ti.attribute << 8);
        for (i = 0; i < len; i++)
        {
                cells[i] = (unsigned short) (cellattr | (unsigned char) str[i]);
        }
        puttext((unsigned short) x, y, (unsigned short) (x + len - 1), y, cells);
}


/* --------------------- Box zoom animation (CONIOXLIB_BOX_ZOOM) --------------------- */
/* Draws just the four frame edges of a rectangle: top, bottom, left and right.
   No interior fill, no title, no close glyph. Used only by the zoom animation
   so each step is a handful of puttext calls instead of a full box redraw. */
static void _conioxlib_box_zoom_outline(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned int atr, const char *frame)
{
        unsigned short *cells;
        unsigned short ca;
        unsigned int w, h, need;

        if ((frame == 0) || (x2 < x1) || (y2 < y1))
        {
                return;
        }
        w = (unsigned int) (x2 - x1) + 1;
        h = (unsigned int) (y2 - y1) + 1;
        need = (w > h) ? w : h;
        cells = _conioxlib_need_cells(need);
        if (cells == 0)
        {
                return;
        }
        ca = (unsigned short) ((atr & 0xFF) << 8);
        /* Top edge: TL + horizontals (frame[2]) + TR */
        cells[0] = (unsigned short) (ca | (unsigned char) frame[0]);
        if (w > 2) _conioxlib_memsetw(cells + 1, (unsigned short)(ca | (unsigned char) frame[2]), (unsigned int)(w - 2));
        if (w > 1)
        {
                cells[w - 1] = (unsigned short) (ca | (unsigned char) frame[1]);
        }
        puttext(x1, y1, x2, y1, cells);
        /* Bottom edge: BL + horizontals (frame[3]) + BR */
        if (h > 1)
        {
                cells[0] = (unsigned short) (ca | (unsigned char) frame[5]);
                _conioxlib_memsetw(cells + 1, (unsigned short)(ca | (unsigned char) frame[3]), (unsigned int)(w - 2));
                if (w > 1)
                {
                        cells[w - 1] = (unsigned short) (ca | (unsigned char) frame[6]);
                }
                puttext(x1, y2, x2, y2, cells);
        }
        /* Left and right edges on the interior rows (frame[4]), one puttext per side */
        if (h > 2)
        {
                _conioxlib_memsetw(cells, (unsigned short)(ca | (unsigned char) frame[4]), (unsigned int)(h - 2));
                puttext(x1, (unsigned short) (y1 + 1), x1, (unsigned short) (y2 - 1), cells);
                if (w > 1)
                {
                        puttext(x2, (unsigned short) (y1 + 1), x2, (unsigned short) (y2 - 1), cells);
                }
        }
}

/* Number of zoom steps: kept inside the 8..12 range requested for the effect. */
#define CONIOXLIB_BOX_ZOOM_STEPS        10

/* Grows the frame from the zoom origin to the final rectangle in
   CONIOXLIB_BOX_ZOOM_STEPS steps. The origin is the last menu()/menubar() pick
   when one is pending ("open from the menu bar" combo), otherwise the center
   of the final rectangle. The animation bounding rect is saved with savescr()
   and restored afterwards so intermediate outlines drawn outside the final
   rectangle leave no debris; any key press aborts the animation early. The
   final box() drawing is unaffected. */
static void _conioxlib_box_zoom(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned int atr, const char *frame)
{
        int ox, oy;
        int bx1, by1, bx2, by2;
        unsigned short scrw, scrh;
        unsigned short ax1, ay1, ax2, ay2;
        char *oldb;
        int k;
        long t;
        int rx1, ry1, rx2, ry2;

        _conioxlib_scr_size(&scrw, &scrh);
        /* Origin: last menu item pick when valid, else the rectangle center */
        if (_conioxlib_zoom_valid)
        {
                ox = _conioxlib_zoom_ox;
                oy = _conioxlib_zoom_oy;
        }
        else
        {
                ox = ((int) x1 + (int) x2) / 2;
                oy = ((int) y1 + (int) y2) / 2;
        }
        /* Animation bounding rect = union of the origin and the final rectangle,
           clamped to the visible console so savescr/gettext stay valid. */
        bx1 = ox;
        if ((int) x1 < bx1) { bx1 = (int) x1; }
        by1 = oy;
        if ((int) y1 < by1) { by1 = (int) y1; }
        bx2 = ox;
        if ((int) x2 > bx2) { bx2 = (int) x2; }
        by2 = oy;
        if ((int) y2 > by2) { by2 = (int) y2; }
        if (bx1 < 1) { bx1 = 1; }
        if (by1 < 1) { by1 = 1; }
        if (bx2 > (int) scrw) { bx2 = (int) scrw; }
        if (by2 > (int) scrh) { by2 = (int) scrh; }
        if ((bx2 < bx1) || (by2 < by1))
        {
                return;
        }
        ax1 = (unsigned short) bx1;
        ay1 = (unsigned short) by1;
        ax2 = (unsigned short) bx2;
        ay2 = (unsigned short) by2;
        oldb = conioxlib_savescr(ax1, ay1, ax2, ay2);
        if (oldb == 0)
        {
                return;
        }
        for (k = 1; k <= CONIOXLIB_BOX_ZOOM_STEPS; k++)
        {
                t = (long) k;
                /* Linear interpolation of each corner from the origin toward its
                   final position; integer math, last step (k == STEPS) is exact. */
                rx1 = ox + (int) (((long) (int) x1 - ox) * t / (long) CONIOXLIB_BOX_ZOOM_STEPS);
                ry1 = oy + (int) (((long) (int) y1 - oy) * t / (long) CONIOXLIB_BOX_ZOOM_STEPS);
                rx2 = ox + (int) (((long) (int) x2 - ox) * t / (long) CONIOXLIB_BOX_ZOOM_STEPS);
                ry2 = oy + (int) (((long) (int) y2 - oy) * t / (long) CONIOXLIB_BOX_ZOOM_STEPS);
                if (rx2 < rx1) { rx2 = rx1; }
                if (ry2 < ry1) { ry2 = ry1; }
                if (rx1 < 1) { rx1 = 1; }
                if (ry1 < 1) { ry1 = 1; }
                if (rx2 > (int) scrw) { rx2 = (int) scrw; }
                if (ry2 > (int) scrh) { ry2 = (int) scrh; }
                _conioxlib_box_zoom_outline((unsigned short) rx1, (unsigned short) ry1, (unsigned short) rx2, (unsigned short) ry2, atr, frame);
                /* Pace each zoom step so the effect is visible regardless of CPU speed.
                   Without this delay the 10 steps complete in <1 ms on modern CPUs and
                   the effect is imperceptible. ~30 ms per step → ~300 ms total for the
                   10-step zoom, matching the original Borland TV look.
                   On Windows WaitForSingleObject on STD_INPUT_HANDLE also wakes up
                   immediately if a key arrives, so a keypress aborts the animation
                   early rather than after the full delay. */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                {
                        /* Reuse the cached stdin handle so each of the 10 zoom steps
                           avoids a kernel32 GetStdHandle round-trip. */
                        HANDLE h = _conioxlib_hStdIn_cache;
                        if (h == 0)
                        {
                                h = GetStdHandle(STD_INPUT_HANDLE);
                                _conioxlib_hStdIn_cache = h;
                        }
                        if ((h != INVALID_HANDLE_VALUE) && (h != 0))
                        {
                                WaitForSingleObject(h, 30);
                        }
                        else
                        {
                                Sleep(30);
                        }
                }
#else
                delay(30);
#endif
                /* Abort the animation on any key press; the key stays buffered. */
                if (_conioxlib_keyhit())
                {
                        break;
                }
        }
        conioxlib_restorescr(ax1, ay1, ax2, ay2, oldb);
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
        unsigned short sx1, sx2, sy1, sy2;
        unsigned short scrw, scrh;
        char *tmp;
        size_t tmpsz;

        _conioxlib_scr_size(&scrw, &scrh);
        /* Shadow scratch sized from live width (2 rows * scrw cells * 2 bytes) + margin */
        if ((frame == 0) || (x2 < x1) || (y2 < y1))
        {
                return;
        }
        /* CONIOXLIB_BOX_ZOOM: animate the frame from the center or from the last
           menu/menubar pick before drawing the final window. Skipped for NOWINDOW
           (no final frame would be drawn, so the animation would be wasted). */
        if ((flags & CONIOXLIB_BOX_ZOOM) && ((flags & NOWINDOW) == 0))
        {
                _conioxlib_box_zoom(x1, y1, x2, y2, atr, frame);
        }
        /* Only allocate the shadow buffer when SHADOW is actually requested; box()
           is called for every dialog, menu and zoom animation, and the previous
           code did malloc/free on every call even for shadowless boxes */
        tmp = 0;
        if (flags & SHADOW)
        {
                tmpsz = (size_t) scrw * 4 + (size_t) scrh * 4 + 16;
                if (tmpsz < 330)
                {
                        tmpsz = 330;
                }
                tmp = (char *) malloc(tmpsz);
                if (tmp == 0)
                {
                        /* No memory for shadow: still draw the frame without it */
                        flags &= ~(unsigned int) SHADOW;
                }
        }
        /* Shadow: clamp to the visible console so gettext/puttext stay valid */
        if ((flags & SHADOW) && (tmp != 0))
        {
                if (y2 < scrh)
                {
                        sx1 = (unsigned short) (x1 + 2);
                        sx2 = (unsigned short) (x2 + 2);
                        if (sx1 < 1)
                        {
                                sx1 = 1;
                        }
                        if (sx2 > scrw)
                        {
                                sx2 = scrw;
                        }
                        if (sx1 <= sx2)
                        {
                                _conioxlib_box_darken(sx1, (unsigned short) (y2 + 1), sx2, (unsigned short) (y2 + 1), tmp);
                        }
                }
                if (x2 < scrw)
                {
                        sx1 = (unsigned short) (x2 + 1);
                        sx2 = (unsigned short) (x2 + 2);
                        sy1 = (unsigned short) (y1 + 1);
                        sy2 = (unsigned short) (y2 + 1);
                        if (sx2 > scrw)
                        {
                                sx2 = scrw;
                        }
                        if (sy2 > scrh)
                        {
                                sy2 = scrh;
                        }
                        if ((sx1 <= sx2) && (sy1 <= sy2))
                        {
                                _conioxlib_box_darken(sx1, sy1, sx2, sy2, tmp);
                        }
                }
        }
        /* No window */
        if ((flags & NOWINDOW) == 0)
        {
                unsigned short *cells;
                unsigned short ca;
                unsigned int maxcells = (unsigned int) scrw + 2;

                if (maxcells > 256)
                {
                        maxcells = 256;
                }
                cells = _conioxlib_need_cells(maxcells);
                if (cells == 0)
                {
                        if (tmp != 0)
                        {
                                free(tmp);
                        }
                        return;
                }
                x = (x2 - x1);
                if (x + 1 > maxcells)
                {
                        x = maxcells - 1;
                }
                ca = (unsigned short) ((atr & 0xFF) << 8);

                /* Top border + optional centered title + CLOSE glyph in one puttext.
                   Avoids a second gettextinfo/puttext from cputsc and a gotoxy/cputs for [■]. */
                cells[0] = (unsigned short) (ca | (unsigned char) frame[0]);
                _conioxlib_memsetw(cells + 1, (unsigned short)(ca | (unsigned char) frame[2]), (unsigned int)(x - 1));
                cells[x] = (unsigned short) (ca | (unsigned char) frame[1]);
                if ((title != 0) && (title[0] != 0) && (x >= 4))
                {
                        unsigned int tl = (unsigned int) strlen(title);
                        unsigned int max_tl;
                        unsigned int tstart;
                        unsigned int ti;

                        /* Leave room for corners and a space on each side of the caption */
                        max_tl = (unsigned int) (x - 3);
                        if (tl > max_tl)
                        {
                                tl = max_tl;
                        }
                        /* Center inside the horizontal rule (indices 1..x-1) */
                        tstart = 1 + (((unsigned int) (x - 1) - (tl + 2)) >> 1);
                        if (tstart < 1)
                        {
                                tstart = 1;
                        }
                        if (tstart + tl + 1 >= x)
                        {
                                tstart = 1;
                        }
                        cells[tstart] = (unsigned short) (ca | ' ');
                        for (ti = 0; ti < tl; ti++)
                        {
                                cells[tstart + 1 + ti] = (unsigned short) (ca | (unsigned char) title[ti]);
                        }
                        cells[tstart + 1 + tl] = (unsigned short) (ca | ' ');
                }
                if ((flags & CLOSE) && (x >= 5))
                {
                        /* "[■]" at columns x1+2..x1+4 (cells 2..4), classic Borland close */
                        cells[2] = (unsigned short) (ca | '[');
                        cells[3] = (unsigned short) (ca | 0xFE);
                        cells[4] = (unsigned short) (ca | ']');
                }
                puttext(x1, y1, (unsigned short) (x1 + x), y1, cells);

                /* Middle rows: build once, puttext once per row (no gotoxy/cputs per line) */
                cells[0] = (unsigned short) (ca | (unsigned char) frame[4]);
                _conioxlib_memsetw(cells + 1, (unsigned short)(ca | ' '), (unsigned int)(x - 1));
                cells[x] = (unsigned short) (ca | (unsigned char) frame[4]);
                for (y = y1 + 1; y < y2; y++)
                {
                        puttext(x1, (unsigned short) y, (unsigned short) (x1 + x), (unsigned short) y, cells);
                }
                /* Bottom border */
                cells[0] = (unsigned short) (ca | (unsigned char) frame[5]);
                _conioxlib_memsetw(cells + 1, (unsigned short)(ca | (unsigned char) frame[3]), (unsigned int)(x - 1));
                cells[x] = (unsigned short) (ca | (unsigned char) frame[6]);
                puttext(x1, y2, (unsigned short) (x1 + x), y2, cells);
        }
        if (tmp != 0)
        {
                free(tmp);
        }
}


/* ------------------- Paints the clock cells of line 25 ------------------- */
/* Writes the time in columns 76 to 80 with the status color of the active
   theme straight through puttext(), so neither the window nor the cursor
   move; conioxlib_clock() and conioxlib_background() share it. */

/* Status strip layout: on classic 80-col screens uses fixed columns;
   when scr_w > 80 the locks+clock pack against the right edge and status
   fills the remaining left area. Row is the last console row.
   The geometry is cached for CONIOXLIB_GEOM_TICK poll cycles: the
   console size rarely changes mid-session, and querying it via
   gettextinfo() on every poll burns ~400 syscalls/second. */
#define CONIOXLIB_GEOM_TICK     64
static unsigned short _conioxlib_geom_w = 0;
static unsigned short _conioxlib_geom_h = 0;
static unsigned int _conioxlib_geom_tick = 0;

static void _conioxlib_status_layout(unsigned short *row, unsigned short *status_end,
        unsigned short *locks_col, unsigned short *clock_col, unsigned short *clock_end)
{
        unsigned short w;
        unsigned short h;

        /* Refresh the cached geometry every CONIOXLIB_GEOM_TICK calls; the steady
           state falls through with zero cost. */
        if ((_conioxlib_geom_w == 0) || ((++_conioxlib_geom_tick & (CONIOXLIB_GEOM_TICK - 1)) == 0))
        {
                _conioxlib_geom_w = _conioxlib_scr_w();
                _conioxlib_geom_h = _conioxlib_scr_h();
        }
        w = _conioxlib_geom_w;
        h = _conioxlib_geom_h;

        if (w < 40)
        {
                w = 40;
        }
        if (h < 2)
        {
                h = 2;
        }
        *row = h;
        *clock_end = w;
        *clock_col = (unsigned short) (w >= 5 ? (w - 4) : 1);
        /* 15 lock cells + 1 gap before clock */
        if (*clock_col > 16)
        {
                *locks_col = (unsigned short) (*clock_col - 16);
        }
        else
        {
                *locks_col = 1;
        }
        *status_end = (*locks_col > 1) ? (unsigned short) (*locks_col - 1) : 1;
}

static conioxlib_inline void _conioxlib_paintclock (const struct tm *t)
{
        unsigned short *cells;
        unsigned short cellattr;
        unsigned int h, m;

        unsigned short row, st_end, locks_col, clock_col, clock_end;

        cells = _conioxlib_need_cells(5);
        if (cells == 0)
        {
                return;
        }
        _conioxlib_status_layout(&row, &st_end, &locks_col, &clock_col, &clock_end);
        /* Hand-format "HH:MM" (5 chars) instead of sprintf("%2u:%02u",...):
           sprintf parses the format string every call; this is hot at ~100 Hz.
           Layout: [tens_of_hour | ones_of_hour | ':' | tens_of_min | ones_of_min]
           Hours shown space-padded to width 2 (matching "%2u"). */
        h = (unsigned int) t->tm_hour;
        m = (unsigned int) t->tm_min;
        cellattr = (unsigned short) ((unsigned int) _conioxlib_curtheme.atrstatus << 8);
        cells[0] = (unsigned short) (cellattr | (unsigned char) ((h >= 10) ? ('0' + (h / 10)) : ' '));
        cells[1] = (unsigned short) (cellattr | (unsigned char) ('0' + (h % 10)));
        cells[2] = (unsigned short) (cellattr | (unsigned char) ':');
        cells[3] = (unsigned short) (cellattr | (unsigned char) ('0' + ((m / 10) % 10)));
        cells[4] = (unsigned short) (cellattr | (unsigned char) ('0' + (m % 10)));
        puttext(clock_col, row, clock_end, row, cells);
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
        static time_t oldt = 0;
        static unsigned int clock_tick = 0;
        time_t t;
        struct tm *ti;

        /* A wiped line 25 lost the clock cells too: force the repaint */
        if (_conioxlib_blocksdirty != 0)
        {
                oldmin = 0xFFFF;
        }
        /* Refresh the lock indicators on every call: blocks() repaints them only
           when their state changed or line 25 was wiped by the library */
        conioxlib_blocks();
        /* Throttle time(0): only check every 8 polls (~12.5 Hz instead of ~100 Hz).
           time() is a kernel syscall on Windows (GetSystemTimeAsFileTime) and
           a DOS int 21h AH=2Ch call. 12.5 Hz is enough to detect a second change
           within ~80 ms. */
        if ((++clock_tick & 7) != 0 && oldmin != 0xFFFF && !_conioxlib_blocksdirty)
        {
                return;
        }
        t = time(0);
        if ((t != oldt) || (oldmin == 0xFFFF))
        {
                oldt = t;
                ti = localtime(&t);
                /* localtime() can fail: never dereference a NULL struct tm */
                if ((ti != 0) && (oldmin != (unsigned int) ti->tm_min))
                {
                        oldmin = (unsigned int) ti->tm_min;
                        _conioxlib_paintclock(ti);
                }
        }
}


/* ------------------- Refreshes clock, locks and mouse ------------------ */
/* poll:
   Internal (Private)
   Runs the conioxlib_clock() plus conioxlib_pollmouse() pair every
   dialog wait loop needs to keep the clock, the lock indicators
   and the mouse status up to date.
   */
static void _conioxlib_pollkey_refresh (void)
{
        conioxlib_clock();
        conioxlib_pollmouse();
}


#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)

#ifndef MOUSE_HWHEELED
#define MOUSE_HWHEELED  0x0008
#endif
#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE  0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS   0x0080
#endif
#ifndef ENABLE_INSERT_MODE
#define ENABLE_INSERT_MODE      0x0020
#endif
#ifndef MOUSE_WHEELED
#define MOUSE_WHEELED   0x0004
#endif

/* Open Watcom / older SDKs lack prototypes; WINAPI (stdcall) so linker resolves GetConsoleWindow@0 not GetConsoleWindow_ */
HWND WINAPI GetConsoleWindow(void);
SHORT WINAPI GetAsyncKeyState(int vKey);
BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);

/* Button edges for asynchronous polling (Windows Terminal does not deliver
   mouse events through ReadConsoleInput); GetDoubleClickTime sets the double click */
static int _conioxlib_oldlbtn = 0;
static int _conioxlib_oldrbtn = 0;
static DWORD _conioxlib_lastlclick = 0;
static DWORD _conioxlib_lastrclick = 0;
/* Cache of the STD_OUTPUT_HANDLE: GetStdHandle is cheap but still a kernel call,
   and pollmouse() used to invoke it twice per frame. The handle is constant for
   the lifetime of the process. */
static HANDLE _conioxlib_hStdOut_cache = 0;
/* _conioxlib_hStdIn_cache moved to file scope (above _conioxlib_pollkey) so the
   per-poll wait in _conioxlib_pollkey and per-zoom-step in _conioxlib_box_zoom
   can reuse it without calling GetStdHandle. */
/* Cache of the last mode we successfully set on STD_INPUT_HANDLE: skip the
   GetConsoleMode probe when it has not drifted (the steady state). */
static DWORD _conioxlib_last_set_mode = 0xFFFFFFFFUL;
/* Tick counter so we re-probe the console mode every N polls even in the
   steady state (in case an external reader clears ENABLE_MOUSE_INPUT). */
static unsigned int _conioxlib_modetick = 0;
/* Double-click threshold: constant for the session, fetched lazily on first click */
static DWORD _conioxlib_dblclk_threshold = 0;

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
        int ch;
        int orgx, orgy;
        CONSOLE_SCREEN_BUFFER_INFO info;
        HWND hwnd;
        POINT pt;
        int ptok;
        CONSOLE_FONT_INFO fi;
        int col, row;
        DWORD want;
        DWORD mf;
        WORD vk;
        /* Cache of the console window handle and of the font cell size:
           the old code refetched both on every poll cycle (~100 Hz) */
        static HWND _conioxlib_hwndcache = 0;
        static int _conioxlib_fszx = 8;
        static int _conioxlib_fszy = 16;
        static unsigned int _conioxlib_fsztick = 0;

        conioxlib_mousestatus.lclick = 0;
        conioxlib_mousestatus.rclick = 0;
        conioxlib_mousestatus.wheelx = 0;
        conioxlib_mousestatus.wheely = 0;
        /* Cached: STD_INPUT_HANDLE is constant for the process lifetime */
        if (_conioxlib_hStdIn_cache == 0)
        {
                _conioxlib_hStdIn_cache = GetStdHandle(STD_INPUT_HANDLE);
        }
        hStdInput = _conioxlib_hStdIn_cache;
        /* Re-assert the mode only when our last set mode drifted (other console
           readers can clear ENABLE_MOUSE_INPUT). The steady state used to do a
           GetConsoleMode syscall every poll (~100 Hz). We probe every 64 polls so
           a mode drift is detected without burning syscalls in the steady state. */
        want = (ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
        if ((_conioxlib_last_set_mode != want) || ((++_conioxlib_modetick & 63) == 0))
        {
                DWORD cur_mode;
                if (GetConsoleMode(hStdInput, &cur_mode))
                {
                        DWORD target = (cur_mode & ~(DWORD) (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE)) | want;
                        if (target != cur_mode)
                        {
                                SetConsoleMode(hStdInput, target);
                        }
                        /* Cache 'want' so the steady-state guard matches: comparing target
                           would always fail because cur_mode carries bits we do not own. */
                        _conioxlib_last_set_mode = want;
                }
        }
        /* Cache STD_OUTPUT_HANDLE: it is constant for the lifetime of the process */
        if (_conioxlib_hStdOut_cache == 0)
        {
                _conioxlib_hStdOut_cache = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        orgx = 0;
        orgy = 0;
        /* GetConsoleScreenBufferInfo on every poll burned a kernel syscall just to
           read srWindow.Left/Top (visible buffer origin). The origin only changes
           when the user scrolls the console buffer, so we re-probe every 64 polls
           to pick that up at near-zero steady-state cost. */
        {
                static short _conioxlib_orgx_cache = 0;
                static short _conioxlib_orgy_cache = 0;
                static unsigned int _conioxlib_org_tick = 0;
                        static int _conioxlib_org_init = 0;
                if (!_conioxlib_org_init || ((++_conioxlib_org_tick & 63) == 0))
                {
                        if (GetConsoleScreenBufferInfo(_conioxlib_hStdOut_cache, &info))
                        {
                                _conioxlib_orgx_cache = (short) info.srWindow.Left;
                                _conioxlib_orgy_cache = (short) info.srWindow.Top;
                                _conioxlib_org_init = 1;
                        }
                }
                orgx = _conioxlib_orgx_cache;
                orgy = _conioxlib_orgy_cache;
        }
        /* Always refresh position from the OS cursor so the demo updates every frame even without a new MOUSE_EVENT. */
        /* The console window handle never changes during a session */
        if (_conioxlib_hwndcache == 0)
        {
                _conioxlib_hwndcache = GetConsoleWindow();
        }
        hwnd = _conioxlib_hwndcache;
        ptok = (hwnd != 0) && GetCursorPos(&pt) && ScreenToClient(hwnd, &pt);
        if (ptok)
        {
                /* The font size is constant for practical purposes: refetch it every 64
                   polls so a mid-session font change is picked up at no steady-state cost */
                if ((++_conioxlib_fsztick & 63) == 1)
                {
                        _conioxlib_fszx = 8;
                        _conioxlib_fszy = 16;
                        if (GetCurrentConsoleFont(_conioxlib_hStdOut_cache, FALSE, &fi))
                        {
                                if (fi.dwFontSize.X > 0)
                                {
                                        _conioxlib_fszx = fi.dwFontSize.X;
                                }
                                if (fi.dwFontSize.Y > 0)
                                {
                                        _conioxlib_fszy = fi.dwFontSize.Y;
                                }
                        }
                }
                col = (pt.x / _conioxlib_fszx) + 1;
                row = (pt.y / _conioxlib_fszy) + 1;
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
                                                conioxlib_mousestatus.wheely += (int) (short) HIWORD(ir[i].Event.MouseEvent.dwButtonState) / 120;
                                        }
                                        else if (ir[i].Event.MouseEvent.dwEventFlags & MOUSE_HWHEELED)
                                        {
                                                conioxlib_mousestatus.wheelx += (int) (short) HIWORD(ir[i].Event.MouseEvent.dwButtonState) / 120;
                                        }
                                        /* A click is a button edge: moving the mouse with a button held down reports the button as pressed on every event and must not repeat the click */
                                        mf = ir[i].Event.MouseEvent.dwEventFlags;
                                        if ((mf & (MOUSE_MOVED | MOUSE_WHEELED | MOUSE_HWHEELED)) == 0)
                                        {
                                                if (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
                                                {
                                                        conioxlib_mousestatus.lclick = (mf & DOUBLE_CLICK) ? 2 : 1;
                                                }
                                                else if (ir[i].Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
                                                {
                                                        conioxlib_mousestatus.rclick = (mf & DOUBLE_CLICK) ? 2 : 1;
                                                }
                                        }
                                        /* Keep the edge detectors of the asynchronous path in step, or the same press would be reported twice */
                                        _conioxlib_oldlbtn = ((ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0);
                                        _conioxlib_oldrbtn = ((ir[i].Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0);
                                }
                                else if ((ir[i].EventType == KEY_EVENT) && (ir[i].Event.KeyEvent.bKeyDown))
                                {
                                        vk = ir[i].Event.KeyEvent.wVirtualKeyCode;
                                        /* Pressing Shift, Ctrl, Alt or a lock key alone is not a key */
                                        if ((vk == VK_SHIFT) || (vk == VK_CONTROL) || (vk == VK_MENU) || (vk == VK_CAPITAL) || (vk == VK_NUMLOCK) || (vk == VK_SCROLL))
                                        {
                                                continue;
                                        }
                                        ch = (int) (unsigned char) ir[i].Event.KeyEvent.uChar.AsciiChar;
                                        if (ch == 0)
                                        {
                                                ch = ((int) ir[i].Event.KeyEvent.wVirtualScanCode) << 8;
                                        }
                                        else if ((ch == KBD_TAB) && (ir[i].Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED))
                                        {
                                                ch = KBD_STAB;
                                        }
                                        /* Queue every key of the batch: a single slot dropped the rest */
                                        _conioxlib_keyqueue[_conioxlib_keyqtail] = ch;
                                        _conioxlib_keyqtail = (unsigned char) ((_conioxlib_keyqtail + 1) & (CONIOXLIB_KEYQUEUE - 1));
                                        if (_conioxlib_keyqtail == _conioxlib_keyqhead)
                                        {
                                                _conioxlib_keyqhead = (unsigned char) ((_conioxlib_keyqhead + 1) & (CONIOXLIB_KEYQUEUE - 1));
                                        }
                                }
                        }
                }
        }
        /* Windows Terminal does not deliver mouse events: poll the buttons
           asynchronously with edge detection. Clicks are only accepted when the console
           has focus and the cursor is inside the client area; in the classic
           console (conhost) the events were already read above. */
        if ((conioxlib_mousestatus.lclick == 0) && (conioxlib_mousestatus.rclick == 0) && ptok && (GetForegroundWindow() == hwnd))
        {
                RECT arc;

                /* pt already holds the cursor in client coordinates (read above) */
                GetClientRect(hwnd, &arc);
                if ((pt.x >= arc.left) && (pt.x < arc.right) && (pt.y >= arc.top) && (pt.y < arc.bottom))
                {
                        /* Single GetTickCount() per branch; double-click threshold is cached */
                        DWORD now = GetTickCount();
                        if (_conioxlib_dblclk_threshold == 0)
                        {
                                _conioxlib_dblclk_threshold = GetDoubleClickTime();
                        }
                        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
                        {
                                if (_conioxlib_oldlbtn == 0)
                                {
                                        conioxlib_mousestatus.lclick = ((now - _conioxlib_lastlclick) <= _conioxlib_dblclk_threshold) ? 2 : 1;
                                        _conioxlib_lastlclick = now;
                                }
                                _conioxlib_oldlbtn = 1;
                        }
                        else
                        {
                                _conioxlib_oldlbtn = 0;
                        }
                        if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
                        {
                                if (_conioxlib_oldrbtn == 0)
                                {
                                        conioxlib_mousestatus.rclick = ((now - _conioxlib_lastrclick) <= _conioxlib_dblclk_threshold) ? 2 : 1;
                                        _conioxlib_lastrclick = now;
                                }
                                _conioxlib_oldrbtn = 1;
                        }
                        else
                        {
                                _conioxlib_oldrbtn = 0;
                        }
                }
        }
}


#endif


#if ((__DOS__) || (__MSDOS__))

static int _conioxlib_ismouse = -1;

/* Library-drawn pointer under DOS: with conioxlib_vgaremap(1) the driver
   pointer is hidden and glyph 0x01 is drawn with its attribute nibbles swapped;
   _conioxlib_ptrcell keeps the original cell to restore it on every move */
static int _conioxlib_mouseptr = 0;
static int _conioxlib_ptrx = -1;
static int _conioxlib_ptry = -1;
static unsigned short _conioxlib_ptrcell = 0;

/* --------------------------- Initializes the mouse ------------------------ */
static void _conioxlib_mouseinit (void)
{
        union REGS r;

        if (_conioxlib_ismouse == -1)
        {
                _conioxlib_rax(r) = 0;
                _coniox_int86(0x33, &r, &r);
                if (_conioxlib_rax(r) == 0xFFFF)
                {
                        _conioxlib_ismouse = _conioxlib_rbx(r);
                        /* Show mouse */
                        _conioxlib_rax(r) = 1;
                        _coniox_int86(0x33, &r, &r);
                }
                else
                {
                        _conioxlib_ismouse = 0;
                }
        }
}


/* --------------------------- Reads the mouse state -------------------- */
void conioxlib_pollmouse (void)
{
        union REGS r;
        static int _conioxlib_haswheel = -1;
        unsigned int bx;
        signed char wheel;

        conioxlib_mousestatus.lclick = 0;
        conioxlib_mousestatus.rclick = 0;
        conioxlib_mousestatus.wheelx = 0;
        conioxlib_mousestatus.wheely = 0;
        _conioxlib_mouseinit();
        if (_conioxlib_ismouse > 0)
        {
                /* CuteMouse and compatible: AX=0011h returns AX=574Dh ('MW') when the wheel API is present */
                if (_conioxlib_haswheel < 0)
                {
                        _conioxlib_rax(r) = 0x11;
                        _coniox_int86(0x33, &r, &r);
                        _conioxlib_haswheel = (_conioxlib_rax(r) == 0x574D) ? 1 : 0;
                }
                _conioxlib_rax(r) = 3;
                _coniox_int86(0x33, &r, &r);
                bx = _conioxlib_rbx(r);
                conioxlib_mousestatus.x = (_conioxlib_rcx(r) >> 3) + 1;
                conioxlib_mousestatus.y = (_conioxlib_rdx(r) >> 3) + 1;
                /* Presses since the last poll (AX=0005h): a held button does not repeat the click and a short click is never lost between two polls */
                _conioxlib_rax(r) = 5;
                _conioxlib_rbx(r) = 0;
                _coniox_int86(0x33, &r, &r);
                conioxlib_mousestatus.lclick = (_conioxlib_rbx(r) != 0) ? 1 : 0;
                _conioxlib_rax(r) = 5;
                _conioxlib_rbx(r) = 1;
                _coniox_int86(0x33, &r, &r);
                conioxlib_mousestatus.rclick = (_conioxlib_rbx(r) != 0) ? 1 : 0;
                if (_conioxlib_haswheel)
                {
                        /* BH = vertical wheel delta since last call (signed). Prefer BX high byte: portable on Watcom/Borland. */
                        wheel = (signed char) ((bx >> 8) & 0xFF);
                        conioxlib_mousestatus.wheely = (int) wheel;
                        /* Horizontal wheel when present is returned by AX=0048h (CuteMouse extension) */
                        _conioxlib_rax(r) = 0x48;
                        _coniox_int86(0x33, &r, &r);
                        if (_conioxlib_rax(r) == 0x48)
                        {
                                conioxlib_mousestatus.wheelx = (int) (signed char) ((_conioxlib_rbx(r) >> 8) & 0xFF);
                                if (conioxlib_mousestatus.wheely == 0)
                                {
                                        conioxlib_mousestatus.wheely = (int) (signed char) (_conioxlib_rbx(r) & 0xFF);
                                }
                        }
                }
                /* Library pointer: erase the previous cell if nobody painted over it and
                   draw the glyph at the current position */
                if (_conioxlib_mouseptr != 0)
                {
                        unsigned short _conioxlib_cells[2];
                        unsigned char _conioxlib_attr;
                        unsigned char _conioxlib_sattr;

                        /* Nothing moved and the pointer cell is intact: one gettext instead of two gettext plus a puttext on every poll */
                        if (((int) conioxlib_mousestatus.x == _conioxlib_ptrx) && ((int) conioxlib_mousestatus.y == _conioxlib_ptry))
                        {
                                _conioxlib_attr = (unsigned char) ((_conioxlib_ptrcell >> 8) & 0xFF);
                                _conioxlib_sattr = (unsigned char) (((_conioxlib_attr & 0x0F) << 4) | (_conioxlib_attr >> 4));
                                if ((gettext((unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (void *) _conioxlib_cells) != 0) && (_conioxlib_cells[0] == (unsigned short) (0x01 | ((unsigned short) _conioxlib_sattr << 8))))
                                {
                                        return;
                                }
                        }
                        if ((_conioxlib_ptrx >= 1) && (_conioxlib_ptry >= 1))
                        {
                                if (gettext((unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (void *) _conioxlib_cells) != 0)
                                {
                                        _conioxlib_attr = (unsigned char) ((_conioxlib_ptrcell >> 8) & 0xFF);
                                        _conioxlib_sattr = (unsigned char) (((_conioxlib_attr & 0x0F) << 4) | (_conioxlib_attr >> 4));
                                        if (_conioxlib_cells[0] == (unsigned short) (0x01 | ((unsigned short) _conioxlib_sattr << 8)))
                                        {
                                                _conioxlib_cells[0] = _conioxlib_ptrcell;
                                                puttext((unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, _conioxlib_cells);
                                        }
                                }
                        }
                        if ((conioxlib_mousestatus.x >= 1) && (conioxlib_mousestatus.x <= (unsigned int) _conioxlib_scr_w()) && (conioxlib_mousestatus.y >= 1) && (conioxlib_mousestatus.y <= (unsigned int) _conioxlib_scr_h()))
                        {
                                _conioxlib_ptrx = (int) conioxlib_mousestatus.x;
                                _conioxlib_ptry = (int) conioxlib_mousestatus.y;
                                if (gettext((unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (void *) _conioxlib_cells) != 0)
                                {
                                        _conioxlib_ptrcell = _conioxlib_cells[0];
                                        _conioxlib_attr = (unsigned char) ((_conioxlib_ptrcell >> 8) & 0xFF);
                                        _conioxlib_sattr = (unsigned char) (((_conioxlib_attr & 0x0F) << 4) | (_conioxlib_attr >> 4));
                                        _conioxlib_cells[0] = (unsigned short) (0x01 | ((unsigned short) _conioxlib_sattr << 8));
                                        puttext((unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, (unsigned short) _conioxlib_ptrx, (unsigned short) _conioxlib_ptry, _conioxlib_cells);
                                }
                        }
                        else
                        {
                                _conioxlib_ptrx = -1;
                                _conioxlib_ptry = -1;
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
        unsigned short *cells;
        unsigned int i, state;
        unsigned short cellattr;
        int on_ins, on_cap, on_num, on_scr;
        static unsigned int oldstate = 0xFFFF;

        cells = _conioxlib_need_cells(15);
        if (cells == 0)
        {
                return;
        }
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        /* Single GetKeyboardState(256 bytes) covers all 256 virtual keys at once;
           the old code made 4 separate GetKeyState calls per poll (~100 Hz).
           Only the lock toggle bits (high bit of VK_INSERT/CAPITAL/NUMLOCK/SCROLL)
           are read here; the rest of the 256-byte table is discarded. */
        {
                BYTE kbd[256];
                if (GetKeyboardState(kbd))
                {
                        on_ins = (kbd[VK_INSERT]  & 0x01) != 0;
                        on_cap = (kbd[VK_CAPITAL] & 0x01) != 0;
                        on_num = (kbd[VK_NUMLOCK] & 0x01) != 0;
                        on_scr = (kbd[VK_SCROLL]  & 0x01) != 0;
                }
                else
                {
                        on_ins = on_cap = on_num = on_scr = 0;
                }
        }
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
        /* The four locks and the status color packed in one word: this runs on every
           poll cycle (~100 Hz), so nothing is rebuilt or painted unless the word
           changed or line 25 was wiped (blocksdirty) */
        state = (unsigned int) ((on_ins ? 1 : 0) | (on_cap ? 2 : 0) | (on_num ? 4 : 0) | (on_scr ? 8 : 0) | ((unsigned int) _conioxlib_curtheme.atrstatus << 4));
        if ((_conioxlib_blocksdirty == 0) && (state == oldstate))
        {
                return;
        }
        oldstate = state;
        _conioxlib_blocksdirty = 0;
        /* Always show the four mnemonics; uppercase when the lock is active */
        memset(status, 32, sizeof(status) - 1);
        status[sizeof(status) - 1] = 0;
        memcpy(&status[0], on_ins ? "INS" : "ins", 3);
        memcpy(&status[4], on_cap ? "CAP" : "cap", 3);
        memcpy(&status[8], on_num ? "NUM" : "num", 3);
        memcpy(&status[12], on_scr ? "SCR" : "scr", 3);
        /* puttext avoids window/gotoxy trouble on Win32 buffered consoles; the status
           line is a fixed 80x25 area: status() writes 1-59, the lock indicators 60-74 */
        cellattr = (unsigned short) ((unsigned int) _conioxlib_curtheme.atrstatus << 8);
        for (i = 0; i < 15; i++)
        {
                cells[i] = (unsigned short) (cellattr | (unsigned char) status[i]);
        }
        {
                unsigned short row, st_end, locks_col, clock_col, clock_end;
                _conioxlib_status_layout(&row, &st_end, &locks_col, &clock_col, &clock_end);
                puttext(locks_col, row, (unsigned short) (locks_col + 14), row, cells);
        }
}


#if ((__DOS__) || (__MSDOS__))

/* Little endian dword inside a raw DOS structure */
#define _conioxlib_le32(p)      (((unsigned long int) (p)[0]) | (((unsigned long int) (p)[1]) << 8) | (((unsigned long int) (p)[2]) << 16) | (((unsigned long int) (p)[3]) << 24))

/* Decodes the 34 bytes of the DOS COUNTRY INFO block (int 21h AH=38h) field by field:
   struct conioxlib_scountry does not match the DOS layout (unsigned int is 4 bytes and
   unsigned long gets padded in the 32 bit builds), so a raw copy shifted every field */
static void _conioxlib_getcountry_parse (struct conioxlib_scountry *sc, const unsigned char *raw)
{
        memset(sc, 0, sizeof(struct conioxlib_scountry));
        sc->date = (unsigned int) raw[0] | ((unsigned int) raw[1] << 8);
        memcpy(sc->curr, &raw[2], 4);
        memcpy(sc->thsep, &raw[7], 2);
        memcpy(sc->dsep, &raw[9], 2);
        memcpy(sc->dtsep, &raw[11], 2);
        memcpy(sc->tmsep, &raw[13], 2);
        sc->currstyle[0] = raw[15];
        sc->digits = raw[16];
        sc->time = raw[17];
        sc->convcase = _conioxlib_le32(&raw[18]);
        sc->dasep = raw[22];
}

#if defined(__FLAT__) && !defined(__DJGPP__)
/* Real mode register block of DPMI int 31h AX=0300h */
struct _conioxlib_rmregs
{
        unsigned long int edi, esi, ebp, reserved, ebx, edx, ecx, eax;
        unsigned short flags, es, ds, fs, gs, ip, cs, sp, ss;
};

/* Calls the real mode int 21h from a DOS extender: int 21h itself would not translate the
   pointers of AX=7303h (the extender only knows the classic functions). A real mode block of
   paras paragraphs is allocated (int 31h AX=0100h returns the segment in AX and the selector
   in DX), inlen bytes are copied to its start, the interrupt is simulated (AX=0300h) with
   DS and ES pointing to the block, and outlen bytes are copied back from offset outoff */
static int _conioxlib_rmint21 (struct _conioxlib_rmregs *rr, unsigned int paras, const void *in, unsigned int inlen, void *out, unsigned int outoff, unsigned int outlen)
{
        union REGS r;
        struct SREGS s;
        unsigned char *low;
        unsigned int sel;
        unsigned int seg;
        int ok;

        memset(&r, 0, sizeof(r));
        _conioxlib_rax(r) = 0x0100;
        _conioxlib_rbx(r) = (unsigned short) paras;
        int386(0x31, &r, &r);
        if (_conioxlib_rcflag(r))
        {
                return(0);
        }
        seg = _conioxlib_rax(r);
        sel = _conioxlib_rdx(r);
        low = (unsigned char *) ((unsigned long int) seg << 4);
        memset(low, 0, (size_t) paras << 4);
        if (inlen != 0)
        {
                memcpy(low, in, inlen);
        }
        rr->ds = (unsigned short) seg;
        rr->es = (unsigned short) seg;
        rr->flags = 0;
        rr->ss = 0;
        rr->sp = 0;
        memset(&r, 0, sizeof(r));
        segread(&s);
        s.es = s.ds;
        _conioxlib_rax(r) = 0x0300;
        _conioxlib_rbx(r) = 0x0021;
        _conioxlib_rcx(r) = 0;
        r.x.edi = (unsigned int) rr;
        int386x(0x31, &r, &r, &s);
        ok = (_conioxlib_rcflag(r) == 0);
        if (ok && (outlen != 0))
        {
                memcpy(out, low + outoff, outlen);
        }
        _conioxlib_rax(r) = 0x0101;
        _conioxlib_rdx(r) = (unsigned short) sel;
        int386(0x31, &r, &r);
        return(ok);
}
#endif

#endif

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
        /* Locale never changes during a process; cache the struct on the first
           call. The old code issued 9 GetLocaleInfoA + 1 GetConsoleOutputCP
           per invocation. */
        static struct conioxlib_scountry _cached_country;
        static int _cache_valid = 0;

        if (sc == 0)
        {
                return;
        }
        if (!_cache_valid)
        {
                char tmp[8] = {0};
                struct conioxlib_scountry *c = &_cached_country;

                memset(c, 0, sizeof(*c));
                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ILDATE, tmp, 8))
                {
                        c->date = (unsigned int) atoi(tmp);
                }
                GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY,  c->curr,   5);
                GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,  c->thsep,  2);
                GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,   c->dsep,   2);
                GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDATE,      c->dtsep,  2);
                GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIME,      c->tmsep,  2);
                tmp[0] = 0;
                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICURRENCY, tmp, 8))
                {
                        c->currstyle[0] = (unsigned char) atoi(tmp);
                }
                tmp[0] = 0;
                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_INEGCURR, tmp, 8))
                {
                        c->currstyle[1] = (unsigned char) atoi(tmp);
                }
                tmp[0] = 0;
                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, tmp, 8))
                {
                        c->digits = (unsigned char) atoi(tmp);
                }
                tmp[0] = 0;
                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITIME, tmp, 8))
                {
                        c->time = (unsigned char) atoi(tmp);
                }
                c->convcase = 0;
                tmp[0] = 0;
                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SLIST, tmp, 8))
                {
                        c->dasep = (unsigned char) tmp[0];
                }
                c->code = (unsigned int) GetConsoleOutputCP();
                _cache_valid = 1;
        }
        *sc = _cached_country;
#else
        unsigned char raw[34];
        unsigned int code;

        memset(raw, 0, sizeof(raw));
        #if defined(__DJGPP__)
        {
                __dpmi_regs r;

                memset(&r, 0, sizeof(r));
                r.x.ax = 0x3800;
                r.x.ds = (unsigned short) (__tb >> 4);
                r.x.dx = (unsigned short) (__tb & 0x0F);
                __dpmi_int(0x21, &r);
                dosmemget(__tb, 34, raw);
                code = r.x.bx;
        }
        #elif defined(__FLAT__)
        {
                struct _conioxlib_rmregs rr;

                memset(&rr, 0, sizeof(rr));
                rr.eax = 0x3800;
                if (!_conioxlib_rmint21(&rr, 3, 0, 0, raw, 0, 34))
                {
                        memset(sc, 0, sizeof(struct conioxlib_scountry));
                        return;
                }
                code = (unsigned int) (rr.ebx & 0xFFFF);
        }
        #else
        {
                union REGS r;
                struct SREGS s;

                memset(&r, 0, sizeof(r));
                memset(&s, 0, sizeof(s));
                _conioxlib_rax(r) = 0x3800;
                _conioxlib_rdx(r) = FP_OFF(raw);
                s.ds = FP_SEG(raw);
                _coniox_int86x(0x21, &r, &r, &s);
                code = _conioxlib_rbx(r);
        }
        #endif
        _conioxlib_getcountry_parse(sc, raw);
        sc->code = code;
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
        unsigned char raw[44];
        char path[4];
        int ok;

        if (df == 0)
        {
                return(1);
        }
        memset(df, 0, sizeof(struct conioxlib_sdiskfree));
        if (drive == 0)
        {
                drv = (unsigned char) _conioxlib_getdisk();
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
        memset(raw, 0, sizeof(raw));
        ok = 0;
        /* FAT32 aware call first. The 44 byte DOS structure is decoded by offsets: struct
           conioxlib_sdiskfree has 4 byte members in the 32 bit builds and does not match it */
        #if defined(__DJGPP__)
        {
                __dpmi_regs r;

                memset(&r, 0, sizeof(r));
                r.x.ax = 0x7303;
                r.x.cx = (unsigned short) sizeof(raw);
                r.x.ds = (unsigned short) (__tb >> 4);
                r.x.dx = (unsigned short) (__tb & 0x0F);
                r.x.es = (unsigned short) ((__tb + 16) >> 4);
                r.x.di = (unsigned short) ((__tb + 16) & 0x0F);
                dosmemput(path, 4, __tb);
                dosmemput(raw, sizeof(raw), __tb + 16);
                __dpmi_int(0x21, &r);
                dosmemget(__tb + 16, sizeof(raw), raw);
                ok = (((r.x.flags & 1) == 0) && (r.x.ax != 0x7300));
        }
        #elif defined(__FLAT__)
        {
                struct _conioxlib_rmregs rr;

                memset(&rr, 0, sizeof(rr));
                rr.eax = 0x7303;
                rr.ecx = sizeof(raw);
                rr.edx = 0;
                rr.edi = 16;
                ok = _conioxlib_rmint21(&rr, 4, path, 4, raw, 16, sizeof(raw));
                ok = (ok && ((rr.flags & 1) == 0) && ((rr.eax & 0xFFFF) != 0x7300));
        }
        #else
        {
                union REGS r;
                struct SREGS s;

                memset(&r, 0, sizeof(r));
                memset(&s, 0, sizeof(s));
                _conioxlib_rax(r) = 0x7303;
                _conioxlib_rcx(r) = (unsigned short) sizeof(raw);
                s.ds = FP_SEG(path);
                _conioxlib_rdx(r) = FP_OFF(path);
                s.es = FP_SEG(raw);
                _conioxlib_rdi(r) = FP_OFF(raw);
                _coniox_int86x(0x21, &r, &r, &s);
                ok = ((_conioxlib_rcflag(r) == 0) && (_conioxlib_rax(r) != 0x7300));
        }
        #endif
        if (ok)
        {
                df->sectors_per_cluster = _conioxlib_le32(&raw[4]);
                df->bytes_per_sector = _conioxlib_le32(&raw[8]);
                df->avail_clusters = _conioxlib_le32(&raw[12]);
                df->total_clusters = _conioxlib_le32(&raw[16]);
                return(0);
        }
        /* Classic AH=36h fallback (no FAT32 support) */
        #if defined(__DJGPP__)
        {
                __dpmi_regs r;

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
        #else
        {
                union REGS r;

                memset(&r, 0, sizeof(r));
                _conioxlib_rax(r) = 0x3600;
                _conioxlib_rdx(r) = (unsigned short) (drv + 1);
                _coniox_int86(0x21, &r, &r);
                if (_conioxlib_rax(r) == 0xFFFF)
                {
                        return(1);
                }
                df->sectors_per_cluster = (unsigned long int) _conioxlib_rax(r);
                df->bytes_per_sector = (unsigned long int) _conioxlib_rcx(r);
                df->avail_clusters = (unsigned long int) _conioxlib_rbx(r);
                df->total_clusters = (unsigned long int) _conioxlib_rdx(r);
        }
        #endif
        return(0);
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
        const unsigned char *auxpal;
        union REGS r;
        /* The three tables never change: declaring them static const avoids re-initializing 112 bytes on every call */
        static const unsigned char dac[] = {0, 1, 2, 3, 4, 5, 20, 7, 56, 57, 58, 59, 60, 61, 62, 63};
        static const unsigned char defpal[] = {0x00, 0x00, 0x00, /* 00 */
                0x00, 0x00, 0x2A, /* 01 */
                0x00, 0x2A, 0x00, /* 02 */
                0x00, 0x2A, 0x2A, /* 03 */
                0x2A, 0x00, 0x00, /* 04 */
                0x2A, 0x00, 0x2A, /* 05 */
                0x2A, 0x15, 0x00, /* 06 */
                0x2A, 0x2A, 0x2A, /* 07 */
                0x15, 0x15, 0x15, /* 08 */
                0x00, 0x00, 0x3F, /* 09 */
                0x00, 0x3F, 0x00, /* 10 */
                0x00, 0x3F, 0x3F, /* 11 */
                0x3F, 0x00, 0x00, /* 12 */
                0x3F, 0x00, 0x3F, /* 13 */
                0x3F, 0x3F, 0x00, /* 14 */
                0x3F, 0x3F, 0x3F};/* 15 */
        static const unsigned char newpal[] = {0x00, 0x00, 0x00, /* 00 */
                0x00, 0x00, 0x24, /* 01 */
                0x00, 0x24, 0x04, /* 02 */
                0x00, 0x22, 0x28, /* 03 */
                0x24, 0x00, 0x00, /* 04 */
                0x28, 0x08, 0x2A, /* 05 */
                0x30, 0x30, 0x08, /* 06 */
                0x2B, 0x2B, 0x2B, /* 07 */
                0x00, 0x00, 0x1D, /* 08 */
                0x04, 0x08, 0x3C, /* 09 */
                0x00, 0x28, 0x0E, /* 10 */
                0x00, 0x1F, 0x3F, /* 11 */
                0x28, 0x04, 0x10, /* 12 */
                0x2C, 0x08, 0x3F, /* 13 */
                0x30, 0x2A, 0x10, /* 14 */
                0x30, 0x30, 0x3F};/* 15 */

        /* Keep bright backgrounds (bit7 = intensity), not blink */
        _conioxlib_rax(r) = 0x1003;
        r.h.bl = 0;
        _coniox_int86(0x10, &r, &r);
        auxpal = (e == 0) ? defpal : newpal;
        /* Program the DAC register used by each text-mode color (the dac table), not registers 0-15 */
        for (i = 0; i <= 15; i++)
        {
                _conioxlib_rax(r) = 0x1010;
                _conioxlib_rbx(r) = dac[i];
                r.h.dh = *auxpal; /* Red */
                auxpal++;
                r.h.ch = *auxpal; /* Green */
                auxpal++;
                r.h.cl = *auxpal; /* Blue */
                auxpal++;
                _coniox_int86(0x10, &r, &r);
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
        static unsigned char _conioxlib_glyphtab[5][16] = {
                {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xBC, 0x98, 0x88, 0x80, 0x00, 0x00, 0x00, 0x00},
                {0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00},
                {0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00},
                {0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
                {0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00}};
        static unsigned char _conioxlib_glyphchr[5] = {0x01, 0x10, 0x11, 0x18, 0x19};
        unsigned int i;
#endif
        union REGS r;

        /* Enable the library pointer and hide the driver one (or restore it) */
        _conioxlib_mouseptr = (on != 0);
        _conioxlib_mouseinit();
        if (_conioxlib_ismouse > 0)
        {
                _conioxlib_rax(r) = (on != 0) ? 2 : 1;
                _coniox_int86(0x33, &r, &r);
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
                        rp.r_dx = _conioxlib_glyphchr[i];
                        rp.r_es = FP_SEG(_conioxlib_glyphtab[i]);
                        rp.r_bp = FP_OFF(_conioxlib_glyphtab[i]);
                        intr(0x10, &rp);
                }
        }
        #elif defined(__DJGPP__)
        {
                __dpmi_regs r2;

                for (i = 0; i < 5; i++)
                {
                        dosmemput(_conioxlib_glyphtab[i], 16, __tb);
                        memset(&r2, 0, sizeof(r2));
                        r2.x.ax = 0x1110;
                        r2.x.bx = 0x0010;
                        r2.x.cx = 1;
                        r2.x.dx = _conioxlib_glyphchr[i];
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

        do
        {
                key = _conioxlib_pollkey();
        }
        while (key == 0);
        return(key);
}


/* ----------------- Clears the screen with a push effect ---------------- */
/* push:
   Prototype:
   void conioxlib_push(void);
   Clears the screen with a push effect.
   Parameters: none.
   Six absolute-screen zones are shifted with coniox movetext() each
   frame, then the vacated edge is blanked. Any key aborts the effect.
   */
void conioxlib_push (void)
{
        unsigned int i;
        unsigned char pushattr = (unsigned char) CONIOXLIB_ATTR(LIGHTGRAY, BLACK);

        _conioxlib_blocksdirty = 1;
        _conioxlib_status_dirty = 1;
        for (i = 0; i != 13; i++)
        {
                /* Zone 1: rows 1-13, columns 1-14, down */
                movetext(1, 1, 14, 12, 1, 2);
                _conioxlib_fillch(1, 1, 14, ' ', pushattr);
                /* Zone 2: rows 1-13, columns 15-64, up */
                movetext(15, 2, 64, 13, 15, 1);
                _conioxlib_fillch(15, 13, 50, ' ', pushattr);
                /* Zone 3: rows 1-13, columns 65-80, down */
                movetext(65, 1, CONIOXLIB_SCR_W, 12, 65, 2);
                _conioxlib_fillch(65, 1, (int)(CONIOXLIB_SCR_W - 64), ' ', pushattr);
                /* Zone 4: rows 14-25, columns 1-14, up */
                movetext(1, 15, 14, CONIOXLIB_SCR_H, 1, 14);
                _conioxlib_fillch(1, CONIOXLIB_SCR_H, 14, ' ', pushattr);
                /* Zone 5: rows 14-25, columns 15-64, down */
                movetext(15, 14, 64, 24, 15, 15);
                _conioxlib_fillch(15, 14, 50, ' ', pushattr);
                /* Zone 6: rows 14-25, columns 65-80, up */
                movetext(65, 15, CONIOXLIB_SCR_W, CONIOXLIB_SCR_H, 65, 14);
                _conioxlib_fillch(65, CONIOXLIB_SCR_H, (int)(CONIOXLIB_SCR_W - 64), ' ', pushattr);
                delay(50);
                if (_conioxlib_keyhit())
                {
                        break;
                }
        }
}


/* ---------------- Shared body of getstr() and getpw() ------------------- */
/* getfield:
   Internal (Private)
   Reads a string at (x,y) editing a local copy of str. With
   mask 0 the characters are echoed and str is also the initial
   value shown; with any other character every key is drawn as
   mask, the initial value is ignored and ESC clears str. ENTER
   accepts the edit, BACKSPACE erases and ESC keeps str as it
   was on entry.
   */
static void _conioxlib_getfield (unsigned short x, unsigned short y, unsigned char atr, char *str, char len, char mask)
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
        _conioxlib_hint((mask != 0) ? "Type password" : "Input a valid string");
        _setcursortype((mask != 0) ? _NOCURSOR : _SOLIDCURSOR);
        /* One puttext of N spaces replaces the field[80] memset+cputs combo and
           drops 80 bytes from the 16-bit DOS near stack */
        _conioxlib_fillch(x, y, (int) len, ' ', atr);
        if (mask != 0)
        {
                aux[0] = 0;
        }
        else
        {
                /* Bound the initial value: never copy past the field width */
                strncpy(aux, str, (size_t) len);
                aux[len] = 0;
        }
        i = (int) strlen(aux);
        textattr(atr);
        gotoxy(x, y);
        cputs(aux);
        do
        {
                tec = conioxlib_ckey();
                /* textattr(atr) was called per key inside this loop; moved outside
                   because atr never changes during the dialog */
                if (tec == KBD_BACK)
                {
                        if (i > 0)
                        {
                                cputs("\b \b");
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
                                putch((mask != 0) ? mask : (char) tec);
                                aux[i] = (char) tec;
                                i++;
                                aux[i] = 0;
                        }
                }
        }
        while ((tec != KBD_ENTER) && (tec != KBD_ESC));
        if (tec == KBD_ENTER)
        {
                strcpy(str, aux);
        }
        else if (mask != 0)
        {
                str[0] = 0;
        }
        _setcursortype(_NOCURSOR);
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
        _conioxlib_getfield(x, y, atr, str, len, 0);
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
        _conioxlib_getfield(x, y, atr, pass, len, '*');
}


/* --------------------- Draws Turbo Vision style buttons ---------------- */
/* Button copied from the Turbo Vision 2.0 TButton::drawState(): a face of
   colored spaces (in the gray Borland dialogs: green background, black text
   and yellow accelerator; white text when selected) with the centered title,
   top-right corner 0xDC and a bottom row of two spaces followed by the
   0xDF strip up to the last column, all with the shadow attribute 0x70 and
   no frame characters. The width of each button is the visible title
   length + 4; "separation" is the horizontal gap between buttons. Returns the
   index of the pressed button, 254 when F1 (help) was pressed or 255 when
   cancelled with ESC. */
/* Draws one button of the row: face, top-right corner and shadow strip.
   Function version of the old macro: identical code, but with real type
   checking and arguments evaluated exactly once. */
static void _conioxlib_buttons_draw(int b, unsigned char c, int *pos, int *wide, int *vislen, unsigned short y1, unsigned char atrhot, const char items[][CONIOXLIB_BTN_W])
{
        unsigned short *cells;
        unsigned short ca;
        int z, w;

        cells = _conioxlib_need_cells(40);
        if (cells == 0)
        {
                return;
        }

        w = wide[b];
        if (w > 24)
        {
                w = 24;
        }
        /* Face: one puttext of spaces, then the centered hot title */
        _conioxlib_fillch((unsigned short) pos[b], y1, w, ' ', c);
        conioxlib_cputs_hot((unsigned short) (pos[b] + 1 + ((wide[b] - vislen[b] - 2) / 2)), y1, c, atrhot, items[b]);
        /* Top-right corner: half block with the dialog-bg shadow color.
           atrdialog matches the surface the button sits on; atrlabel is
           for captions and must not drive the 3-D shadow (PC Tools etc.). */
        ca = ((unsigned short) _conioxlib_curtheme.atrdialog << 8);
        cells[0] = (unsigned short) (ca | 0xDC);
        puttext((unsigned short) (pos[b] + wide[b] - 1), y1, (unsigned short) (pos[b] + wide[b] - 1), y1, cells);
        /* Bottom row: two spaces and the 0xDF strip with the shadow color */
        cells[0] = (unsigned short) (ca | ' ');
        cells[1] = (unsigned short) (ca | ' ');
        for (z = 2; z < w; z++)
        {
                cells[z] = (unsigned short) (ca | 0xDF);
        }
        puttext((unsigned short) pos[b], (unsigned short) (y1 + 1), (unsigned short) (pos[b] + w - 1), (unsigned short) (y1 + 1), cells);
}

/* buttons:
   Prototype:
   unsigned char conioxlib_buttons(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       const char items[][CONIOXLIB_BTN_W], const char desc[][CONIOXLIB_HINT_W],
       unsigned char noptions, unsigned char separation);
   Draws a row of Turbo Vision style buttons.
   Parameters: (x1,y1) is the top left corner, atr0 and atr1
   are the normal and pressed colors, atrhot the accelerator
   color, items the labels, desc the status line hints,
   noptions their number and separation the gap between
   buttons. Returns the index of the pressed button, 254
   when F1 was pressed or 255 when cancelled with ESC.
   */
unsigned char conioxlib_buttons (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_BTN_W], const char desc[][CONIOXLIB_HINT_W], unsigned char noptions, unsigned char separation)
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

        /* A zero count used to fall through with pos[]/wide[]/vislen[]
           uninitialized and let _conioxlib_buttons_draw() paint garbage all
           over the screen on the final redraw */
        if ((noptions == 0) || (items == 0) || (desc == 0))
        {
                return(255);
        }
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
                _conioxlib_buttons_draw(i, c, pos, wide, vislen, y1, atrhot, items);
        }
        _conioxlib_hint(desc[0]);
        x = 0;
        do
        {
                do
                {
                        key = _conioxlib_pollkey();
                        if (key != 0)
                        {
                                if ((key >= 32) || ((key & 0xFF) == 0))
                                {
                                        for (y = 0; y < (int) noptions; y++)
                                        {
                                                if (_conioxlib_hotmatch(items[y], key))
                                                {
                                                        if (y != x)
                                                        {
                                                                _conioxlib_buttons_draw(x, atr0, pos, wide, vislen, y1, atrhot, items);
                                                                x = y;
                                                                _conioxlib_buttons_draw(x, atr1, pos, wide, vislen, y1, atrhot, items);
                                                                _conioxlib_hint(desc[x]);
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
                                                        _conioxlib_buttons_draw(x, atr0, pos, wide, vislen, y1, atrhot, items);
                                                        x = y;
                                                        _conioxlib_buttons_draw(x, atr1, pos, wide, vislen, y1, atrhot, items);
                                                        _conioxlib_hint(desc[x]);
                                                }
                                                key = KBD_ENTER;
                                                break;
                                        }
                                }
                        }
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_LEFT, KBD_RIGHT);
                        }

                }
                while ((key != KBD_LEFT) && (key != KBD_RIGHT) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
                if (key == KBD_LEFT)
                {
                        _conioxlib_buttons_draw(x, atr0, pos, wide, vislen, y1, atrhot, items);
                        x--;
                }
                else if (key == KBD_RIGHT)
                {
                        _conioxlib_buttons_draw(x, atr0, pos, wide, vislen, y1, atrhot, items);
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
                _conioxlib_buttons_draw(x, atr1, pos, wide, vislen, y1, atrhot, items);
                _conioxlib_hint(desc[x]);
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
                _conioxlib_buttons_draw(x, (unsigned char) (((atr1 & 0x0F) << 4) | (atr1 >> 4)), pos, wide, vislen, y1, atrhot, items);
                delay(120);
                _conioxlib_buttons_draw(x, atr1, pos, wide, vislen, y1, atrhot, items);
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
/* Fills n cells at (x,y) with the same character/attribute via one puttext. */
static conioxlib_inline void _conioxlib_fillch(unsigned short x, unsigned short y, int n, unsigned char ch, unsigned char atr)
{
        unsigned short *cells;
        unsigned short cell;

        if (n <= 0)
        {
                return;
        }
        if (n > 80)
        {
                n = 80;
        }
        cells = _conioxlib_need_cells((unsigned int) n);
        if (cells == 0)
        {
                return;
        }
        cell = (unsigned short) (((unsigned int) atr << 8) | ch);
        _conioxlib_memsetw(cells, cell, (unsigned int) n);
        puttext(x, y, (unsigned short) (x + n - 1), y, cells);
}

/* Draw a vertical scrollbar column: up arrow (0x18), track of 0xB0, down arrow (0x19).
   Replaces the gotoxy+putch-per-row pattern duplicated in conioxlib_view and
   _conioxlib_help_view. Single puttext for the entire column. */
static void _conioxlib_draw_vscroll(unsigned short x, unsigned short y1, unsigned short y2, unsigned char atr)
{
        unsigned short *cells;
        unsigned short ca;
        int n;

        if (y2 <= y1 + 1)
        {
                return;
        }
        n = (int) y2 - (int) y1 + 1;
        if (n > 80)
        {
                n = 80;
        }
        cells = _conioxlib_need_cells((unsigned int) n);
        if (cells == 0)
        {
                return;
        }
        ca = (unsigned short) ((unsigned int) atr << 8);
        cells[0] = (unsigned short) (ca | 0x18);
        _conioxlib_memsetw(cells + 1, (unsigned short)(ca | 0xB0), (unsigned int)(n - 2));
        cells[n - 1] = (unsigned short) (ca | 0x19);
        puttext(x, y1, x, y2, cells);
}
#define _conioxlib_menuline(idx, row, a) \
        do { \
                if (items[idx][0] == '-') \
                { \
                        _conioxlib_fillch(x1, (unsigned short) (y1 + (row) * separation), l, 0xC4, (unsigned char) (a)); \
                } \
                else \
                { \
                        conioxlib_cputs_hot(x1, (unsigned short) (y1 + (row) * separation), a, atrhot, items[idx]); \
                        if (vislen[idx] < l) \
                        { \
                                _conioxlib_fillch((unsigned short) (x1 + vislen[idx]), (unsigned short) (y1 + (row) * separation), l - vislen[idx], ' ', (unsigned char) (a)); \
                        } \
                } \
        } while (0)


/* Visible length of a menu item (ampersands are not drawn). */
static conioxlib_inline int _conioxlib_item_vislen(const char *item)
{
        int n = 0;
        int i;
        if (item == 0)
        {
                return 0;
        }
        for (i = 0; item[i] != 0; i++)
        {
                if (item[i] != '&')
                {
                        n++;
                }
        }
        return n;
}

/* True when the row is a non-selectable separator. */
static conioxlib_inline int _conioxlib_item_is_sep(const char *item)
{
        return (item != 0) && (item[0] == '-');
}

/* Move sel by dir (+1/-1) skipping separators; wraps within 0..n-1. */
static int _conioxlib_menu_skip_sep(const char items[][CONIOXLIB_ITEM_W], int n, int sel, int dir)
{
        int guard;
        if ((n <= 0) || (dir == 0))
        {
                return sel;
        }
        guard = n;
        do
        {
                sel += dir;
                if (sel < 0)
                {
                        sel = n - 1;
                }
                else if (sel >= n)
                {
                        sel = 0;
                }
                guard--;
        }
        while (guard > 0 && _conioxlib_item_is_sep(items[sel]));
        return sel;
}

/* First selectable index at or after `from`, or -1. */
static int _conioxlib_menu_first_sel(const char items[][CONIOXLIB_ITEM_W], int n, int from)
{
        int i;
        for (i = from; i < n; i++)
        {
                if (!_conioxlib_item_is_sep(items[i]))
                {
                        return i;
                }
        }
        for (i = 0; i < from && i < n; i++)
        {
                if (!_conioxlib_item_is_sep(items[i]))
                {
                        return i;
                }
        }
        return -1;
}

/* Index of the item whose accelerator matches key, or -1. Skips separators.
   Works on any 2-D item table (menu/menubar/option use CONIOXLIB_ITEM_W,
   listbox reuses it with CONIOXLIB_LISTITEM_W) by taking a base pointer and
   the row width, so the listbox accelerator feature shares one matcher. */
static int _conioxlib_menu_find_hot(const char *base, int roww, int n, unsigned int key)
{
        int i;
        const char *row;
        for (i = 0; i < n; i++)
        {
                row = base + (int) ((size_t) i * (size_t) roww);
                if (!_conioxlib_item_is_sep(row) && _conioxlib_hotmatch(row, key))
                {
                        return i;
                }
        }
        return -1;
}

/* menu:
   Prototype:
   unsigned char conioxlib_menu(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W],
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
unsigned char conioxlib_menu (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], unsigned char noptions, unsigned char separation, unsigned char sel)
{
        int y, prev;
        unsigned int key;
        int x, l, i, n;
        int top, nvis, row, redraw;
        int vislen[CONIOXLIB_MENU_MAX];

        if (noptions > CONIOXLIB_MENU_MAX)
        {
                noptions = CONIOXLIB_MENU_MAX;
        }
        /* The menu needs at least one selectable item and real tables */
        if ((items == 0) || (desc == 0))
        {
                return(CONIOXLIB_MENU_ESC);
        }
        /* A zero separation would divide by zero when counting the visible rows */
        if (separation == 0)
        {
                separation = 1;
        }
        l = 1;
        for (y = 0; y < (int) noptions; y++)
        {
                n = _conioxlib_item_is_sep(items[y]) ? 0 : _conioxlib_item_vislen(items[y]);
                vislen[y] = n;
                if (n > l)
                {
                        l = n;
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
        y = _conioxlib_menu_first_sel(items, (int) noptions, y);
        if (y < 0)
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
                                        _conioxlib_fillch(x1, (unsigned short) (y1 + row * separation), l, ' ', atr0);
                                }
                                else
                                {
                                        _conioxlib_menuline(i, row, (i == y) ? atr1 : atr0);
                                }
                        }
                        /* Scroll bar in column x1+l (shared helper; step = separation) */
                        _conioxlib_draw_scrollbar((unsigned short) (x1 + l), y1, nvis, top, (int) noptions, atr0, (int) separation);
                        redraw = 0;
                }
                _conioxlib_hint(desc[y]);
                do
                {
                        key = _conioxlib_pollkey();
                        if (key != 0)
                        {
                                if ((key >= 32) || ((key & 0xFF) == 0))
                                {
                                        x = _conioxlib_menu_find_hot((const char *) items, CONIOXLIB_ITEM_W, (int) noptions, key);
                                        if (x >= 0)
                                        {
                                                prev = y;
                                                y = x;
                                                key = KBD_ENTER;
                                        }
                                }
                        }
                        if (conioxlib_mousestatus.lclick != 0)
                        {
                                if ((conioxlib_mousestatus.y >= (unsigned int) y1) && (conioxlib_mousestatus.y <= (unsigned int) (y1 + (nvis - 1) * separation)))
                                {
                                        row = ((int) conioxlib_mousestatus.y - (int) y1) / (int) separation;
                                        if (((int) conioxlib_mousestatus.x >= (int) x1) && ((int) conioxlib_mousestatus.x < ((int) x1 + l)))
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
                                                        /* nvis == 3 leaves a single track row: divide by 1 instead of by zero */
                                                        top = ((row - 1) * ((int) noptions - nvis)) / ((nvis > 3) ? (nvis - 3) : 1);
                                                }
                                                if (y < top)
                                                {
                                                        y = top;
                                                        /* Bounded search: with trailing separators the old unbounded
                                                           loop ran past the end of the items table */
                                                        while ((y < (int) noptions) && (items[y][0] == '-'))
                                                        {
                                                                y++;
                                                        }
                                                        if (y >= (int) noptions)
                                                        {
                                                                /* Only separators from the new page on: fall back to the
                                                                   last selectable item below the page start */
                                                                y = top;
                                                                while ((y > 0) && (items[y][0] == '-'))
                                                                {
                                                                        y--;
                                                                }
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
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
                        }
                }
                while ((key != 1) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_LEFT) && (key != KBD_RIGHT) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != KBD_F1));
                if (key != 0)
                {
                        if (key == KBD_UP)
                        {
                                prev = y;
                                y = _conioxlib_menu_skip_sep(items, (int) noptions, y, -1);
                        }
                        else if (key == KBD_DOWN)
                        {
                                prev = y;
                                y = _conioxlib_menu_skip_sep(items, (int) noptions, y, 1);
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
                                        /* Wrap like UP/DOWN so the page keys never leave the list */
                                        if (y > 0)
                                        {
                                                y--;
                                        }
                                        else
                                        {
                                                y = (int) noptions - 1;
                                        }
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
                        _conioxlib_menuline(prev, prev - top, atr0);
                        _conioxlib_menuline(y, y - top, atr1);
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
                        /* Record the picked item's screen cell so a box() opened next with
                           CONIOXLIB_BOX_ZOOM grows from the menu item ("open from the menu"). */
                        _conioxlib_zoom_ox = (int) x1;
                        _conioxlib_zoom_oy = (int) y1 + (y - top) * (int) separation;
                        _conioxlib_zoom_valid = 1;
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
       unsigned char atrhot, char items[][CONIOXLIB_ITEM_W],
       char desc[][CONIOXLIB_HINT_W], unsigned char noptions,
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
unsigned char conioxlib_menubar (unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], unsigned char noptions, unsigned char init)
{
        int i, sel, prev, pos, cell;
        unsigned int key;
        int start[CONIOXLIB_MENU_MAX];
        int width[CONIOXLIB_MENU_MAX];
        struct text_info tiinfo;
        unsigned int oldwl, oldwt, oldwr, oldwb;
        unsigned int oldx, oldy;

        if ((noptions == 0) || (noptions > CONIOXLIB_MENU_MAX) || (items == 0) || (desc == 0))
        {
                return(255);
        }
        /* gettextinfo already returns cursor position in curx/cury; reusing it
           avoids two extra BIOS calls (wherex/wherey each do INT 10h AH=03h) */
        gettextinfo(&tiinfo);
        oldx = (unsigned int) tiinfo.curx;
        oldy = (unsigned int) tiinfo.cury;
        oldwl = tiinfo.winleft;
        oldwt = tiinfo.wintop;
        oldwr = tiinfo.winright;
        oldwb = tiinfo.winbottom;
        window(1, 1, (int) tiinfo.screenwidth, (int) tiinfo.screenheight);
        _wscroll = 0;
        for (i = 0; i < (int) noptions; i++)
        {
                /* Visible glyphs only: the ampersand of the hot key takes no cell */
                width[i] = _conioxlib_item_vislen(items[i]);
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
        _conioxlib_fillch(1, y, (int) tiinfo.screenwidth, ' ', atr0);
        /* _conioxlib_fillch already painted the whole row with atr0 spaces, so
           the per-item putch(' ') before/after cputs_hot just repaints atr0 over
           atr0 — pure overhead. cputs_hot uses puttext and does not depend on
           textattr, so we can drop the gotoxy/textattr/putch calls. */
        for (i = 0; i < (int) noptions; i++)
        {
                conioxlib_cputs_hot((unsigned short) (start[i] + 1), y, atr0, atrhot, items[i]);
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
        _conioxlib_hint(desc[sel]);
        do
        {
                do
                {
                        key = _conioxlib_pollkey();
                        if (key != 0)
                        {
                                if ((key >= 32) || ((key & 0xFF) == 0))
                                {
                                        i = _conioxlib_menu_find_hot((const char *) items, CONIOXLIB_ITEM_W, (int) noptions, key);
                                        if (i >= 0)
                                        {
                                                sel = i;
                                                key = KBD_ENTER;
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
                        else if (!_conioxlib_mouse_nav(&key, KBD_LEFT, KBD_RIGHT))
                        {
                                /* Horizontal wheel: jump to the previous or next menu */
                                if (conioxlib_mousestatus.wheelx < 0)
                                {
                                        key = KBD_RIGHT;
                                }
                                else if (conioxlib_mousestatus.wheelx > 0)
                                {
                                        key = KBD_LEFT;
                                }
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
                _conioxlib_hint(desc[sel]);
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
                /* Record the picked item's screen cell so a box() opened next with
                   CONIOXLIB_BOX_ZOOM grows from the bar item ("open from the menu bar"). */
                _conioxlib_zoom_ox = start[sel];
                _conioxlib_zoom_oy = (int) y;
                _conioxlib_zoom_valid = 1;
                return((unsigned char) sel);
        }
}


/* ---------------- Shared body of option() and property() ---------------- */
/* checklist:
   Internal (Private)
   Runs the check box list of option() when radio is 0 and the
   radio button list of property() when it is not: markopen and
   markclose draw the marks, radio keeps a single entry marked
   and the check boxes advance after every toggle.
   */
static unsigned char _conioxlib_checklist (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled, char markopen, char markclose, unsigned char radio)
{
        int y;
        unsigned int key;
        int x;

        /* Guard the tables: option()/property() used to crash on NULL input */
        if ((items == 0) || (desc == 0) || (sel == 0) || (noptions == 0))
        {
                return(0);
        }
        if (noptions > CONIOXLIB_MENU_MAX)
        {
                noptions = CONIOXLIB_MENU_MAX;
        }
        /* A zero separation would divide by zero on the mouse click */
        if (separation == 0)
        {
                separation = 1;
        }
        textattr(atr0);
        for (y = 0; y != noptions; y++)
        {
                gotoxy(x1, y1 + y * separation);
                cprintf("%c%c%c ", markopen, sel[y], markclose);
                conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr0, atrhot, items[y]);
        }
        if (enabled == 0)
        {
                return(0);
        }
        textattr(atr1);
        gotoxy(x1, y1);
        cprintf("%c%c%c ", markopen, sel[0], markclose);
        conioxlib_cputs_hot((unsigned short) (x1 + 4), y1, atr1, atrhot, items[0]);
        _conioxlib_hint(desc[0]);
        y = 0;
        do
        {
                do
                {
                        key = _conioxlib_pollkey();
                        if (key != 0)
                        {
                                if ((key >= 32) || ((key & 0xFF) == 0))
                                {
                                        for (x = 0; x < (int) noptions; x++)
                                        {
                                                if (_conioxlib_hotmatch(items[x], key))
                                                {
                                                        y = x;
                                                        key = KBD_INS;
                                                        break;
                                                }
                                        }
                                }
                        }
                        /* Left button */
                        if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 20)) && (conioxlib_mousestatus.y >= y1))
                        {
                                x = ((int) conioxlib_mousestatus.y - (int) y1) / separation;
                                if ((x >= 0) && (x < noptions))
                                {
                                        y = x;
                                        key = KBD_INS;
                                }
                        }
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
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
                        if (radio != 0)
                        {
                                /* memset + sel[y] = '\xFE' is done in the repaint block below
                                   so that prev_mark can be computed before clearing */
                        }
                        else
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
                }
                if (y > noptions - 1)
                {
                        y = 0;
                }
                else if (y < 0)
                {
                        y = noptions - 1;
                }
                /* Repaint only what changed:
                   - radio mode: when toggled, only the previously marked row needs
                     atr0 repaint (the new mark is painted atr1 below). When moved
                     by UP/DOWN/TAB without toggle, the previously highlighted row
                     (tracked by _conioxlib_chk_prev) is repainted atr0.
                   - checkbox mode: only the previous highlight row and the current
                     row (painted below) change. */
                if (radio != 0 && ((key == KBD_SPACE) || (key == KBD_INS)))
                {
                        /* SPACE/INS in radio: find the previously marked row (it
                           becomes unmarked) and repaint it in atr0 */
                        int prev_mark = -1;
                        int k2;
                        for (k2 = 0; k2 < (int) noptions; k2++)
                        {
                                if (sel[k2] == '\xFE') { prev_mark = k2; break; }
                        }
                        memset(sel, ' ', noptions);
                        sel[y] = '\xFE';
                        if (prev_mark >= 0 && prev_mark != y)
                        {
                                textattr(atr0);
                                gotoxy(x1, y1 + prev_mark * separation);
                                cprintf("%c%c%c ", markopen, sel[prev_mark], markclose);
                                conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + prev_mark * separation), atr0, atrhot, items[prev_mark]);
                        }
                }
                else if (radio != 0)
                {
                        /* Radio mode, no toggle: track movement with _conioxlib_chk_prev
                           (initialized lazily). The previously highlighted row needs atr0. */
                        /* fall through to checkbox logic for movement repaint */
                }
                if (radio == 0 || !((key == KBD_SPACE) || (key == KBD_INS)))
                {
                        int prev;
                        static int _conioxlib_chk_prev = 0;
                        prev = _conioxlib_chk_prev;
                        if ((prev != y) && (prev >= 0) && (prev < (int) noptions))
                        {
                                textattr(atr0);
                                gotoxy(x1, y1 + prev * separation);
                                cprintf("%c%c%c ", markopen, sel[prev], markclose);
                                conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + prev * separation), atr0, atrhot, items[prev]);
                        }
                        else if ((key == KBD_SPACE) || (key == KBD_INS))
                        {
                                textattr(atr0);
                                gotoxy(x1, y1 + y * separation);
                                cprintf("%c%c%c ", markopen, sel[y], markclose);
                                conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr0, atrhot, items[y]);
                        }
                        _conioxlib_chk_prev = y;
                }
                textattr(atr1);
                gotoxy(x1, y1 + y * separation);
                cprintf("%c%c%c ", markopen, sel[y], markclose);
                conioxlib_cputs_hot((unsigned short) (x1 + 4), (unsigned short) (y1 + y * separation), atr1, atrhot, items[y]);
                _conioxlib_hint(desc[y]);
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


/* ---------------- Option list with check boxes ---------------- */
/* Shows [X] boxes that toggle with the space bar or the left mouse button
   Returns 255 for help (F1) or 0 when done. */
/* option:
   Prototype:
   unsigned char conioxlib_option(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel,
       unsigned char noptions, unsigned char separation,
       unsigned char enabled);
   Runs a list of check boxes.
   Parameters: sel holds one mark byte per item, a space for
   unchecked and the character \\xFE for checked; enabled
   must be nonzero to allow interaction. The space bar and
   the left button toggle the entries. Returns 255 when F1
   was pressed or 0 when the dialog is closed.
   */
unsigned char conioxlib_option (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled)
{
        return(_conioxlib_checklist(x1, y1, atr0, atr1, atrhot, items, desc, sel, noptions, separation, enabled, '[', ']', 0));
}


/* ---------------------- Property menu (buttons) --------------------- */
/* Shows the same check boxes of option() grouped as properties with a side
   mark; the behaviour is identical. */
/* property:
   Prototype:
   unsigned char conioxlib_property(unsigned short x1,
       unsigned short y1, unsigned char atr0,
       unsigned char atr1, unsigned char atrhot,
       const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel,
       unsigned char noptions, unsigned char separation,
       unsigned char enabled);
   Runs a radio button list: exactly one item is marked.
   Parameters: identical to conioxlib_option(), but only one
   entry holds the character \\xFE mark at a time.
   Returns 255 when F1 was pressed or 0 on close.
   */
unsigned char conioxlib_property (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled)
{
        return(_conioxlib_checklist(x1, y1, atr0, atr1, atrhot, items, desc, sel, noptions, separation, enabled, '(', ')', 1));
}


#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)

#pragma warn -par
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
        /* INT 24h may fire while the heap is in the middle of a malloc/free,
           so do NOT call malloc/free from here. Use a static buffer big enough
           for the 63x7 screen region (63*7*2 = 882 bytes) saved below. */
        static char err_oldb[1800];
        static const char *errmsg[13] = {"Disk is write protected", "Unknown unit", "Drive is not ready", "Unknown command", "Data error (Bad CRC)", "Bad request structure length", "Seek error", "Unknown media type", "Sector not found", "Printer is out of paper", "Write fault", "Read fault", "General failure"};
        static const unsigned char answer[4] = {1, 3, 0, 2};
        char butt[][CONIOXLIB_BTN_W] = {"Retry", "Fail", "Ignore", "Abort"};
        char desc[][CONIOXLIB_HINT_W] = {"Retry the process that have caused the error                        ",
                                                                                "Fail the process that have caused the error                         ",
                                                                                "Ignore the occurred error and continue execution                     ",
                                                                                "Terminate the execution of the program                              "};

        err = di & 0x00FF;
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        gettext(8, 10, 70, 16, err_oldb);
        box(8, 10, 68, 15, CONIOXLIB_ATTR(WHITE, RED), A_DOUBLE, "Error", 1);
        cputsc(8, 68, 11, "A fatal error occurred");
        if (err <= 0x0C)
        {
                cputsc(8, 68, 12, errmsg[err]);
        }
        err = conioxlib_buttons (19, 13, _conioxlib_curtheme.btnatr0, _conioxlib_curtheme.btnatr1, _conioxlib_curtheme.btnatrhot, butt, desc, 4, 2);
        gotoxy(oldx, oldy);
        puttext(8, 10, 70, 16, err_oldb);
        /* Retry, Fail, Ignore and Abort map to the DOS answers; ESC or F1 on the dialog fail the call */
        ax = (err < 4) ? answer[err] : 3;
}
#pragma warn .par

#endif


/* -------------------------- Displays a text --------------------------- */
/* view:
   Prototype:
   unsigned char conioxlib_view(unsigned short x1,
       unsigned short y1, unsigned short x2,
       unsigned short y2, unsigned char atr0,
       unsigned char atr1, char buffer[][CONIOXLIB_HELP_W],
       unsigned char last, const char *title);
   Displays a scrollable text viewer.
   Parameters: buffer holds the lines (60 usable columns),
       last is the number of lines stored in buffer (rows 0..last-1
       hold content), atr0 the text color, atr1 the selected line
       color and title the frame title. The wheel, the arrows,
       PGUP, PGDN, HOME and END move; clicking a line jumps to it.
       Returns 255 when F1 was pressed and 0 on ESC or ENTER.
   */
unsigned char conioxlib_view (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char buffer[][CONIOXLIB_HELP_W], unsigned char last, const char *title)
{
        unsigned int c, w, len;
        unsigned char oldx, oldy;
        int f, ini, oldini, maxini;
        unsigned int key = 0;
        char *oldb;

        /* Degenerate rectangles used to loop forever building the frame */
        if ((y2 < (unsigned short) (y1 + 5)) || (x2 < (unsigned short) (x1 + 3)))
        {
                return(255);
        }
        oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
        if (oldb == 0)
        {
                return(0);
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        _conioxlib_hint("ENTER / ESC close; arrows and pages move");
        box(x1, y1, x2, y2, atr0, A_DOUBLE, title, 3);
        _conioxlib_draw_vscroll(x2, (unsigned short)(y1 + 1), (unsigned short)(y2 - 1), atr0);
        ini = 0;
        oldini = -1;
        /* Text columns between the left margin and the scroll bar, never more than the 60 of a line */
        w = (unsigned int) (x2 - x1 - 2);
        if (w > 60)
        {
                w = 60;
        }
        /* Last first line that still fills the window */
        maxini = (int) last - (int) (y2 - y1 - 1);
        if (maxini < 0)
        {
                maxini = 0;
        }
        do
        {
                /* Repaint only when the first visible line changed: the thumb moves and the rows are redrawn */
                if (ini != oldini)
                {
                        oldini = ini;
                        {
                                int vis = (int) (y2 - y1 - 1);
                                if (vis < 1)
                                {
                                        vis = 1;
                                }
                                _conioxlib_draw_scrollbar(x2, (unsigned short) (y1 + 1), vis, ini, (int) last, atr0, 1);
                        }
                        c = y1 + 1;
                        {
                                unsigned short *cells;
                                unsigned short ca;
                                unsigned int i;
                                int vis = (int) (y2 - y1 - 1);
                                unsigned int row_idx;

                                if (vis < 1)
                                {
                                        vis = 1;
                                }
                                /* Build the entire visible block (vis rows × w
                                   cells) in one buffer and emit one puttext for
                                   the whole rectangle. Replaces vis separate
                                   puttext calls (one per row). Max ~22*60=1320
                                   cells = 2640 bytes, well within the cell pool. */
                                cells = _conioxlib_need_cells((unsigned int) vis * w);
                                if (cells == 0)
                                {
                                        break;
                                }
                                ca = (unsigned short) ((unsigned int) atr1 << 8);

                                for (row_idx = 0; row_idx < (unsigned int) vis; row_idx++)
                                {
                                        unsigned short *rowcells = cells + row_idx * w;
                                        f = ini + (int) row_idx;
                                        /* Blank-fill the row baseline in one shot */
                                        _conioxlib_memsetw(rowcells, (unsigned short)(ca | ' '), w);
                                        if (f < (int) last)
                                        {
                                                len = (unsigned int) strlen(buffer[f]);
                                                if (len > w)
                                                {
                                                        len = w;
                                                }
                                                for (i = 0; i < len; i++)
                                                {
                                                        rowcells[i] = (unsigned short) (ca | (unsigned char) buffer[f][i]);
                                                }
                                        }
                                }
                                puttext((unsigned short) (x1 + 2), (unsigned short) c, (unsigned short) (x1 + 1 + w), (unsigned short) (c + vis - 1), cells);
                        }
                }
                do
                {
                        key = _conioxlib_pollkey();
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
                                                ini = (conioxlib_mousestatus.y - y1 - 2) * maxini / (y2 - y1 - 4);
                                                key = 0x1312;
                                        }
                                }
                                else if ((conioxlib_mousestatus.x == x1 + 3) && (conioxlib_mousestatus.y == y1))
                                {
                                        key = KBD_ESC;
                                }
                        }
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
                        }

                }
                while ((key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != 0x1312) && (key != KBD_F1));
                {
                        int vis = (int) (y2 - y1 - 1);
                        if (vis < 1)
                        {
                                vis = 1;
                        }
                        if (_conioxlib_scroll_nav(key, &ini, (int) last, vis) == 0)
                        {
                                /* non-scroll key (ENTER/ESC/F1/thumb click) */
                        }
                        maxini = (int) last - vis;
                        if (maxini < 0)
                        {
                                maxini = 0;
                        }
                        if (ini > maxini)
                        {
                                ini = maxini;
                        }
                }
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
static char *_conioxlib_filemultiselector_readblock (const char *name, unsigned long int *size)
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

/* CR/LF splitting of the DESCRIPT.ION block is done inline by the loop in
   conioxlib_filemultiselector() (each CR/LF becomes a terminator), so no
   separate line-walker helper is needed. */



/* ===================== Help topic cache =====================
   Built on first help()/helpindex() for a given file. Entries store
   the topic name (without '@') and the file offset of the first body
   line after the @header. Rebuilt when the path changes or the file
   size differs from the cached size (dirty). Subsequent lookups are
   a linear scan of the small table + fseek to the body. */
#define CONIOXLIB_HELP_CACHE_MAX        128
#define CONIOXLIB_HELP_CACHE_NAME       31
#define CONIOXLIB_HELP_CACHE_PATH       128

typedef struct
{
        char name[CONIOXLIB_HELP_CACHE_NAME + 1];
        long pos;       /* byte offset of first body line (after @topic) */
} _conioxlib_helptopic;

static char _conioxlib_helpcache_path[CONIOXLIB_HELP_CACHE_PATH];
static long _conioxlib_helpcache_size = -1L;
static long _conioxlib_helpcache_mtime = 0;
static int _conioxlib_helpcache_has_mtime = 0;
static unsigned int _conioxlib_helpcache_n = 0;
/* Topic table is malloc'd on first build and grown to fit the file's topics
   (capped at CONIOXLIB_HELP_CACHE_MAX). Kept on the heap instead of a fixed
   static array so a 16-bit DGROUP does not carry ~3 KB of rarely used data,
   and freed by _conioxlib_helpcache_free() from conioxlib_bye(). */
static _conioxlib_helptopic *_conioxlib_helpcache = 0;
static unsigned int _conioxlib_helpcache_cap = 0;

/* File length in bytes, or -1 on error. */
static long _conioxlib_helpcache_path_size(const char *name)
{
        FILE *f;
        long len;

        if (name == 0 || name[0] == 0)
        {
                return -1L;
        }
        f = fopen(name, "rb");
        if (f == 0)
        {
                return -1L;
        }
        if (fseek(f, 0L, SEEK_END) != 0)
        {
                fclose(f);
                return -1L;
        }
        len = ftell(f);
        fclose(f);
        return len;
}

/* Last-write stamp when the platform provides it; returns 1 and sets *mt, or 0
   (caller treats mtime as unavailable and relies on size alone). */
static int _conioxlib_helpcache_path_mtime(const char *name, long *mt)
{
        if ((name == 0) || (mt == 0))
        {
                return 0;
        }
        *mt = 0;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        {
                WIN32_FILE_ATTRIBUTE_DATA fad;
                if (GetFileAttributesExA(name, GetFileExInfoStandard, &fad))
                {
                        /* Pack low+high into a long-sized fingerprint (enough for dirty check). */
                        *mt = (long) (fad.ftLastWriteTime.dwLowDateTime ^ fad.ftLastWriteTime.dwHighDateTime);
                        return 1;
                }
        }
#elif defined(__WATCOMC__) || defined(__DJGPP__) || defined(_MSC_VER)
        {
                struct stat st;
                if (stat(name, &st) == 0)
                {
                        *mt = (long) st.st_mtime;
                        return 1;
                }
        }
#else
        (void) name;
#endif
        return 0;
}

/* Strip trailing CR/LF from a line buffer in place. */
static void _conioxlib_helpcache_chomp(char *s)
{
        size_t n;

        if (s == 0)
        {
                return;
        }
        n = strlen(s);
        while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        {
                s[--n] = 0;
        }
}

/* True when the cache matches this path and the on-disk size is unchanged. */
static int _conioxlib_helpcache_valid(const char *name)
{
        long sz;
        long mt;

        if ((_conioxlib_helpcache_n == 0) || (_conioxlib_helpcache_path[0] == 0) || (name == 0))
        {
                return 0;
        }
        if (strcmp(_conioxlib_helpcache_path, name) != 0)
        {
                return 0;
        }
        sz = _conioxlib_helpcache_path_size(name);
        if ((sz < 0L) || (sz != _conioxlib_helpcache_size))
        {
                return 0;
        }
        /* When mtime is available it must match; if the platform has no mtime, size alone decides. */
        if (_conioxlib_helpcache_has_mtime)
        {
                if (!_conioxlib_helpcache_path_mtime(name, &mt) || (mt != _conioxlib_helpcache_mtime))
                {
                        return 0;
                }
        }
        return 1;
}

/* Scan the help file once: record every @topic and the offset of its body. */
static int _conioxlib_helpcache_build(const char *name)
{
        FILE *f;
        char line[256];
        long bodypos;
        unsigned int n;
        size_t len;

        _conioxlib_helpcache_n = 0;
        _conioxlib_helpcache_path[0] = 0;
        _conioxlib_helpcache_size = -1L;
        _conioxlib_helpcache_mtime = 0;
        _conioxlib_helpcache_has_mtime = 0;
        if ((name == 0) || (name[0] == 0))
        {
                return 0;
        }
        f = fopen(name, "rb");
        if (f == 0)
        {
                return 0;
        }
        if (fseek(f, 0L, SEEK_END) != 0)
        {
                fclose(f);
                return 0;
        }
        _conioxlib_helpcache_size = ftell(f);
        _conioxlib_helpcache_has_mtime = _conioxlib_helpcache_path_mtime(name, &_conioxlib_helpcache_mtime);
        if ((_conioxlib_helpcache_size < 0L) || (fseek(f, 0L, SEEK_SET) != 0))
        {
                fclose(f);
                _conioxlib_helpcache_size = -1L;
                return 0;
        }
        /* Heap-allocate the topic table once (capped at MAX entries); reused
           across rebuilds so repeated help()/helpindex() calls do not churn the
           heap. Freed by _conioxlib_helpcache_free() from conioxlib_bye(). */
        if (_conioxlib_helpcache_cap < (unsigned int) CONIOXLIB_HELP_CACHE_MAX)
        {
                _conioxlib_helptopic *p;

                p = (_conioxlib_helptopic *) malloc((size_t) CONIOXLIB_HELP_CACHE_MAX * sizeof(_conioxlib_helptopic));
                if (p == 0)
                {
                        fclose(f);
                        _conioxlib_helpcache_size = -1L;
                        return 0;
                }
                if (_conioxlib_helpcache != 0)
                {
                        free(_conioxlib_helpcache);
                }
                _conioxlib_helpcache = p;
                _conioxlib_helpcache_cap = (unsigned int) CONIOXLIB_HELP_CACHE_MAX;
        }
        n = 0;
        while (fgets(line, (int) sizeof(line), f) != 0)
        {
                _conioxlib_helpcache_chomp(line);
                if ((line[0] == '@') && (line[1] != 0))
                {
                        bodypos = ftell(f);     /* first byte after this header line */
                        if (n < CONIOXLIB_HELP_CACHE_MAX)
                        {
                                len = strlen(&line[1]);
                                if (len > CONIOXLIB_HELP_CACHE_NAME)
                                {
                                        len = CONIOXLIB_HELP_CACHE_NAME;
                                }
                                memcpy(_conioxlib_helpcache[n].name, &line[1], len);
                                _conioxlib_helpcache[n].name[len] = 0;
                                _conioxlib_helpcache[n].pos = bodypos;
                                n++;
                        }
                }
        }
        fclose(f);
        _conioxlib_helpcache_n = n;
        strncpy(_conioxlib_helpcache_path, name, CONIOXLIB_HELP_CACHE_PATH - 1);
        _conioxlib_helpcache_path[CONIOXLIB_HELP_CACHE_PATH - 1] = 0;
        return (n > 0) ? 1 : 0;
}

/* Ensure a valid cache for name; rebuilds when missing or dirty. */
static int _conioxlib_helpcache_ensure(const char *name)
{
        if (_conioxlib_helpcache_valid(name))
        {
                return 1;
        }
        return _conioxlib_helpcache_build(name);
}

/* Release the heap-allocated topic table and reset every cache field. Called
   from conioxlib_bye() so the help cache is freed exactly once on exit. Safe
   to call when the cache was never built (pointer is null). */
static void _conioxlib_helpcache_free(void)
{
        if (_conioxlib_helpcache != 0)
        {
                free(_conioxlib_helpcache);
                _conioxlib_helpcache = 0;
        }
        _conioxlib_helpcache_cap = 0;
        _conioxlib_helpcache_n = 0;
        _conioxlib_helpcache_path[0] = 0;
        _conioxlib_helpcache_size = -1L;
        _conioxlib_helpcache_mtime = 0;
        _conioxlib_helpcache_has_mtime = 0;
}

/* Linear lookup of topic name (no '@'); returns index or -1. */
static int _conioxlib_helpcache_find(const char *topic)
{
        unsigned int i;

        if ((topic == 0) || (topic[0] == 0))
        {
                return -1;
        }
        for (i = 0; i < _conioxlib_helpcache_n; i++)
        {
                if (strcmp(_conioxlib_helpcache[i].name, topic) == 0)
                {
                        return (int) i;
                }
        }
        return -1;
}

/* Load topic body at cache index into buffer[][HELP_W]; sets *plast lines (0..255).
   Returns 1 on success, 0 on I/O error. */
static int _conioxlib_helpcache_load_body(const char *name, int idx, char buffer[][CONIOXLIB_HELP_W], unsigned int *plast)
{
        FILE *f;
        char line[256];
        unsigned int last;
        size_t len;

        if ((idx < 0) || ((unsigned int) idx >= _conioxlib_helpcache_n) || (buffer == 0) || (plast == 0))
        {
                return 0;
        }
        f = fopen(name, "rb");
        if (f == 0)
        {
                return 0;
        }
        if (fseek(f, _conioxlib_helpcache[idx].pos, SEEK_SET) != 0)
        {
                fclose(f);
                return 0;
        }
        last = 0;
        while (fgets(line, (int) sizeof(line), f) != 0)
        {
                _conioxlib_helpcache_chomp(line);
                if (line[0] == '@')
                {
                        break;
                }
                if (last < 255)
                {
                        memset(&buffer[last][0], 0, CONIOXLIB_HELP_W);
                        len = strlen(line);
                        if (len > (size_t) (CONIOXLIB_HELP_W - 1))
                        {
                                len = (size_t) (CONIOXLIB_HELP_W - 1);
                        }
                        memcpy(&buffer[last][0], line, len);
                        last++;
                }
        }
        fclose(f);
        *plast = last;
        return 1;
}

/* ---------------------------- Loads the help ---------------------------- */
/* --- Help hyperlink support ---
   Inline "@word" is a hyperlink; "@@" is a literal '@' (e.g. email@@x.com).
   Topic headers (line starting with @name) are handled by the cache builder. */
static conioxlib_inline int _conioxlib_help_linkword_char (char c)
{
        return (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9')) || (c == '_'));
}

static conioxlib_inline unsigned int _conioxlib_help_linklen (const char *s)
{
        unsigned int n = 0;

        while (_conioxlib_help_linkword_char(s[n]))
        {
                n++;
        }
        return(n);
}

static void _conioxlib_help_linkcpy (char *out, const char *s)
{
        unsigned int n = _conioxlib_help_linklen(s);

        if (n > 31)
        {
                n = 31;
        }
        memcpy(out, s, n);
        out[n] = 0;
}

/* True when line[i] starts a real "@word" link (not part of an "@@" escape). */
static conioxlib_inline int _conioxlib_help_is_link (const char *line, unsigned int i)
{
        if (line[i] != '@')
        {
                return(0);
        }
        if ((i > 0) && (line[i - 1] == '@'))
        {
                return(0);
        }
        if (line[i + 1] == '@')
        {
                return(0);
        }
        return(_conioxlib_help_linkword_char(line[i + 1]));
}

#define CONIOXLIB_HELP_LINKS_MAX        64
struct _conioxlib_helplink
{
        unsigned short line;
        unsigned char col;
        unsigned char len;
        char topic[32];
};

/* Hyperlink-aware viewer used only by conioxlib_help(). Mirrors conioxlib_view()
   (scrollable text + scrollbar) but renders every inline "@word" as a highlighted
   hyperlink: TAB cycles the active link, ENTER (or a mouse click on the link)
   follows it. conioxlib_view() itself is left untouched so the public API and the
   demo viewer keep working. Returns 0 on ESC/close, 255 on F1 (show the topic
   index) and 2 when a link was followed (the target topic name is written to
   out_link, 15 chars + NUL). */
static unsigned char _conioxlib_help_view (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2,
        unsigned char atr0, unsigned char atr1, const char buffer[][CONIOXLIB_HELP_W], unsigned char last,
        const char *title, char *out_link)
{
        unsigned int c, w, len, i, k, wl, col, li, nlinks;
        unsigned char oldx, oldy;
        int f, ini, oldini, maxini, vis, active, done, result, isactive;
        unsigned int key;
        char *oldb;
        unsigned char link_attr, active_attr;
        unsigned short la;
        unsigned short *cells;
        unsigned short ca;
        /* TV 2.0 help viewer palette (matches Turbo C++ viewer / BC++ 3.1 help):
           CYAN background throughout. Body = BLACK on CYAN (TV pal[5]=0x30),
           frame = BLACK on CYAN, links = YELLOW on CYAN, active link = WHITE on RED.
           The viewer (conioxlib_view) uses the same look, so the help window
           blends with it. atr0/atr1 are accepted for API symmetry with
           conioxlib_view() but are intentionally ignored. */
        unsigned char help_text_attr = (unsigned char) CONIOXLIB_ATTR(BLACK, CYAN);
        unsigned char help_frame_attr = (unsigned char) CONIOXLIB_ATTR(BLACK, CYAN);
        static struct _conioxlib_helplink links[CONIOXLIB_HELP_LINKS_MAX];

        (void) atr0;
        (void) atr1;
        if ((y2 < (unsigned short) (y1 + 5)) || (x2 < (unsigned short) (x1 + 3)))
        {
                return(255);
        }
        if (out_link == 0)
        {
                return(0);
        }
        oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
        if (oldb == 0)
        {
                return(0);
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        _conioxlib_hint("ENTER follow; TAB/RIGHT next; SHIFT-TAB/LEFT prev; ESC close");
        /* TV help window: white-on-blue double frame, with the title prefixed by 0x11
           (solid left arrow) like Turbo C++ did. The frame attr is forced so a
           green/red/amber theme does not leak into the help window. */
        box(x1, y1, x2, y2, help_frame_attr, A_DOUBLE, title, 3);
        /* Scrollbar on the right edge spans only the body (y1+2 .. y2-1), leaving
           the title row and the rule row clean. */
        _conioxlib_draw_vscroll(x2, (unsigned short)(y1 + 2), (unsigned short)(y2 - 1), help_frame_attr);
        /* Horizontal rule under the title bar (TV help has a thin separator below
           the topic name). Painted as a single puttext row of CD4 across the width. */
        {
                unsigned short *rule_cells = _conioxlib_need_cells((unsigned int)(x2 - x1 - 1));
                if (rule_cells != 0)
                {
                        unsigned short rule_attr = (unsigned short) ((unsigned int) help_frame_attr << 8);
                        unsigned int rj;
                        for (rj = 0; rj < (unsigned int)(x2 - x1 - 1); rj++)
                        {
                                rule_cells[rj] = (unsigned short) (rule_attr | (unsigned char) 0xC4);
                        }
                        puttext((unsigned short)(x1 + 1), (unsigned short)(y1 + 1), (unsigned short)(x2 - 1), (unsigned short)(y1 + 1), rule_cells);
                }
        }
        ini = 0;
        oldini = -1;
        w = (unsigned int) (x2 - x1 - 2);
        if (w > 60)
        {
                w = 60;
        }
        /* Body starts at y1+2: row 0 is the title bar, row 1 is the horizontal rule,
           rows 2.. are the topic body. vis counts only body rows. */
        vis = (int) (y2 - y1 - 2);
        if (vis < 1)
        {
                vis = 1;
        }
        maxini = (int) last - vis;
        if (maxini < 0)
        {
                maxini = 0;
        }
        /* Hyperlinks = YELLOW on CYAN (stands out from black body text);
           active link = WHITE on RED (Borland's selection look) */
        link_attr = (unsigned char) CONIOXLIB_ATTR(YELLOW, CYAN);
        active_attr = (unsigned char) CONIOXLIB_ATTR(WHITE, RED);
        active = -1;
        done = 0;
        result = 0;
        out_link[0] = 0;
        key = 0;
        do
        {
                if (ini != oldini)
                {
                        oldini = ini;
                        /* Rebuild the link table for the visible page */
                        nlinks = 0;
                        for (f = ini; (f < ini + vis) && (f < (int) last); f++)
                        {
                                len = (unsigned int) strlen(buffer[f]);
                                col = 0;
                                for (i = 0; i < len; )
                                {
                                        if ((buffer[f][i] == '@') && (buffer[f][i + 1] == '@'))
                                        {
                                                col++;
                                                i += 2;
                                                continue;
                                        }
                                        if (_conioxlib_help_is_link(buffer[f], i))
                                        {
                                                wl = _conioxlib_help_linklen(&buffer[f][i + 1]);
                                                if (nlinks < CONIOXLIB_HELP_LINKS_MAX)
                                                {
                                                        links[nlinks].line = (unsigned short) f;
                                                        links[nlinks].col = (unsigned char) col;
                                                        links[nlinks].len = (unsigned char) (wl + 1);
                                                        _conioxlib_help_linkcpy(links[nlinks].topic, &buffer[f][i + 1]);
                                                        nlinks++;
                                                }
                                                col += wl + 1;
                                                i += wl + 1;
                                                continue;
                                        }
                                        col++;
                                        i++;
                                }
                        }
                        if (active >= (int) nlinks)
                        {
                                active = (nlinks > 0) ? 0 : -1;
                        }
                        _conioxlib_draw_scrollbar(x2, (unsigned short) (y1 + 2), vis, ini, (int) last, help_frame_attr, 1);
                        c = y1 + 2;       /* body starts below the horizontal rule */
                        /* Allocate one buffer for the whole visible block (vis*w cells)
                           and emit a single puttext for the entire rectangle. Replaces
                           vis separate per-row puttext calls. */
                        cells = _conioxlib_need_cells((unsigned int) vis * w);
                        if (cells == 0)
                        {
                                break;
                        }
                        /* TV help body: YELLOW on BLUE (help_text_attr) */
                        ca = (unsigned short) ((unsigned int) help_text_attr << 8);
                        for (f = ini; f != ini + vis; f++)
                        {
                                unsigned short *rowcells = cells + (unsigned int)(f - ini) * w;
                                /* Blank-fill the row baseline in one shot */
                                _conioxlib_memsetw(rowcells, (unsigned short)(ca | ' '), w);
                                if (f < (int) last)
                                {
                                        len = (unsigned int) strlen(buffer[f]);
                                        i = 0;
                                        col = 0;
                                        while ((i < len) && (col < w))
                                        {
                                                if ((buffer[f][i] == '@') && (buffer[f][i + 1] == '@'))
                                                {
                                                        rowcells[col] = (unsigned short) (ca | (unsigned char) '@');
                                                        col++;
                                                        i += 2;
                                                        continue;
                                                }
                                                if (_conioxlib_help_is_link(buffer[f], i))
                                                {
                                                        wl = _conioxlib_help_linklen(&buffer[f][i + 1]);
                                                        /* O(1) check: the active link's position is known */
                                                        isactive = (active >= 0)
                                                                && (links[active].line == (unsigned short) f)
                                                                && (links[active].col == (unsigned char) col);
                                                        la = (unsigned short) ((isactive) ? ((unsigned int) active_attr << 8) : ((unsigned int) link_attr << 8));
                                                        rowcells[col] = (unsigned short) (la | (unsigned char) buffer[f][i]);
                                                        col++;
                                                        i++;
                                                        for (k = 0; (k < wl) && (col < w); k++)
                                                        {
                                                                rowcells[col] = (unsigned short) (la | (unsigned char) buffer[f][i]);
                                                                col++;
                                                                i++;
                                                        }
                                                }
                                                else
                                                {
                                                        rowcells[col] = (unsigned short) (ca | (unsigned char) buffer[f][i]);
                                                        col++;
                                                        i++;
                                                }
                                        }
                                }
                        }
                        puttext((unsigned short) (x1 + 2), (unsigned short) c, (unsigned short) (x1 + 1 + w), (unsigned short) (c + vis - 1), cells);
                }
                do
                {
                        key = _conioxlib_pollkey();
                        if (conioxlib_mousestatus.lclick != 0)
                        {
                                if (conioxlib_mousestatus.x == x2)
                                {
                                        /* Scroll arrows sit on the rule row + bottom row */
                                        if (conioxlib_mousestatus.y == y1 + 2)
                                        {
                                                key = KBD_UP;
                                        }
                                        else if (conioxlib_mousestatus.y == y2 - 1)
                                        {
                                                key = KBD_DOWN;
                                        }
                                        else if ((conioxlib_mousestatus.y >= y1 + 3) && (conioxlib_mousestatus.y <= y2 - 2) && (y2 - y1 > 5) && (last != 0))
                                        {
                                                ini = (conioxlib_mousestatus.y - y1 - 3) * maxini / (y2 - y1 - 5);
                                                key = 0x1312;
                                        }
                                }
                                else if ((conioxlib_mousestatus.x == x1 + 3) && (conioxlib_mousestatus.y == y1))
                                {
                                        key = KBD_ESC;
                                }
                                else if ((conioxlib_mousestatus.x > (unsigned int) (x1 + 1)) && (conioxlib_mousestatus.x < (unsigned int) (x1 + 2 + w)) &&
                                        (conioxlib_mousestatus.y > (unsigned int) (y1 + 1)) && (conioxlib_mousestatus.y < (unsigned int) y2))
                                {
                                        /* Click inside the body: follow a link if the click lands on one.
                                           Body starts at y1+2 (row 0 is the title, row 1 is the rule). */
                                        int clickline = ini + (int) (conioxlib_mousestatus.y - y1 - 2);
                                        int clickcol = (int) (conioxlib_mousestatus.x - x1 - 2);

                                        if ((clickline >= 0) && (clickline < (int) last) && (clickcol >= 0))
                                        {
                                                for (li = 0; li < nlinks; li++)
                                                {
                                                        if ((links[li].line == (unsigned short) clickline) &&
                                                                (clickcol >= (int) links[li].col) && (clickcol < (int) (links[li].col + links[li].len)))
                                                        {
                                                                strcpy(out_link, links[li].topic);
                                                                result = 2;
                                                                done = 1;
                                                                key = KBD_ENTER;
                                                                break;
                                                        }
                                                }
                                        }
                                }
                        }
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
                                /* Horizontal wheel cycles hyperlinks (same as LEFT/RIGHT) */
                                if ((key == 0) && (conioxlib_mousestatus.wheelx < 0))
                                {
                                        key = KBD_LEFT;
                                }
                                else if ((key == 0) && (conioxlib_mousestatus.wheelx > 0))
                                {
                                        key = KBD_RIGHT;
                                }
                        }
                }
                while ((key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER) && (key != 0x1312) && (key != KBD_F1) && (key != KBD_TAB) && (key != KBD_STAB) && (key != KBD_LEFT) && (key != KBD_RIGHT) && (done == 0));
                if (done == 0)
                {
                        /* Vertical nav only; LEFT/RIGHT are reserved for hyperlink cycling */
                        if ((key == KBD_UP) || (key == KBD_DOWN) || (key == KBD_PGUP) || (key == KBD_PGDW) || (key == KBD_ORG) || (key == KBD_END))
                        {
                                _conioxlib_scroll_nav(key, &ini, (int) last, vis);
                                maxini = (int) last - vis;
                                if (maxini < 0)
                                {
                                        maxini = 0;
                                }
                                if (ini > maxini)
                                {
                                        ini = maxini;
                                }
                        }
                }
                if (done == 0)
                {
                        if ((key == KBD_TAB) || (key == KBD_RIGHT))
                        {
                                if (nlinks > 0)
                                {
                                        active++;
                                        if (active >= (int) nlinks)
                                        {
                                                active = 0;
                                        }
                                        /* Scroll the body so the active link stays visible:
                                           if the link is above the top, scroll up; if it's below
                                           the bottom, scroll down. This matches BC++ 3.1 help
                                           where TAB walks links and the page scrolls to follow. */
                                        if ((active >= 0) && (active < (int) nlinks))
                                        {
                                                int linkline = (int) links[active].line;
                                                if (linkline < ini)
                                                {
                                                        ini = linkline;
                                                }
                                                else if (linkline >= ini + vis)
                                                {
                                                        ini = linkline - vis + 1;
                                                        if (ini > maxini) ini = maxini;
                                                }
                                        }
                                        oldini = -1;
                                }
                        }
                        else if ((key == KBD_STAB) || (key == KBD_LEFT))
                        {
                                if (nlinks > 0)
                                {
                                        if (active <= 0)
                                        {
                                                active = (int) nlinks - 1;
                                        }
                                        else
                                        {
                                                active--;
                                        }
                                        /* Same scroll-to-link logic as TAB */
                                        if ((active >= 0) && (active < (int) nlinks))
                                        {
                                                int linkline = (int) links[active].line;
                                                if (linkline < ini)
                                                {
                                                        ini = linkline;
                                                }
                                                else if (linkline >= ini + vis)
                                                {
                                                        ini = linkline - vis + 1;
                                                        if (ini > maxini) ini = maxini;
                                                }
                                        }
                                        oldini = -1;
                                }
                        }
                        else if (key == KBD_ENTER)
                        {
                                if ((active >= 0) && (active < (int) nlinks))
                                {
                                        strcpy(out_link, links[active].topic);
                                        result = 2;
                                        done = 1;
                                }
                                else
                                {
                                        result = 0;
                                        done = 1;
                                }
                        }
                        else if (key == KBD_ESC)
                        {
                                result = 0;
                                done = 1;
                        }
                        else if (key == KBD_F1)
                        {
                                result = 255;
                                done = 1;
                        }
                }
        }
        while (done == 0);
        conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
        gotoxy(oldx, oldy);
        return((unsigned char) result);
}


/* help:
   Prototype:
   unsigned char conioxlib_help(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char *name, const char *topic,
       char buffer[][CONIOXLIB_HELP_W]);
   Shows one topic of a help file.
   Parameters: name is the help file, topic the topic to
   find (its header line is @topic) and buffer the work area
   for the text. Returns 1 when the topic was found and 0
   when it was not or the file is missing. Lines are cut at
   60 characters and the next @ line ends the entry. The
   Topics are indexed once (offset cache); the body is loaded with
   fseek. The cache is rebuilt when the file size changes.
   */
unsigned char conioxlib_help (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char *name, const char *topic, char buffer[][CONIOXLIB_HELP_W])
{
        unsigned int last;
        int idx;
        char title[80], tmp[256];
        char curtopic[32];
        char prevtopic[32];
        char linktarget[32];
        unsigned char vres;
        int have_prev;

        _conioxlib_hint("Loading help...");
        strncpy(title, (topic != 0) ? topic : "", sizeof(title) - 1);
        title[sizeof(title) - 1] = 0;
        strncpy(curtopic, (topic != 0) ? topic : "", sizeof(curtopic) - 1);
        curtopic[sizeof(curtopic) - 1] = 0;
        prevtopic[0] = 0;
        have_prev = 0;

        /* Build or reuse the topic offset table (invalidated when the file size changes). */
        if (!_conioxlib_helpcache_ensure(name))
        {
                strcpy(&buffer[0][0], "Help is not available.");
                /* TV help palette: black-on-cyan so the not-available message
                   still looks like a help window, not a generic dialog. */
                conioxlib_view(x1, y1, x2, y2, CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), buffer, 1, "Help: Not available");
                return(0);
        }
help_reload:
        idx = _conioxlib_helpcache_find(curtopic);
        if (idx < 0)
        {
                sprintf(&buffer[0][0], "No help for '%.40s'", curtopic);
                conioxlib_view(x1, y1, x2, y2, CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), buffer, 1, "Help: Nothing");
                return(0);
        }
        /* fseek to the cached body offset and read lines until the next @topic */
        if (!_conioxlib_helpcache_load_body(name, idx, buffer, &last))
        {
                strcpy(&buffer[0][0], "Help is not available.");
                conioxlib_view(x1, y1, x2, y2, CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), buffer, 1, "Help: Not available");
                return(0);
        }
        /* Navigation chrome (no history stack): always offer Index; if we arrived
           via a hyperlink, also offer one-level Back. CP437 0x11 = solid left
           triangle; 0xC4 rule under the bar. Reserved link target "__index__"
           opens the interactive topic list (not the @Index prose topic). */
        if (last < 253)
        {
                if (last > 0) memmove(buffer[2], buffer[0], (size_t) last * CONIOXLIB_HELP_W);
                if (have_prev && (prevtopic[0] != 0))
                {
                        /* buffer[0] holds CONIOXLIB_HELP_W (61) bytes; the literal
                           "\x11 Back (@...)   Index (@__index__)" takes 32 + NUL,
                           so cap prevtopic at 28 chars to stay inside the buffer */
                        sprintf(buffer[0],
                                "\x11 Back (@%.28s)   Index (@__index__)",
                                prevtopic);
                }
                else
                {
                        sprintf(buffer[0], "Index (@__index__)");
                }
                {
                        buffer[1][0] = ' ';
                        buffer[1][1] = ' ';
                        memset(&buffer[1][2], 0xC4, 56);
                        buffer[1][58] = 0;
                }
                last += 2;
        }
        sprintf(tmp, "Help: %.60s", curtopic);
        /* TAB/RIGHT next link, SHIFT-TAB/LEFT previous, ENTER/click follows. */
        vres = _conioxlib_help_view(x1, y1, x2, y2, atr0, atr1, buffer, (unsigned char) last, tmp, linktarget);
        if (vres == 2)
        {
                /* Synthetic Index: jump to the topic list, discard one-level Back. */
                if (strcmp(linktarget, "__index__") == 0)
                {
                        conioxlib_helpindex(x1, y1, atr1, _conioxlib_curtheme.atr1, name, (char (*)[32]) buffer);
                        return(1);
                }
                if (_conioxlib_helpcache_find(linktarget) >= 0)
                {
                        strncpy(prevtopic, curtopic, sizeof(prevtopic) - 1);
                        prevtopic[sizeof(prevtopic) - 1] = 0;
                        have_prev = 1;
                        strncpy(curtopic, linktarget, sizeof(curtopic) - 1);
                        curtopic[sizeof(curtopic) - 1] = 0;
                }
                else
                {
                        putch('\a');
                }
                goto help_reload;
        }
        if (vres == 255)
        {
                /* F1 also opens the index, no history kept */
                conioxlib_helpindex(x1, y1, atr1, _conioxlib_curtheme.atr1, name, (char (*)[32]) buffer);
        }
        return(1);
}


#if ((__DOS__) || (__MSDOS__)) && defined(__WATCOMC__)
/* ------------------ Long file names (DOS 7 / Windows 9x) ---------------------- */
/* Int 21h AX=7160h CL=2: true name in its long form. path is a short path such as C:\DIR\REPORT~1.TXT
   and the last component of the answer (the long name of the file) is copied to out, outsize bytes at
   most. Returns 0 on success, 0x7100 when the services do not exist (DOS 6 and older, or LFN
   disabled) and the DOS error code otherwise. The lists show the long name; the callers always get
   the short one back */
static unsigned int _conioxlib_lfnlong (const char *path, char *out, unsigned int outsize)
{
        unsigned char raw[262];
        unsigned int res;
        int carry;
        char *p;
#if defined(__FLAT__)
        struct _conioxlib_rmregs rr;
#else
        union REGS r;
        struct SREGS s;
#endif

        if (outsize == 0)
        {
                return(1);
        }
#if defined(__FLAT__)
        memset(&rr, 0, sizeof(rr));
        rr.eax = 0x7160;
        rr.ecx = 2;
        rr.esi = 0;
        rr.edi = 128;
        /* Real mode block of 32 paragraphs: the path at offset 0 and the answer at offset 128 */
        if (!_conioxlib_rmint21(&rr, 32, path, (unsigned int) strlen(path) + 1, raw, 128, sizeof(raw)))
        {
                return(0x7100);
        }
        res = (unsigned int) (rr.eax & 0xFFFF);
        carry = ((rr.flags & 1) != 0);
#else
        memset(&r, 0, sizeof(r));
        memset(&s, 0, sizeof(s));
        memset(raw, 0, sizeof(raw));
        _conioxlib_rax(r) = 0x7160;
        _conioxlib_rcx(r) = 2;
        s.ds = FP_SEG(path);
        r.w.si = FP_OFF(path);
        s.es = FP_SEG(raw);
        _conioxlib_rdi(r) = FP_OFF(raw);
        _coniox_int86x(0x21, &r, &r, &s);
        res = _conioxlib_rax(r);
        carry = (_conioxlib_rcflag(r) != 0);
#endif
        /* DOS 6 answers AX=7100h (with or without carry) to a function it does not know */
        if (res == 0x7100)
        {
                return(0x7100);
        }
        if (carry)
        {
                return((res != 0) ? res : 1);
        }
        raw[sizeof(raw) - 1] = 0;
        p = strrchr((char *) raw, '\\');
        p = (p != 0) ? p + 1 : (char *) raw;
        strncpy(out, p, outsize - 1);
        out[outsize - 1] = 0;
        return(0);
}
#endif


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

/* Sort files[0..last] with matching lnames[] (insertion sort).
   column is CONIOXLIB_SORT_NAME/SIZE/DATE/TIME; desc is nonzero for descending. */
static void _conioxlib_filemultiselector_sort_table(struct conioxlib_sfiles *files, char lnames[][CONIOXLIB_LISTITEM_W], unsigned int last, unsigned char column, int desc)
{
        unsigned int i;
        int j;
        struct conioxlib_sfiles tmpf;
        char tmpl[CONIOXLIB_LISTITEM_W];

        for (i = 1; i <= last; i++)
        {
                memcpy(&tmpf, &files[i], sizeof(tmpf));
                memcpy(tmpl, lnames[i], CONIOXLIB_LISTITEM_W);
                for (j = (int) i - 1; (j >= 0) && (_conioxlib_filemultiselector_sort(&files[j], lnames[j], &tmpf, tmpl, column, desc) > 0); j--)
                {
                        memcpy(&files[j + 1], &files[j], sizeof(tmpf));
                        memcpy(lnames[j + 1], lnames[j], CONIOXLIB_LISTITEM_W);
                }
                memcpy(&files[j + 1], &tmpf, sizeof(tmpf));
                memcpy(lnames[j + 1], tmpl, CONIOXLIB_LISTITEM_W);
        }
}

/* Name column 12 or 29 when any long name is present; comment width follows. */

/* Persistent sort state for filemultiselector: survives directory rescans
   so the user's chosen column/direction is kept when navigating into a
   subdirectory or switching drives. Declared BEFORE _conioxlib_filemultiselector_paint_header
   because that helper reads these to draw the sort arrow. */
static unsigned char _conioxlib_filemultiselector_sort_column = CONIOXLIB_SORT_NAME;
static int _conioxlib_filemultiselector_sort_desc = 0;

static void _conioxlib_filemultiselector_calc_cols(const char lnames[][CONIOXLIB_LISTITEM_W], unsigned int last, int *nw, int *cw)
{
        unsigned int i, j;
        *nw = 12;
        for (i = 0; i <= last; i++)
        {
                for (j = 0; j <= 12; j++)
                {
                        if (lnames[i][j] == 0)
                                break;
                }
                if (j > 12)
                {
                        *nw = 29;
                        break;
                }
        }
        *cw = 45 - *nw;
}

/* Paint the filemultiselector column header row (row 3) with column
   separators, labels and the sort direction arrow. Extracted from 3
   duplicated blocks that were ~25 lines each. */
static void _conioxlib_filemultiselector_paint_header(int nw, int cw)
{
        char header[80];
        memset(header, 32, 78);
        header[0] = '\xBA';
        header[nw + 1] = '\xBA';
        header[nw + 16] = '\xBA';
        header[nw + 25] = '\xBA';
        header[nw + 31] = '\xBA';
        header[77] = '\xBA';
        header[78] = 0;
        memcpy(&header[1 + ((nw - 4) >> 1)], "Name", 4);
        memcpy(&header[nw + 7], "Size", 4);
        memcpy(&header[nw + 19], "Date", 4);
        memcpy(&header[nw + 26], "Time", 4);
        memcpy(&header[nw + 32 + ((cw - 7) >> 1)], "Comment", 7);
        {
                char arrow = _conioxlib_filemultiselector_sort_desc ? '\x1F' : '\x1E';
                switch (_conioxlib_filemultiselector_sort_column)
                {
                        case CONIOXLIB_SORT_NAME:  header[1 + ((nw - 4) >> 1) + 4] = arrow; break;
                        case CONIOXLIB_SORT_SIZE:  header[nw + 7 + 4] = arrow; break;
                        case CONIOXLIB_SORT_DATE: header[nw + 19 + 4] = arrow; break;
                        case CONIOXLIB_SORT_TIME: header[nw + 26 + 4] = arrow; break;
                }
        }
        {
                unsigned short *cells;
                unsigned short ca;
                unsigned int hi;
                cells = _conioxlib_need_cells(78);
                if (cells != 0)
                {
                        ca = (unsigned short) ((unsigned int) _conioxlib_curtheme.atrdialog << 8);
                        for (hi = 0; hi < 78; hi++)
                                cells[hi] = (unsigned short) (ca | (unsigned char) header[hi]);
                        puttext(1, 3, 78, 3, cells);
                }
        }
}

/* filemultiselector stages (helpers):
   - _conioxlib_enum_drives
   - directory scan (select_1: findfirst/next + DESCRIPT.ION)  [scan_dir]
   - _conioxlib_filemultiselector_sort_table / _conioxlib_filemultiselector_calc_cols
   - interactive UI loop (keys, marks, navigation)             [run_ui]
   - _conioxlib_filemultiselector_row / format / sort compare
   Full platform scan stays in this function (DOS/Win ifdefs); the pure
   post-scan work is factored above for testability. */
/* Sort state declared above, before _conioxlib_filemultiselector_paint_header */
/* Paint all 18 visible rows of the filemultiselector list. Extracted from 4
   duplicated blocks (initial paint, click-sort repaint, Alt+letter repaint,
   PGUP/PGDN repaint). Each block was 12 lines; now each is a single call. */
static void _conioxlib_filemultiselector_paint_rows(int last, int selrow, int nw, int cw,
        const char lnames[][CONIOXLIB_LISTITEM_W], const struct conioxlib_sfiles *files)
{
        int i;
        for (i = 0; i < 18; i++)
        {
                if (i <= last)
                {
                        _conioxlib_filemultiselector_row((unsigned short) (5 + i), i,
                                (i == selrow) ? _conioxlib_curtheme.atr1 :
                                (files[i].selected ? _conioxlib_curtheme.atr0 : _conioxlib_curtheme.atrdialog),
                                nw, cw, lnames, files);
                }
                else
                {
                        _conioxlib_filemultiselector_row((unsigned short) (5 + i), -1, _conioxlib_curtheme.atrdialog, nw, cw, lnames, files);
                }
        }
}


unsigned char conioxlib_filemultiselector (unsigned char *sdisk, char *sdir, struct conioxlib_sfiles *files)
{
        int i, j, row, click;
        unsigned char disk, olddrive;
        int ndisks;
        unsigned char oldx, oldy;
        unsigned int last, key;
        int done;
#if ((__DOS__) || (__MSDOS__))
        struct
        {
                /* unsigned short guarantees 16-bit storage in both 16-bit and 32-bit
                   builds, matching the layout of the DOS directory entry that the
                   memcpy(&ff_ftime, &_conioxlib_fftime(ffblk), 2) reads */
                unsigned short sec: 5;
                unsigned short min: 6;
                unsigned short hour: 5;
        } ff_ftime;
        struct
        {
                unsigned short day: 5;
                unsigned short mon: 4;
                unsigned short year: 7;
        } ff_fdate;
        _conioxlib_ffblk ffblk;
#endif
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        WIN32_FIND_DATA ffd;
        FILETIME ft;
        SYSTEMTIME st;
        void *h;
#endif
        char *descbuf;
        char *dend;
        char *dline;
        unsigned long descsize;
        char *oldb;
        char *s;
        char mask[15];
        /* Near-stack must stay small on 16-bit DOS. The previous layout put
           ~1.8 KB of locals here (128+128+80+260 path temps + nested 26*13+26*34
           drive tables). That is enough to smash a default Turbo C / Watcom 16-bit
           stack: the list can still paint, then the key loop never sees input
           because the return address / near data is corrupted. DOS32 and Win32
           use a flat stack and never showed the hang. Drive tables live in far
           statics; path temps are modest. */
        char tmp1[96], tmp2[96], dir[80];
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        char olddir[260];
#else
        char olddir[80];
#endif
        /* Long names of the entries (what the list shows) in the same order as the table; the table keeps the short ones */
        static char CONIOXLIB_FARDATA lnames[CONIOXLIB_MAXFILES][CONIOXLIB_LISTITEM_W];
        int nw, cw;
        unsigned char nd;
#if ((__DOS__) || (__MSDOS__)) && defined(__WATCOMC__)
        int lfn;
        unsigned int lfnr;
#endif

        _setcursortype(_NOCURSOR);
        /* Cache scr_w/scr_h once: each call hits the BIOS video state (DOS) or
           the console info (Windows). The old code called _conioxlib_scr_h() three
           times in the same expression */
        {
                unsigned short fmsw, fmsh;
                _conioxlib_scr_size(&fmsw, &fmsh);
                oldb = conioxlib_savescr(1, 2, fmsw, (unsigned short)(fmsh > 1 ? fmsh - 1 : 1));
        }
        if (oldb == 0)
        {
                return(0);
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        /* Enumerate drives into far static scratch (shared with fileselector) */
        nd = _conioxlib_enum_drives(_conioxlib_drivenames, _conioxlib_drivecomments, 26);
        for (i = 0; i < (int) nd; i++)
        {
                strcpy(files[i].name, _conioxlib_drivenames[i]);
                strcpy(files[i].comment, _conioxlib_drivecomments[i]);
                strcpy(files[i].size, "\xF0   DRIVE   \xF0");
                files[i].date[0] = 0;
                files[i].time[0] = 0;
                files[i].selected = 0;
        }
        olddrive = _conioxlib_getdisk();
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
        if (_conioxlib_getcwd(olddir, (int) sizeof(olddir) - 1) == 0)
        {
                olddir[0] = '\\';
                olddir[1] = 0;
        }
#else
        getcurdir(0, tmp2);
        sprintf(olddir, "\\%s", tmp2);
#endif
        ndisks = i - 1;
        textattr(_conioxlib_curtheme.atrdialog);
        strcpy(mask, "*.*");
        for (j = 0; j <= (int) ndisks; j++)
        {
                strcpy(lnames[j], files[j].name);
        }
#if ((__DOS__) || (__MSDOS__)) && defined(__WATCOMC__)
        lfn = 1;
#endif
        select_1:
        _conioxlib_hint("ESC close; ENTER accept; SPACE/INS mark; Alt+N/S/D/T sort; click header to sort");
        for (i = 0; i != CONIOXLIB_MAXFILES; i++)
        {
                files[i].selected = 0;
        }
        disk = _conioxlib_build_curdir(dir);
        /* Ensure we are on the selected drive/dir so relative find and DESCRIPT.ION work on DOS and Windows */
        _conioxlib_setdisk(disk - 'A');
        _conioxlib_chdir(dir);
        /* DESCRIPT.ION is loaded once into memory: the old code reopened and
           rescanned the file for every single enumerated entry (rewind plus
           a full fgets pass per file), which is quadratic on big directories */
        sprintf(tmp2, "%c:%.*sDESCRIPT.ION", disk, 70, dir);
        descbuf = _conioxlib_filemultiselector_readblock(tmp2, &descsize);
        if (descbuf == 0)
        {
                /* Also try lowercase name used by some tools */
                sprintf(tmp2, "%c:%.*sdescript.ion", disk, 70, dir);
                descbuf = _conioxlib_filemultiselector_readblock(tmp2, &descsize);
        }
        if (descbuf != 0)
        {
                dend = descbuf + descsize;
                /* Split the lines once: every CR and LF becomes a terminator, so each entry walks the
                   block without rewriting it (the old in place splitting broke the second pass over
                   files with LF only line ends) */
                for (dline = descbuf; dline < dend; dline++)
                {
                        if ((*dline == '\r') || (*dline == '\n'))
                        {
                                *dline = 0;
                        }
                }
        }
        i = ndisks + 1;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        /* FindFirstFileA("*.*") already returns "." and ".." in the enumeration: they must not
           be added by hand (manual insertion duplicated them in the list) */
        h = (void *) FindFirstFileA(mask, &ffd);
        done = (h == INVALID_HANDLE_VALUE);
#else
        /* Relative mask after chdir is more reliable on DOS (Watcom/Borland/DJGPP) */
        done = _conioxlib_findfirst(mask, &ffblk);
#endif
        while (!done)
        {
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                memset(files[i].name, 0, 13);
                /* The table keeps the 8.3 alias (it is what the caller gets back and can open); the list shows the long name */
                strncpy(files[i].name, (ffd.cAlternateFileName[0] != 0) ? ffd.cAlternateFileName : ffd.cFileName, 12);
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
                memcpy(files[i].name, _conioxlib_ffname(ffblk), 12);
                files[i].name[12] = 0;
                if (_conioxlib_ffattrib(ffblk) & 0x10)
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
                        _conioxlib_filemultiselector_format(files[i].size, (ffd.nFileSizeHigh != 0) ? 0xFFFFFFFFUL : (unsigned long int) ffd.nFileSizeLow);
#else
                        _conioxlib_filemultiselector_format(files[i].size, _conioxlib_ffsize(ffblk));
#endif
                        strlwr(files[i].name);
                }
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                strncpy(lnames[i], ffd.cFileName, 40);
                lnames[i][40] = 0;
#else
                strcpy(lnames[i], files[i].name);
#if defined(__WATCOMC__)
                if (lfn && (strcmp(files[i].name, ".") != 0) && (strcmp(files[i].name, "..") != 0))
                {
                        sprintf(tmp1, "%c:%.*s%.*s", disk, 70, dir, 12, files[i].name);
                        lfnr = _conioxlib_lfnlong(tmp1, lnames[i], 41);
                        if (lfnr == 0x7100)
                        {
                                lfn = 0;
                        }
                }
#endif
#endif
                /* Classic case already applied to the short name (dirs upper / files lower).
                   If the long name is the same ignoring case, force that classic case onto the display. */
                if (stricmp(lnames[i], files[i].name) == 0)
                {
                        strcpy(lnames[i], files[i].name);
                }
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                FileTimeToLocalFileTime(&ffd.ftLastWriteTime, &ft);
                FileTimeToSystemTime(&ft, &st);
                _conioxlib_fmt_date(files[i].date, st.wDay, st.wMonth, st.wYear % 100);
                _conioxlib_fmt_time(files[i].time, st.wHour, st.wMinute);
#else
                memcpy(&ff_fdate, &_conioxlib_ffdate(ffblk), 2);
                _conioxlib_fmt_date(files[i].date, ff_fdate.day, ff_fdate.mon, _conioxlib_dos_yy(ff_fdate.year));
                memcpy(&ff_ftime, &_conioxlib_fftime(ffblk), 2);
                _conioxlib_fmt_time(files[i].time, ff_ftime.hour, ff_ftime.min);
#endif
                if (descbuf != 0)
                {
                        /* 4DOS / Take Command DESCRIPT.ION: "name" desc or name desc
                           (case-insensitive). Lines come from the in-memory copy and
                           the split loop above already turned each CR/LF into a terminator. */
                        char *dp;
                        char *nm;
                        unsigned int nlen;

                        for (dp = descbuf; dp < dend; dp += strlen(dp) + 1)
                        {
                                nm = dp;
                                while (*nm == ' ' || *nm == '\t')
                                {
                                        nm++;
                                }
                                if (*nm == 0 || *nm == ';')
                                {
                                        continue;
                                }
                                if (*nm == '"')
                                {
                                        nm++;
                                        s = strchr(nm, '"');
                                        if (s == 0)
                                        {
                                                continue;
                                        }
                                        nlen = (unsigned int) (s - nm);
                                        s++;
                                }
                                else
                                {
                                        nlen = 0;
                                        while (nm[nlen] && (nm[nlen] != ' ') && (nm[nlen] != '\t'))
                                        {
                                                nlen++;
                                        }
                                        s = nm + nlen;
                                }
                                while (*s == ' ' || *s == '\t')
                                {
                                        s++;
                                }
                                /* Bound the name copy to tmp1, like the old fgets(79) parser */
                                if (nlen > sizeof(tmp1) - 1)
                                {
                                        nlen = (unsigned int) (sizeof(tmp1) - 1);
                                }
                                memcpy(tmp1, nm, nlen);
                                tmp1[nlen] = 0;
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
                done = _conioxlib_findnext(&ffblk);
#endif
                i++;
                if (i >= CONIOXLIB_MAXFILES)
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
        if (descbuf != 0)
        {
                free(descbuf);
                descbuf = 0;
        }
        /* Guard against the theoretical case where no drives were enumerated
           (ndisks == -1 leaves i == 0): the unsigned last = i - 1 would
           underflow to UINT_MAX and the sort/scan loops would walk off the
           end of files[] */
        if (i == 0)
        {
                i = 1;
        }
        last = i - 1;
        for (i = last + 1; i != CONIOXLIB_MAXFILES; i++)
        {
                files[i].selected = 0;
                files[i].name[0] = 0;
                files[i].size[0] = 0;
                files[i].date[0] = 0;
                files[i].time[0] = 0;
                files[i].comment[0] = 0;
                lnames[i][0] = 0;
        }
        /* Sort + column widths (helpers keep the UI loop readable).
           Use the persistent sort state so column/direction survives rescans. */
        _conioxlib_filemultiselector_sort_table(files, lnames, last, _conioxlib_filemultiselector_sort_column, _conioxlib_filemultiselector_sort_desc);
        _conioxlib_filemultiselector_calc_cols(lnames, last, &nw, &cw);
        box(2, 2, 78, 23, 0, A_NONE, "", 5);
        textattr(_conioxlib_curtheme.atrdialog);
        memset(tmp1, 0xCD, 77);
        tmp1[0] = '\xC9';
        tmp1[77] = '\xBB';
        tmp1[78] = 0;
        gotoxy (1, 2);
        cputs(tmp1);
        textattr(_conioxlib_curtheme.atrdialog);
        sprintf(tmp1, "[%c:%.*s]", disk, 90, dir);
        cputsc(1, 78, 2, tmp1);
        gotoxy(3, 2);
        cputs("[\xFE]");
        _conioxlib_filemultiselector_paint_header(nw, cw);
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
                tmp1[nw + 1] = '\xBA';
                tmp1[nw + 16] = '\xBA';
                tmp1[nw + 25] = '\xBA';
                tmp1[nw + 31] = '\xBA';
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
        _conioxlib_filemultiselector_paint_rows((int) last, 0, nw, cw, lnames, files);
        i = 0;
        j = 0;
        do
        {
                do
                {
                        click = 0;
                        key = _conioxlib_pollkey();
                        /* Left button: close, scroll arrows, or pick a row (like listbox) */
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
                                else if ((conioxlib_mousestatus.y == 3) && (conioxlib_mousestatus.x > 1) && (conioxlib_mousestatus.x < 77))
                                {
                                        /* Click on a column header: determine which column was
                                           clicked and toggle sort direction (or switch column) */
                                        int mx = (int) conioxlib_mousestatus.x - 1;
                                        unsigned char new_col;
                                        /* Column boundaries (0-based): 0..nw-1=Name, nw+1..nw+14=Size,
                                           nw+16..nw+23=Date, nw+25..nw+29=Time, nw+31..=Comment */
                                        if (mx <= nw)
                                                new_col = CONIOXLIB_SORT_NAME;
                                        else if (mx <= nw + 15)
                                                new_col = CONIOXLIB_SORT_SIZE;
                                        else if (mx <= nw + 24)
                                                new_col = CONIOXLIB_SORT_DATE;
                                        else if (mx <= nw + 30)
                                                new_col = CONIOXLIB_SORT_TIME;
                                        else
                                                new_col = CONIOXLIB_SORT_NAME; /* Comment column: ignore */
                                        if (new_col == _conioxlib_filemultiselector_sort_column)
                                        {
                                                /* Same column: toggle direction */
                                                _conioxlib_filemultiselector_sort_desc = !_conioxlib_filemultiselector_sort_desc;
                                        }
                                        else
                                        {
                                                /* New column: switch and default to ascending */
                                                _conioxlib_filemultiselector_sort_column = new_col;
                                                _conioxlib_filemultiselector_sort_desc = 0;
                                        }
                                        /* Re-sort immediately without rescanning the directory */
                                        _conioxlib_filemultiselector_sort_table(files, lnames, last, _conioxlib_filemultiselector_sort_column, _conioxlib_filemultiselector_sort_desc);
                                        /* Repaint all rows */
_conioxlib_filemultiselector_paint_rows((int) last, -1, nw, cw, lnames, files);
                                        _conioxlib_filemultiselector_paint_header(nw, cw);
                                        /* Reset cursor to top of the list */
                                        i = 0;
                                        j = 0;
                                        _conioxlib_filemultiselector_row(5, 0, _conioxlib_curtheme.atr1, nw, cw, lnames, files);
                                        key = 0;
                                }
                                else if ((conioxlib_mousestatus.x > 1) && (conioxlib_mousestatus.x < 77) && (conioxlib_mousestatus.y > 4) && (conioxlib_mousestatus.y < 23))
                                {
                                        row = (int) conioxlib_mousestatus.y - 5;
                                        /* Clicks on the empty rows below the last entry are ignored */
                                        if ((j - i + row) <= (int) last)
                                        {
                                                /* Repaint the row that loses the cursor, then move the cursor to the clicked row */
                                                _conioxlib_filemultiselector_row((unsigned short) (5 + i), j,
                                                        files[j].selected ? _conioxlib_curtheme.atr0 : _conioxlib_curtheme.atrdialog,
                                                        nw, cw, lnames, files);
                                                j = j - i + row;
                                                i = row;
                                                click = 1;
                                                key = (conioxlib_mousestatus.lclick == 2) ? KBD_ENTER : KBD_SPACE;
                                        }
                                }
                        }
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
                        }

                }
                while ((key != KBD_DOWN) && (key != KBD_UP) && (key != KBD_PGDW) && (key != KBD_PGUP) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_INS) && (key != KBD_SPACE) && (key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F4) && (key != KBD_F3) && (key != KBD_F2) && (key != KBD_F1) && !((key & 0xFF) == 0 && ((key >> 8) == 0x31 || (key >> 8) == 0x1F || (key >> 8) == 0x20 || (key >> 8) == 0x14)));
                _conioxlib_filemultiselector_row((unsigned short) (5 + i), j,
                        files[j].selected ? _conioxlib_curtheme.atr0 : _conioxlib_curtheme.atrdialog,
                        nw, cw, lnames, files);

                if (key == KBD_ENTER)
                {
                        if (strncmp(files[j].size, "\xF0   DRIVE   \xF0", 15) == 0)
                        {
                                _conioxlib_setdisk(files[j].name[1] - 'A');
                                goto select_1;
                        }
                        else if (strncmp(files[j].size, "\xF0 DIRECTORY \xF0", 15) == 0)
                        {
                                _conioxlib_chdir(files[j].name);
                                goto select_1;
                        }
                }
                else if (key == KBD_F2)
                {
                        box (23, 12, 56, 14, _conioxlib_curtheme.atrframe, _conioxlib_curtheme.frame, "Mask", 1);
                        textattr(_conioxlib_curtheme.atrdialog);
                        gotoxy(24, 13);
                        cputs("Enter file mask:");
                        conioxlib_getstr(42, 13, _conioxlib_curtheme.atrinput, mask, 13);
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
                        /* Signed comparison: with last < 17 an unsigned test would underflow and push j past the end of the table */
                        if ((j - i + 17) <= (int) last)
                        {
                                j += 17 - i;
                        }
                }
                else if (key == KBD_PGUP)
                {
                        /* The page above starts 17 rows before the first visible row */
                        j -= 17 + i;
                        if (j < 0)
                        {
                                j = 0;
                        }
                }
                else if (key == KBD_ORG)
                {
                        j = 0;
                }
                else if (key == KBD_END)
                {
                        j = (int) last - 17;
                        if (j < 0)
                        {
                                j = 0;
                        }
                }
else if ((key & 0xFF) == 0)
                {
                        /* Alt+letter accelerators for column sorting.
                           Alt+N=scan 0x31, Alt+S=scan 0x1F, Alt+D=scan 0x20, Alt+T=scan 0x14 */
                        unsigned char scan = (key >> 8) & 0xFF;
                        unsigned char new_col = 0xFF;
                        switch (scan)
                        {
                                case 0x31: new_col = CONIOXLIB_SORT_NAME; break;
                                case 0x1F: new_col = CONIOXLIB_SORT_SIZE; break;
                                case 0x20: new_col = CONIOXLIB_SORT_DATE; break;
                                case 0x14: new_col = CONIOXLIB_SORT_TIME; break;
                        }
                        if (new_col != 0xFF)
                        {
                                if (new_col == _conioxlib_filemultiselector_sort_column)
                                {
                                        _conioxlib_filemultiselector_sort_desc = !_conioxlib_filemultiselector_sort_desc;
                                }
                                else
                                {
                                        _conioxlib_filemultiselector_sort_column = new_col;
                                        _conioxlib_filemultiselector_sort_desc = 0;
                                }
                                _conioxlib_filemultiselector_sort_table(files, lnames, last, _conioxlib_filemultiselector_sort_column, _conioxlib_filemultiselector_sort_desc);
                                _conioxlib_filemultiselector_paint_header(nw, cw);
                                for (i = 0; i < 18; i++)
                                {
                                        if (i <= (int) last)
                                        {
                                                _conioxlib_filemultiselector_row((unsigned short) (5 + i), i,
                                                        files[i].selected ? _conioxlib_curtheme.atr0 : _conioxlib_curtheme.atrdialog,
                                                        nw, cw, lnames, files);
                                        }
                                        else
                                        {
                                                _conioxlib_filemultiselector_row((unsigned short) (5 + i), -1, _conioxlib_curtheme.atrdialog, nw, cw, lnames, files);
                                        }
                                }
                                i = 0;
                                j = 0;
                                _conioxlib_filemultiselector_row(5, 0, _conioxlib_curtheme.atr1, nw, cw, lnames, files);
                                _conioxlib_hint("Sorted. Alt+N/S/D/T or click header to change column");
                        }
                }

                else if ((key == KBD_SPACE) || (key == KBD_INS))
                {
                        if ((strcmp(files[j].size, "\xF0 DIRECTORY \xF0") != 0) && (strcmp(files[j].size, "\xF0   DRIVE   \xF0") != 0) && (j <= last))
                        {
                                files[j].selected++;
                                files[j].selected = files[j].selected & 1;
                                _conioxlib_filemultiselector_row((unsigned short) (5 + i), j,
                                        files[j].selected ? _conioxlib_curtheme.atr0 : _conioxlib_curtheme.atrdialog,
                                        nw, cw, lnames, files);
                        }
                        /* A mouse click marks the clicked entry and stays on it */
                        if ((j < last) && (click == 0))
                        {
                                i++;
                                j++;
                        }
                }
                /* Keep the absolute index inside the table: the old page keys
                   could walk j past last on short directories */
                if (j < 0)
                {
                        j = 0;
                }
                else if (j > (int) last)
                {
                        j = (int) last;
                }
                if (i < 0)
                {
                        movetext(2, 5, 77, 21, 2, 6);
                        _conioxlib_filemultiselector_row(5, j, _conioxlib_curtheme.atr1, nw, cw, lnames, files);
                        i = 0;
                }
                else if (i > 17)
                {
                        movetext(2, 6, 77, 22, 2, 5);
                        _conioxlib_filemultiselector_row(22, j, _conioxlib_curtheme.atr1, nw, cw, lnames, files);
                        i = 17;
                }
                if ((key == KBD_PGDW) || (key == KBD_PGUP) || (key == KBD_ORG) || (key == KBD_END))
                {
                        for (i = 0; i < 18; i++)
                        {
                                if ((i + j) <= (int) last)
                                {
                                        _conioxlib_filemultiselector_row((unsigned short) (5 + i), i + j,
                                                files[i + j].selected ? _conioxlib_curtheme.atr0 : _conioxlib_curtheme.atrdialog,
                                                nw, cw, lnames, files);
                                }
                                else
                                {
                                        _conioxlib_filemultiselector_row((unsigned short) (5 + i), -1, _conioxlib_curtheme.atrdialog, nw, cw, lnames, files);
                                }
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
                _conioxlib_filemultiselector_row((unsigned short) (5 + i), j, _conioxlib_curtheme.atr1, nw, cw, lnames, files);
        }
        while ((key != KBD_ENTER) && (key != KBD_ESC) && (key != KBD_F1));
        if (key == KBD_ESC)
        {
                for (i = 0; i != CONIOXLIB_MAXFILES; i++)
                {
                        files[i].selected = 0;
                }
        }
        else if (key != KBD_F1)
        {
                /* Sentinel-free: use a plain flag instead of magic 0x0AAA */
                int any_selected = 0;
                for (i = 0; i <= last; i++)
                {
                        if (files[i].selected)
                        {
                                any_selected = 1;
                                break;
                        }
                }
                if (!any_selected)
                {
                        files[j].selected = 1;
                }
        }
        *sdisk = disk;
        /* Keep the trailing backslash of the \sub\ form, the same shape
           conioxlib_fileselector() returns, so callers build X:\sub\name.ext
           paths by plain concatenation */
        strcpy(sdir, dir);
        _conioxlib_setdisk(olddrive);
        _conioxlib_chdir(olddir);
        /* Use the cached scr_w/scr_h pair instead of three calls to _conioxlib_scr_h() */
        {
                unsigned short fmsw = _conioxlib_scr_w();
                unsigned short fmsh = _conioxlib_scr_h();
                conioxlib_restorescr(1, 2, fmsw, (unsigned short)(fmsh > 1 ? fmsh - 1 : 1), oldb);
        }
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


/* -------------------- Enumerate logical drives (shared) -------------------- */
/* Fills names[] with the classic "\x10A\x11" form and, when comments != NULL,
   a short human description. Returns the number of drives found (capped by max).
   Used by both fileselector and filemultiselector so the BIOS / GetLogicalDrives
   logic lives in one place.

   Callers must not put the names/comments tables on the near stack under
   16-bit DOS: 26*13 + 26*34 is already >1 KB and filemultiselector already
   carries path temps. Prefer the far static scratch below. */
#if ((__DOS__) || (__MSDOS__))
/* CMOS read: no motor, no int 13h. Index port 0x70, data port 0x71.
   Reg 0x10 holds floppy types (hi nibble = A:, lo = B:):
     1=360K 5.25, 2=1.2M 5.25, 3=720K 3.5, 4=1.44M 3.5, 5=2.88M 3.5. */
static unsigned char _conioxlib_enum_drives_cmos(unsigned char reg)
{
#if defined(__DJGPP__)
        outportb(0x70, reg);
        return((unsigned char) inportb(0x71));
#elif defined(__WATCOMC__)
        outp(0x70, (int) reg);
        return((unsigned char) inp(0x71));
#else
        /* Turbo C / Borland: outportb/inportb in dos.h */
        outportb(0x70, reg);
        return((unsigned char) inportb(0x71));
#endif
}

static void _conioxlib_enum_drives_floppy_comment(char *out, unsigned char cmos_nibble)
{
        if ((cmos_nibble == 1) || (cmos_nibble == 2))
        {
                strcpy(out, "5\xAC Floppy Disk");       /* 5¼ */
        }
        else if ((cmos_nibble >= 3) && (cmos_nibble <= 5))
        {
                strcpy(out, "3\xAB Floppy Disk");       /* 3½ */
        }
        else
        {
                strcpy(out, "Floppy Disk");
        }
}
#endif

/* Build the "\x10 L \x11" drive label directly without sprintf parsing cost.
   Lives OUTSIDE the DOS-only #if above because _conioxlib_enum_drives uses it
   in BOTH its Windows branch (GetLogicalDrives path) and its DOS branch
   (BIOS equipment word path). Putting it inside the DOS-only block made
   Watcom emit E1035 ("static function has not been defined") on Windows
   builds because the forward declaration at the top of the file referred
   to a function the Windows build never sees. */
static conioxlib_inline void _conioxlib_enum_drives_set_name(char *out, char letter)
{
        out[0] = '\x10';
        out[1] = letter;
        out[2] = '\x11';
        out[3] = 0;
}

static unsigned char _conioxlib_enum_drives(char names[][13], char comments[][34], unsigned char max)
{
        unsigned char i = 0;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        /* Drive enumeration cache. The drive set rarely changes during a process
           (file pickers don't see USB plug-ins mid-pick), and GetLogicalDrives
           + GetDriveTypeA per drive was the bulk of enum_drives' cost. Cache
           bitmap + per-drive type on first call, then cheaply replay. */
        static unsigned int _cached_drmask = 0;
        static unsigned char _cached_types[26];
        static int _cache_valid = 0;
        unsigned int dr;

        if ((names == 0) || (max == 0))
        {
                return(0);
        }
        if (!_cache_valid)
        {
                char root[4];
                unsigned int t;
                root[1] = ':';
                root[2] = '\\';
                root[3] = 0;
                _cached_drmask = GetLogicalDrives();
                for (dr = 0; dr < 26; dr++)
                {
                        if ((_cached_drmask & (1u << dr)) == 0)
                        {
                                _cached_types[dr] = 0xFF; /* sentinel: not present */
                                continue;
                        }
                        root[0] = (char) ('A' + dr);
                        t = (unsigned int) GetDriveTypeA(root);
                        /* Map to small indices, saving the comment string later */
                        switch (t)
                        {
                                case DRIVE_REMOVABLE: _cached_types[dr] = 1; break;
                                case DRIVE_FIXED:     _cached_types[dr] = 2; break;
                                case DRIVE_REMOTE:    _cached_types[dr] = 3; break;
                                case DRIVE_CDROM:     _cached_types[dr] = 4; break;
                                case DRIVE_RAMDISK:   _cached_types[dr] = 5; break;
                                default:              _cached_types[dr] = 6; break;
                        }
                }
                _cache_valid = 1;
        }
        for (dr = 0; (dr < 26) && (i < max); dr++)
        {
                if ((_cached_drmask & (1u << dr)) == 0)
                {
                        continue;
                }
                _conioxlib_enum_drives_set_name(names[i], (char) ('A' + dr));
                if (comments != 0)
                {
                        switch (_cached_types[dr])
                        {
                                case 1:  strcpy(comments[i], "Removable Disk"); break;
                                case 2:  strcpy(comments[i], "Hard Disk"); break;
                                case 3:  strcpy(comments[i], "Network Disk"); break;
                                case 4:  strcpy(comments[i], "CD-ROM Disk"); break;
                                case 5:  strcpy(comments[i], "Virtual Floppy Disk"); break;
                                default: strcpy(comments[i], "Disk Drive"); break;
                        }
                }
                i++;
        }
#else
        unsigned int key;
        unsigned int j;
        unsigned char cmos_fl;

        if ((names == 0) || (max == 0))
        {
                return(0);
        }
        /* Floppy count from the equipment word (BDA 40:10 bits 6-7). Never use
           int 13h AH=8 here: on real hardware a probe of a BIOS-listed but missing
           floppy spins the motor and can wait forever for drive ready, freezing
           the picker before the list is drawn. 5¼/3½ flavour comes from CMOS 0x10
           instead (port I/O only, no disk activity). */
        cmos_fl = _conioxlib_enum_drives_cmos(0x10);
        key = ((peekb(0x40, 0x10) & 192) >> 6) + 1;
        for (j = 0; (j < key) && (i < max); j++)
        {
                _conioxlib_enum_drives_set_name(names[i], (char) (i + 'A'));
                if (comments != 0)
                {
                        /* Drive 0 = high nibble, drive 1 = low nibble of CMOS 0x10 */
                        _conioxlib_enum_drives_floppy_comment(comments[i],
                                (unsigned char) ((j == 0) ? ((cmos_fl >> 4) & 0x0F) : (cmos_fl & 0x0F)));
                }
                i++;
        }
        if ((key == 1) && (i < max))
        {
                _conioxlib_enum_drives_set_name(names[i], (char) (i + 'A'));
                if (comments != 0)
                {
                        strcpy(comments[i], "Virtual Floppy Disk");
                }
                i++;
        }
        key = peekb(0x40, 0x75);
        /* Never exceed 26 letters even if the BIOS byte is garbage */
        if ((unsigned int) i + key > 26)
        {
                key = (i < 26) ? (unsigned int) (26 - i) : 0;
        }
        for (j = 0; (j < key) && (i < max); j++)
        {
                _conioxlib_enum_drives_set_name(names[i], (char) (i + 'A'));
                if (comments != 0)
                {
                        strcpy(comments[i], "Hard Disk");
                }
                i++;
        }
#endif
        return(i);
}


/* -------------------- Sorts the select ---------------------------------- */
/* Rank helper: drives (0), "." (1), ".." (2), directories (3) and files (4).
   Inline char-by-char compare avoids strcmp for the short "." and ".." cases. */
static int _conioxlib_filemultiselector_rank(const struct conioxlib_sfiles *f)
{
        const char *n = f->name;
        if (n[0] == '\x10') return 0;
        if (n[0] == '.')
        {
                if (n[1] == 0) return 1;
                if (n[1] == '.' && n[2] == 0) return 2;
        }
        return ((unsigned char) f->size[0] == 0xF0) ? 3 : 4;
}

/* Parse a size string like "1,234" into unsigned long for numeric comparison.
   The size field in conioxlib_sfiles is a char[15] formatted by
   _conioxlib_filemultiselector_format(); drives/dirs carry the \xF0 marker. */
static unsigned long _conioxlib_filemultiselector_parsesize(const char *s)
{
        unsigned long n = 0;
        /* Skip the \xF0 marker entries (drives/dirs) - they sort by rank anyway */
        if ((unsigned char) s[0] == 0xF0) return 0;
        while (*s != 0)
        {
                if (*s >= '0' && *s <= '9')
                {
                        n = n * 10UL + (unsigned long) (*s - '0');
                }
                s++;
        }
        return n;
}

/* Parse a date string "DD-MM-YY" into a comparable value YYMMDD */
static unsigned long _conioxlib_filemultiselector_parsedate(const char *s)
{
        unsigned long d, m, y;
        if (s[0] == 0 || s[0] == ' ') return 0;
        /* Format from sprintf: "%02u-%02u-%02u" or "%02u-%02u-XX" */
        d = (s[0] - '0') * 10UL + (s[1] - '0');
        m = (s[3] - '0') * 10UL + (s[4] - '0');
        if (s[6] == 'X' || s[7] == 'X')
                y = 0;
        else
                y = (s[6] - '0') * 10UL + (s[7] - '0');
        return y * 10000UL + m * 100UL + d;
}

/* Parse a time string "HH:MM" into a comparable value HHMM */
static unsigned long _conioxlib_filemultiselector_parsetime(const char *s)
{
        unsigned long h, m;
        if (s[0] == 0 || s[0] == ' ') return 0;
        h = (s[0] - '0') * 10UL + (s[1] - '0');
        m = (s[3] - '0') * 10UL + (s[4] - '0');
        return h * 100UL + m;
}

/* Sort comparator: column is CONIOXLIB_SORT_NAME/SIZE/DATE/TIME;
   desc is nonzero for descending order. Drives, "." and ".." always sort
   to the top regardless of column/direction. */
static int _conioxlib_filemultiselector_sort (const struct conioxlib_sfiles *fa, const char *la, const struct conioxlib_sfiles *fb, const char *lb, unsigned char column, int desc)
{
        int ra, rb;
        int cmp;

        ra = _conioxlib_filemultiselector_rank(fa);
        rb = _conioxlib_filemultiselector_rank(fb);
        if (ra != rb)
        {
                return(ra - rb);
        }
        /* Drives/dots/dirs sort by name ascending regardless of column */
        if (ra < 4)
        {
                cmp = stricmp(la, lb);
                return(desc ? -cmp : cmp);
        }
        /* Files: sort by the selected column */
        switch (column & 0x7F)
        {
                case CONIOXLIB_SORT_SIZE:
                {
                        unsigned long sa = _conioxlib_filemultiselector_parsesize(fa->size);
                        unsigned long sb = _conioxlib_filemultiselector_parsesize(fb->size);
                        cmp = (sa < sb) ? -1 : (sa > sb) ? 1 : 0;
                        break;
                }
                case CONIOXLIB_SORT_DATE:
                {
                        unsigned long da = _conioxlib_filemultiselector_parsedate(fa->date);
                        unsigned long db = _conioxlib_filemultiselector_parsedate(fb->date);
                        cmp = (da < db) ? -1 : (da > db) ? 1 : 0;
                        break;
                }
                case CONIOXLIB_SORT_TIME:
                {
                        unsigned long ta = _conioxlib_filemultiselector_parsetime(fa->time);
                        unsigned long tb = _conioxlib_filemultiselector_parsetime(fb->time);
                        cmp = (ta < tb) ? -1 : (ta > tb) ? 1 : 0;
                        break;
                }
                case CONIOXLIB_SORT_NAME:
                default:
                        cmp = stricmp(la, lb);
                        break;
        }
        return(desc ? -cmp : cmp);
}


/* ------------------------- Formats a long with thousand separators ------------------------------- */
static void _conioxlib_filemultiselector_format (char *res, unsigned long int n)
{
        unsigned int i, j, k, len;
        char tmp[16];
        unsigned long v;
        v = n;
        if (v == 0)
        {
                tmp[0] = '0';
                len = 1;
        }
        else
        {
                len = 0;
                while (v != 0 && len < 15)
                {
                        tmp[len++] = (char) ('0' + (v % 10UL));
                        v /= 10UL;
                }
                for (i = 0, j = len - 1; i < j; i++, j--)
                {
                        char c = tmp[i];
                        tmp[i] = tmp[j];
                        tmp[j] = c;
                }
        }
        if (n < 1000)
        {
                for (i = 0; i < len; i++) res[i] = tmp[i];
                res[len] = 0;
                return;
        }
        j = ((len - 1) % 3) + 1;
        k = 0;
        for (i = 0; i < len; i++)
        {
                res[k++] = tmp[i];
                j--;
                if ((j == 0) && ((i + 1) < len))
                {
                        res[k++] = ',';
                        j = 3;
                }
        }
        res[k] = 0;
}


/* Compact size for fileselector rows (fits an 8-char field). */
static void _conioxlib_fileselector_format_size(char *res, unsigned long int n)
{
        unsigned long v;
        char tmp[12];
        int i, j;
        if (n < 10000000UL)
                v = n;
        else if (n < 1000000000UL)
                v = n / 1024UL;
        else
                v = n / 1048576UL;
        if (v == 0)
        {
                tmp[0] = '0';
                i = 1;
        }
        else
        {
                i = 0;
                while (v != 0 && i < 10)
                {
                        tmp[i++] = (char) ('0' + (v % 10UL));
                        v /= 10UL;
                }
        }
        j = 0;
        while (i > 0)
                res[j++] = tmp[--i];
        if (n >= 1000000000UL)
                res[j++] = 'M';
        else if (n >= 10000000UL)
                res[j++] = 'K';
        res[j] = 0;
}

static conioxlib_inline void _conioxlib_fmt_u2(char *dst, unsigned int v)
{
        dst[0] = (char) ('0' + ((v / 10) % 10));
        dst[1] = (char) ('0' + (v % 10));
}

static conioxlib_inline void _conioxlib_fmt_date(char *dest, unsigned int day, unsigned int mon, unsigned int yy)
{
        _conioxlib_fmt_u2(dest, day);
        dest[2] = '-';
        _conioxlib_fmt_u2(dest + 3, mon);
        dest[5] = '-';
        if (yy == 0xFFu)
        {
                dest[6] = 'X';
                dest[7] = 'X';
        }
        else
                _conioxlib_fmt_u2(dest + 6, yy);
        dest[8] = 0;
}

static conioxlib_inline void _conioxlib_fmt_time(char *dest, unsigned int hour, unsigned int min)
{
        _conioxlib_fmt_u2(dest, hour);
        dest[2] = ':';
        _conioxlib_fmt_u2(dest + 3, min);
        dest[5] = 0;
}

static conioxlib_inline unsigned int _conioxlib_dos_yy(unsigned int year)
{
        if (year < 20) return year + 80;
        if (year < 120) return year - 20;
        return 0xFFu;
}

static unsigned char _conioxlib_build_curdir(char *out)
{
        unsigned char disk;
        char tmp[80];
        unsigned int i;
        disk = (unsigned char) (_conioxlib_getdisk() + 'A');
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
        if (_conioxlib_getcwd(tmp, 79) == 0)
        {
                tmp[0] = (char) disk;
                tmp[1] = ':';
                tmp[2] = '\\';
                tmp[3] = 0;
        }
        if (tmp[3] == 0)
        {
                out[0] = '\\';
                out[1] = 0;
        }
        else
        {
                out[0] = '\\';
                i = 0;
                while (tmp[3 + i] != 0 && i < 76)
                {
                        out[1 + i] = tmp[3 + i];
                        i++;
                }
                out[1 + i] = '\\';
                out[2 + i] = 0;
        }
#else
        getcurdir(disk - 'A' + 1, tmp);
        if (tmp[0] == 0)
        {
                out[0] = '\\';
                out[1] = 0;
        }
        else
        {
                out[0] = '\\';
                i = 0;
                while (tmp[i] != 0 && i < 76)
                {
                        out[1 + i] = tmp[i];
                        i++;
                }
                out[1 + i] = '\\';
                out[2 + i] = 0;
        }
#endif
        return disk;
}

/* Paints one filemultiselector row at screen column 2..77 with puttext.
   idx < 0 or an empty name paints a blank row (used for short directories). */
static void _conioxlib_filemultiselector_row(unsigned short y, int idx, unsigned char atr, int nw, int cw, const char lnames[][CONIOXLIB_LISTITEM_W], const struct conioxlib_sfiles *files)
{
        unsigned short *cells;
        unsigned short ca, sep;
        unsigned int pos, j, n;
        const char *p;

        cells = _conioxlib_need_cells(76);
        if (cells == 0)
        {
                return;
        }
        ca = (unsigned short) ((unsigned int) atr << 8);
        sep = (unsigned short) (ca | 0xBA);
        _conioxlib_memsetw(cells, (unsigned short)(ca | ' '), 76);
        if ((idx < 0) || (files[idx].name[0] == 0))
        {
                puttext(2, y, 77, y, cells);
                return;
        }
        /* name[nw] | size[14] | date[8] | time[5] | comment[cw] */
        p = lnames[idx];
        n = (unsigned int) nw;
        for (j = 0; j < n && p[j] != 0; j++)
                cells[j] = (unsigned short) (ca | (unsigned char) p[j]);
        pos = (unsigned int) nw;
        cells[pos++] = sep;
        p = files[idx].size;
        {
                unsigned int sl = 0;
                while (p[sl] != 0) sl++;
                if (sl > 14) sl = 14;
                for (j = 0; j < sl; j++)
                        cells[pos + (14 - sl) + j] = (unsigned short) (ca | (unsigned char) p[j]);
        }
        pos += 14;
        cells[pos++] = sep;
        p = files[idx].date;
        for (j = 0; j < 8 && p[j] != 0; j++)
                cells[pos + j] = (unsigned short) (ca | (unsigned char) p[j]);
        pos += 8;
        cells[pos++] = sep;
        p = files[idx].time;
        for (j = 0; j < 5 && p[j] != 0; j++)
                cells[pos + j] = (unsigned short) (ca | (unsigned char) p[j]);
        pos += 5;
        cells[pos++] = sep;
        p = files[idx].comment;
        n = (unsigned int) cw;
        for (j = 0; j < n && p[j] != 0; j++)
                cells[pos + j] = (unsigned short) (ca | (unsigned char) p[j]);
        puttext(2, y, 77, y, cells);
}


/* ------------------------- Compact file selector ------------------------- */
/* fileselector:
   Prototype:
   unsigned char conioxlib_fileselector(unsigned char *sdisk, char *sdir, char *name);
   Compact single-file picker for Open/Save. File rows show name/size/date
   fitting CONIOXLIB_LISTITEM_W. Returns 1 when a file was chosen and 0 when
   cancelled. The short 8.3 name is returned for portability.
   */
unsigned char conioxlib_fileselector (unsigned char *sdisk, char *sdir, char *name)
{
        static char CONIOXLIB_FARDATA items[CONIOXLIB_MAXFILES][CONIOXLIB_LISTITEM_W];
        static char CONIOXLIB_FARDATA shorts[CONIOXLIB_MAXFILES][CONIOXLIB_LISTITEM_W];
        char lname[CONIOXLIB_LISTITEM_W];
        int i;
        int done;
        unsigned char disk, olddrive, res;
#if ((__DOS__) || (__MSDOS__))
        _conioxlib_ffblk ffblk;
#if defined(__WATCOMC__)
        unsigned int lfnr;
        int lfn;
        char path[128];
#endif
#endif
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        WIN32_FIND_DATA ffd;
        void *h;
#endif
        char tmp[80], dir[80];
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        char olddir[260];
#else
        char olddir[80];
#endif
        char *oldb;
        unsigned char nd;
        unsigned char d;

        _setcursortype(_NOCURSOR);
        oldb = conioxlib_savescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
        if (oldb == 0)
        {
                return(0);
        }
        olddrive = _conioxlib_getdisk();
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__WATCOMC__)
        if (_conioxlib_getcwd(olddir, (int) sizeof(olddir) - 1) == 0)
        {
                olddir[0] = '\\';
                olddir[1] = 0;
        }
#else
        getcurdir(0, tmp);
        sprintf(olddir, "\\%s", tmp);
#endif
        fileselector_1:
        disk = _conioxlib_build_curdir(dir);
        /* Ensure we are on the selected drive and directory so the relative scan works everywhere */
        _conioxlib_setdisk(disk - 'A');
        _conioxlib_chdir(dir);
        /* Enumerate drives into far static scratch (no near-stack table on DOS16) */
        nd = _conioxlib_enum_drives(_conioxlib_drivenames, 0, 26);
        for (d = 0; d < nd; d++)
        {
                strcpy(items[d], _conioxlib_drivenames[d]);
                shorts[d][0] = 0;
        }
        i = (int) nd;
        strcpy(items[i], "..");
        strcpy(shorts[i], "..");
        i++;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        h = (void *) FindFirstFileA("*.*", &ffd);
        done = (h == INVALID_HANDLE_VALUE);
#else
        done = _conioxlib_findfirst("*.*", &ffblk);
#if defined(__WATCOMC__)
        lfn = 1;
#endif
#endif
        while (!done)
        {
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                /* The 8.3 alias is what the caller gets back; the list shows the long name */
                strncpy(tmp, (ffd.cAlternateFileName[0] != 0) ? ffd.cAlternateFileName : ffd.cFileName, sizeof(tmp) - 1);
                tmp[sizeof(tmp) - 1] = 0;
                strncpy(lname, ffd.cFileName, sizeof(lname) - 1);
                lname[sizeof(lname) - 1] = 0;
#else
                strcpy(tmp, _conioxlib_ffname(ffblk));
                strcpy(lname, tmp);
#if defined(__WATCOMC__)
                if (lfn && (strcmp(tmp, ".") != 0) && (strcmp(tmp, "..") != 0))
                {
                        sprintf(path, "%c:%.*s%.*s", disk, 80, dir, 40, tmp);
                        lfnr = _conioxlib_lfnlong(path, lname, sizeof(lname));
                        if (lfnr == 0x7100)
                        {
                                lfn = 0;
                        }
                }
#endif
#endif
                if ((strcmp(tmp, ".") != 0) && (strcmp(tmp, "..") != 0) && (i < CONIOXLIB_MAXFILES))
                {
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
                        if (_conioxlib_ffattrib(ffblk) & 0x10)
#endif
                        {
                                /* Decide classic case once on the short name (dirs upper, files lower).
                                   If the long name is the same ignoring case, force that classic case onto the display. */
                                strupr(tmp);
                                if (stricmp(lname, tmp) == 0)
                                {
                                        strcpy(lname, tmp);
                                }
                                sprintf(items[i], "[%.38s]", lname);
                        }
                        else
                        {
                                strlwr(tmp);
                                if (stricmp(lname, tmp) == 0)
                                {
                                        strcpy(lname, tmp);
                                }
                                /* Multi-column item fitting CONIOXLIB_LISTITEM_W (41 incl. NUL):
                                   name 14 + space + size 8 + space + "DD-MM-YY HH:MM" 14 = 38 chars.
                                   Previous 18+10+16 wrote 46 bytes and overflowed into the next row. */
                                {
                                        char szbuf[12];
                                        char dtbuf[16];
                                        unsigned long fsize;
                                        unsigned int fday, fmon, fyear, fhour, fmin;
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                                        fsize = (unsigned long) ffd.nFileSizeLow;
                                        {
                                                SYSTEMTIME st;
                                                FILETIME ft_l;
                                                FileTimeToLocalFileTime(&ffd.ftLastWriteTime, &ft_l);
                                                if (FileTimeToSystemTime(&ft_l, &st))
                                                {
                                                        fday = st.wDay; fmon = st.wMonth; fyear = st.wYear % 100;
                                                        fhour = st.wHour; fmin = st.wMinute;
                                                }
                                                else
                                                {
                                                        fday = fmon = fyear = fhour = fmin = 0;
                                                }
                                        }
#else
                                        fsize = (unsigned long) _conioxlib_ffsize(ffblk);
                                        {
                                                struct
                                                {
                                                        unsigned short day: 5;
                                                        unsigned short mon: 4;
                                                        unsigned short year: 7;
                                                } fdatebits;
                                                struct
                                                {
                                                        unsigned short sec: 5;
                                                        unsigned short min: 6;
                                                        unsigned short hour: 5;
                                                } ftimebits;
                                                unsigned short raw_date, raw_time;
                                                memcpy(&raw_date, &_conioxlib_ffdate(ffblk), 2);
                                                memcpy(&raw_time, &_conioxlib_fftime(ffblk), 2);
                                                memcpy(&fdatebits, &raw_date, 2);
                                                memcpy(&ftimebits, &raw_time, 2);
                                                fday = (unsigned int) fdatebits.day;
                                                fmon = (unsigned int) fdatebits.mon;
                                                fyear = _conioxlib_dos_yy((unsigned int) fdatebits.year);
                                                fhour = (unsigned int) ftimebits.hour;
                                                fmin = (unsigned int) ftimebits.min;
                                        }
#endif
                                        _conioxlib_fileselector_format_size(szbuf, fsize);
                                        _conioxlib_fmt_date(dtbuf, fday, fmon, fyear);
                                        dtbuf[8] = ' ';
                                        _conioxlib_fmt_time(dtbuf + 9, fhour, fmin);
                                        /* Pad name / size into fixed fields without sprintf. */
                                        {
                                                unsigned int j, n;
                                                char *d = items[i];
                                                for (j = 0; j < 14; j++) d[j] = ' ';
                                                for (j = 0; j < 14 && lname[j]; j++) d[j] = lname[j];
                                                d[14] = ' ';
                                                n = 0;
                                                while (szbuf[n]) n++;
                                                if (n > 8) n = 8;
                                                for (j = 0; j < 8; j++) d[15 + j] = ' ';
                                                for (j = 0; j < n; j++) d[15 + (8 - n) + j] = szbuf[j];
                                                d[23] = ' ';
                                                for (j = 0; j < 14 && dtbuf[j]; j++) d[24 + j] = dtbuf[j];
                                                d[24 + j] = 0;
                                        }
                                }
                        }
                        sprintf(shorts[i], "%.40s", tmp);
                        i++;
                }
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
                done = (FindNextFileA(h, &ffd) == 0);
#else
                done = _conioxlib_findnext(&ffblk);
#endif
        }
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        if (h != INVALID_HANDLE_VALUE)
        {
                FindClose(h);
        }
#endif
        /* sprintf returns the number of chars written; reuse it instead of
           calling strlen twice on the same buffer */
        {
                int plen = sprintf(tmp, "%c:%.*s", disk, 70, dir);
                if (plen > 56)
                {
                        /* Use a separate buffer: sprintf src/dst overlap is undefined behavior
                           and produced truncated paths under Turbo C/Borland */
                        char tmp_b[80];
                        sprintf(tmp_b, "...%s", &tmp[plen - 53]);
                        strcpy(tmp, tmp_b);
                }
        }
        res = conioxlib_listbox(10, 4, 68, 21, _conioxlib_curtheme.atrdialog, _conioxlib_curtheme.atr1, items, (unsigned char) i, tmp);
        if (res == 255)
        {
                _conioxlib_setdisk(olddrive);
                _conioxlib_chdir(olddir);
                conioxlib_restorescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h(), oldb);
                return(0);
        }
        if (items[res][0] == '\x10')
        {
                /* Drive: switch to it and rescan */
                _conioxlib_setdisk(items[res][1] - 'A');
                goto fileselector_1;
        }
        if (strcmp(items[res], "..") == 0)
        {
                /* Directory: go one level up and rescan */
                _conioxlib_chdir("..");
                goto fileselector_1;
        }
        if (items[res][0] == '[')
        {
                /* Directory: enter it (by its short name) and rescan */
                _conioxlib_chdir(shorts[res]);
                goto fileselector_1;
        }
        /* Plain file: accept it; the caller gets the short name back */
        strcpy(name, shorts[res]);
        *sdisk = disk;
        strcpy(sdir, dir);
        _conioxlib_setdisk(olddrive);
        _conioxlib_chdir(olddir);
        conioxlib_restorescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h(), oldb);
        return(1);
}


/* ---------------- Compiler identification string ------------------------- */
/* conioxlib_compilerstring:
   Prototype:
   const char * conioxlib_compilerstring(void);
   Returns a static string with the name and version of the compiler
   used to build the library, taken from the predefined macros, e.g.
   "Open Watcom C/C++ 2.0 beta" for __WATCOMC__ 1300, "Borland C++ 3.10"
   for __BORLANDC__ 0x0310 or "Turbo C/C++ 2.0" for __TURBOC__ 0x0200.
   The splash and the About box show it.
   */
const char * conioxlib_compilerstring (void)
{
        /* Bumped from 48 to 80 bytes: __VERSION__ on GCC/DJGPP can exceed 40 chars
           ("DJGPP GCC " + 40 + NUL = 51) and would overflow the old buffer */
        static char buf[80];
#if defined(__WATCOMC__)
        if (__WATCOMC__ >= 1200)
        {
                /* __WATCOMC__ is vvrr: 1200 = 1.0, 1290 = 1.9, 1300 = 2.0 */
                sprintf(buf, "Open Watcom C/C++ %u.%u%s", (unsigned int) ((__WATCOMC__ / 100) - 11), (unsigned int) ((__WATCOMC__ % 100) / 10), (__WATCOMC__ >= 1300) ? " beta" : "");
        }
        else
        {
                sprintf(buf, "Watcom C/C++ %u.%u", (unsigned int) (__WATCOMC__ / 100), (unsigned int) ((__WATCOMC__ % 100) / 10));
        }
#elif defined(__BORLANDC__)
        /* __BORLANDC__ is the version as hex digit pairs vvrr: 0x0200 = 2.0, 0x0310 = 3.1, 0x0450 = 4.5, 0x0500 = 5.0, 0x0520 = 5.2 (the 5.02 box, also C++Builder 1.0), 0x0550 = C++Builder 5.0 */
        sprintf(buf, "Borland C++ %u.%u%u", (unsigned int) ((__BORLANDC__ >> 8) & 0xFF), (unsigned int) ((__BORLANDC__ >> 4) & 0xF), (unsigned int) (__BORLANDC__ & 0xF));
#elif defined(__TURBOC__) || defined(TURBOC)
        /* TURBOC/__TURBOC__ is the version as bytes: 0x0100 = Turbo C 1.0, 0x0105 = 1.5, 0x0200 = 2.0; Turbo C++ 1.0 reports 0x0295 */
#if defined(__TURBOC__)
        sprintf(buf, "Turbo C/C++ %u.%u", (unsigned int) ((__TURBOC__ >> 8) & 0xFF), (unsigned int) (__TURBOC__ & 0xFF));
#else
        sprintf(buf, "Turbo C/C++ %u.%u", (unsigned int) ((TURBOC >> 8) & 0xFF), (unsigned int) (TURBOC & 0xFF));
#endif
#elif defined(__DJGPP__)
        /* __VERSION__ may be longer than the buf can hold; cap it explicitly */
        sprintf(buf, "DJGPP GCC %.68s", __VERSION__);
#elif defined(__GNUC__)
        sprintf(buf, "GCC %.74s", __VERSION__);
#elif defined(_MSC_VER)
        sprintf(buf, "Microsoft C/C++ %u", (unsigned int) _MSC_VER);
#else
        sprintf(buf, "Unknown compiler");
#endif
        return(buf);
}


/* --------------------------- Startup function ---------------------------- */
/* hello:
   Prototype:
   void conioxlib_hello(const char *name, unsigned char commercial);
   Saves the screen to a file and shows the about box.
   Parameters: name is the file used to save the screen that
   conioxlib_bye() will restore; commercial selects between
   the shareware and the personal about text.
   Under Turbo C it also installs the critical error
   handler restored by conioxlib_bye().
   */
void conioxlib_hello (const char *name, unsigned char commercial)
{
        unsigned char oldx, oldy;
        FILE *f;
        char *oldb;
        char splash[96];
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
        _conioxlib_keyqhead = 0;
        _conioxlib_keyqtail = 0;
#endif
        gettext(1, 1, 80, 25, oldb);
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        _setcursortype(_NOCURSOR);
        textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
        conioxlib_setpal(1);
        conioxlib_push();
        sprintf(splash, " CONXLIB %u.%02u - %s - %s %s", (unsigned int) (conioxlib_version / 100), (unsigned int) (conioxlib_version % 100), conioxlib_compilerstring(), conioxlib_build, conioxlib_buildtime);
        conioxlib_background((unsigned char *) splash);
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
/* Helper for bye()'s error paths: reset video to the pre-hello state and exit.
   Both the malloc-failed and fopen-failed paths used to duplicate this
   sequence (vgaremap + setpal(0) + window + textattr + clrscr + cursor +
   helpcache_free + exit). Optional oldb free at the end. */
static void _conioxlib_bye_reset_video(unsigned char e, char *oldb)
{
#if ((__DOS__) || (__MSDOS__))
        if (_conioxlib_mouseptr != 0)
        {
                conioxlib_vgaremap(0);
        }
#endif
        conioxlib_setpal(0);
        window(1, 1, CONIOXLIB_SCR_W, CONIOXLIB_SCR_H);
        textattr(CONIOXLIB_ATTR(LIGHTGRAY, BLACK));
        clrscr();
        _setcursortype(_NORMALCURSOR);
        _conioxlib_helpcache_free();
        if (oldb != 0)
        {
                free(oldb);
        }
        exit(e);
}

/* bye:
   Prototype:
   void conioxlib_bye(const char *name, unsigned char e,
       unsigned char commercial);
   Restores the screen saved by conioxlib_hello() and ends
   the program.
   Parameters: name is the same file given to hello(), e is
   the exit code and commercial selects the farewell text.
   */
void conioxlib_bye (const char *name, unsigned char e, unsigned char commercial)
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
                /* Restore the video state hello() changed even when the save buffer
                   cannot be allocated, so the process never exits with the cursor
                   hidden (which would look like a hang). */
                _conioxlib_bye_reset_video(e, 0);
        }
        f = fopen(name, "rb");
        if (f == 0)
        {
                /* The save file hello() wrote is gone or out of reach (the cwd may
                   have been left elsewhere after fileselector): still reset the video
                   state so the program does not exit with the cursor hidden and the
                   demo frame frozen on screen. */
                _conioxlib_bye_reset_video(e, oldb);
        }
        memset(oldb, 0, 4000);
        fread(oldb, 1, 4000, f);
        fread(&oldx, 1, 1, f);
        fread(&oldy, 1, 1, f);
#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
        /* Only restore the critical error handler if the saved pointer was actually
           read from the file; an uninitialized pointer would install garbage as the
           INT 24h handler and crash DOS on the next disk error */
        if (fread(&oldvect, 1, sizeof(oldvect), f) == sizeof(oldvect))
        {
                setvect(0x24, oldvect);
        }
#endif
        fclose(f);
        remove(name);
#if ((__DOS__) || (__MSDOS__))
        /* hello() reprogrammed the palette (and vgaremap() may have hidden the driver pointer): leave the machine as it was found */
        if (_conioxlib_mouseptr != 0)
        {
                conioxlib_vgaremap(0);
        }
#endif
        conioxlib_setpal(0);
        puttext(1, 1, CONIOXLIB_SCR_W, CONIOXLIB_SCR_H, oldb);
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
        cprintf("CONXLIB %u.%02u for DOS and Windows\r\n", (unsigned int) (conioxlib_version / 100), (unsigned int) (conioxlib_version % 100));
        cputs("(c) Copyright 1997-2026 " CONIOXLIB_AUTHOR "\r\n");
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
        window(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
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
        _conioxlib_helpcache_free();
        free(oldb);
        exit(e);
}


/* ------------------- Draws the application background -------------------- */
/* background:
   Prototype:
   void conioxlib_background(const unsigned char *msg);
   Paints the application background with the colors of the
   active theme and writes msg on the top line.
   Parameters: msg is the text for row 1; an empty or null
   string leaves it untouched.
   */
void conioxlib_background (const unsigned char *msg)
{
        unsigned int i;
        unsigned short *oldb, *olda;
        unsigned short fill;
        unsigned short status;
        time_t t;
        struct tm *ti;

        /* Cell is always 2 bytes (char + attr). Never use unsigned int here: on Win32/DOS32 that is 4 bytes and overflows. */
        oldb = (unsigned short *) malloc(CONIOXLIB_SCR_W * CONIOXLIB_SCR_H * sizeof(unsigned short));
        if (oldb == 0)
        {
                return;
        }
        fill = (unsigned short) ((CONIOXLIB_ATTR(CYAN, BLUE) << 8) | 0xB0);
        olda = oldb;
        _conioxlib_memsetw(olda, fill, (unsigned int)(CONIOXLIB_SCR_W * (CONIOXLIB_SCR_H - 2)));
        olda += (unsigned int)(CONIOXLIB_SCR_W * (CONIOXLIB_SCR_H - 2));
        puttext(1, 2, CONIOXLIB_SCR_W, CONIOXLIB_SCR_H - 1, oldb);
        status = (unsigned short) (((unsigned int) _conioxlib_curtheme.atrstatus << 8) | 0x20);
        olda = oldb;
        _conioxlib_memsetw(olda, status, CONIOXLIB_SCR_W);
        olda += CONIOXLIB_SCR_W;
        puttext(1, CONIOXLIB_STATUS_ROW, CONIOXLIB_SCR_W, CONIOXLIB_STATUS_ROW, oldb);
        /* Row 1 is the title bar: always paint all columns so a short msg
           does not leave the right side black. */
        {
                unsigned short titleattr;
                unsigned int len;
                const char *p;

                titleattr = (unsigned short) (CONIOXLIB_ATTR(BLACK, LIGHTGRAY) << 8);
                p = ((msg != 0) && (msg[0] != 0)) ? (const char *) msg : "";
                len = (unsigned int) strlen(p);
                if (len > CONIOXLIB_SCR_W)
                {
                        len = CONIOXLIB_SCR_W;
                }
                olda = oldb;
                for (i = 0; i < len; i++)
                {
                        *olda++ = (unsigned short) (titleattr | (unsigned char) p[i]);
                }
                _conioxlib_memsetw(olda, (unsigned short)(titleattr | ' '), CONIOXLIB_SCR_W - len);
                olda += CONIOXLIB_SCR_W - len;
                puttext(1, CONIOXLIB_TITLE_ROW, CONIOXLIB_SCR_W, CONIOXLIB_TITLE_ROW, oldb);
        }
        time(&t);
        ti = localtime(&t);
        /* localtime() can fail: never hand a NULL struct tm to the painter */
        if (ti != 0)
        {
                _conioxlib_paintclock(ti);
        }
        /* Line 25 was just cleared to spaces: force the next blocks() repaint */
        _conioxlib_blocksdirty = 1;
        _conioxlib_status_dirty = 1;
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
        char ver[64];
        unsigned char oldx, oldy;
        char *oldb;

        oldb = malloc(gettextsize(x1, y1, x2 + 2, y2 + 1));
        if (oldb == 0)
        {
                return;
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        gettext(x1, y1, x2 + 2, y2 + 1, oldb);
        _setcursortype(_NOCURSOR);
        window(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
        _wscroll = 0;
        _conioxlib_hint("ENTER / ESC close about");
        box (x1, y1, x2, y2, _conioxlib_curtheme.atrframe, _conioxlib_curtheme.frame, "About", 3);
        textattr(_conioxlib_curtheme.atrdialog);
        cputsc(x1, x2, y1 + 1, "CONXLIB for DOS and Windows");
        sprintf(ver, "Version  %u.%02u", (unsigned int) (conioxlib_version / 100), (unsigned int) (conioxlib_version % 100));
        cputsc(x1, x2, y1 + 2, ver);
        sprintf(ver, "Build  %s %s", conioxlib_build, conioxlib_buildtime);
        cputsc(x1, x2, y1 + 3, ver);
        cputsc(x1, x2, y1 + 4, "(c) Copyright 1997-2026 " CONIOXLIB_AUTHOR);
        cputsc(x1, x2, y1 + 5, "https://nikkhokkho.sourceforge.io");
        sprintf(ver, "CONIOX %u.%02u based - Public Domain source code", (unsigned int) (coniox_version / 100), (unsigned int) (coniox_version % 100));
        cputsc(x1, x2, y1 + 6, ver);
        sprintf(ver, "Compiler  %s", conioxlib_compilerstring());
        cputsc(x1, x2, y1 + 7, ver);
        sprintf(ver, "Memory model  %s", conioxlib_model);
        cputsc(x1, x2, y1 + 8, ver);
        textattr(_conioxlib_curtheme.atrhot);
        if (commercial)
        {
                cputsc(x1, x2, y1 + 12, "Registered version");
        }
        else
        {
                cputsc(x1, x2, y1 + 12, "Free distribution");
        }
        do
        {
                key = _conioxlib_pollkey();
                /* Left button on the close button */
                if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.y == (unsigned int) y1) && (conioxlib_mousestatus.x == (unsigned int) x1 + 3))
                {
                        key = KBD_ESC;
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
                _conioxlib_keyqhead = 0;
                _conioxlib_keyqtail = 0;
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
        /* 4000 bytes: too much for the stack of a 16 bit DOS program */
        static char CONIOXLIB_FARDATA oldb[8][5][10 * 5 * 2];
        unsigned int key;

        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
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
                if (_conioxlib_keyhit())
                {
                        _conioxlib_readkey(&key);
                        goto done;
                }
                /* A click or a wheel movement wakes the saver up too */
                conioxlib_pollmouse();
                if ((conioxlib_mousestatus.lclick != 0) || (conioxlib_mousestatus.rclick != 0) || (conioxlib_mousestatus.wheelx != 0) || (conioxlib_mousestatus.wheely != 0))
                {
                        goto done;
                }
                do
                {
                        i = rand() % 8;
                        j = rand() % 5;
                }
                while (rep[i][j] != 0);
                rep[i][j] = 1;
                window(i * 10 + 1, j * 5 + 1, i * 10 + 10, j * 5 + 5);
                textattr(CONIOXLIB_ATTR(BLACK, BLACK));
                clrscr();
                window(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
                delay(200);
        }
        /* Start putting back */
        memset(&rep, 0, sizeof(rep));
        for (k = 0; k != 40; k++)
        {
                if (_conioxlib_keyhit())
                {
                        _conioxlib_readkey(&key);
                        goto done;
                }
                /* A click or a wheel movement wakes the saver up too */
                conioxlib_pollmouse();
                if ((conioxlib_mousestatus.lclick != 0) || (conioxlib_mousestatus.rclick != 0) || (conioxlib_mousestatus.wheelx != 0) || (conioxlib_mousestatus.wheely != 0))
                {
                        goto done;
                }
                do
                {
                        i = rand() % 8;
                        j = rand() % 5;
                }
                while (rep[i][j] != 0);
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
   The screen is snapshot with gettext, then puttext paints that
   frozen image at progressive offsets (fall/bounce). movetext is
   not used here on purpose: shifting live VRAM would mix frames of
   the bounce. push() is the effect that relies on movetext.
   Any key aborts the effect.
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
        _conioxlib_blocksdirty = 1;
        _conioxlib_status_dirty = 1;
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
                                puttext(1, (int) (i + 1), (int) sw, (int) sh, fb);
                        }
                        if (_conioxlib_keyhit())
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
                                puttext(1, (int) i, (int) sw, (int) sh, fb);
                                if (_conioxlib_keyhit())
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
                                puttext(1, (int) (i + 1), (int) sw, (int) sh, fb);
                        }
                        if (_conioxlib_keyhit())
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
        unsigned short wipe_blank;
        unsigned int phase;
        unsigned int x, y;
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
        _conioxlib_blocksdirty = 1;
        _conioxlib_status_dirty = 1;
        wipe_blank = (unsigned short) ((CONIOXLIB_ATTR(LIGHTGRAY, BLACK) << 8) | 0x20);
        for (phase = 0; phase < 8; phase++)
        {
                for (y = 0; y < sh; y++)
                {
                        for (x = 0; x < sw; x++)
                        {
                                if (((x + y) & 7) <= phase)
                                {
                                        fb[(y * sw) + x] = wipe_blank;
                                }
                        }
                }
                puttext(1, 1, (int) sw, (int) sh, fb);
                if (_conioxlib_keyhit())
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
   unsigned char conioxlib_exittodos(const char *name);
   Asks for confirmation before leaving to DOS.
   Parameters: name is the help file shown by its F1 button.
   Returns 1 when the user confirms the exit and 0 when he
   stays. The Shell answer runs conioxlib_shell() and also
   returns 0.
   */
unsigned char conioxlib_exittodos (const char *name)
{
        unsigned int i;
        unsigned char oldx, oldy;
        char *oldb, *buf;
        char but1[][CONIOXLIB_BTN_W] = {"No", "Yes", "Shell"};
        char dbut1[][CONIOXLIB_HINT_W] = {"Continue running program                                            ",
                                                "Exit to DOS and finish program                                      ",
                                                "Exit to DOS and continue running program                            "};

        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        oldb = conioxlib_savescr(20, 10, 62, 15);
        if (oldb == 0)
        {
                return(0);
        }
        box(20, 10, 60, 14, _conioxlib_curtheme.atrframe, _conioxlib_curtheme.frame, "Exit", SHADOW);
        textattr(_conioxlib_curtheme.atrdialog);
        cputsc(20, 60, 11, "Do you want to exit to DOS?");
        do
        {
                i = conioxlib_buttons(28, 12, _conioxlib_curtheme.btnatr0, _conioxlib_curtheme.btnatr1, _conioxlib_curtheme.btnatrhot, but1, dbut1, 3, 2);
                /* F1 (254) opens help and loops back to the buttons; ESC (255) stays (returns 0) */
                if (i == 254)
                {
                        buf = malloc(16384);
                        if (buf == 0)
                        {
                                conioxlib_restorescr(20, 10, 62, 15, oldb);
                                return(0);
                        }
                        conioxlib_help (7, 3, 69, 21, _conioxlib_curtheme.atr0, _conioxlib_curtheme.atr1, name, "Exit", (char (*)[CONIOXLIB_HELP_W]) buf);
                        free(buf);
                }
        }
        while (i == 254);
        conioxlib_restorescr(20, 10, 62, 15, oldb);
        gotoxy(oldx, oldy);
        /* ESC: stay (return 0); Shell: spawn and stay; Yes/No: pass the index through */
        if (i == 255)
        {
                i = 0;
        }
        else if (i == 2)
        {
                conioxlib_shell(3, 2, 75, 22, "");
                i = 0;
        }
        return(i);
}


/* ----------------------------- Shell to DOS ------------------------------- */
/* shell:
   Prototype:
   void conioxlib_shell(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, const char *command);
   Runs a shell of the command interpreter.
   Parameters: the rectangle frames the shell output under
   DOS and is ignored on Windows, where the shell runs on
   the full screen; command is the program to run and an
   empty command starts the interactive interpreter named
   by COMSPEC (COMMAND.COM or CMD.EXE). Type EXIT inside
   the shell to come back; the screen is restored on return.
   */
void conioxlib_shell (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, const char *command)
{
        unsigned char oldx, oldy;
        char *oldb;
        int i;

        if (command == 0)
        {
                command = "";
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        /* On Windows the interpreter needs no frame: run it on the whole screen and
           restore the previous one when it exits */
        (void) x1;
        (void) y1;
        (void) x2;
        (void) y2;
        oldb = conioxlib_savescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
        if (oldb == 0)
        {
                return;
        }
        window(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
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
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        {
                char *comspec;

                comspec = getenv("COMSPEC");
                i = system((command[0] != 0) ? command : ((comspec != 0) ? comspec : "cmd"));
        }
#else
        {
                char *comspec;
#if defined(__WATCOMC__) || defined(__TURBOC__)
                _conioxlib_intvec (*oldvect)();
#endif

                /* DOS keeps the real interpreter path in COMSPEC; do not rely on command.com being in the current directory or PATH */
                comspec = getenv("COMSPEC");
#if ((__DOS__) || (__MSDOS__))
#if defined(__WATCOMC__) || defined(__TURBOC__)
                /* Hook INT 29h so the CON output of the child lands in the shell window: every DOS compiler installs the filter through _dos_setvect (Watcom, Borland C) or the Turbo C alias */
                oldvect = _dos_getvect(0x29);
                _dos_setvect(0x29, conioxlib_shell_patch);
#endif
                i = system((command[0] != 0) ? command : ((comspec != 0) ? comspec : "command.com"));
#if defined(__WATCOMC__) || defined(__TURBOC__)
                _dos_setvect(0x29, oldvect);
#endif
#else
                i = system((command[0] != 0) ? command : ((comspec != 0) ? comspec : "cmd"));
#endif
        }
#endif
        _setcursortype(_NOCURSOR);
        window(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
        if (i == -1)
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
        conioxlib_restorescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h(), oldb);
#else
        conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
#endif
        gotoxy(oldx, oldy);
}


#if ((__DOS__) || (__MSDOS__)) && (defined(__TURBOC__) || defined(__WATCOMC__))

/* ------------------- Exit routine of conioxlib_shell -------------------------- */
/* conioxlib_shell_patch:
   Prototype (Turbo C and Watcom DOS builds):
   void interrupt conioxlib_shell_patch(unsigned int, ...);
   void __interrupt __far conioxlib_shell_patch(union INTPACK r);
   Int 29h filter installed by conioxlib_shell().
   Parameters: the interrupt frame registers.
   It redirects the CON output of the child process into the
   shell window. Do not call it directly.
   */
#if defined(__TURBOC__)
#pragma warn -par
void interrupt conioxlib_shell_patch (unsigned int bp, unsigned int di, unsigned int si, unsigned int ds, unsigned int es, unsigned int dx, unsigned int cx, unsigned int bx, unsigned int ax)
{
        unsigned char al;

        al = ax & 255;
        putch(al);
}
#pragma warn .par
#else
void __interrupt __far conioxlib_shell_patch (union INTPACK r)
{
        putch(r.h.al);
}
#endif

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
/* Leap year test: was duplicated 3 times in conioxlib_calendar before */
static conioxlib_inline int _conioxlib_calendar_is_leap(unsigned int y)
{
        return (((y % 4) == 0) && ((y % 100) != 0)) || ((y % 400) == 0);
}

unsigned char conioxlib_calendar (unsigned short x, unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atr2, unsigned char *day, unsigned char *month, unsigned int *year)
{
        unsigned char i, days, start, first, cday, cmon, prevdays;
        unsigned char oldx, oldy;
        unsigned int key, cyear;
        char *oldb;
        struct tm *now;
        time_t curr_time;
        unsigned long int wy;
        static const unsigned char monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        static const unsigned char sakamoto[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        static const char * const monthnameuk[12] = {       "January", "February", "March", "April",
                                                                "May", "June", "July", "August",
                                                                "September", "October", "November",
                                                                "December" };

        _setcursortype(_NOCURSOR);
        oldb = conioxlib_savescr(x, y, x + 40, y + 11);
        if (oldb == 0)
        {
                return(0);
        }
        /* Guard NULL pointers: caller may pass NULL when the initial date is not needed */
        if ((day == 0) || (month == 0) || (year == 0))
        {
                conioxlib_restorescr(x, y, x + 40, y + 11, oldb);
                return(0);
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        /* Get the current date */
        curr_time = time(0);
        now = localtime(&curr_time);
        /* localtime() can return NULL on failure: never dereference it without checking */
        if (now == 0)
        {
                conioxlib_restorescr(x, y, x + 40, y + 11, oldb);
                gotoxy(oldx, oldy);
                return(0);
        }
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
                cmon = (unsigned char) (now->tm_mon + 1);
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
                /* Sakamoto's method (0 = Sunday): unlike mktime() it is valid for every year */
                wy = (unsigned long int) cyear - ((cmon < 3) ? 1UL : 0UL);
                start = (unsigned char) (((wy + (wy / 4UL) - (wy / 100UL) + (wy / 400UL) + (unsigned long int) sakamoto[cmon - 1] + 1UL) % 7UL) + 1UL);
                /* start walks the cells while drawing: first keeps the column of day 1 for the mouse */
                first = start;
                days = monthdays[cmon - 1];
                if ((cmon == 2) && _conioxlib_calendar_is_leap(cyear))
                {
                        days = 29;
                }
                if (cday > days)
                {
                        cday = days;
                }
                /* Days of the previous month, used when the day wraps over a month edge */
                prevdays = monthdays[(cmon == 1) ? 11 : (cmon - 2)];
                if ((cmon == 3) && _conioxlib_calendar_is_leap(cyear))
                {
                        prevdays = 29;
                }
                /* Keep status text in columns 1..59 so clock (76-80) and locks (60-74) never collide */
                _conioxlib_hint("ENTER accepts; ESC closes; arrows, PGUP/PGDN, HOME/END");
                /* atr0 = normal day cells, atr1 = selected day, atr2 = Sundays */
                box(x, y, x + 38, y + 10, atr0, A_DOUBLE, "Calendar", SHADOW | CLOSE);
                textattr(atr0);
                gotoxy(x + 2, y + 1);
                cprintf("%-30s %u", monthnameuk[cmon - 1], cyear);
                gotoxy(x + 5, y + 2);
                cputs("Sun Mon Tue Wed Thu Fri Sat");
                /* Build the entire 7×6 day grid in one buffer and emit a single
                   puttext. Each cell is 4 chars wide: "DD " (2 digits + space).
                   7 columns × 4 chars = 28 chars per row, 6 rows = 168 cells.
                   Replaces ~31 gotoxy+textattr+cprintf calls per repaint. */
                {
                        unsigned short *calcells;
                        unsigned short ca_day;
                        unsigned char daynum;
                        unsigned char col_pos;
                        unsigned char row_pos;

                        calcells = _conioxlib_need_cells(7 * 4 * 6);
                        if (calcells != 0)
                        {
                                /* Fill all 168 cells with spaces in atr0 */
                                ca_day = (unsigned short) ((unsigned int) atr0 << 8);
                                _conioxlib_memsetw(calcells, (unsigned short)(ca_day | ' '), 7 * 4 * 6);
                                /* Place day numbers */
                                col_pos = start - 1;
                                row_pos = 0;
                                for (daynum = 1; daynum <= days; daynum++)
                                {
                                        unsigned short ca_cell;
                                        if (daynum == cday)
                                        {
                                                ca_cell = (unsigned short) ((unsigned int) atr1 << 8);
                                        }
                                        else if (col_pos == 0)
                                        {
                                                ca_cell = (unsigned short) ((unsigned int) atr2 << 8);
                                        }
                                        else
                                        {
                                                ca_cell = ca_day;
                                        }
                                        {
                                                unsigned int base = (unsigned int) row_pos * 7 * 4 + (unsigned int) col_pos * 4;
                                                if (daynum >= 10)
                                                {
                                                        calcells[base + 0] = (unsigned short) (ca_cell | (unsigned char) ('0' + daynum / 10));
                                                        calcells[base + 1] = (unsigned short) (ca_cell | (unsigned char) ('0' + daynum % 10));
                                                }
                                                else
                                                {
                                                        calcells[base + 0] = (unsigned short) (ca_cell | ' ');
                                                        calcells[base + 1] = (unsigned short) (ca_cell | (unsigned char) ('0' + daynum));
                                                }
                                                calcells[base + 2] = (unsigned short) (ca_cell | ' ');
                                                calcells[base + 3] = (unsigned short) (ca_cell | ' ');
                                        }
                                        col_pos++;
                                        if (col_pos >= 7)
                                        {
                                                col_pos = 0;
                                                row_pos++;
                                        }
                                }
                                puttext((unsigned short) (x + 5), (unsigned short) (y + 4),
                                        (unsigned short) (x + 5 + 28 - 1), (unsigned short) (y + 4 + 6 - 1), calcells);
                        }
                }
                i = 0;
                do
                {
                        key = _conioxlib_pollkey();
                        /* Left button on the close button */
                        if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.y == y) && (conioxlib_mousestatus.x == (unsigned int) x + 3))
                        {
                                key = KBD_ESC;
                        }
                        /* Left button on a day cell picks that day */
                        else if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= (unsigned int) x + 6) && (conioxlib_mousestatus.x <= (unsigned int) x + 33) && (conioxlib_mousestatus.y >= (unsigned int) y + 4) && (conioxlib_mousestatus.y <= (unsigned int) y + 9))
                        {
                                i = (unsigned char) ((((unsigned int) (conioxlib_mousestatus.y - y) - 4) * 7) + (((unsigned int) (conioxlib_mousestatus.x - x) - 6) >> 2) - (first - 1) + 1);
                                if ((i >= 1) && (i <= days))
                                {
                                        cday = i;
                                        key = 1;
                                }
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
                        if ((cmon == 2) && _conioxlib_calendar_is_leap(cyear))
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
       unsigned char atr1, const char items[][CONIOXLIB_GRID_W],
       unsigned char noptions);
   Picks an option from a grid of four columns per row.
   Parameters: atr0 is the normal color and atr1 the selected
   one, items holds the texts and noptions their number.
   The arrow keys, the mouse wheel and the left button move
   the selection. Returns the index of the chosen item or 255
   when cancelled with ESC.
   */
unsigned char conioxlib_grid (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, const char items[][CONIOXLIB_GRID_W], unsigned char noptions)
{
        int y;
        unsigned int key;
        int x;

        if ((noptions == 0) || (items == 0))
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
        textattr(_conioxlib_curtheme.atrdialog);
        _wscroll = 0;
        y = 0;
        do
        {
                do
                {
                        key = _conioxlib_pollkey();
                        /* Left button */
                        if ((conioxlib_mousestatus.lclick != 0) && (conioxlib_mousestatus.x >= x1) && (conioxlib_mousestatus.x <= (x1 + 60)) && (conioxlib_mousestatus.y >= y1) && (conioxlib_mousestatus.y <= (y1 + ((noptions + 3) >> 2) - 1)))
                        {
                                /* A click on an empty cell of the last row is ignored */
                                x = (int) (((conioxlib_mousestatus.y - y1) << 2) + (((conioxlib_mousestatus.x - x1) / 15) & 3));
                                if (x < (int) noptions)
                                {
                                        textattr(atr0);
                                        gotoxy(((y & 3) * 15) + x1, (y >> 2) + y1);
                                        cputs(items[y]);
                                        y = x;
                                        key = 0xFFFF;
                                }
                        }
                        else
                        {
                                _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
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
        if ((key == KBD_ESC) || (key == KBD_F1))
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
   unsigned char conioxlib_helpindex(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, const char *name, char buffer[][32]);
   Lists every topic of a help file and opens the picked
   one.
   Parameters: name is the help file and buffer a work area
   that receives the topic names. Returns the index of the
   topic shown or 255 when cancelled. The whole file is
   listed from the shared topic cache (same dirty check as help).
   */
unsigned char conioxlib_helpindex (unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, const char *name, char buffer[][32])
{
        unsigned int i;
        unsigned int last;
        unsigned char oldx, oldy;
        char *oldb;
        char (*helpbuf)[CONIOXLIB_HELP_W];

        helpbuf = (char (*)[CONIOXLIB_HELP_W]) buffer;
        oldb = conioxlib_savescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h());
        if (oldb == 0)
        {
                return(255);
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        _conioxlib_hint("Loading help...");
        /* Index comes from the topic cache: one scan of @headers, then only the names. */
        if (!_conioxlib_helpcache_ensure(name) || (_conioxlib_helpcache_n == 0))
        {
                strcpy(&helpbuf[0][0], "Help is not available.");
                conioxlib_view(x1, y1, x1 + 65, y1 + 20, CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), helpbuf, 1, "Help: Not available");
                conioxlib_restorescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h(), oldb);
                gotoxy(oldx, oldy);
                return(255);
        }
        last = _conioxlib_helpcache_n;
        if (last > 255)
        {
                last = 255;
        }
        for (i = 0; i < last; i++)
        {
                strncpy(&buffer[i][0], _conioxlib_helpcache[i].name, 31);
                buffer[i][31] = 0;
        }
        box (x1, y1, x1 + 65, y1 + 19, CONIOXLIB_ATTR(BLACK, CYAN), A_DOUBLE, "Help: Index", 1);
        /* 2-column layout (31 chars each). Scrolling is mandatory when the topic
           count exceeds visrows * cols (255 topics / 2 cols / 18 rows = ~8 pages).
           The old code painted all entries on the first pass and ran off the
           window; now only visible rows are painted and `top` tracks the scroll.
           Colors are forced to the TV help palette (black-on-cyan frame, white-on-red
           selection) so the index matches the help body window. */
        {
                unsigned char idx_atr0 = (unsigned char) CONIOXLIB_ATTR(BLACK, CYAN);
                unsigned char idx_atr1 = (unsigned char) CONIOXLIB_ATTR(WHITE, RED);
                int sel = 0;
                int colw = 31;
                int cols = 2;
                int visrows = 18;
                int top = 0;
                int row, col;
                unsigned int gkey;
                int repaint = 1;
                int maxtop;
                /* maxtop: last valid top index so the bottom row is never empty */
                maxtop = ((int) last - visrows * cols + cols - 1) / cols * cols;
                if (maxtop < 0) maxtop = 0;
                do
                {
                        if (repaint)
                        {
                                /* Paint visible rows only: top..top+visrows*cols-1.
                                   Entries beyond `last` are blanked so the window
                                   does not show stale text from the previous page. */
                                for (row = 0; row < visrows; row++)
                                {
                                        for (col = 0; col < cols; col++)
                                        {
                                                int idx = top + row * cols + col;
                                                textattr((idx == sel) ? idx_atr1 : idx_atr0);
                                                gotoxy(x1 + 2 + col * (colw + 1), y1 + 1 + row);
                                                if (idx < (int) last)
                                                {
                                                        cprintf("%-*.*s", colw, colw, buffer[idx]);
                                                }
                                                else
                                                {
                                                        cprintf("%*.*s", colw, colw, "");
                                                }
                                        }
                                }
                                /* Scrollbar on the right edge shows position in the topic list */
                                _conioxlib_draw_scrollbar((unsigned short)(x1 + 65), (unsigned short)(y1 + 1), visrows, top / cols, ((int) last + cols - 1) / cols, CONIOXLIB_ATTR(BLACK, CYAN), 1);
                                repaint = 0;
                        }
                        gkey = _conioxlib_pollkey();
                        if (gkey == KBD_UP)        { if (sel >= cols) sel -= cols; }
                        else if (gkey == KBD_DOWN) { if (sel + cols < (int) last) sel += cols; }
                        else if (gkey == KBD_LEFT) { if (sel > 0) sel--; }
                        else if (gkey == KBD_RIGHT){ if (sel < (int) last - 1) sel++; }
                        else if (gkey == KBD_PGUP) { sel -= visrows * cols; if (sel < 0) sel = 0; }
                        else if (gkey == KBD_PGDW) { sel += visrows * cols; if (sel >= (int) last) sel = last - 1; }
                        else if (gkey == KBD_ORG)  { sel = 0; }
                        else if (gkey == KBD_END)  { sel = last - 1; }
                        /* Scroll if the selection moved out of the visible window */
                        if (sel < top) { top = (sel / cols) * cols; if (top < 0) top = 0; repaint = 1; }
                        else if (sel >= top + visrows * cols) { top = ((sel / cols) - visrows + 1) * cols; if (top > maxtop) top = maxtop; repaint = 1; }
                        else if (gkey == KBD_UP || gkey == KBD_DOWN || gkey == KBD_LEFT || gkey == KBD_RIGHT)
                        {
                                repaint = 1;
                        }
                }
                while (gkey != KBD_ENTER && gkey != KBD_ESC && gkey != KBD_F1);
                i = (unsigned int) ((gkey == KBD_ENTER) ? sel : 255);
                (void) atr0; (void) atr1;
        }
        conioxlib_restorescr(1, 1, _conioxlib_scr_w(), _conioxlib_scr_h(), oldb);
        gotoxy(oldx, oldy);
        if (i == 255)
        {
                return(255);
        }
        conioxlib_help (x1, y1, x1 + 62, y1 + 18, _conioxlib_curtheme.atr0, _conioxlib_curtheme.atr1, name, &buffer[i][0], helpbuf);
        return((unsigned char) i);
}

/* ----------------------------- Apply theme ------------------------------- */
/* theme:
   Prototype:
   void conioxlib_theme(unsigned char id);
   Applies one of the CONIOXLIB_THEMES (12) visual themes.
   Parameters: id is
     0 Borland C++ 3.1 (Turbo Vision 2.0 cpColor),
     1 Turbo C 2.0, 2 Turbo C 1.x, 3 Norton Utilities 9,
     4 PC Tools, 5 Norton Commander, 6 MS-DOS Edit,
     7 Lotus 1-2-3, 8 SideKick, 9 Volkov Commander,
     10 DOS Navigator, 11 XTree Gold.
   Out-of-range ids fall back to theme 0.
   */
void conioxlib_theme (unsigned char id)
{
        /* Field order matches struct conioxlib_stheme:
           atr0, atr1, atrhot, atrstatus, atrdialog,
           btnatr0, btnatr1, btnatrhot,
           atrinput, atrinputsel, atrlabel, atrlabelhot, atrinterior, atrframe,
           frame, name.
           Palettes verified against the original Turbo Vision 2.0 cpColor
           (tv.enzolombardi.net/reference/palette-chart) and against screenshots
           of each original DOS program (Norton Commander 5.0, NU 8/9, PC Tools,
           MS-DOS Edit/QBASIC, Lotus 1-2-3 r4). The "TV pure" entries (BC++ 3.1
           and TP 7.0) match the cpColor bytes literally:
             menu normal    = 0x70 (Black on LightGray)
             menu selected   = 0x20 (Black on Green)
             menu hotkey     = 0x74 (Red on LightGray)
             status normal   = 0x70
             status selected = 0x20
             dialog frame    = 0x70
             dialog active   = 0x7F (White on LightGray)
             dialog interior = 0x7A (LightGreen on LightGray)
             label normal    = 0x70
             label hotkey    = 0x7E (Yellow on LightGray)
             button normal   = 0x20 (Black on Green)
             button focused  = 0x2F (White on Green)
             button hotkey   = 0x2E (Yellow on Green)
             input passive   = 0x1F (White on Blue)
             input selected  = 0x2F (White on Green)
           Theme 0 keeps the raw Turbo Vision 2.0 cpColor values (lightgray
           menus, green selection). Theme 1 (Turbo C 2.0) is the yellow-on-blue
           IDE look. Themes 3-11 follow screenshots of each original program. */
        static const struct conioxlib_stheme themes[CONIOXLIB_THEMES] = {
                /* 0 - Borland C++ 3.1 / Turbo Vision 2.0 IDE palette (verified against TV cpColor) */
                {CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(RED, LIGHTGRAY),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
                 CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, LIGHTGRAY),
                 CONIOXLIB_ATTR(LIGHTGREEN, LIGHTGRAY), CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
                 CONIOXLIB_BOX_A_DOUBLE, "Borland C++ 3.1"},
                /* 1 - Turbo C 2.0: yellow-on-blue menu, cyan dropdowns */
                {CONIOXLIB_ATTR(YELLOW, BLUE), CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, CYAN),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(LIGHTGREEN, CYAN), CONIOXLIB_ATTR(WHITE, CYAN),
                 CONIOXLIB_BOX_A_DOUBLE, "Turbo C 2.0"},
                /* 2 - Turbo C 1.x: monochrome (black on lightgray) */
                {CONIOXLIB_ATTR(LIGHTGRAY, BLACK), CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, BLACK),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, BLACK),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, LIGHTGRAY), CONIOXLIB_ATTR(BLACK, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(WHITE, LIGHTGRAY),
                 CONIOXLIB_ATTR(LIGHTGRAY, BLACK), CONIOXLIB_ATTR(YELLOW, BLACK),
                 CONIOXLIB_ATTR(LIGHTGRAY, BLACK), CONIOXLIB_ATTR(WHITE, BLACK),
                 CONIOXLIB_BOX_A_SINGLE, "Turbo C 1.x"},
                /* 3 - Norton Utilities 9: blue desktop, cyan dropdowns, blue dialogs */
                {CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(CYAN, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(CYAN, BLUE), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(LIGHTGREEN, BLUE), CONIOXLIB_ATTR(WHITE, BLUE),
                 CONIOXLIB_BOX_A_DOUBLE, "Norton Utilities 9"},
                /* 4 - PC Tools Deluxe: cyan desktop, lightgray dialogs */
                {CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, BLUE),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, LIGHTGRAY),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, LIGHTGRAY),
                 CONIOXLIB_ATTR(LIGHTGREEN, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, LIGHTGRAY),
                 CONIOXLIB_BOX_A_DOUBLE, "PC Tools"},
                /* 5 - Norton Commander: cyan panels, black selection bar, blue dialogs */
                {CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(CYAN, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(CYAN, BLUE), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(LIGHTGREEN, BLUE), CONIOXLIB_ATTR(WHITE, BLUE),
                 CONIOXLIB_BOX_A_DOUBLE, "Norton Commander"},
                /* 6 - MS-DOS Edit / QBASIC: blue desktop, cyan selection, single frame */
                {CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(CYAN, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(CYAN, BLUE), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(LIGHTGREEN, BLUE), CONIOXLIB_ATTR(WHITE, BLUE),
                 CONIOXLIB_BOX_A_SINGLE, "MS-DOS Edit"},
                /* 7 - Lotus 1-2-3: high-contrast black on white, single frame */
                {CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(BLACK, WHITE), CONIOXLIB_ATTR(YELLOW, BLACK),
                 CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(WHITE, BLACK),
                 CONIOXLIB_ATTR(BLACK, WHITE), CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(YELLOW, WHITE),
                 CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(WHITE, BLACK),
                 CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(YELLOW, BLACK),
                 CONIOXLIB_ATTR(LIGHTGRAY, BLACK), CONIOXLIB_ATTR(WHITE, BLACK),
                 CONIOXLIB_BOX_A_SINGLE, "Lotus 1-2-3"},
                /* 8 - Borland SideKick: blue/cyan TV-like, green buttons */
                {CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
                 CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, LIGHTGRAY),
                 CONIOXLIB_ATTR(LIGHTGREEN, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, LIGHTGRAY),
                 CONIOXLIB_BOX_A_DOUBLE, "SideKick"},
                /* 9 - Volkov Commander: red/cyan NC clone, amber accent */
                {CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, BLACK), CONIOXLIB_ATTR(LIGHTRED, CYAN),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(CYAN, BLUE), CONIOXLIB_ATTR(LIGHTRED, BLUE),
                 CONIOXLIB_ATTR(LIGHTGREEN, BLUE), CONIOXLIB_ATTR(WHITE, BLUE),
                 CONIOXLIB_BOX_A_SINGLE, "Volkov Commander"},
                /* 10 - DOS Navigator: blue/cyan with green accents, double frame */
                {CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, BLUE),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(BLACK, LIGHTGRAY),
                 CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(YELLOW, LIGHTGRAY),
                 CONIOXLIB_ATTR(LIGHTGREEN, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, LIGHTGRAY),
                 CONIOXLIB_BOX_A_DOUBLE, "DOS Navigator"},
                /* 11 - XTree Gold: yellow-on-blue file manager, single frame */
                {CONIOXLIB_ATTR(YELLOW, BLUE), CONIOXLIB_ATTR(BLACK, YELLOW), CONIOXLIB_ATTR(WHITE, BLUE),
                 CONIOXLIB_ATTR(YELLOW, BLUE), CONIOXLIB_ATTR(BLACK, CYAN),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(WHITE, GREEN),
                 CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(YELLOW, CYAN),
                 CONIOXLIB_ATTR(LIGHTGREEN, CYAN), CONIOXLIB_ATTR(WHITE, CYAN),
                 CONIOXLIB_BOX_A_SINGLE, "XTree Gold"}
        };

        _conioxlib_curtheme = themes[(id < CONIOXLIB_THEMES) ? id : 0];
        /* The status color changed: the lock indicators and the clock of line 25 must be repainted */
        _conioxlib_blocksdirty = 1;
        _conioxlib_status_dirty = 1;
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
                *t = _conioxlib_curtheme;
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

        /* The library works on the 80x25 screen: a rectangle sticking out of it (the shadow of a box
           at the right edge, say) is clamped, because gettext() refuses it altogether */
        if (x2 > CONIOXLIB_SCR_W)
        {
                x2 = CONIOXLIB_SCR_W;
        }
        if (y2 > CONIOXLIB_SCR_H)
        {
                y2 = CONIOXLIB_SCR_H;
        }
        if ((x1 < 1) || (y1 < 1) || (x1 > x2) || (y1 > y2))
        {
                return(0);
        }
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
                /* Same clamp as savescr(): the buffer holds the clamped rectangle */
                if (x2 > CONIOXLIB_SCR_W)
                {
                        x2 = CONIOXLIB_SCR_W;
                }
                if (y2 > CONIOXLIB_SCR_H)
                {
                        y2 = CONIOXLIB_SCR_H;
                }
                if ((x1 >= 1) && (y1 >= 1) && (x1 <= x2) && (y1 <= y2))
                {
                        unsigned short srow, st_end, locks_col, clock_col, clock_end;

                        puttext(x1, y1, x2, y2, buf);
                        /* A restore that overlaps the status text strip can change what is
                           on screen there without status() knowing; force a repaint so the
                           double-buffer stays in sync. Restores that only touch the clock
                           or the lock indicators (past st_end) leave the text untouched. */
                        _conioxlib_status_layout(&srow, &st_end, &locks_col, &clock_col, &clock_end);
                        if ((y1 <= srow) && (y2 >= srow) && (x1 <= st_end))
                        {
                                _conioxlib_status_dirty = 1;
                        }
                }
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
        unsigned short *cells;
        unsigned short cellattr;
        unsigned int i, len, ncols;
        unsigned int cap;
        const char *p;
        unsigned short row, st_end, locks_col, clock_col, clock_end;
        /* Sized exactly to _conioxlib_status_last (the dirty-checked buffer):
           no need for 256 bytes on the 16-bit DOS stack, status is at most ~80 cols */
        char newstr[sizeof(_conioxlib_status_last)];

        _conioxlib_status_layout(&row, &st_end, &locks_col, &clock_col, &clock_end);
        ncols = (unsigned int) st_end;
        if (ncols < 1)
        {
                ncols = 1;
        }
        cap = (unsigned int) sizeof(_conioxlib_status_last) - 1;
        if (ncols > cap)
        {
                ncols = cap;
        }
        /* Build the space-padded string that will actually be painted, so the
           double-buffer comparison sees the same bytes the screen would receive. */
        p = (msg != 0) ? msg : "";
        len = (unsigned int) strlen(p);
        if (len > ncols)
        {
                len = ncols;
        }
        memcpy(newstr, p, len);
        if (ncols > len) memset(&newstr[len], ' ', ncols - len);
        /* Skip the repaint when the padded text is identical to the last paint
           and nothing has wiped the strip since. The hint macro calls status()
           on every poll cycle, so this avoids a puttext per frame in idle loops. */
        if ((_conioxlib_status_dirty == 0) && (memcmp(newstr, _conioxlib_status_last, ncols) == 0))
        {
                return;
        }
        cells = _conioxlib_need_cells(ncols);
        if (cells == 0)
        {
                return;
        }
        cellattr = (unsigned short) ((unsigned int) _conioxlib_curtheme.atrstatus << 8);
        _conioxlib_memsetw(cells, (unsigned short)(cellattr | ' '), ncols);
        for (i = 0; i < ncols; i++)
        {
                if (newstr[i] != ' ') cells[i] = (unsigned short) (cellattr | (unsigned char) newstr[i]);
        }
        memcpy(_conioxlib_status_last, newstr, ncols);
        _conioxlib_status_last[ncols] = 0;
        _conioxlib_status_dirty = 0;
        puttext(1, row, st_end, row, cells);
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
        /* Keep the tip inside the line: it must end before column 80 so the console
           cannot wrap at the last cell and scroll the screen */
        if (x > 78)
        {
                x = 78;
        }
        if (len > (unsigned int) (79 - x))
        {
                len = (unsigned int) (79 - x);
        }
        x2 = (unsigned short) (x + len + 1);
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
        unsigned int i, filled, barw, pct;
        unsigned short *cells;
        unsigned short ca;
        char pctbuf[8];

        /* barw (≤ 75) + " 100%" = 6 chars max → 81 cells is enough; 256 was wasteful */
        cells = _conioxlib_need_cells(81);
        if (cells == 0)
        {
                return;
        }

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
        /* Multiply in unsigned long: values in bytes overflow 16 bit ints under DOS */
        barw = (unsigned int) width - 5;
        if (barw > 75)
        {
                barw = 75;
        }
        filled = (unsigned int) (((unsigned long int) value * barw) / maxv);
        ca = (unsigned short) ((unsigned int) atr << 8);
        if (filled > 0)
        {
                _conioxlib_memsetw(cells, (unsigned short)(ca | 0xDB), (unsigned int) filled);
        }
        if (barw > filled)
        {
                _conioxlib_memsetw(cells + filled, (unsigned short)(ca | 0xB0), (unsigned int)(barw - filled));
        }
        i = barw;
        pct = (unsigned int) (((unsigned long int) value * 100) / maxv);
        /* Hand-format " %3u%%" (5 chars) instead of sprintf: avoids the
           format-string parse per progress update. Layout: [' ', H, T, U, '%'] */
        pctbuf[0] = ' ';
        if (pct >= 100)
        {
                pctbuf[1] = (char) ('0' + (pct / 100));
                pctbuf[2] = (char) ('0' + ((pct / 10) % 10));
                pctbuf[3] = (char) ('0' + (pct % 10));
        }
        else
        {
                pctbuf[1] = (pct >= 10) ? (char) ('0' + (pct / 10)) : ' ';
                pctbuf[2] = (pct >= 10) ? (char) ('0' + (pct % 10)) : (char) ('0' + pct);
                pctbuf[3] = ' ';
        }
        pctbuf[4] = '%';
        pctbuf[5] = 0;
        for (i = 0; pctbuf[i] != 0 && (barw + i) < 80; i++)
        {
                cells[barw + i] = (unsigned short) (ca | (unsigned char) pctbuf[i]);
        }
        puttext(x, y, (unsigned short) (x + barw + i - 1), y, cells);
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
        unsigned short *cells;
        unsigned short ca;
        unsigned int i, len;
        const char *p;

        cells = _conioxlib_need_cells(28);
        if (cells == 0)
        {
                return;
        }

        if (maxv == 0)
        {
                maxv = 1;
        }
        if (value > maxv)
        {
                value = maxv;
        }
        _wscroll = 0;
        ca = (unsigned short) ((unsigned int) _conioxlib_curtheme.atrstatus << 8);
        p = ((title != 0) && (title[0] != 0)) ? title : "";
        len = (unsigned int) strlen(p);
        if (len > 28)
        {
                len = 28;
        }
        for (i = 0; i < len; i++)
        {
                cells[i] = (unsigned short) (ca | (unsigned char) p[i]);
        }
        if (28 > len) _conioxlib_memsetw(cells + len, (unsigned short)(ca | ' '), 28 - len);
        puttext(1, CONIOXLIB_STATUS_ROW, 28, CONIOXLIB_STATUS_ROW, cells);
        conioxlib_progressbar(30, CONIOXLIB_STATUS_ROW, 20, _conioxlib_curtheme.atrstatus, value, maxv);
}

/* --------------------------- Message box dialog -------------------------- */
/* msgbox:
   Prototype:
   unsigned char conioxlib_msgbox(const char *title,
       const char *msg, char buttons[][CONIOXLIB_BTN_W],
       unsigned char nbuttons);
   Shows a message box with up to three buttons.
   Parameters: title is the caption, msg the text, buttons
   the labels and nbuttons their number (1 to 3); passing 0
   as buttons uses the default OK button. Returns as
   conioxlib_buttons() does: the index of the pressed
   button, 254 when F1 was pressed or 255 when cancelled with ESC.
   */
/* Helper for blank-padded button hints (CONIOXLIB_HINT_W = 71). Used by
   msgbox/inputbox/inputmask to avoid repeating the memset+terminator pattern. */
static conioxlib_inline void _conioxlib_blank_hint(char *hint)
{
        memset(hint, 32, CONIOXLIB_HINT_W - 1);
        hint[CONIOXLIB_HINT_W - 1] = 0;
}

unsigned char conioxlib_msgbox (const char *title, const char *msg, char buttons[][CONIOXLIB_BTN_W], unsigned char nbuttons)
{
        unsigned char oldx, oldy;
        char *oldb;
        unsigned int i;
        unsigned char r;
        char defbtn[3][CONIOXLIB_BTN_W];
        char defdesc[3][CONIOXLIB_HINT_W];
        char (*btns)[11];
        char line[64];
        unsigned short x1, y1, x2, y2;
        unsigned int len, bw, k;

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
                        _conioxlib_blank_hint(defdesc[i]);
                }
        }
        len = (msg != 0) ? (unsigned int) strlen(msg) : 0;
        if (len < 24)
        {
                len = 24;
        }
        /* The buttons must fit inside the frame: each one is its visible title + 4 wide and they are 2 apart */
        bw = 0;
        for (i = 0; i < nbuttons; i++)
        {
                for (k = 0; btns[i][k] != 0; k++)
                {
                        if (btns[i][k] != '&')
                        {
                                bw++;
                        }
                }
                bw += 4 + (((i + 1) < nbuttons) ? 2 : 0);
        }
        if (len < bw)
        {
                len = bw;
        }
        if (len > 60)
        {
                len = 60;
        }
        x1 = (unsigned short) (((unsigned int)_conioxlib_scr_w() - (len + 4)) / 2);
        x2 = (unsigned short) (x1 + len + 3);
        y1 = 9;
        y2 = 14;
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
        if (oldb == 0)
        {
                return(255);
        }
        box(x1, y1, x2, y2, _conioxlib_curtheme.atrframe, _conioxlib_curtheme.frame, (char *) ((title != 0) ? title : "Message"), SHADOW);
        textattr(_conioxlib_curtheme.atrdialog);
        if (msg != 0)
        {
                /* A message longer than the frame is cut instead of spilling out of it */
                sprintf(line, "%.*s", (int) len, msg);
                cputsc(x1, x2, y1 + 1, line);
        }
        r = conioxlib_buttons((unsigned short) (x1 + 2), (unsigned short) (y1 + 3), _conioxlib_curtheme.btnatr0, _conioxlib_curtheme.btnatr1, _conioxlib_curtheme.btnatrhot, btns, defdesc, nbuttons, 2);
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
   when accepted with OK, 1 when cancelled, 254 when F1
   was pressed and 255 when cancelled with ESC.
   */
unsigned char conioxlib_inputbox (const char *title, const char *label, char *buf, int len)
{
        unsigned char oldx, oldy;
        char *oldb;
        unsigned char r;
        char btns[2][CONIOXLIB_BTN_W];
        char desc[2][CONIOXLIB_HINT_W];
        char field[40];
        unsigned short x1, y1, x2, y2;

        if ((buf == 0) || (len < 1))
        {
                return(255);
        }
        /* The field must fit inside the 45 column frame */
        if (len > 38)
        {
                len = 38;
        }
        x1 = 18;
        x2 = 62;
        y1 = 9;
        y2 = 15;
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
        oldb = conioxlib_savescr(x1, y1, x2 + 2, y2 + 1);
        if (oldb == 0)
        {
                return(255);
        }
        box(x1, y1, x2, y2, _conioxlib_curtheme.atrframe, _conioxlib_curtheme.frame, (char *) ((title != 0) ? title : "Input"), SHADOW);
        textattr(_conioxlib_curtheme.atrlabel);
        gotoxy(x1 + 2, y1 + 1);
        cputs((label != 0) ? (char *) label : "Value:");
        /* The field edits a copy: the caller's string changes only when OK is pressed */
        strncpy(field, buf, (size_t) len);
        field[(int) len] = 0;
        conioxlib_getstr((unsigned short) (x1 + 2), (unsigned short) (y1 + 2), _conioxlib_curtheme.atrinput, field, len);
        strcpy(btns[0], "OK");
        strcpy(btns[1], "Cancel");
        _conioxlib_blank_hint(desc[0]);
        _conioxlib_blank_hint(desc[1]);
        r = conioxlib_buttons((unsigned short) (x1 + 14), (unsigned short) (y1 + 4), _conioxlib_curtheme.btnatr0, _conioxlib_curtheme.btnatr1, _conioxlib_curtheme.btnatrhot, btns, desc, 2, 2);
        if (r == 0)
        {
                strcpy(buf, field);
        }
        conioxlib_restorescr(x1, y1, x2 + 2, y2 + 1, oldb);
        gotoxy(oldx, oldy);
        return(r);
}

/* -------------- Input box digits-only with minimum length ---------------- */
/* conioxlib_inputmask(): numeric field that refuses invalid characters as they
   are typed: only digits are accepted (BACK erases, ENTER accepts, ESC
   cancels, F1 asks for help) and after OK it checks that at least mindigits
   digits were entered. Returns 0 (OK), 1 (Cancel) or 254 (help). */
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
   cancelled and 254 when F1 was pressed.
   */
unsigned char conioxlib_inputmask (const char *title, const char *label, char *buf, int len, char mindigits)
{
        unsigned char oldx, oldy;
        char *oldb;
        unsigned char r;
        unsigned int key;
        char btns[2][CONIOXLIB_BTN_W];
        char desc[2][CONIOXLIB_HINT_W];
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
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
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
                box(x1, y1, x2, y2, _conioxlib_curtheme.atrframe, _conioxlib_curtheme.frame, (char *) ((title != 0) ? title : "Numeric"), SHADOW);
                textattr(_conioxlib_curtheme.atrlabel);
                gotoxy(x1 + 2, y1 + 1);
                cputs((label != 0) ? (char *) label : "Number:");
                _conioxlib_hint("Only digits; BACK deletes, ENTER accepts");
                _setcursortype(_SOLIDCURSOR);
                /* Initial field paint: only once per dialog iteration, not per key.
                   The previous code did cprintf("%-*.*s", len, len, buf) inside the
                   for(;;) loop, redrawing the whole field on every keystroke when
                   only one char actually changed. textattr is also set once here;
                   it never changes during the input loop. */
                textattr(_conioxlib_curtheme.atrinput);
                gotoxy((unsigned short) (x1 + 2), (unsigned short) (y1 + 2));
                cprintf("%-*.*s", (int) len, (int) len, buf);
                gotoxy((unsigned short) (x1 + 2 + (int) digits), (unsigned short) (y1 + 2));
                /* Input: any key that is not a digit is ignored the very moment it is
                   pressed, so invalid characters can never get in */
                for (;;)
                {
                        while ((key = _conioxlib_pollkey()) == 0)
                        {
                        }
                        if ((key >= '0') && (key <= '9') && ((int) digits < (int) len))
                        {
                                putch((char) key);
                                buf[digits] = (char) key;
                                digits++;
                                buf[digits] = 0;
                        }
                        else if ((key == KBD_BACK) && (digits > 0))
                        {
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
                        /* Same contract as buttons(): 254 tells the caller to show help */
                        r = 254;
                }
                else
                {
                        strcpy(btns[0], "OK");
                        strcpy(btns[1], "Cancel");
                        _conioxlib_blank_hint(desc[0]);
                        _conioxlib_blank_hint(desc[1]);
                        r = conioxlib_buttons((unsigned short) (x1 + 14), (unsigned short) (y1 + 4), _conioxlib_curtheme.btnatr0, _conioxlib_curtheme.btnatr1, _conioxlib_curtheme.btnatrhot, btns, desc, 2, 2);
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
       unsigned char atr1, char items[][CONIOXLIB_LISTITEM_W],
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
unsigned char conioxlib_listbox (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char items[][CONIOXLIB_LISTITEM_W], unsigned char nitems, const char *title)
{
        unsigned char oldx, oldy;
        char *oldb;
        int top, sel, vis, row, repaint, itemw, prevtop, prevsel, i;
        unsigned int key;

        if ((nitems == 0) || (items == 0))
        {
                return(255);
        }
        { struct text_info _ti; gettextinfo(&_ti); oldx = _ti.curx; oldy = _ti.cury; }
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
        itemw = (int) (x2 - x1 - 2);
        if (itemw < 1)
        {
                itemw = 1;
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
                        {
                                unsigned short *cells;
                                unsigned short ca;
                                unsigned int col, len;
                                const char *s;
                                int iw = itemw;

                                if (iw > 40)
                                {
                                        iw = 40;
                                }
                                /* Build the whole list body in one buffer and emit a single puttext
                                   instead of vis calls. iw*vis is at most 40*22=880 cells; the
                                   _conioxlib_need_cells pool grows once and reuses it on every repaint */
                                cells = _conioxlib_need_cells((unsigned int) iw * (unsigned int) vis);
                                if (cells == 0)
                                {
                                        break;
                                }
                                for (row = 0; row < vis; row++)
                                {
                                        unsigned int base = (unsigned int) row * (unsigned int) iw;
                                        ca = (unsigned short) ((unsigned int) ((top + row == sel) ? atr1 : atr0) << 8);
                                        _conioxlib_memsetw(cells + base, (unsigned short)(ca | ' '), (unsigned int) iw);
                                        if (top + row < (int) nitems)
                                        {
                                                s = items[top + row];
                                                len = (unsigned int) strlen(s);
                                                if (len > (unsigned int) iw)
                                                {
                                                        len = (unsigned int) iw;
                                                }
                                                for (col = 0; col < len; col++)
                                                {
                                                        cells[base + col] = (unsigned short) (ca | (unsigned char) s[col]);
                                                }
                                        }
                                }
                                puttext((unsigned short) (x1 + 1), (unsigned short) (y1 + 1), (unsigned short) (x1 + iw), (unsigned short) (y1 + vis), cells);
                        }
                        /* Scroll bar in the last interior column when the list overflows */
                        _conioxlib_draw_scrollbar((unsigned short) (x2 - 1), (unsigned short) (y1 + 1), vis, top, (int) nitems, atr0, 1);
                        repaint = 0;
                }
                prevtop = top;
                prevsel = sel;
                do
                {
                        key = _conioxlib_pollkey();
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
                                                        /* vis == 3 leaves a single track row: divide by 1 instead of by zero */
                                                        top = ((row - 1) * ((int) nitems - vis)) / ((vis > 3) ? (vis - 3) : 1);
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
                        else
                        {
                                /* Accelerator: a printable letter or an Alt+letter scan jumps to
                                   the matching item. Reuses _conioxlib_menu_find_hot so the
                                   listbox shares the menu hot-key matcher; ENTER still picks. */
                                if ((key != 0) && ((key >= 32) || ((key & 0xFF) == 0)))
                                {
                                        i = _conioxlib_menu_find_hot((const char *) items, CONIOXLIB_LISTITEM_W, (int) nitems, key);
                                        if (i >= 0)
                                        {
                                                sel = i;
                                                if (sel < top)
                                                {
                                                        top = sel;
                                                }
                                                else if (sel >= top + vis)
                                                {
                                                        top = sel - vis + 1;
                                                }
                                                key = 1;
                                                repaint = 1;
                                        }
                                }
                                if (key != 1)
                                {
                                        _conioxlib_mouse_nav(&key, KBD_UP, KBD_DOWN);
                                }
                        }

                }
                while ((key != 1) && (key != KBD_UP) && (key != KBD_DOWN) && (key != KBD_PGUP) && (key != KBD_PGDW) && (key != KBD_ORG) && (key != KBD_END) && (key != KBD_ESC) && (key != KBD_ENTER));
                if (_conioxlib_listbox_nav(key, &sel, &top, (int) nitems, vis) == 0)
                {
                        /* non-nav key handled above (ENTER/ESC/click) */
                }
                if (top != prevtop)
                {
                        repaint = 1;
                }
                else if ((repaint == 0) && (sel != prevsel) && (key != KBD_ENTER) && (key != KBD_ESC))
                {
                        /* Same page: only the row that loses the highlight and the one that gets it change */
                        for (row = 0; row < 2; row++)
                        {
                                i = (row == 0) ? prevsel : sel;
                                gotoxy(x1 + 1, y1 + 1 + i - top);
                                textattr((i == sel) ? atr1 : atr0);
                                cprintf("%-*.*s", itemw, itemw, items[i]);
                        }
                }
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
