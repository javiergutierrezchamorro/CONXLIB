/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        #include <windows.h>    /* _demo_edit_shift() polls GetAsyncKeyState() */
#endif
#include "coniox.h"
#include "conxlib.h"
/* Demo windows animate their frame with the zoom effect (CONIOXLIB_BOX_ZOOM);
   the library's own boxes (bye / exittodos / shell) keep the plain SHADOW. */
#undef SHADOW
#define SHADOW (CONIOXLIB_BOX_SHADOW | CONIOXLIB_BOX_ZOOM)
/* ----------------------------------------------------------------------------------------------------------------- */
#define DEMO_OPTIONS    28
#define DEMO_TITLE      "CONXLIB DEMO 2.0               (c) Copyright 1997-2026 Javier Gutierrez Chamorro"
#define DEMO_HELP       "CONXDEMO.HLP"
#define DEMO_HOT        CONIOXLIB_ATTR(RED, LIGHTGRAY)
#define DEMO_FILES      CONIOXLIB_MAXFILES
#define DEMO_HELPBUF    (256 * 61)
#if ((__DOS__) || (__MSDOS__)) && !defined(__DJGPP__)
        #if defined(__TURBOC__)
                extern unsigned _stklen = 16384U;
        #elif defined(__WATCOMC__)
                unsigned __stack = 16384U;
        #endif
#endif
static const char *_demooptions[DEMO_OPTIONS] = {       "&Exit",
                                                                                        "&Windows",
                                                                                        "&Mouse info",
                                                                                        "&Text input",
                                                                                        "&Checkboxes",
                                                                                        "&Radio buttons",
                                                                                        "&Grid table",
                                                                                        "C&alendar",
                                                                                        "&File selector",
                                                                                        "File &picker",
                                                                                        "&Viewer",
                                                                                        "&Screensaver",
                                                                                        "&Effects",
                                                                                        "&Buttons bar",
                                                                                        "&DOS shell",
                                                                                        "&Palette",
                                                                                        "Color attr&s",
                                                                                        "Menu ba&r",
                                                                                        "A&bout",
                                                                                        "&Msgbox",
                                                                                        "&Input box",
                                                                                        "Input &mask",
                                                                                        "&Listbox",
                                                                                        "&Progress",
                                                                                        "&Themes",
                                                                                        "&Editor",
                                                                                        "S&tatus/tooltip",
                                                                                        "&Tetris"};
static const char *_demohints[DEMO_OPTIONS] = { "Leave the demo: conioxlib_exittodos() confirms and bye restores       ",
                                                                                        "All box() frame styles: double, single, rounded, solid                ",
                                                                                        "Live conioxlib_mousestatus: position, buttons and wheel               ",
                                                                                        "conioxlib_getstr() and getpw(): field editors with cursor keys        ",
                                                                                        "conioxlib_option(): toggle marks with Space or Ins keys               ",
                                                                                        "conioxlib_property(): only one of the marks stays active              ",
                                                                                        "conioxlib_grid(): grid picker returning the selected cell             ",
                                                                                        "conioxlib_calendar(): interactive month view returning the date       ",
                                                                                        "filemultiselector(): drives, folders, files and marked picks          ",
                                                                                        "conioxlib_fileselector(): compact single file picker                  ",
                                                                                        "conioxlib_view(): scrollable text window with a scrollbar             ",
                                                                                        "conioxlib_screensaver(): random effects until a key                   ",
                                                                                        "push, bounce and wipe screen effects                                  ",
                                                                                        "conioxlib_buttons(): horizontal button bar with hints                 ",
                                                                                        "conioxlib_shell(): spawns COMMAND.COM or CMD.EXE                      ",
                                                                                        "conioxlib_setpal(): DAC palette; conioxlib_vgaremap(): font glyphs    ",
                                                                                        "CONIOXLIB_ATTR and the 16 CONIOX COLORS foreground/background         ",
                                                                                        "Borland-style top bar + dropdown with separators                      ",
                                                                                        "getcountry() data, versions, builds and credits                       ",
                                                                                        "conioxlib_msgbox(): alert with 1-3 buttons                            ",
                                                                                        "conioxlib_inputbox(): label + text field + OK/Cancel                  ",
                                                                                        "conioxlib_inputmask(): digits-only field with minimum length           ",
                                                                                        "conioxlib_listbox(): scrollable list with wheel support               ",
                                                                                        "conioxlib_progress / progressbar animated demo                        ",
                                                                                        "conioxlib_theme(): Borland, Norton, PC Tools, Edit, Lotus             ",
                                                                                        "_demo_edit(): Borland-style editor moved from the library              ",
                                                                                        "conioxlib_status and tooltip without touching clock/locks             ",
                                                                                        "_demo_tetris(): falling blocks game in a boxed window                  "};
static char _optmenu[5][21] = { "&Icons",
                                                                "&Sound",
                                                                "&Colors",
                                                                "&Mouse",
                                                                "&Keys"};
static char _optdesc[5][71] = { "Toggle it with Space or Ins, Enter returns                ",
                                                                "Use TAB, arrows or hotkeys to move along items            ",
                                                                "Left click marks the item under the cursor                ",
                                                                "F1 opens help; ESC returns without changes                ",
                                                                "Radio buttons keep a single active option                 "};
static char _tabmenu[12][16] = {        "File",
                                                                "Edit",
                                                                "Search",
                                                                "Run",
                                                                "Compile",
                                                                "Debug",
                                                                "Project",
                                                                "Options",
                                                                "Window",
                                                                "Help",
                                                                "Tools",
                                                                "Track"};
static char _btnmenu[4][11] = { "OK",
                                                                "Cancel",
                                                                "Help",
                                                                "Apply"};
static char _btndesc[4][71] = { "Accept the current selection and close                   ",
                                                                "Discard changes and return                               ",
                                                                "Open context help for this dialog                        ",
                                                                "Apply changes without closing the dialog                 "};
static const char *_demotext[] = {      "CONXLIB 2.0 - Retro console library over CONIOX 5.03",
                                                        "",
                                                        "Accelerators: mark a letter with & in the item text.",
                                                        "Press that letter to activate the option. atrhot sets",
                                                        "the colour of the hot letter (passed to menu/buttons).",
                                                        "",
                                                        "Works with Borland Turbo C, Watcom C (16 and 32 bits),",
                                                        "DJGPP, and Microsoft Visual C++ for Windows 32/64 bits.",
                                                        0};
/* Allocates a rows*w items table on the heap copying the rows strings of list */
static char *_demo_newtable(const char **list, unsigned int rows, unsigned int w)
{
        char *table;
        unsigned int i;

        table = (char *) malloc((size_t) (rows * w));
        if (table != 0)
        {
                for (i = 0; i < rows; i++)
                {
                        strncpy(table + (i * w), list[i], w - 1);
                        table[(i * w) + (w - 1)] = 0;
                }
        }
        return(table);
}
/* Copies item into out skipping the & accelerator marks: returns out */
static char *_demo_plainitem(const char *item, char *out)
{
        char *o = out;

        while (*item != 0)
        {
                if (*item != '&')
                {
                        *o++ = *item;
                }
                item++;
        }
        *o = 0;
        return(out);
}
/* Visible length of an item with '&' accelerators (does not count the '&') */
static int _demo_vislen(const char *s)
{
        int n = 0;
        while (*s != 0)
        {
                if (*s != '&')
                {
                        n++;
                }
                s++;
        }
        return(n);
}
/* ------------------ Clears the output window ------------------------- */
static void _demo_clear_output(void)
{
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        window(39, 4, 76, 22);
        clrscr();
        window(1, 1, 80, 25);
}
/* --------------------- Shows the box() styles --------------------- */
static void _demo_windows(void)
{
        box(39, 4, 76, 7, CONIOXLIB_ATTR(BLACK, MAGENTA), A_DOUBLE, "A_DOUBLE", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, MAGENTA));
        cputsc(39, 76, 5, "Double line frame");
        box(39, 8, 76, 11, CONIOXLIB_ATTR(BLACK, RED), A_SINGLE, "A_SINGLE", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, RED));
        cputsc(39, 76, 9, "Single line frame");
        box(39, 12, 76, 15, CONIOXLIB_ATTR(BLACK, GREEN), U_DOUBLE, "U_DOUBLE", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, GREEN));
        cputsc(39, 76, 13, "Rounded double frame");
        box(39, 16, 76, 19, CONIOXLIB_ATTR(BLACK, BROWN), U_SINGLE, "U_SINGLE", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, BROWN));
        cputsc(39, 76, 17, "Rounded single frame");
        box(39, 20, 76, 23, CONIOXLIB_ATTR(BLACK, BLUE), A_BLOCK, "A_BLOCK", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, BLUE));
        cputsc(39, 76, 21, "Solid block frame");
        conioxlib_ckey();
}
/* ---------------- Live mouse state with pollmouse() --------------- */
static void _demo_mouse(void)
{
        box(39, 4, 76, 18, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Mouse", SHADOW);
        window(41, 5, 74, 16);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        do
        {
                conioxlib_pollmouse();
                gotoxy(1, 1);
                cprintf("x:      %03u\r\n", conioxlib_mousestatus.x);
                cprintf("y:      %03u\r\n", conioxlib_mousestatus.y);
                cprintf("lclick: %01u  (1=click 2=double)\r\n", conioxlib_mousestatus.lclick);
                cprintf("rclick: %01u\r\n", conioxlib_mousestatus.rclick);
                cprintf("wheelx: %d  (horizontal)\r\n", conioxlib_mousestatus.wheelx);
                cprintf("wheely: %d  (vertical)\r\n", conioxlib_mousestatus.wheely);
                cprintf("\r\nMove, click and scroll the wheel.\r\n");
                cprintf("DOS needs a CuteMouse-compatible driver.\r\n");
                cprintf("Press any key to return.");
                conioxlib_blocks();
                conioxlib_clock();
                if (!conioxlib_kbhit())
                {
                        delay(20);
                }
        }
        while (!conioxlib_kbhit());
        conioxlib_getkey();
        window(1, 1, 80, 25);
}
/* -------------- Text input with getstr() and getpw() ----------------- */
static void _demo_textinput(void)
{
        char str[80];
        char pass[16];
        box(39, 6, 76, 18, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Text input", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 8);
        cputs("conioxlib_getstr():");
        strcpy(str, "CONXLIB");
        conioxlib_getstr(41, 9, CONIOXLIB_ATTR(WHITE, BLUE), str, 20);
        gotoxy(41, 11);
        cputs("conioxlib_getpw():");
        pass[0] = 0;
        conioxlib_getpw(41, 12, CONIOXLIB_ATTR(WHITE, BLUE), pass, 8);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 14);
        cprintf("String: [%s]", str);
        gotoxy(41, 15);
        cprintf("Password: [%s]", pass);
        conioxlib_ckey();
}
/* ------------------ Check boxes with option() --------------------- */
static void _demo_checkboxes(void)
{
        unsigned char sel[5];
        box(39, 5, 76, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "option()", SHADOW);
        memset(sel, ' ', 5);
        conioxlib_option(41, 7, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, RED), DEMO_HOT, _optmenu, _optdesc, (char *) sel, 5, 1, 1);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 13);
        cprintf("Marks: [%c] [%c] [%c] [%c] [%c]", sel[0], sel[1], sel[2], sel[3], sel[4]);
        conioxlib_ckey();
}
/* ------------------ Radio buttons with property() --------------------- */
static void _demo_radiobuttons(void)
{
        unsigned char propsel[5];
        char plain[21];
        unsigned int i;
        box(39, 5, 76, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "property()", SHADOW);
        memset(propsel, ' ', 5);
        propsel[0] = '\xFE';
        conioxlib_property(41, 7, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, RED), DEMO_HOT, _optmenu, _optdesc, (char *) propsel, 5, 1, 1);
        for (i = 0; (i < 5) && (propsel[i] != '\xFE'); i++);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 13);
        if (i < 5)
        {
                cprintf("Choice: %s", _demo_plainitem(_optmenu[i], plain));
        }
        conioxlib_ckey();
}
/* -------------------- Cell grid with grid() ------------------------- */
static void _demo_grid(void)
{
        unsigned int i;
        box(9, 6, 73, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "grid()", SHADOW);
        i = conioxlib_grid(11, 8, CONIOXLIB_ATTR(BLACK, BROWN), CONIOXLIB_ATTR(WHITE, RED), _tabmenu, 12);
        if (i != 255)
        {
                textattr(CONIOXLIB_ATTR(WHITE, BROWN));
                cputsc(10, 72, 13, _tabmenu[i]);
        }
        conioxlib_ckey();
}
/* ---------------------- Calendar with calendar() ------------------------ */
static void _demo_calendar(void)
{
        unsigned char day, month;
        unsigned int year;
        day = 0;
        month = 0;
        year = 0;
        if (conioxlib_calendar(21, 5, CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(LIGHTRED, CYAN), &day, &month, &year) != 0)
        {
                box(31, 8, 50, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Date", SHADOW);
                textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
                gotoxy(33, 10);
                cprintf("Selected: %02u-%02u-%04u", day, month, year);
                gotoxy(33, 11);
                cputs("Press a key to close");
                conioxlib_ckey();
        }
}
/* ----------- File, drive and file size picker ------------- */
static void _demo_fileselector(void)
{
        char dir[80];
        /* str holds "X:\sub1\sub2\...\filename.ext" which can exceed 80 chars when dir is near its limit */
        char str[96];
        unsigned char disk;
        unsigned int i;
        struct conioxlib_sfiles *files;
        struct conioxlib_sdiskfree df;
        unsigned long int fsz;
        files = (struct conioxlib_sfiles *) malloc(DEMO_FILES * sizeof(struct conioxlib_sfiles));
        if (files == 0)
        {
                return;
        }
        strcpy(dir, "\\");
        disk = 'C';
        if (conioxlib_filemultiselector(&disk, dir, files) == 0)
        {
                box(20, 7, 60, 16, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Selection", SHADOW);
                sprintf(str, "%c:%s", disk, dir);
                textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
                cputsc(20, 60, 9, str);
                i = 0;
                while ((i < DEMO_FILES) && ((files[i].selected == 0) || (files[i].size[0] == '\xF0')))
                {
                        i++;
                }
                gotoxy(22, 11);
                if (i < DEMO_FILES)
                {
                        cprintf("Marked: %s", files[i].name);
                }
                else
                {
                        cputs("Marked: <none>");
                }
                /* Free space of the chosen drive with getdiskfree_ext() */
                gotoxy(22, 13);
                if (conioxlib_getdiskfree_ext((unsigned char) (disk - 'A' + 1), &df) == 0)
                {
                        /* Integer-only arithmetic to avoid 32-bit overflow on large
                           Windows drives. Decompose avail_clusters = q * 1024 + r
                           with r < 1024: (r * sps * bps) / 1024 fits in unsigned
                           long (r < 2^10, sps <= 255, bps <= 4096 => < 2^30), and
                           q * sps * bps matches the integer-KB count exactly. */
                        unsigned long sps = df.sectors_per_cluster;
                        unsigned long bps = df.bytes_per_sector;
                        unsigned long free_kb  = (df.avail_clusters / 1024UL) * sps * bps
                                              + ((df.avail_clusters % 1024UL) * sps * bps) / 1024UL;
                        unsigned long total_kb = (df.total_clusters / 1024UL) * sps * bps
                                               + ((df.total_clusters % 1024UL) * sps * bps) / 1024UL;
                        cprintf("Free: %lu KB of %lu KB", free_kb, total_kb);
                }
                else
                {
                        cputs("Free: <not available>");
                }
                /* Size of the marked file with filesize() */
                gotoxy(22, 14);
                if (i < DEMO_FILES)
                {
                        sprintf(str, "%c:%s%s", disk, dir, files[i].name);
                        fsz = conioxlib_filesize(str);
                        if (fsz != 0xFFFFFFFFUL)
                        {
                                cprintf("Size: %lu bytes", fsz);
                        }
                        else
                        {
                                cputs("Size: <not found>");
                        }
                }
                else
                {
                        cputs("Size: <none>");
                }
                conioxlib_ckey();
        }
        free(files);
}
/* ------------- Simple single file picker with fileselector() ------------- */
static void _demo_simplepicker(void)
{
        unsigned char disk;
        char dir[80];
        char name[41];

        disk = 'C';
        dir[0] = 0;
        name[0] = 0;
        /* conioxlib_fileselector() browses from the current drive and directory and fills
           the three outputs only when a plain file is picked with ENTER */
        if (conioxlib_fileselector(&disk, dir, name) != 0)
        {
                box(20, 8, 60, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Picked file", SHADOW);
                textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
                gotoxy(22, 10);
                cprintf("Drive: %c", disk);
                gotoxy(22, 11);
                cprintf("Path:  %s", dir);
                gotoxy(22, 12);
                cprintf("File:  %s", name);
                gotoxy(22, 13);
                cputs("Press a key to close");
                conioxlib_ckey();
        }
}
/* ------------------------ Text viewer with view() ---------------------- */
static void _demo_viewer(void)
{
        char (*helpbuf)[61];
        unsigned int i;
        unsigned int last;
        i = 0;
        while (_demotext[i] != 0)
        {
                i++;
        }
        /* conioxlib_view() reads only the rows 0..last-1 and paints the blank trailing
           rows by itself, so the buffer needs just the content lines:
           DEMO_HELPBUF (256*61) is the same work area size helpindex uses */
        last = i;
        helpbuf = (char (*)[61]) malloc((size_t) DEMO_HELPBUF);
        if (helpbuf == 0)
        {
                return;
        }
        for (i = 0; _demotext[i] != 0; i++)
        {
                memset(helpbuf[i], 0, 61);
                strncpy(helpbuf[i], _demotext[i], 60);
        }
        conioxlib_view(6, 3, 72, 21, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), helpbuf, (unsigned char) last, "CONXLIB overview");
        free(helpbuf);
}
/* -------------------- Screen saver with screensaver() ------------------- */
static void _demo_screensaver(void)
{
        conioxlib_screensaver();
}
/* -------------------- Push, bounce and wipe effects ------------------------ */
static void _demo_effects(void)
{
        conioxlib_push();
        conioxlib_bounce();
        conioxlib_wipe();
}
/* ------------------- Button bar with buttons() ---------------------- */
static void _demo_buttons(void)
{
        unsigned int i;
        box(39, 6, 78, 16, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "buttons()", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 8);
        cputs("Use arrows, TAB, hotkeys or the mouse.");
        gotoxy(41, 9);
        cputs("Enter selects, ESC cancels.");
        gotoxy(41, 10);
        cputs("Borland Turbo Vision 2.0 style buttons.");
        i = conioxlib_buttons(40, 12, CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), _btnmenu, _btndesc, 4, 1);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 14);
        if (i < 4)
        {
                cprintf("Selected: %s", _btnmenu[i]);
        }
        else
        {
                cputs("Cancelled");
        }
        conioxlib_ckey();
}
/* -------------------------- Exit to DOS with shell() ---------------------- */
static void _demo_shell(void)
{
        conioxlib_shell(3, 2, 75, 22, "");
}
/* ---------------- VGA palette with setpal() and vgaremap() ------------------- */
static void _demo_palette(void)
{
        box(39, 5, 78, 17, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "setpal() / vgaremap()", SHADOW);
        textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
        gotoxy(41, 7);
        cputs("Applying alternate VGA palette...");
        conioxlib_setpal(1);
        gotoxy(41, 8);
        cputs("Press a key to restore default.");
        conioxlib_ckey();
        conioxlib_setpal(0);
        gotoxy(41, 10);
        cputs("Default palette restored.");
        /* vgaremap(): triangles and chunky arrows in the 8x16 font (DOS only);
           on Windows the console already ships the CP437 characters */
        gotoxy(41, 12);
        cputs("vgaremap(1) glyphs (DOS only):");
        conioxlib_vgaremap(1);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 14);
        cputs("Triangles \x10 \x11, chunky arrows \x18 \x19");
        gotoxy(41, 15);
        cputs("Mouse pointer glyph: \x01");
        gotoxy(41, 16);
        cputs("Press a key to restore the driver pointer.");
        conioxlib_ckey();
        conioxlib_vgaremap(0);
}
/* --------------- All colors with CONIOXLIB_ATTR -------------------- */
static void _demo_colorattrs(void)
{
        int fg, bg;
        char *names[8] = {"BLK","BLU","GRN","CYN","RED","MAG","BRN","GRY"};
        box(39, 4, 76, 22, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "CONIOXLIB_ATTR", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 5);
        cputs("fg\\bg  0 1 2 3 4 5 6 7");
        for (fg = 0; fg < 8; fg++)
        {
                gotoxy(41, 6 + fg);
                textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
                cprintf("%s ", names[fg]);
                for (bg = 0; bg < 8; bg++)
                {
                        textattr(CONIOXLIB_ATTR(fg, bg));
                        cputs(" X");
                }
        }
        gotoxy(41, 15);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        cputs("Bright fg (8..15):");
        for (fg = 8; fg < 16; fg++)
        {
                textattr(CONIOXLIB_ATTR(fg, BLACK));
                cprintf(" %X", fg);
        }
        gotoxy(41, 17);
        textattr(CONIOXLIB_ATTR(WHITE, BLUE));
        cputs(" WHITE on BLUE ");
        textattr(CONIOXLIB_ATTR(YELLOW, RED));
        cputs(" YELLOW on RED ");
        gotoxy(41, 19);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        cputs("Use CONIOXLIB_ATTR(fg, bg) with COLORS.");
        conioxlib_ckey();
}
/* -------------- Menu bar and pull-downs IDE style ---------------- */
static void _demo_menubar(void)
{
        static const char *baritems_list[10] = {"&File", "&Edit", "&Search", "&Run", "&Compile", "&Debug", "&Project", "&Options", "&Window", "&Help"};
        static const char *bardesc_list[10] = {
                "File commands: new, open, save, quit                              ",
                "Edit commands: cut, copy, paste                                   ",
                "Search and replace                                                ",
                "Run the current program                                           ",
                "Compile the current project                                       ",
                "Debugger commands                                                 ",
                "Project management                                                ",
                "Environment and compiler options                                  ",
                "Window arrangement                                                ",
                "Help contents and about                                           "};
        static const char *fileitems_list[11] = {
                "&New           ",
                "&Open...       ",
                "&Save          ",
                "Save &as...    ",
                "Save a&ll      ",
                "-",
                "&Change dir... ",
                "&Print         ",
                "&DOS shell     ",
                "-",
                "&Quit          "};
        static const char *filedesc_list[11] = {
                "Create a new file in a new Edit window                            ",
                "Open an existing file in an Edit window                           ",
                "Save the file in the active Edit window                           ",
                "Save the file under a different name                              ",
                "Save all modified files                                           ",
                "                                                                  ",
                "Change the current directory                                      ",
                "Print the file in the active Edit window                          ",
                "Shell out to DOS / command processor                              ",
                "                                                                  ",
                "Exit the demonstration                                            "};
        static const char *edititems_list[5] = {
                "Cu&t           ",
                "&Copy          ",
                "&Paste         ",
                "-",
                "Cl&ear         "};
        static const char *editdesc_list[5] = {
                "Cut selection to clipboard                                        ",
                "Copy selection to clipboard                                       ",
                "Paste from clipboard                                              ",
                "                                                                  ",
                "Clear the current selection                                       "};
        unsigned char bar, sub;
        char (*baritems)[21];
        char (*bardesc)[71];
        char (*fileitems)[21];
        char (*filedesc)[71];
        char (*edititems)[21];
        char (*editdesc)[71];
        char plain[21];
        char *oldscr;
        char *olddrop;
        int donebar;
        int bi, pos, visible;
        int oldbar;
        int cellpos[10];
        int dropx1, dropy1, dropx2, dropy2;
        struct conioxlib_stheme th;
        baritems = (char (*)[21]) _demo_newtable(baritems_list, 10, 21);
        bardesc = (char (*)[71]) _demo_newtable(bardesc_list, 10, 71);
        fileitems = (char (*)[21]) _demo_newtable(fileitems_list, 11, 21);
        filedesc = (char (*)[71]) _demo_newtable(filedesc_list, 11, 71);
        edititems = (char (*)[21]) _demo_newtable(edititems_list, 5, 21);
        editdesc = (char (*)[71]) _demo_newtable(editdesc_list, 5, 71);
        if ((baritems == 0) || (bardesc == 0) || (fileitems == 0) || (filedesc == 0) || (edititems == 0) || (editdesc == 0))
        {
                free(baritems);
                free(bardesc);
                free(fileitems);
                free(filedesc);
                free(edititems);
                free(editdesc);
                return;
        }
        oldscr = (char *) malloc(textsize(1, 1, 80, 25));
        if (oldscr != 0)
        {
                gettext(1, 1, 80, 25, oldscr);
        }
        /* Clear the menu bar row: 80 putch calls would be slow on DOS, but this
           only runs once when _demo_menubar opens. Using clrscr() inside a 1-row
           window is an alternative but messes with the cursor/window state. */
        {
                static char _sp80[81];
                if (_sp80[0] != ' ') { memset(_sp80, ' ', 80); _sp80[80] = 0; }
                textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
                gotoxy(1, 1);
                cputs(_sp80);
        }
        /* Redraw the bar with the colors of the active theme */
        conioxlib_gettheme(&th);
        /* Start of every bar cell: switching drop-downs repaints single cells */
        pos = 1;
        for (bi = 0; bi < 10; bi++)
        {
                /* Cell width = visible length (without the ampersand) + 2 */
                visible = _demo_vislen(baritems[bi]);
                cellpos[bi] = pos;
                pos += visible + 2;
        }
        bar = conioxlib_menubar(1, th.atr0, th.atr1, th.atrhot, baritems, bardesc, 10, 0);
        donebar = 0;
        while ((bar != CONIOXLIB_MENU_ESC) && (bar != CONIOXLIB_MENU_HELP) && (!donebar))
        {
                olddrop = 0;
                if (bar == 0)
                {
                        /* The saved area covers the drop-down plus its shadow */
                        dropx1 = 1;
                        dropy1 = 2;
                        dropx2 = 19;
                        dropy2 = 15;
                        olddrop = conioxlib_savescr(dropx1, dropy1, dropx2, dropy2);
                        box(1, 2, 17, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_SINGLE, "", CONIOXLIB_BOX_SHADOW);
                        sub = conioxlib_menu(2, 3, th.atr0, th.atr1, th.atrhot, fileitems, filedesc, 11, 1, 0);
                }
                else if (bar == 1)
                {
                        dropx1 = 7;
                        dropy1 = 2;
                        dropx2 = 25;
                        dropy2 = 9;
                        olddrop = conioxlib_savescr(dropx1, dropy1, dropx2, dropy2);
                        box(7, 2, 23, 8, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_SINGLE, "", CONIOXLIB_BOX_SHADOW);
                        sub = conioxlib_menu(8, 3, th.atr0, th.atr1, th.atrhot, edititems, editdesc, 5, 1, 0);
                }
                else
                {
                        sub = 255;
                        donebar = 1;
                        window(1, 1, 80, 25);
                        if (oldscr != 0)
                        {
                                puttext(1, 1, 80, 25, oldscr);
                        }
                        box(39, 6, 76, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Menu bar", SHADOW);
                        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
                        gotoxy(41, 8);
                        cprintf("Selected: %s", _demo_plainitem(baritems[bar], plain));
                        conioxlib_ckey();
                        break;
                }
                window(1, 1, 80, 25);
                if ((sub == CONIOXLIB_MENU_LEFT) || (sub == CONIOXLIB_MENU_RIGHT))
                {
                        /* Close only the drop-down and move the highlight: the bar stays on screen */
                        if (olddrop != 0)
                        {
                                conioxlib_restorescr(dropx1, dropy1, dropx2, dropy2, olddrop);
                        }
                        oldbar = (int) bar;
                        if (sub == CONIOXLIB_MENU_LEFT)
                        {
                                if (bar == 0)
                                {
                                        bar = 9;
                                }
                                else
                                {
                                        bar--;
                                }
                        }
                        else
                        {
                                bar++;
                                if (bar > 9)
                                {
                                        bar = 0;
                                }
                        }
                        textattr(th.atr0);
                        gotoxy(cellpos[oldbar], 1);
                        putch(' ');
                        conioxlib_cputs_hot((unsigned short) (cellpos[oldbar] + 1), 1, th.atr0, th.atrhot, baritems[oldbar]);
                        textattr(th.atr0);
                        putch(' ');
                        textattr(th.atr1);
                        gotoxy(cellpos[bar], 1);
                        putch(' ');
                        conioxlib_cputs_hot((unsigned short) (cellpos[bar] + 1), 1, th.atr1, th.atrhot, baritems[bar]);
                        textattr(th.atr1);
                        putch(' ');
                        conioxlib_status(bardesc[bar]);
                        continue;
                }
                if (olddrop != 0)
                {
                        conioxlib_restorescr(dropx1, dropy1, dropx2, dropy2, olddrop);
                }
                if (oldscr != 0)
                {
                        puttext(1, 1, 80, 25, oldscr);
                }
                box(39, 6, 76, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Menu bar", SHADOW);
                textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
                gotoxy(41, 8);
                if ((sub == CONIOXLIB_MENU_ESC) || (sub == CONIOXLIB_MENU_HELP))
                {
                        cputs("Cancelled");
                }
                else if (bar == 0)
                {
                        cprintf("File / %s", _demo_plainitem(fileitems[sub], plain));
                }
                else if (bar == 1)
                {
                        cprintf("Edit / %s", _demo_plainitem(edititems[sub], plain));
                }
                conioxlib_ckey();
                donebar = 1;
        }
        if (((bar == CONIOXLIB_MENU_ESC) || (bar == CONIOXLIB_MENU_HELP)) && (oldscr != 0))
        {
                puttext(1, 1, 80, 25, oldscr);
        }
        if (oldscr != 0)
        {
                free(oldscr);
        }
        free(baritems);
        free(bardesc);
        free(fileitems);
        free(filedesc);
        free(edititems);
        free(editdesc);
}
/* ------------- Environment and credits with getcountry() and about() ------------- */
static void _demo_about(void)
{
        struct conioxlib_scountry sc;
        conioxlib_getcountry(&sc);
        box(39, 4, 76, 22, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Environment", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 6);
        cprintf("CONXLIB %u.%02u (%s)", conioxlib_version / 100, conioxlib_version % 100, conioxlib_build);
        gotoxy(41, 7);
        cprintf("CONIOX  %u.%02u (%s)", coniox_version / 100, coniox_version % 100, coniox_build);
        gotoxy(41, 9);
        cprintf("Code page: %u", sc.code);
        gotoxy(41, 10);
        cprintf("Currency: [%s]  Thousands: [%s]", sc.curr, sc.thsep);
        gotoxy(41, 11);
        cprintf("Decimal: [%s]  Date: %u  Time: %u", sc.dsep, sc.date, sc.time);
        gotoxy(41, 13);
        cputs("From conioxlib_getcountry()");
        conioxlib_ckey();
        conioxlib_about(0);
}
/* -------------------- Message box with msgbox() ---------------------- */
static void _demo_msgbox(void)
{
        char btns[3][11] = {"Yes", "No", "Cancel"};
        unsigned char r;
        r = conioxlib_msgbox("Confirm", "Apply these settings now?", btns, 3);
        box(39, 8, 76, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Result", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 10);
        if (r < 3)
        {
                cprintf("Button: %s", btns[r]);
        }
        else
        {
                cputs("Cancelled");
        }
        conioxlib_ckey();
}
/* --------------------- Input box with inputbox() -------------------- */
static void _demo_inputbox(void)
{
        char buf[40];
        unsigned char r;
        strcpy(buf, "CONXLIB");
        r = conioxlib_inputbox("Name", "Enter name:", buf, 20);
        box(39, 8, 76, 13, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Result", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 10);
        cprintf("btn=%u  text=[%s]", (unsigned) r, buf);
        conioxlib_ckey();
}
/* -------------------- Numeric field with inputmask() --------------------- */
static void _demo_inputmask(void)
{
        char buf[16];
        unsigned char r;
        strcpy(buf, "42");
        r = conioxlib_inputmask("PIN", "Digits (min 2):", buf, 8, 2);
        box(39, 8, 76, 13, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Result", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 10);
        cprintf("btn=%u  text=[%s]", (unsigned) r, buf);
        conioxlib_ckey();
}
/* ------------------ Scrollable list with listbox() ----------------------- */
static void _demo_listbox(void)
{
        static const char *items_list[26] = {
                "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot",
                "Golf", "Hotel", "India", "Juliet", "Kilo", "Lima",
                "Mike", "November", "Oscar", "Papa", "Quebec", "Romeo",
                "Sierra", "Tango", "Uniform", "Victor", "Whiskey", "X-ray",
                "Yankee", "Zulu"
        };
        char (*items)[41];
        unsigned char r;
        items = (char (*)[41]) _demo_newtable(items_list, 26, 41);
        if (items == 0)
        {
                return;
        }
        /* 26 items and only 12 visible rows: forces trying the scroll, the wheel
           and the side bar of the listbox */
        r = conioxlib_listbox(20, 5, 55, 18, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), items, 26, "Listbox");
        box(39, 8, 76, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Result", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 10);
        if (r < 26)
        {
                cprintf("Selected: %s", items[r]);
        }
        else
        {
                cputs("Cancelled");
        }
        conioxlib_ckey();
        free(items);
}
/* ---------------- Progress with progress() and progressbar() --------------- */
static void _demo_progress(void)
{
        unsigned int i;
        box(39, 6, 76, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Progress", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 8);
        cputs("conioxlib_progress / progressbar");
        for (i = 0; i <= 100; i += 5)
        {
                conioxlib_progress("Loading", i, 100);
                conioxlib_progressbar(41, 10, 30, CONIOXLIB_ATTR(WHITE, BLUE), i, 100);
                delay(40);
        }
        gotoxy(41, 12);
        cputs("Done. Press a key.");
        conioxlib_ckey();
}
/* ------------------------- Visual themes ------------------------------- */
/* Shows the nine themes as radio buttons with property(); choosing one
   applies it with conioxlib_theme() and the whole demo changes colors (the
   boxes and menus of main() already use gettheme()). */
static void _demo_themes(void)
{
        static unsigned char _demo_theme = 0;
        static const char *themenames_list[CONIOXLIB_THEMES] = {"Borland C++ 3.1", "Turbo C 2.0", "Turbo C 1.x", "Norton Utilities 9", "PC Tools", "Norton Commander", "MS-DOS Edit", "Lotus 1-2-3", "SideKick", "Volkov Commander", "DOS Navigator", "XTree Gold"};
        static const char *themedesc_list[CONIOXLIB_THEMES] = {"Turbo Vision 2.0: blue desktop, gray dialogs    ", "Blue desktop, yellow text and cyan menus        ", "Monochrome with black background                 ", "Blue with cyan frames and yellow hotkeys         ", "White on cyan, PC Tools look                    ", "Cyan panels, black selection, blue dialogs        ", "Blue screen with single cyan frames              ", "White on black with inverted selection           ", "Blue/cyan TSR PIM with green buttons              ", "Red accent NC clone, amber hotkeys               ", "Blue/cyan with green accents, double frame        ", "Yellow-on-blue file manager, single frame         "};
        char (*themenames)[21];
        char (*themedesc)[71];
        unsigned char sel[CONIOXLIB_THEMES];
        unsigned int i;
        struct conioxlib_stheme th;
        themenames = (char (*)[21]) _demo_newtable(themenames_list, CONIOXLIB_THEMES, 21);
        themedesc = (char (*)[71]) _demo_newtable(themedesc_list, CONIOXLIB_THEMES, 71);
        if ((themenames == 0) || (themedesc == 0))
        {
                free(themenames);
                free(themedesc);
                return;
        }
        conioxlib_gettheme(&th);
        box(39, 4, 76, 19, th.atrdialog, th.frame, "Themes", SHADOW);
        memset(sel, ' ', CONIOXLIB_THEMES);
        sel[_demo_theme] = '\xFE';
        conioxlib_property(41, 6, th.atr0, th.atr1, th.atrhot, themenames, themedesc, (char *) sel, CONIOXLIB_THEMES, 1, 1);
        for (i = 0; i < CONIOXLIB_THEMES; i++)
        {
                if (sel[i] == '\xFE')
                {
                        _demo_theme = (unsigned char) i;
                        break;
                }
        }
        conioxlib_theme(_demo_theme);
        conioxlib_gettheme(&th);
        textattr(th.atr0);
        gotoxy(41, 16);
        cprintf("Theme applied: %-18s", th.name);
        gotoxy(41, 17);
        cputs("Main menu and boxes follow it now.");
        conioxlib_ckey();
        free(themenames);
        free(themedesc);
}
/* --------------------------- Text editor ------------------------------- */
/* _demo_edit() moved here from the library (it was conioxlib_edit()): it
   embeds a Borland-style editor in a rectangle (x1,y1)-(x2,y2) with a
   File/Edit/Search menu bar (F10 or click), atr0/atr1 colors and an
   optional title. It redraws only what is needed, highlights the active
   line with atr1, expands tabs to 4 spaces when painting, loads up to 250
   lines, supports mouse/wheel, block selection with Shift, paging with
   PGUP/PGDW and undo/redo (4 levels). File > Open picks the file with
   conioxlib_fileselector(). Returns 0. */

/* Returns whether a shift key is held: Windows polls the async key state and DOS reads the BIOS shift flags */
static int _demo_edit_shift (void)
{
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
        return ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
#else
        return ((peekb(0x40, 0x17) & 3) != 0);
#endif
}

/* Editor undo stacks: _demo_pushundo() pushes a full copy of the text onto
   the stack (dst) and _demo_clearundo() empties the opposite stack after a
   change; they only expand inside _demo_edit(). */
#define _demo_pushundo(dst, ndst) \
        do { \
                if ((ndst) == 4) \
                { \
                        free((dst)[0]); \
                        (dst)[0] = (dst)[1]; \
                        (dst)[1] = (dst)[2]; \
                        (dst)[2] = (dst)[3]; \
                        (ndst) = 3; \
                } \
                (dst)[(ndst)] = (struct _demo_seditundo *) malloc(sizeof(struct _demo_seditundo)); \
                if ((dst)[(ndst)] != 0) \
                { \
                        memcpy((dst)[(ndst)]->lines, lines, sizeof(lines)); \
                        (dst)[(ndst)]->nlines = nlines; \
                        (dst)[(ndst)]->cur = cur; \
                        (dst)[(ndst)]->col = col; \
                        (ndst)++; \
                } \
        } while (0)

#define _demo_clearundo(dst, ndst) \
        do { \
                while ((ndst) > 0) \
                { \
                        (ndst)--; \
                        free((dst)[(ndst)]); \
                } \
        } while (0)

static unsigned char _demo_edit (unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned char atr0, unsigned char atr1, const char *title, const char *filename)
{
#define DEMO_EDIT_MAXL  250
#define DEMO_EDIT_MAXC  76
        static char CONIOXLIB_FARDATA lines[DEMO_EDIT_MAXL][DEMO_EDIT_MAXC];
        static char CONIOXLIB_FARDATA clip[DEMO_EDIT_MAXL][DEMO_EDIT_MAXC];
        /* fname holds a full path "X:\sub1\sub2\...\file.ext" and must fit the
           same space as outln below; truncating to 80 chars silently dropped
           the tail of long paths and broke Save after Open */
        char fname[160];
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
        /* Static so the ~1656 bytes of menu tables do not live on the DOS 16-bit
           near stack; the initializers run once at program load. The desc tables
           are zeroed and refilled by _demo_fill_desc() below. */
        static char baritems[3][21] = {"&File", "&Edit", "&Search"};
        static char bardesc[3][71];
        static char fileitems[5][21] = {"&Open...", "&Save", "Save &as...", "-", "E&xit"};
        static char filedesc[5][71];
        static char edititems[7][21] = {"Cu&t", "&Copy", "&Paste", "-", "C&lear line", "&Undo", "&Redo"};
        static char editdesc[7][71];
        static char searchitems[3][21] = {"&Find...", "&Replace...", "&Goto line..."};
        static char searchdesc[3][71];
        int barpos[3];
        int mbsel;
        int nclip;
        int selrow, selcol, selactive, selpaint, shiftpressed;
        int clr, ln, r1, c1, r2, c2, a, b, vs, ve, k;
        unsigned int selatr;
        unsigned char bar, sub;
        struct conioxlib_stheme th;
        struct _demo_seditundo
        {
                char lines[DEMO_EDIT_MAXL][DEMO_EDIT_MAXC];
                int nlines;
                int cur;
                int col;
        };
        struct _demo_seditundo *undo[4];
        struct _demo_seditundo *redo[4];
        int nundo, nredo;

        /* Expand tabs (stop every 4) into outln; returns visual length */
#define _demo_edit_expand(src, dst, maxw) \
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
#define _demo_edit_selcmp(r1, c1, r2, c2)       (((r1) != (r2)) ? ((r1) - (r2)) : ((c1) - (c2)))

        /* Copies the active block selection, or the current line when none, to the multiline clipboard */
#define _demo_edit_selcopy() \
        do { \
                int _r1, _c1, _r2, _c2, _k, _from, _len; \
                if (selactive != 0) \
                { \
                        if (_demo_edit_selcmp(selrow, selcol, cur, col) <= 0) \
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
                                if (_len > DEMO_EDIT_MAXC - 1) \
                                { \
                                        _len = DEMO_EDIT_MAXC - 1; \
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
#define _demo_edit_delsel() \
        do { \
                int _r1, _c1, _r2, _c2, _k; \
                if (selactive != 0) \
                { \
                        if (_demo_edit_selcmp(selrow, selcol, cur, col) <= 0) \
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
                        _demo_pushundo(undo, nundo); \
                        _demo_clearundo(redo, nredo); \
                        if (_r1 == _r2) \
                        { \
                                memmove(&lines[_r1][_c1], &lines[_r1][_c2], (size_t) (strlen(&lines[_r1][_c2]) + 1)); \
                        } \
                        else \
                        { \
                                strncpy(&lines[_r1][_c1], &lines[_r2][_c2], (size_t) (DEMO_EDIT_MAXC - 2 - _c1)); \
                                lines[_r1][DEMO_EDIT_MAXC - 2] = 0; \
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
#define _demo_edit_menuw(items, nitems, res) \
        do { \
                int _w, _n; \
                (res) = 1; \
                for (_w = 0; _w < (nitems); _w++) \
                { \
                        if ((items)[_w][0] != '-') \
                        { \
                                _n = _demo_vislen((items)[_w]); \
                                if (_n > (res)) \
                                { \
                                        (res) = _n; \
                                } \
                        } \
                } \
        } while (0)

        /* Loads the whole file f into the lines buffer: nl receives the line count, never zero */
#define _demo_edit_load(f, nl) \
        do { \
                int _ll; \
                (nl) = 0; \
                while (((nl) < DEMO_EDIT_MAXL - 1) && (fgets(lines[(nl)], DEMO_EDIT_MAXC - 1, (f)) != 0)) \
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
        if (width > DEMO_EDIT_MAXC - 1)
        {
                width = DEMO_EDIT_MAXC - 1;
        }
        memset(bardesc, 32, sizeof(bardesc));
        memset(filedesc, 32, sizeof(filedesc));
        memset(editdesc, 32, sizeof(editdesc));
        memset(searchdesc, 32, sizeof(searchdesc));
        /* Force NUL terminators: the rows are passed to conioxlib_menu() which
           feeds them to strlen(). Without terminators, strlen() reads past the
           end of each row into adjacent stack memory. CONIOXLIB_HINT_W is 71,
           so the last column (index 70) is the terminator slot. */
        for (i = 0; i < 3; i++)
        {
                bardesc[i][70] = 0;
                searchdesc[i][70] = 0;
        }
        for (i = 0; i < 5; i++)
        {
                filedesc[i][70] = 0;
        }
        for (i = 0; i < 7; i++)
        {
                editdesc[i][70] = 0;
        }
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
                strncpy(fname, filename, sizeof(fname) - 1);
                fname[sizeof(fname) - 1] = 0;
                f = fopen(fname, "rb");
                if (f != 0)
                {
                        _demo_edit_load(f, nlines);
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
                                if (_demo_edit_selcmp(selrow, selcol, cur, col) <= 0)
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
                                _demo_edit_expand(lines[ln], outln, width);
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
                {
                        {
                                int wx = wherex();
                                int remain = (int) x2 - wx + 1;
                                if (remain > 0)
                                {
                                        static char _sp_status[81];
                                        if (_sp_status[0] != ' ') { memset(_sp_status, ' ', 80); _sp_status[80] = 0; }
                                        if (((int) x2 >= 80) && ((int) y2 >= 25) && (wx + remain > 80))
                                        {
                                                remain = 80 - wx;
                                        }
                                        if (remain > 0 && remain <= 80)
                                        {
                                                _sp_status[remain] = 0;
                                                cputs(_sp_status);
                                                _sp_status[remain] = ' ';
                                        }
                                }
                        }
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
                        if (conioxlib_kbhit())
                        {
                                key = conioxlib_getkey();
                                shiftpressed = _demo_edit_shift();
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
                                        if (i > DEMO_EDIT_MAXC - 2)
                                        {
                                                i = DEMO_EDIT_MAXC - 2;
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
                        bar = conioxlib_menubar(y1, atr0, atr1, th.atrhot, baritems, bardesc, 3, (unsigned char) mbsel);
                        /* Keep the last visited menu so the next F10 comes back to it */
                        if (bar < 3)
                        {
                                mbsel = (int) bar;
                                if (bar == 0)
                                {
                                        _demo_edit_menuw(fileitems, 5, l);
                                        box(x1, (unsigned short) (y1 + 1), (unsigned short) (x1 + l + 2), (unsigned short) (y1 + 7), th.atrdialog, th.frame, "", CONIOXLIB_BOX_SHADOW);
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
                                                        strncpy(fname, outln, sizeof(fname) - 1);
                                                        fname[sizeof(fname) - 1] = 0;
                                                        memset(lines, 0, sizeof(lines));
                                                        _demo_edit_load(f, nlines);
                                                        fclose(f);
                                                        cur = 0;
                                                        col = 0;
                                                        selactive = 0;
                                                        top = 0;
                                                        /* Opening a new file resets the undo/redo history: stale
                                                           snapshots from the previous file must not survive */
                                                        _demo_clearundo(undo, nundo);
                                                        _demo_clearundo(redo, nredo);
                                                        drawfull = 1;
                                                }
                                        }
                                        else if ((sub == 1) || (sub == 2))
                                        {
                                                if ((sub == 2) || (fname[0] == 0))
                                                {
                                                        conioxlib_inputbox("Save as", "File:", fname, (int)sizeof(fname) - 1);
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
                                        _demo_edit_menuw(edititems, 7, l);
                                        box((unsigned short) (x1 + 6), (unsigned short) (y1 + 1), (unsigned short) (x1 + l + 8), (unsigned short) (y1 + 9), th.atrdialog, th.frame, "", CONIOXLIB_BOX_SHADOW);
                                        sub = conioxlib_menu((unsigned short) (x1 + 7), (unsigned short) (y1 + 2), atr0, atr1, th.atrhot, edititems, editdesc, 7, 1, 0);
                                        if (sub == 0)
                                        {
                                                if (selactive != 0)
                                                {
                                                        _demo_edit_selcopy();
                                                        _demo_edit_delsel();
                                                }
                                                else
                                                {
                                                        _demo_pushundo(undo, nundo);
                                                        _demo_clearundo(redo, nredo);
                                                        strcpy(clip[0], lines[cur]);
                                                        nclip = 1;
                                                        lines[cur][0] = 0;
                                                        col = 0;
                                                        drawline = 1;
                                                }
                                        }
                                        else if (sub == 1)
                                        {
                                                _demo_edit_selcopy();
                                        }
                                        else if (sub == 2)
                                        {
                                                if (nclip > 0)
                                                {
                                                        if (selactive != 0)
                                                        {
                                                                _demo_edit_delsel();
                                                        }
                                                        else
                                                        {
                                                                _demo_pushundo(undo, nundo);
                                                                _demo_clearundo(redo, nredo);
                                                        }
                                                        if ((nclip == 1) && ((int) strlen(lines[cur]) + (int) strlen(clip[0]) < DEMO_EDIT_MAXC - 1))
                                                        {
                                                                memmove(&lines[cur][col + (int) strlen(clip[0])], &lines[cur][col], (size_t) (strlen(&lines[cur][col]) + 1));
                                                                memcpy(&lines[cur][col], clip[0], (size_t) strlen(clip[0]));
                                                                col += (int) strlen(clip[0]);
                                                                drawline = 1;
                                                        }
                                                        else if ((nclip > 1) && (nlines + nclip - 1 < DEMO_EDIT_MAXL) && (col + (int) strlen(clip[0]) < DEMO_EDIT_MAXC - 1) && ((int) strlen(clip[nclip - 1]) + (int) strlen(&lines[cur][col]) < DEMO_EDIT_MAXC - 1))
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
                                                        _demo_edit_delsel();
                                                }
                                                else
                                                {
                                                        _demo_pushundo(undo, nundo);
                                                        _demo_clearundo(redo, nredo);
                                                        lines[cur][0] = 0;
                                                        col = 0;
                                                        drawline = 1;
                                                }
                                        }
                                        else if (sub == 5)
                                        {
                                                if (nundo > 0)
                                                {
                                                        _demo_pushundo(redo, nredo);
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
                                                        _demo_pushundo(undo, nundo);
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
                                        _demo_edit_menuw(searchitems, 3, l);
                                        box((unsigned short) (x1 + 12), (unsigned short) (y1 + 1), (unsigned short) (x1 + l + 14), (unsigned short) (y1 + 5), th.atrdialog, th.frame, "", CONIOXLIB_BOX_SHADOW);
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
                                                                if (off + lenrep + lenrest < DEMO_EDIT_MAXC - 1)
                                                                {
                                                                        _demo_pushundo(undo, nundo);
                                                                        _demo_clearundo(redo, nredo);
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
                        if ((col < (int) strlen(lines[cur])) && (col < DEMO_EDIT_MAXC - 2))
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
                        { int lc = (int) strlen(lines[cur]); if (col > lc) col = lc; }
                }
                else if (key == KBD_PGDW)
                {
                        cur += vis;
                        if (cur > nlines - 1)
                        {
                                cur = nlines - 1;
                        }
                        { int lc = (int) strlen(lines[cur]); if (col > lc) col = lc; }
                }
                else if (key == KBD_ORG)
                {
                        col = 0;
                }
                else if (key == KBD_END)
                {
                        col = (int) strlen(lines[cur]);
                        if (col > DEMO_EDIT_MAXC - 2)
                        {
                                col = DEMO_EDIT_MAXC - 2;
                        }
                }
                else if (key == KBD_ENTER)
                {
                        j = (selactive != 0) ? 1 : 0;
                        if (selactive != 0)
                        {
                                _demo_edit_delsel();
                        }
                        if (nlines < DEMO_EDIT_MAXL - 1)
                        {
                                if (j == 0)
                                {
                                        _demo_pushundo(undo, nundo);
                                        _demo_clearundo(redo, nredo);
                                }
                                if (nlines > cur + 1) memmove(&lines[cur + 2][0], &lines[cur + 1][0], (size_t)(nlines - cur - 1) * DEMO_EDIT_MAXC);
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
                                _demo_edit_delsel();
                        }
                        else if (col > 0)
                        {
                                _demo_pushundo(undo, nundo);
                                _demo_clearundo(redo, nredo);
                                {
                                        int tail = (int) strlen(&lines[cur][col]);
                                        memmove(&lines[cur][col - 1], &lines[cur][col], (size_t)(tail + 1));
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
                                _demo_edit_delsel();
                        }
                        i = (int) strlen(lines[cur]);
                        if (i < DEMO_EDIT_MAXC - 2)
                        {
                                if (j == 0)
                                {
                                        _demo_pushundo(undo, nundo);
                                        _demo_clearundo(redo, nredo);
                                }
                                memmove(&lines[cur][col + 1], &lines[cur][col], (size_t)(i - col + 1));
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
                                _demo_edit_delsel();
                        }
                        i = (int) strlen(lines[cur]);
                        if (i < DEMO_EDIT_MAXC - 2)
                        {
                                if (j == 0)
                                {
                                        _demo_pushundo(undo, nundo);
                                        _demo_clearundo(redo, nredo);
                                }
                                memmove(&lines[cur][col + 1], &lines[cur][col], (size_t)(i - col + 1));
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
        _demo_clearundo(undo, nundo);
        _demo_clearundo(redo, nredo);
        conioxlib_restorescr(x1, y1, x2, y2, oldb);
        gotoxy(oldx, oldy);
        return(0);
#undef DEMO_EDIT_MAXL
#undef DEMO_EDIT_MAXC
#undef _demo_edit_expand
#undef _demo_edit_load
#undef _demo_edit_menuw
#undef _demo_edit_selcmp
#undef _demo_edit_selcopy
#undef _demo_edit_delsel
}

/* --------------------- Text editor with _demo_edit() ------------------- */
static void _demo_editor(void)
{
        _demo_edit(1, 1, 80, 25, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), "Editor", "");
}
/* --------------------- Status line and tooltip ------------------------- */
static void _demo_statustip(void)
{
        box(39, 6, 76, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Status/Tooltip", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(41, 8);
        cputs("Status line (cols 1-59)...");
        conioxlib_status("Ready. Clock and locks stay intact.");
        gotoxy(41, 10);
        cputs("Tooltip near (50,12) for 1.5s...");
        conioxlib_tooltip(50, 12, "Tooltip sample", 1500);
        gotoxy(41, 12);
        cputs("Press a key.");
        conioxlib_ckey();
}
/* ------------------------------- Tetris -------------------------------- */
static const int _tetrisshapes[7][4][2] = {     {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
                                        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
                                        {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
                                        {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
                                        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
                                        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
                                        {{2, 0}, {0, 1}, {1, 1}, {2, 1}}};
static const unsigned char _tetrissize[7] = {4, 2, 3, 3, 3, 3, 3};
static const unsigned char _tetrisattrs[7] = {  CONIOXLIB_ATTR(CYAN, BLACK),
                                                CONIOXLIB_ATTR(YELLOW, BLACK),
                                                CONIOXLIB_ATTR(MAGENTA, BLACK),
                                                CONIOXLIB_ATTR(GREEN, BLACK),
                                                CONIOXLIB_ATTR(RED, BLACK),
                                                CONIOXLIB_ATTR(BLUE, BLACK),
                                                CONIOXLIB_ATTR(BROWN, BLACK)};
/* Checks if the piece cells fit at (px,py); rows above the well only test the columns */
static int _demo_tetris_fit(int px, int py, int cells[4][2], unsigned char board[20][10])
{
        int i;
        int bx, by;
        for (i = 0; i != 4; i++)
        {
                bx = px + cells[i][0];
                by = py + cells[i][1];
                if ((bx < 0) || (bx > 9) || (by > 19))
                {
                        return(0);
                }
                if ((by >= 0) && (board[by][bx] != 0))
                {
                        return(0);
                }
        }
        return(1);
}
/* Draws one well cell; val 0 erases and 1 to 7 draw the piece colors */
static void _demo_tetris_drawcell(int bx, int by, unsigned char val)
{
        if ((bx < 0) || (bx > 9) || (by < 0) || (by > 19))
        {
                return;
        }
        gotoxy(35 + (bx << 1), 3 + by);
        if (val != 0)
        {
                textattr(_tetrisattrs[val - 1]);
                cputs("\xDB\xDB");
        }
        else
        {
                textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
                cputs("  ");
        }
}
static void _demo_tetris(void)
{
        int cur, next, px, py, key, i, j, k, n, speed, redraw;
        long score;
        int lines, level;
        int cells[4][2], saved[4][2];
        /* Static: 200 bytes that don't need to live on the 16-bit DOS near stack.
           memset below still clears it on every entry to _demo_tetris(). */
        static unsigned char board[20][10];
        char str[40];
        char btns[1][11] = {"OK"};
        memset(board, 0, sizeof(board));
        score = 0;
        lines = 0;
        level = 1;
        srand((unsigned int) time(0) + 1);
        /* Panels: keys on the left, the well in the middle and the status on the right */
        box(4, 2, 20, 23, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Keys", SHADOW);
        textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
        gotoxy(6, 4);
        cputs("Left/Right: move");
        gotoxy(6, 5);
        cputs("Up:         rotate");
        gotoxy(6, 6);
        cputs("Down:       one row");
        gotoxy(6, 7);
        cputs("Space:      hard drop");
        gotoxy(6, 8);
        cputs("P:          pause");
        gotoxy(6, 9);
        cputs("ESC:        exit");
        box(34, 2, 55, 23, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Tetris", SHADOW);
        /* Clear the well with a single pass per column pair instead of 200 drawcell calls.
           Each cell is 2 chars wide; 10 columns × 2 chars = 20 chars per row.
           Build one row of spaces and puttext it for each of the 20 rows. */
        {
                unsigned short wellcells[20 * 20];
                unsigned short wellattr = (unsigned short) (((unsigned int) CONIOXLIB_ATTR(BLACK, LIGHTGRAY) << 8) | ' ');
                int wi;
                for (wi = 0; wi < 20 * 20; wi++) wellcells[wi] = wellattr;
                puttext(35, 3, 54, 22, wellcells);
        }
        box(57, 2, 77, 23, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Status", SHADOW);
        conioxlib_status("Left/Right move; Up rotates; Space drops");
        next = rand() % 7;
        tetris_1:
        /* Spawn the next piece at the top of the well */
        cur = next;
        next = rand() % 7;
        px = (10 - _tetrissize[cur]) >> 1;
        /* Spawn at py = -1 so the piece's top cell (y=0 in shape coords) is on row
           -1 (still above the well, so it does not overwrite the well border) but
           the y=1 cells become visible on row 0 immediately. The old py = -2 hid
           the whole piece for the first ~2 gravity steps (~800 ms at level 1). */
        py = -1;
        memcpy(cells, _tetrisshapes[cur], sizeof(cells));
        if (_demo_tetris_fit(px, py, cells, board) == 0)
        {
                goto gameover_1;
        }
        redraw = 1;
        tetris_2:
        if (redraw)
        {
                for (i = 0; i != 4; i++)
                {
                        _demo_tetris_drawcell(px + cells[i][0], py + cells[i][1], (unsigned char) (cur + 1));
                }
                textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
                gotoxy(59, 5);
                cprintf("Score: %07ld", score);
                gotoxy(59, 7);
                cprintf("Level: %d", level);
                gotoxy(59, 9);
                cprintf("Lines: %d", lines);
                gotoxy(59, 13);
                cputs("Next:");
                /* Clear Next preview area (4 rows × 8 chars) with 4 puttext calls */
                {
                        unsigned short nc[8];
                        unsigned short nca = (unsigned short) (((unsigned int) CONIOXLIB_ATTR(BLACK, LIGHTGRAY) << 8) | ' ');
                        int ni;
                        for (ni = 0; ni < 8; ni++) nc[ni] = nca;
                        for (j = 0; j != 4; j++)
                        {
                                puttext(62, 15 + j, 69, 15 + j, nc);
                        }
                }
                textattr(_tetrisattrs[next]);
                for (i = 0; i != 4; i++)
                {
                        gotoxy(62 + (_tetrisshapes[next][i][0] << 1), 15 + _tetrisshapes[next][i][1]);
                        cputs("\xDB\xDB");
                }
                redraw = 0;
        }
        /* Gravity: speed ticks of 20 ms per row, faster every level */
        speed = 21 - level;
                if (speed < 2) speed = 2;
        for (i = 0; i != speed; i++)
        {
                conioxlib_clock();
                if (conioxlib_kbhit())
                {
                        key = (int) conioxlib_getkey();
                        if (key == KBD_ESC)
                        {
                                goto exit_1;
                        }
                        else if ((key == 'p') || (key == 'P'))
                        {
                                conioxlib_status("Paused: press a key");
                                while (!conioxlib_kbhit())
                                {
                                        conioxlib_clock();
                                        delay(20);
                                }
                                (void) conioxlib_getkey();
                                conioxlib_status("Left/Right move; Up rotates; Space drops");
                        }
                        else if (key == KBD_LEFT)
                        {
                                if (_demo_tetris_fit(px - 1, py, cells, board))
                                {
                                        for (j = 0; j != 4; j++)
                                        {
                                                _demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
                                        }
                                        px--;
                                        redraw = 1;
                                }
                        }
                        else if (key == KBD_RIGHT)
                        {
                                if (_demo_tetris_fit(px + 1, py, cells, board))
                                {
                                        for (j = 0; j != 4; j++)
                                        {
                                                _demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
                                        }
                                        px++;
                                        redraw = 1;
                                }
                        }
                        else if (key == KBD_UP)
                        {
                                memcpy(saved, cells, sizeof(cells));
                                for (j = 0; j != 4; j++)
                                {
                                        k = cells[j][0];
                                        cells[j][0] = cells[j][1];
                                        cells[j][1] = _tetrissize[cur] - 1 - k;
                                }
                                if (_demo_tetris_fit(px, py, cells, board))
                                {
                                        for (j = 0; j != 4; j++)
                                        {
                                                _demo_tetris_drawcell(px + saved[j][0], py + saved[j][1], 0);
                                        }
                                        redraw = 1;
                                }
                                else
                                {
                                        memcpy(cells, saved, sizeof(cells));
                                }
                        }
                        else if (key == KBD_DOWN)
                        {
                                if (_demo_tetris_fit(px, py + 1, cells, board))
                                {
                                        for (j = 0; j != 4; j++)
                                        {
                                                _demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
                                        }
                                        py++;
                                        redraw = 1;
                                        goto tetris_2;
                                }
                        }
                        else if (key == KBD_SPACE)
                        {
                                for (j = 0; j != 4; j++)
                                {
                                        _demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
                                }
                                while (_demo_tetris_fit(px, py + 1, cells, board))
                                {
                                        py++;
                                }
                                goto lock_1;
                        }
                        if (redraw)
                        {
                                goto tetris_2;
                        }
                }
                delay(20);
        }
        /* One gravity row */
        if (_demo_tetris_fit(px, py + 1, cells, board))
        {
                for (j = 0; j != 4; j++)
                {
                        _demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
                }
                py++;
                redraw = 1;
                goto tetris_2;
        }
        lock_1:
        /* The piece locks: any cell left above the well means game over */
        j = 0;
        for (i = 0; i != 4; i++)
        {
                if (py + cells[i][1] < j)
                {
                        j = py + cells[i][1];
                }
        }
        if (j < 0)
        {
                goto gameover_1;
        }
        for (i = 0; i != 4; i++)
        {
                board[py + cells[i][1]][px + cells[i][0]] = (unsigned char) (cur + 1);
        }
        /* Clear the full rows and pay the score of the active level */
        n = 0;
        for (i = 19; i >= 0; i--)
        {
                j = 0;
                while ((j != 10) && (board[i][j] != 0))
                {
                        j++;
                }
                if (j == 10)
                {
                        n++;
                        for (k = i; k > 0; k--)
                        {
                                memcpy(board[k], board[k - 1], 10);
                        }
                        memset(board[0], 0, 10);
                        i++;
                }
        }
        if (n != 0)
        {
                lines += n;
                { static const long _tetris_score[4] = { 100, 300, 700, 1500 };
                                score += _tetris_score[n - 1] * level; }
                level = (lines / 10) + 1;
        }
        /* Redraw the well using puttext per row instead of 200 drawcell calls.
           Each row is 10 cells × 2 chars = 20 chars. */
        {
                unsigned short wellcells[20];
                int wj;
                for (wj = 0; wj < 20; wj++)
                {
                        int wi;
                        for (wi = 0; wi < 10; wi++)
                        {
                                unsigned char v = board[wj][wi];
                                unsigned short wa;
                                if (v != 0)
                                {
                                        wa = (unsigned short) (((unsigned int) _tetrisattrs[v - 1] << 8) | 0xDB);
                                }
                                else
                                {
                                        wa = (unsigned short) (((unsigned int) CONIOXLIB_ATTR(BLACK, LIGHTGRAY) << 8) | ' ');
                                }
                                wellcells[wi * 2] = wa;
                                wellcells[wi * 2 + 1] = wa;
                        }
                        puttext(35, 3 + wj, 54, 3 + wj, wellcells);
                }
        }
        goto tetris_1;
        gameover_1:
        sprintf(str, "Game over. Score: %07ld", score);
        conioxlib_msgbox("Tetris", str, btns, 1);
        exit_1:
        ;
}
/* --------------------------- Demo exit --------------------------- */
static int _demo_exit(void)
{
        if (conioxlib_exittodos(DEMO_HELP) == 1)
        {
                return(1);
        }
        return(0);
}
/* ------------------------------ Main -------------------------------- */
int main(void)
{
        unsigned char opt;
        unsigned char sel;
        struct conioxlib_stheme th;
        char (*demomenu)[21];
        char (*demodesc)[71];
        char *helpbuf;
        int done = 0;
        coniox_init(NULL);
        _setcursortype(_NOCURSOR);
        conioxlib_hello("CONXDEMO.TMP", 0);
        /* The menu tables are copied once from the literals into heap buffers */
        demomenu = (char (*)[21]) _demo_newtable(_demooptions, DEMO_OPTIONS, 21);
        demodesc = (char (*)[71]) _demo_newtable(_demohints, DEMO_OPTIONS, 71);
        if ((demomenu == 0) || (demodesc == 0))
        {
                /* Free the buffer that did succeed before bye() takes over (it
                   calls exit(), so this is mostly cosmetic but avoids a leak
                   if bye() ever stops exiting) */
                free(demomenu);
                free(demodesc);
                /* conioxlib_bye(name, e, ...) calls exit(e): pass the real error code */
                conioxlib_bye("CONXDEMO.TMP", 1, 0);
                return(1);
        }
        opt = 0;
        sel = 0;
        for (;;)
        {
                conioxlib_background((unsigned char *) DEMO_TITLE);
                /* Boxes and menu with the colors of the active theme; the menu starts at
                   row 4 so its 19 visible items stay inside the box */
                conioxlib_gettheme(&th);
                box(3, 3, 36, 23, th.atrdialog, th.frame, "CONXLIB 2.0", SHADOW | CLOSE);
                box(38, 3, 77, 23, th.atrdialog, th.frame, "Output", SHADOW);
                conioxlib_blocks();
                opt = conioxlib_menu(5, 4, th.atr0, th.atr1, th.atrhot, demomenu, demodesc, DEMO_OPTIONS, 1, sel);
                if (opt < DEMO_OPTIONS)
                {
                        /* Remember the item so F1 and LEFT/RIGHT come back to it */
                        sel = opt;
                }
                if (opt == CONIOXLIB_MENU_HELP)
                {
                        /* The topic index reuses the same 256*61 work area size of the viewer pages */
                        helpbuf = (char *) malloc(DEMO_HELPBUF);
                        if (helpbuf != 0)
                        {
                                conioxlib_helpindex(5, 3, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), DEMO_HELP, (char (*)[32]) helpbuf);
                                free(helpbuf);
                        }
                        continue;
                }
                if (opt == CONIOXLIB_MENU_ESC)
                {
                        opt = 0;
                }
                /* Left/Right in the menu (CONIOXLIB_MENU_LEFT/RIGHT) keep the menu open without any action */
                if ((opt == CONIOXLIB_MENU_LEFT) || (opt == CONIOXLIB_MENU_RIGHT))
                {
                        continue;
                }
                _demo_clear_output();
                switch (opt)
                {
                        case 1: _demo_windows(); break;
                        case 2: _demo_mouse(); break;
                        case 3: _demo_textinput(); break;
                        case 4: _demo_checkboxes(); break;
                        case 5: _demo_radiobuttons(); break;
                        case 6: _demo_grid(); break;
                        case 7: _demo_calendar(); break;
                        case 8: _demo_fileselector(); break;
                        case 9: _demo_simplepicker(); break;
                        case 10: _demo_viewer(); break;
                        case 11: _demo_screensaver(); break;
                        case 12: _demo_effects(); break;
                        case 13: _demo_buttons(); break;
                        case 14: _demo_shell(); break;
                        case 15: _demo_palette(); break;
                        case 16: _demo_colorattrs(); break;
                        case 17: _demo_menubar(); break;
                        case 18: _demo_about(); break;
                        case 19: _demo_msgbox(); break;
                        case 20: _demo_inputbox(); break;
                        case 21: _demo_inputmask(); break;
                        case 22: _demo_listbox(); break;
                        case 23: _demo_progress(); break;
                        case 24: _demo_themes(); break;
                        case 25: _demo_editor(); break;
                        case 26: _demo_statustip(); break;
                        case 27: _demo_tetris(); break;
                        case 0:
                        default:
                                if (_demo_exit())
                                {
                                        done = 1;
                                }
                                break;
                }
                if (done)
                {
                        break;
                }
        }
        free(demomenu);
        free(demodesc);
        conioxlib_bye("CONXDEMO.TMP", 0, 0);
        return(0);
}
