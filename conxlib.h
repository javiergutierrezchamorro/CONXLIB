#pragma once
#ifndef _CONIOXLIB_H_
#define _CONIOXLIB_H_

#include "coniox.h"

#define conioxlib_version (215)
#define conioxlib_build (__DATE__)
#define conioxlib_buildtime (__TIME__)

/* Memory model in use: DOS flat builds (Watcom 32 bit with PMODE/W,
   DOS/4G, DOS/32A...) report "Flat 32-bit"; 16 bit models report their
   own name */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        #define conioxlib_model "Flat 32-bit (Win32)"
#elif defined(__FLAT__)
        #define conioxlib_model "Flat 32-bit"
#elif defined(__DJGPP__)
        #define conioxlib_model "Flat 32-bit (DPMI)"
#elif defined(__HUGE__)
        #define conioxlib_model "Huge 16-bit"
#elif defined(__LARGE__)
        #define conioxlib_model "Large 16-bit"
#elif defined(__COMPACT__)
        #define conioxlib_model "Compact 16-bit"
#elif defined(__MEDIUM__)
        #define conioxlib_model "Medium 16-bit"
#elif defined(__SMALL__)
        #define conioxlib_model "Small 16-bit"
#else
        #define conioxlib_model "Unknown model"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Box characters in CP437: defined by value so sources compile the same on DOS and Windows with any encoding */
#define CONIOXLIB_BOX_A_NONE    "       "
#define CONIOXLIB_BOX_A_BLOCK   "\xDB\xDB\xDF\xDC\xDB\xDB\xDB"
#define CONIOXLIB_BOX_A_SINGLE  "\xDA\xBF\xC4\xC4\xB3\xC0\xD9"
#define CONIOXLIB_BOX_A_DOUBLE  "\xC9\xBB\xCD\xCD\xBA\xC8\xBC"
#define CONIOXLIB_BOX_U_SINGLE  "\xD6\xB7\xC4\xC4\xBA\xD3\xBD"
#define CONIOXLIB_BOX_S_DOUBLE  CONIOXLIB_BOX_U_SINGLE
#define CONIOXLIB_BOX_U_DOUBLE  "\xD5\xB8\xCD\xCD\xB3\xD4\xBE"
#define CONIOXLIB_BOX_S_SINGLE  CONIOXLIB_BOX_U_DOUBLE

/* Author name shown in the About box and in the farewell screen; the
   accented e is written as its CP437 byte (\x82) so sources compile
   the same on DOS and Windows with any encoding */
#define CONIOXLIB_AUTHOR        "Javier Guti\x82rrez Chamorro (Guti)"

/* box() flags: combine with | */
#define CONIOXLIB_BOX_SHADOW    1
#define CONIOXLIB_BOX_CLOSE     2
#define CONIOXLIB_BOX_NOWINDOW  4
/* CONIOXLIB_BOX_ZOOM: animate the frame in 8..12 steps before drawing the
   final window. The origin is the center of the rectangle, unless a
   menu()/menubar() pick has just registered an item position, in which case
   the box grows from that item ("open from the menu bar" combo). Any key
   press aborts the animation. No effect on the final painted window. */
#define CONIOXLIB_BOX_ZOOM      8

/* Portable key codes built on coniox KEY_* (scan codes). Extended keys arrive as (scan << 8). */
#define KBD_UP    ((KEY_UP) << 8)
#define KBD_DOWN  ((KEY_DOWN) << 8)
#define KBD_LEFT  ((KEY_LEFT) << 8)
#define KBD_RIGHT ((KEY_RIGHT) << 8)
#define KBD_PGUP  ((KEY_PGUP) << 8)
#define KBD_PGDW  ((KEY_PGDN) << 8)
#define KBD_ORG   ((KEY_HOME) << 8)
#define KBD_END   ((KEY_END) << 8)
#define KBD_ENTER 0x0D
#define KBD_ESC   KEY_ESC
#define KBD_BACK  0x08
#define KBD_TAB   0x09
#define KBD_STAB  0x0F00
#define KBD_SPACE 0x20
#define KBD_INS   ((KEY_INSERT) << 8)
#define KBD_F1    ((KEY_F1) << 8)
#define KBD_F2    ((KEY_F2) << 8)
#define KBD_F3    ((KEY_F3) << 8)
#define KBD_F4    ((KEY_F4) << 8)
#define KBD_F5    ((KEY_F5) << 8)
#define KBD_F6    ((KEY_F6) << 8)
#define KBD_F7    ((KEY_F7) << 8)
#define KBD_F8    ((KEY_F8) << 8)
#define KBD_F9    ((KEY_F9) << 8)
#define KBD_F10   ((KEY_F10) << 8)


/* Attribute helpers using CONIOX color names */
#define CONIOXLIB_ATTR(fg, bg)  ((unsigned int)(((bg) << 4) | (fg)))

/* Large static buffers are declared far under 16 bit DOS compilers (Watcom
   large model, Borland) so they do not overflow the 64 KB DGROUP segment;
   the macro is empty everywhere else */
#if ((__DOS__) || (__MSDOS__)) && !defined(__FLAT__) && !defined(__DJGPP__) && (defined(__WATCOMC__) || defined(__TURBOC__))
#define CONIOXLIB_FARDATA       far
#else
#define CONIOXLIB_FARDATA
#endif

/* --------------------------------------------------------------------------
   Layout and capacity limits (single source of truth)
   --------------------------------------------------------------------------
   Default geometry is the classic 80x25 text mode. On Windows, helpers may
   read the live console size via gettextinfo() (ti.screenwidth/height) when
   the buffer is larger; status/clock still assume the classic strip layout
   unless the host reports a different height.

   Fixed table widths in the public API:
     CONIOXLIB_ITEM_W      menu / option / property / menubar item text
     CONIOXLIB_HINT_W      status-line hint / description
     CONIOXLIB_BTN_W       button label
     CONIOXLIB_LISTITEM_W  listbox / file long-name column
     CONIOXLIB_HELP_W      help/viewer line (60 usable + NUL)
     CONIOXLIB_GRID_W      grid() cell text
     CONIOXLIB_MENU_MAX    maximum entries in menu()/menubar()/checklist
     CONIOXLIB_MAXFILES    maximum rows in filemultiselector() table
   Exceeding MENU_MAX clamps; exceeding MAXFILES truncates the directory scan.
   -------------------------------------------------------------------------- */
#define CONIOXLIB_SCR_W                 80
#define CONIOXLIB_SCR_H                 25
#define CONIOXLIB_STATUS_COLS   59
#define CONIOXLIB_STATUS_ROW    25
#define CONIOXLIB_TITLE_ROW             1
#define CONIOXLIB_LOCKS_COL             60
#define CONIOXLIB_LOCKS_END             74
#define CONIOXLIB_CLOCK_COL             76
#define CONIOXLIB_CLOCK_END             80

#define CONIOXLIB_ITEM_W                21
#define CONIOXLIB_HINT_W                71
#define CONIOXLIB_BTN_W                 11
#define CONIOXLIB_LISTITEM_W    41
#define CONIOXLIB_HELP_W                61
#define CONIOXLIB_GRID_W                16
#define CONIOXLIB_MENU_MAX              32
#define CONIOXLIB_KEYQUEUE              16

/* Entries of the files table expected by conioxlib_filemultiselector().
   The caller must provide at least CONIOXLIB_MAXFILES rows. The directory
   scan stops when the table is full. */
#define CONIOXLIB_MAXFILES              220

/* Sort column IDs for conioxlib_filemultiselector() */
#define CONIOXLIB_SORT_NAME     0
#define CONIOXLIB_SORT_SIZE     1
#define CONIOXLIB_SORT_DATE     2
#define CONIOXLIB_SORT_TIME     3
/* Sort direction flags (combine with column ID via |) */
#define CONIOXLIB_SORT_DESC     0x80


struct conioxlib_smousestatus
{
        unsigned int x;
        unsigned int y;
        int wheelx;
        int wheely;
        unsigned int lclick;
        unsigned int rclick;
};


struct conioxlib_sfiles
{
        unsigned char selected;
        char name[13];
        char size[15];
        char date[9];
        char time[6];
        char comment[34];
};

struct conioxlib_scountry
{
        unsigned int date;                                      /* Date format */
        char curr[5];               /* Currency symbol */
        char thsep[2];              /* Thousands separator */
        char dsep[2];               /* Decimal separator */
        char dtsep[2];              /* Date separator */
        char tmsep[2];              /* Time separator */
        unsigned char currstyle[2]; /* Currency style */
        unsigned char digits;       /* Significant currency digits */
        unsigned char time;         /* Time format */
        unsigned long int convcase; /* Uppercase conversion */
        unsigned char dasep;        /* Data separator */
        char reserved[10];          /* Reserved */
        unsigned int code;          /* Code page */
};


/* Extended free disk space: filled by conioxlib_getdiskfree_ext() */
struct conioxlib_sdiskfree
{
        unsigned int size;                  /* Size of this structure */
        unsigned int version;               /* Structure version */
        unsigned long int sectors_per_cluster;
        unsigned long int bytes_per_sector;
        unsigned long int avail_clusters;   /* Free clusters */
        unsigned long int total_clusters;   /* Total clusters */
};



/* --------------------------------------------------------------------------
   Coordinate model
   --------------------------------------------------------------------------
   Almost every widget takes absolute 1-based screen coordinates and ignores
   the active text window set by coniox window(). That is intentional: the UI
   is full-screen (menus, dialogs, file selector, status line on row
   CONIOXLIB_STATUS_ROW). window()/gotoxy() relative addressing remains
   available for application text output, but box/menu/listbox/msgbox/...
   always paint in screen space. Effects (push/bounce/wipe) also operate on
   the full console via movetext/gettext/puttext of absolute rectangles.
   -------------------------------------------------------------------------- */


/* --------------------------------------------------------------------------
   Help file format (.HLP)
   --------------------------------------------------------------------------
   Plain text, any line ending (CR, LF or CRLF). Topics are:

     @topic_name
     body line 1
     body line 2
     ...
     @next_topic

   - A line whose first character is '@' starts a topic; the rest of the line
     (trimmed of CR/LF) is the topic name (no spaces required after '@').
   - Body lines follow until the next '@' line or end of file.
   - help() looks up topic_name in the offset cache and fseek's to the body.
   - helpindex() lists every @ name. The cache is rebuilt when the file size
     or, when available, its last-write time changes.
   -------------------------------------------------------------------------- */

extern struct conioxlib_smousestatus conioxlib_mousestatus;

/* After pollmouse(), keys may sit in the internal queue; use these instead of raw kbhit/getch. */
unsigned int conioxlib_getkey(void);
int conioxlib_kbhit(void);


void conioxlib_pollmouse(void);
/* Absolute screen coords (1-based); ignores active window(). flags: SHADOW|CLOSE|NOWINDOW.
   Title is centered into the top-border buffer (single puttext with the frame). */
void conioxlib_box(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned int atr, const char *frame, const char *title, unsigned int flags);
void conioxlib_cputsc(unsigned short x1, unsigned short x2, unsigned short y, const char *str);
void conioxlib_blocks(void);
void conioxlib_clock(void);

void conioxlib_getcountry(struct conioxlib_scountry *);
/* getdiskfree_ext(): drive 0 = current, 1 = A, 2 = B... Fills df; returns 0 on
   success, 1 on error. DOS uses int 21h AX=7303h (FAT32) with AH=36h fallback. */
unsigned char conioxlib_getdiskfree_ext(unsigned char drive, struct conioxlib_sdiskfree *df);
/* filesize(): file length in bytes, 0xFFFFFFFFUL when it cannot be opened/read */
unsigned long int conioxlib_filesize(const char *path);
void conioxlib_setpal(unsigned char);
/* vgaremap(): DOS only. With 1 remaps font glyphs 0x01 (mouse pointer), 0x10/0x11
   (solid triangles) and 0x18/0x19 (chunky arrows) of the 8x16 VGA font, hides the
   driver pointer and enables the library-drawn mouse pointer; with 0 restores the
   driver pointer. No-op on Windows and 32-bit Watcom. */
void conioxlib_vgaremap(unsigned char on);
void conioxlib_push(void);
void conioxlib_bounce(void);
void conioxlib_wipe(void);
unsigned int conioxlib_ckey(void);
void conioxlib_getstr(unsigned short x, unsigned short y, unsigned char atr, char *str, char len);
void conioxlib_getpw(unsigned short x, unsigned short y, unsigned char atr, char *pass, char len);
unsigned char conioxlib_grid(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, const char items[][CONIOXLIB_GRID_W], unsigned char noptions);
/* Writes s at (x,y) with the atr color; the accelerator key (marked with an
   ampersand or the first letter) is painted with the atrhot foreground over its own background. */
void conioxlib_cputs_hot(unsigned short x, unsigned short y, unsigned char atr, unsigned char atrhot, const char *s);
/* Button in Turbo Vision 2.0 style (TButton::drawState()): a face of colored spaces
   with the centered title (in the gray Borland dialogs: green background, black
   text and yellow accelerator; white text when selected), top-right corner
   0xDC and a bottom row of two spaces plus the 0xDF strip up to the last column
   (shadow attribute 0x70), no frame characters. The width of each button is the
   visible title length + 4; the "separation" parameter is the horizontal gap
   between buttons. atrhot provides only the foreground color of the accelerator;
   the background is always the one of the button. Returns the index of the
   pressed button, 254 on F1 (help) or 255 on ESC (cancel).
   Prefer the active theme button colors (see conioxlib_theme_btnattrs). */
unsigned char conioxlib_buttons(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_BTN_W], const char desc[][CONIOXLIB_HINT_W], unsigned char noptions, unsigned char separation);
/* Item whose first character is '-' is a non-selectable separator (drawn as a line). */
#define CONIOXLIB_SEPARATOR     "-"
/* menu()/menubar() reserved return codes (valid item indices are 0..noptions-1) */
#define CONIOXLIB_MENU_RIGHT    252
#define CONIOXLIB_MENU_LEFT     253
#define CONIOXLIB_MENU_HELP     254
#define CONIOXLIB_MENU_ESC      255
/* menu(): items whose first character is '-' are non-selectable separators.
   Every row is drawn full-width (the widest visible item, up to ITEM_W-1 columns),
   so the highlight bar covers the whole width and redrawing leaves no debris. If
   the items do not fit between y1 and row (SCR_H-1) a scrollbar is drawn to the
   right (0x18/0x19 arrows, 0xB0 track, 0xB2 thumb) and the list pages with
   PGUP/PGDN, the mouse wheel or a click on the bar. sel selects the initial item.
   noptions is clamped to CONIOXLIB_MENU_MAX (32). */
unsigned char conioxlib_menu(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], unsigned char noptions, unsigned char separation, unsigned char sel);
/* Top menu bar (Borland-style). Returns selected index, CONIOXLIB_MENU_ESC/HELP on cancel.
   init sets the initially highlighted item (clamped to noptions-1).
   noptions is clamped to CONIOXLIB_MENU_MAX (32). */
unsigned char conioxlib_menubar(unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], unsigned char noptions, unsigned char init);
/* option(): multi-select checkboxes. sel[i] is ' ' or '\xFE'.
   property(): radio group (only one '\xFE' at a time). Both share the same
   internal checklist controller; noptions is clamped to CONIOXLIB_MENU_MAX. */
unsigned char conioxlib_option(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled);
unsigned char conioxlib_property(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, unsigned char atrhot, const char items[][CONIOXLIB_ITEM_W], const char desc[][CONIOXLIB_HINT_W], char *sel, unsigned char noptions, unsigned char separation, unsigned char enabled);
void conioxlib_background(const unsigned char *msg);
void conioxlib_hello(const char *name, unsigned char commercial);
void conioxlib_bye(const char *name, unsigned char e, unsigned char commercial);
void conioxlib_about(unsigned char);
/* conioxlib_compilerstring(): name and version of the compiler used to build, e.g. "Open Watcom C/C++ 2.0 beta"; shown in the splash and the About box */
const char * conioxlib_compilerstring(void);
unsigned char conioxlib_exittodos(const char *name);
void conioxlib_screensaver(void);
void conioxlib_shell(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, const char *command);
/* conioxlib_calendar(): interactive month view; month is 1 = January .. 12 = December, 0 uses the current month, and day, month and year receive the date chosen with the cursors, PGUP/PGDW (month), HOME/END (year) or the mouse; returns 1 when ENTER picks a day and 0 when ESC cancels */
unsigned char conioxlib_calendar(unsigned short x, unsigned short y, unsigned char atr0, unsigned char atr1, unsigned char atr2, unsigned char *day, unsigned char *month, unsigned int *year);
unsigned char conioxlib_view(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char buffer[][CONIOXLIB_HELP_W], unsigned char last, const char *title);
unsigned char conioxlib_help(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char *name, const char *topic, char buffer[][CONIOXLIB_HELP_W]);
unsigned char conioxlib_helpindex(unsigned short x1, unsigned short y1, unsigned char atr0, unsigned char atr1, const char *name, char buffer[][32]);
unsigned char conioxlib_filemultiselector(unsigned char *sdisk, char *sdir, struct conioxlib_sfiles *files);
/* conioxlib_fileselector(): compact single file picker (drives, directories and names only) for the Open and Save as dialogs; returns 1 when a file is chosen and 0 when cancelled. The list shows long file names when the system has them (Windows, and DOS 7 / Windows 9x with LFN under OpenWatcom) but the name returned is always the short 8.3 one, so it can be opened on any system. filemultiselector() works the same way: its list widens the name column when there are long names and the table keeps the short names */
unsigned char conioxlib_fileselector(unsigned char *sdisk, char *sdir, char *name);

/* Theme package:
   0=Borland C++ 3.1, 1=Turbo C 2.0, 2=Turbo C 1.x, 3=Norton Utilities 9,
   4=PC Tools, 5=Norton Commander, 6=MS-DOS Edit, 7=Lotus 1-2-3,
   8=Borland SideKick, 9=Volkov Commander, 10=DOS Navigator, 11=XTree Gold */
#define CONIOXLIB_THEMES        12
struct conioxlib_stheme
{
        unsigned char atr0;        /* menu bar / list normal text */
        unsigned char atr1;        /* menu bar / list selected (highlighted) */
        unsigned char atrhot;      /* accelerator (hotkey) character in menus */
        unsigned char atrstatus;   /* status line (row 25) text */
        unsigned char atrdialog;   /* dialog frame / unselected text */
        /* Per-theme Turbo Vision button colors (face / selected / accelerator).
           conioxlib_theme_btnattrs() returns these instead of a fixed green. */
        unsigned char btnatr0;     /* button normal face */
        unsigned char btnatr1;     /* button selected (focused) face */
        unsigned char btnatrhot;   /* button accelerator (hotkey) character */
        /* Extended TV palette fields. Used by inputline, label and dialog interior.
           Defaults match the Borland Turbo Vision 2.0 cpColor entries so a theme
           that only sets the basic 9 fields still renders correctly. */
        unsigned char atrinput;    /* input line passive (White on Blue in TV) */
        unsigned char atrinputsel;  /* input line selected/focused (White on Green) */
        unsigned char atrlabel;    /* label normal text (Black on LightGray) */
        unsigned char atrlabelhot;  /* label accelerator (Yellow on LightGray) */
        unsigned char atrinterior;  /* dialog interior / passive frame (LightGray bg) */
        unsigned char atrframe;    /* dialog frame active (White on LightGray) */
        const char *frame;          /* box frame string (CONIOXLIB_BOX_A_*) */
        char name[20];
};
void conioxlib_theme(unsigned char id);
void conioxlib_gettheme(struct conioxlib_stheme *t);
/* Fills atr0/atr1/atrhot with the button colors of the active theme
   (btnatr0/btnatr1/btnatrhot). Each theme owns its triple, so a Norton
   Commander dialog no longer paints Borland green buttons. Dialogs should
   prefer this over hard-coded CONIOXLIB_ATTR(BLACK, GREEN) triples. */
void conioxlib_theme_btnattrs(unsigned char *atr0, unsigned char *atr1, unsigned char *atrhot);

/* Safer screen region save/restore (malloc inside; restorescr frees) */
char *conioxlib_savescr(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2);
void conioxlib_restorescr(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *buf);

void conioxlib_status(const char *msg);
void conioxlib_tooltip(unsigned short x, unsigned short y, const char *msg, unsigned int ms);
void conioxlib_progressbar(unsigned short x, unsigned short y, unsigned short width, unsigned char atr, unsigned int value, unsigned int maxv);
void conioxlib_progress(const char *title, unsigned int value, unsigned int maxv);
unsigned char conioxlib_msgbox(const char *title, const char *msg, char buttons[][CONIOXLIB_BTN_W], unsigned char nbuttons);
/* len is int (not char) so a 160-byte buffer can be passed without sign
   truncation on compilers where `char` is signed (Watcom/Borland/Turbo C). */
unsigned char conioxlib_inputbox(const char *title, const char *label, char *buf, int len);
unsigned char conioxlib_inputmask(const char *title, const char *label, char *buf, int len, char mindigits);
unsigned char conioxlib_listbox(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char items[][CONIOXLIB_LISTITEM_W], unsigned char nitems, const char *title);

#if ((__DOS__) || (__MSDOS__)) && defined(__TURBOC__)
void interrupt error(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
void interrupt conioxlib_shell_patch(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
#endif

/* Compatibility aliases with the original CONXLIB short names */
#define textsize(x1, y1, x2, y2)        gettextsize(x1, y1, x2, y2)
#define cputsc          conioxlib_cputsc
#define box             conioxlib_box
#define conioxlib_watch         conioxlib_clock
#define blocks          conioxlib_blocks
#define A_NONE          CONIOXLIB_BOX_A_NONE
#define A_BLOCK         CONIOXLIB_BOX_A_BLOCK
#define A_SINGLE        CONIOXLIB_BOX_A_SINGLE
#define A_DOUBLE        CONIOXLIB_BOX_A_DOUBLE
#define U_SINGLE        CONIOXLIB_BOX_U_SINGLE
#define S_DOUBLE        CONIOXLIB_BOX_S_DOUBLE
#define U_DOUBLE        CONIOXLIB_BOX_U_DOUBLE
#define S_SINGLE        CONIOXLIB_BOX_S_SINGLE
#define SHADOW          CONIOXLIB_BOX_SHADOW
#define CLOSE           CONIOXLIB_BOX_CLOSE
#define NOWINDOW        CONIOXLIB_BOX_NOWINDOW
#define ZOOM            CONIOXLIB_BOX_ZOOM
#define sfiles          conioxlib_sfiles
#define scountry        conioxlib_scountry

#ifdef __cplusplus
}
#endif

#endif
