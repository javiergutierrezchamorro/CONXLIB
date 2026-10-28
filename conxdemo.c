/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "coniox.h"
#include "conxlib.h"
/* ----------------------------------------------------------------------------------------------------------------- */
#define DEMO_OPTIONS	27
#define DEMO_TITLE	"CONXLIB DEMO 2.0               (c) Copyright 1997-2026 Javier Gutierrez Chamorro"
#define DEMO_HELP	"CONXDEMO.HLP"
#define DEMO_HOT	CONIOXLIB_ATTR(RED, LIGHTGRAY)
#define DEMO_FILES	220
#define DEMO_HELPBUF	(256 * 61)
#if ((__DOS__) || (__MSDOS__)) && !defined(__DJGPP__)
	#if defined(__TURBOC__)
		extern unsigned _stklen = 16384U;
	#elif defined(__WATCOMC__)
		unsigned __stack = 16384U;
	#endif
#endif
static const char *demooptions[DEMO_OPTIONS] = {	"&Exit",
											"&Windows",
											"&Mouse info",
											"&Text input",
											"&Checkboxes",
											"&Radio buttons",
											"&Grid table",
											"C&alendar",
											"&File selector",
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
static const char *demohints[DEMO_OPTIONS] = {	"Leave the demo: conioxlib_exittodos() confirms and bye restores       ",
											"All box() frame styles: double, single, rounded, solid                ",
											"Live conioxlib_mousestatus: position, buttons and wheel               ",
											"conioxlib_getstr() and getpw(): field editors with cursor keys        ",
											"conioxlib_option(): toggle marks with Space or Ins keys               ",
											"conioxlib_property(): only one of the marks stays active              ",
											"conioxlib_grid(): grid picker returning the selected cell             ",
											"conioxlib_calendar(): interactive month view returning the date       ",
											"filemultiselector(): drives, folders, files and marked picks          ",
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
											"conioxlib_edit(): simple Borland-style text editor                    ",
											"conioxlib_status and tooltip without touching clock/locks             ",
											"demo_tetris(): falling blocks game in a boxed window                  "};
static char optmenu[5][21] = {	"&Icons",
								"&Sound",
								"&Colors",
								"&Mouse",
								"&Keys"};
static char optdesc[5][71] = {	"Toggle it with Space or Ins, Enter returns                ",
								"Use TAB, arrows or hotkeys to move along items            ",
								"Left click marks the item under the cursor                ",
								"F1 opens help; ESC returns without changes                ",
								"Radio buttons keep a single active option                 "};
static char tabmenu[12][16] = {	"File",
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
static char btnmenu[4][11] = {	"OK",
								"Cancel",
								"Help",
								"Apply"};
static char btndesc[4][71] = {	"Accept the current selection and close                   ",
								"Discard changes and return                               ",
								"Open context help for this dialog                        ",
								"Apply changes without closing the dialog                 "};
static const char *demotext[] = {	"CONXLIB 2.0 - Retro console library over CONIOX 5.03",
							"",
							"Accelerators: mark a letter with & in the item text.",
							"Press that letter to activate the option. atrhot sets",
							"the colour of the hot letter (passed to menu/buttons).",
							"",
							"Works with Borland Turbo C, Watcom C (16 and 32 bits),",
							"DJGPP, and Microsoft Visual C++ for Windows 32/64 bits.",
							0};
/* Allocates a rows*w items table on the heap copying the rows strings of list */
static char *demo_newtable(const char **list, unsigned int rows, unsigned int w)
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
static char *demo_plainitem(const char *item, char *out)
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
/* ------------------ Clears the output window ------------------------- */
static void demo_clear_output(void)
{
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	window(39, 4, 76, 22);
	clrscr();
	window(1, 1, 80, 25);
}
/* --------------------- Shows the box() styles --------------------- */
static void demo_windows(void)
{
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
}
/* ---------------- Live mouse state with pollmouse() --------------- */
static void demo_mouse(void)
{
	unsigned int t;
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
	t = conioxlib_getkey();
	(void) t;
	window(1, 1, 80, 25);
}
/* -------------- Text input with getstr() and getpw() ----------------- */
static void demo_textinput(void)
{
	char str[80];
	char pass[16];
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
}
/* ------------------ Check boxes with option() --------------------- */
static void demo_checkboxes(void)
{
	unsigned char sel[5];
	unsigned int t;
	box(39, 5, 76, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "option()", SHADOW);
	memset(sel, ' ', 5);
	conioxlib_option(41, 7, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, RED), DEMO_HOT, optmenu, optdesc, (char *) sel, 5, 1, 1);
	textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
	gotoxy(41, 13);
	cprintf("Marks: [%c] [%c] [%c] [%c] [%c]", sel[0], sel[1], sel[2], sel[3], sel[4]);
	t = conioxlib_ckey();
	(void) t;
}
/* ------------------ Radio buttons with property() --------------------- */
static void demo_radiobuttons(void)
{
	unsigned char propsel[5];
	char plain[21];
	unsigned int i;
	unsigned int t;
	box(39, 5, 76, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "property()", SHADOW);
	memset(propsel, ' ', 5);
	propsel[0] = '\xFE';
	conioxlib_property(41, 7, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, RED), DEMO_HOT, optmenu, optdesc, (char *) propsel, 5, 1, 1);
	for (i = 0; (i < 5) && (propsel[i] != '\xFE'); i++);
	textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
	gotoxy(41, 13);
	if (i < 5)
	{
		cprintf("Choice: %s", demo_plainitem(optmenu[i], plain));
	}
	t = conioxlib_ckey();
	(void) t;
}
/* -------------------- Cell grid with grid() ------------------------- */
static void demo_grid(void)
{
	unsigned int i;
	unsigned int t;
	box(9, 6, 73, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "grid()", SHADOW);
	i = conioxlib_grid(11, 8, CONIOXLIB_ATTR(BLACK, BROWN), CONIOXLIB_ATTR(WHITE, RED), tabmenu, 12);
	if (i != 255)
	{
		textattr(CONIOXLIB_ATTR(WHITE, BROWN));
		cputsc(10, 72, 13, tabmenu[i]);
	}
	t = conioxlib_ckey();
	(void) t;
}
/* ---------------------- Calendar with calendar() ------------------------ */
static void demo_calendar(void)
{
	unsigned char day, month;
	unsigned int year;
	unsigned char t;
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
		t = conioxlib_ckey();
		(void) t;
	}
}
/* ----------- File, drive and file size picker ------------- */
static void demo_fileselector(void)
{
	char dir[80];
	char str[80];
	unsigned char disk;
	unsigned int i;
	unsigned int t;
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
			cprintf("Free: %lu KB of %lu KB", (df.avail_clusters * df.sectors_per_cluster * df.bytes_per_sector) / 1024UL, (df.total_clusters * df.sectors_per_cluster * df.bytes_per_sector) / 1024UL);
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
		t = conioxlib_ckey();
		(void) t;
	}
	free(files);
}
/* ------------------------ Text viewer with view() ---------------------- */
static void demo_viewer(void)
{
	char (*helpbuf)[61];
	unsigned int i;
	unsigned int last;
	i = 0;
	while (demotext[i] != 0)
	{
		i++;
	}
	last = i - 1;
	helpbuf = (char (*)[61]) malloc((size_t) ((last + 1) * 61));
	if (helpbuf == 0)
	{
		return;
	}
	for (i = 0; demotext[i] != 0; i++)
	{
		memset(helpbuf[i], 0, 61);
		strncpy(helpbuf[i], demotext[i], 60);
	}
	conioxlib_view(6, 3, 72, 21, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), helpbuf, (unsigned char) last, "CONXLIB overview");
	free(helpbuf);
}
/* -------------------- Screen saver with screensaver() ------------------- */
static void demo_screensaver(void)
{
	conioxlib_screensaver();
}
/* -------------------- Push, bounce and wipe effects ------------------------ */
static void demo_effects(void)
{
	conioxlib_push();
	conioxlib_bounce();
	conioxlib_wipe();
}
/* ------------------- Button bar with buttons() ---------------------- */
static void demo_buttons(void)
{
	unsigned int i;
	unsigned int t;
	box(39, 6, 76, 16, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "buttons()", SHADOW);
	textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
	gotoxy(41, 8);
	cputs("Use arrows, TAB, hotkeys or the mouse.");
	gotoxy(41, 9);
	cputs("Enter selects, ESC cancels.");
	gotoxy(41, 10);
	cputs("Borland Turbo Vision 2.0 style buttons.");
	i = conioxlib_buttons(40, 12, CONIOXLIB_ATTR(BLACK, GREEN), CONIOXLIB_ATTR(WHITE, GREEN), CONIOXLIB_ATTR(YELLOW, GREEN), btnmenu, btndesc, 4, 1);
	textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
	gotoxy(41, 14);
	if (i < 4)
	{
		cprintf("Selected: %s", btnmenu[i]);
	}
	else
	{
		cputs("Cancelled");
	}
	t = conioxlib_ckey();
	(void) t;
}
/* -------------------------- Exit to DOS with shell() ---------------------- */
static void demo_shell(void)
{
	conioxlib_shell(3, 2, 75, 22, "");
}
/* ---------------- VGA palette with setpal() and vgaremap() ------------------- */
static void demo_palette(void)
{
	unsigned int t;
	box(39, 5, 76, 17, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "setpal() / vgaremap()", SHADOW);
	textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
	gotoxy(41, 7);
	cputs("Applying alternate VGA palette...");
	conioxlib_setpal(1);
	gotoxy(41, 8);
	cputs("Press a key to restore default.");
	t = conioxlib_ckey();
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
	t = conioxlib_ckey();
	conioxlib_vgaremap(0);
	(void) t;
}
/* --------------- All colors with CONIOXLIB_ATTR -------------------- */
static void demo_colorattrs(void)
{
	int fg, bg;
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
}
/* -------------- Menu bar and pull-downs IDE style ---------------- */
static void demo_menubar(void)
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
	int bi, pos, k, visible;
	int oldbar;
	int cellpos[10];
	int dropx1, dropy1, dropx2, dropy2;
	unsigned int i;
	unsigned int t;
	struct conioxlib_stheme th;
	baritems = (char (*)[21]) demo_newtable(baritems_list, 10, 21);
	bardesc = (char (*)[71]) demo_newtable(bardesc_list, 10, 71);
	fileitems = (char (*)[21]) demo_newtable(fileitems_list, 11, 21);
	filedesc = (char (*)[71]) demo_newtable(filedesc_list, 11, 71);
	edititems = (char (*)[21]) demo_newtable(edititems_list, 5, 21);
	editdesc = (char (*)[71]) demo_newtable(editdesc_list, 5, 71);
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
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	gotoxy(1, 1);
	for (i = 0; i < 80; i++)
	{
		putch(' ');
	}
	/* Redraw the bar with the colors of the active theme */
	conioxlib_gettheme(&th);
	/* Start of every bar cell: switching drop-downs repaints single cells */
	pos = 1;
	for (bi = 0; bi < 10; bi++)
	{
		/* Cell width = visible length (without the ampersand) + 2 */
		visible = 0;
		for (k = 0; baritems[bi][k] != 0; k++)
		{
			if (baritems[bi][k] != '&')
			{
				visible++;
			}
		}
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
			box(1, 2, 17, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_SINGLE, "", SHADOW);
			sub = conioxlib_menu(2, 3, th.atr0, th.atr1, th.atrhot, fileitems, filedesc, 11, 1, 0);
		}
		else if (bar == 1)
		{
			dropx1 = 7;
			dropy1 = 2;
			dropx2 = 25;
			dropy2 = 9;
			olddrop = conioxlib_savescr(dropx1, dropy1, dropx2, dropy2);
			box(7, 2, 23, 8, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_SINGLE, "", SHADOW);
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
			cprintf("Selected: %s", demo_plainitem(baritems[bar], plain));
			t = conioxlib_ckey();
			(void) t;
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
			cprintf("File / %s", demo_plainitem(fileitems[sub], plain));
		}
		else if (bar == 1)
		{
			cprintf("Edit / %s", demo_plainitem(edititems[sub], plain));
		}
		t = conioxlib_ckey();
		(void) t;
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
static void demo_about(void)
{
	struct conioxlib_scountry sc;
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
	conioxlib_about(0);
}
/* -------------------- Message box with msgbox() ---------------------- */
static void demo_msgbox(void)
{
	char btns[3][11] = {"Yes", "No", "Cancel"};
	unsigned char r;
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
}
/* --------------------- Input box with inputbox() -------------------- */
static void demo_inputbox(void)
{
	char buf[40];
	unsigned char r;
	unsigned int t;
	strcpy(buf, "CONXLIB");
	r = conioxlib_inputbox("Name", "Enter name:", buf, 20);
	box(39, 8, 76, 13, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Result", SHADOW);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	gotoxy(41, 10);
	cprintf("btn=%u  text=[%s]", (unsigned) r, buf);
	t = conioxlib_ckey();
	(void) t;
}
/* -------------------- Numeric field with inputmask() --------------------- */
static void demo_inputmask(void)
{
	char buf[16];
	unsigned char r;
	unsigned int t;
	strcpy(buf, "42");
	r = conioxlib_inputmask("PIN", "Digits (min 2):", buf, 8, 2);
	box(39, 8, 76, 13, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Result", SHADOW);
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	gotoxy(41, 10);
	cprintf("btn=%u  text=[%s]", (unsigned) r, buf);
	t = conioxlib_ckey();
	(void) t;
}
/* ------------------ Scrollable list with listbox() ----------------------- */
static void demo_listbox(void)
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
	unsigned int t;
	items = (char (*)[41]) demo_newtable(items_list, 26, 41);
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
	t = conioxlib_ckey();
	(void) t;
	free(items);
}
/* ---------------- Progress with progress() and progressbar() --------------- */
static void demo_progress(void)
{
	unsigned int i;
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
}
/* ------------------------- Visual themes ------------------------------- */
/* Shows the nine themes as radio buttons with property(); choosing one
   applies it with conioxlib_theme() and the whole demo changes colors (the
   boxes and menus of main() already use gettheme()). */
static void demo_themes(void)
{
	static unsigned char demo_theme = 0;
	static const char *themenames_list[CONIOXLIB_THEMES] = {"Borland C++ 3.1", "Turbo C 2.0", "Turbo C 1.x", "Norton Utilities 9", "PC Tools", "Turbo Pascal 7.0", "Norton Commander", "MS-DOS Edit", "Lotus 1-2-3"};
	static const char *themedesc_list[CONIOXLIB_THEMES] = {"Turbo Vision 2.0: blue desktop, gray dialogs    ", "Blue desktop, yellow text and cyan menus        ", "Monochrome with black background                 ", "Blue with cyan frames and yellow hotkeys         ", "White on cyan, PC Tools look                    ", "Same Turbo Vision 2.0 palette as Borland C++ 3.1", "Blue panels, cyan menus and black focus bars     ", "Blue screen with single cyan frames              ", "White on black with inverted selection           "};
	char (*themenames)[21];
	char (*themedesc)[71];
	unsigned char sel[CONIOXLIB_THEMES];
	unsigned int i;
	unsigned int t;
	unsigned char r;
	struct conioxlib_stheme th;
	themenames = (char (*)[21]) demo_newtable(themenames_list, CONIOXLIB_THEMES, 21);
	themedesc = (char (*)[71]) demo_newtable(themedesc_list, CONIOXLIB_THEMES, 71);
	if ((themenames == 0) || (themedesc == 0))
	{
		free(themenames);
		free(themedesc);
		return;
	}
	conioxlib_gettheme(&th);
	box(39, 4, 76, 19, th.atrdialog, th.frame, "Themes", SHADOW);
	memset(sel, ' ', CONIOXLIB_THEMES);
	sel[demo_theme] = '\xFE';
	r = conioxlib_property(41, 6, th.atr0, th.atr1, th.atrhot, themenames, themedesc, (char *) sel, CONIOXLIB_THEMES, 1, 1);
	(void) r;
	for (i = 0; i < CONIOXLIB_THEMES; i++)
	{
		if (sel[i] == '\xFE')
		{
			demo_theme = (unsigned char) i;
			break;
		}
	}
	conioxlib_theme(demo_theme);
	conioxlib_gettheme(&th);
	textattr(th.atr0);
	gotoxy(41, 16);
	cprintf("Theme applied: %-18s", th.name);
	gotoxy(41, 17);
	cputs("Main menu and boxes follow it now.");
	t = conioxlib_ckey();
	(void) t;
	free(themenames);
	free(themedesc);
}
/* --------------------- Text editor with edit() ------------------------ */
static void demo_editor(void)
{
	conioxlib_edit(1, 1, 80, 25, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), "Editor", "");
}
/* --------------------- Status line and tooltip ------------------------- */
static void demo_statustip(void)
{
	unsigned int t;
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
	t = conioxlib_ckey();
	(void) t;
}
/* ------------------------------- Tetris -------------------------------- */
static const int tetrisshapes[7][4][2] = {	{{0, 0}, {1, 0}, {2, 0}, {3, 0}},
					{{0, 0}, {1, 0}, {0, 1}, {1, 1}},
					{{1, 0}, {0, 1}, {1, 1}, {2, 1}},
					{{1, 0}, {2, 0}, {0, 1}, {1, 1}},
					{{0, 0}, {1, 0}, {1, 1}, {2, 1}},
					{{0, 0}, {0, 1}, {1, 1}, {2, 1}},
					{{2, 0}, {0, 1}, {1, 1}, {2, 1}}};
static const unsigned char tetrissize[7] = {4, 2, 3, 3, 3, 3, 3};
static const unsigned char tetrisattrs[7] = {	CONIOXLIB_ATTR(CYAN, BLACK),
						CONIOXLIB_ATTR(YELLOW, BLACK),
						CONIOXLIB_ATTR(MAGENTA, BLACK),
						CONIOXLIB_ATTR(GREEN, BLACK),
						CONIOXLIB_ATTR(RED, BLACK),
						CONIOXLIB_ATTR(BLUE, BLACK),
						CONIOXLIB_ATTR(BROWN, BLACK)};
/* Checks if the piece cells fit at (px,py); rows above the well only test the columns */
static int demo_tetris_fit(int px, int py, int cells[4][2], unsigned char board[20][10])
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
static void demo_tetris_drawcell(int bx, int by, unsigned char val)
{
	if ((bx < 0) || (bx > 9) || (by < 0) || (by > 19))
	{
		return;
	}
	gotoxy(35 + (bx << 1), 3 + by);
	if (val != 0)
	{
		textattr(tetrisattrs[val - 1]);
		cputs("\xDB\xDB");
	}
	else
	{
		textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
		cputs("  ");
	}
}
static void demo_tetris(void)
{
	unsigned char t;
	int cur, next, px, py, key, i, j, k, n, speed, redraw;
	long score;
	int lines, level;
	int cells[4][2], saved[4][2];
	unsigned char board[20][10];
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
	for (i = 0; i != 10; i++)
	{
		for (j = 0; j != 20; j++)
		{
			demo_tetris_drawcell(i, j, 0);
		}
	}
	box(57, 2, 77, 23, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Status", SHADOW);
	conioxlib_status("Left/Right move; Up rotates; Space drops");
	next = rand() % 7;
	tetris_1:
	/* Spawn the next piece at the top of the well */
	cur = next;
	next = rand() % 7;
	px = (10 - tetrissize[cur]) >> 1;
	py = -2;
	memcpy(cells, tetrisshapes[cur], sizeof(cells));
	if (demo_tetris_fit(px, py, cells, board) == 0)
	{
		goto gameover_1;
	}
	redraw = 1;
	tetris_2:
	if (redraw)
	{
		for (i = 0; i != 4; i++)
		{
			demo_tetris_drawcell(px + cells[i][0], py + cells[i][1], (unsigned char) (cur + 1));
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
		for (j = 0; j != 4; j++)
		{
			for (i = 0; i != 4; i++)
			{
				gotoxy(62 + (i << 1), 15 + j);
				cputs("  ");
			}
		}
		for (i = 0; i != 4; i++)
		{
			textattr(tetrisattrs[next]);
			gotoxy(62 + (tetrisshapes[next][i][0] << 1), 15 + tetrisshapes[next][i][1]);
			cputs("\xDB\xDB");
		}
		redraw = 0;
	}
	/* Gravity: speed ticks of 20 ms per row, faster every level */
	speed = (21 - level > 1) ? (21 - level) : 2;
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
				if (demo_tetris_fit(px - 1, py, cells, board))
				{
					for (j = 0; j != 4; j++)
					{
						demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
					}
					px--;
					redraw = 1;
				}
			}
			else if (key == KBD_RIGHT)
			{
				if (demo_tetris_fit(px + 1, py, cells, board))
				{
					for (j = 0; j != 4; j++)
					{
						demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
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
					cells[j][1] = tetrissize[cur] - 1 - k;
				}
				if (demo_tetris_fit(px, py, cells, board))
				{
					for (j = 0; j != 4; j++)
					{
						demo_tetris_drawcell(px + saved[j][0], py + saved[j][1], 0);
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
				if (demo_tetris_fit(px, py + 1, cells, board))
				{
					for (j = 0; j != 4; j++)
					{
						demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
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
					demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
				}
				while (demo_tetris_fit(px, py + 1, cells, board))
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
	if (demo_tetris_fit(px, py + 1, cells, board))
	{
		for (j = 0; j != 4; j++)
		{
			demo_tetris_drawcell(px + cells[j][0], py + cells[j][1], 0);
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
		score += (long) ((n == 1) ? 100 : (n == 2) ? 300 : (n == 3) ? 700 : 1500) * level;
		level = (lines / 10) + 1;
	}
	for (i = 0; i != 10; i++)
	{
		for (j = 0; j != 20; j++)
		{
			demo_tetris_drawcell(i, j, board[j][i]);
		}
	}
	goto tetris_1;
	gameover_1:
	sprintf(str, "Game over. Score: %07ld", score);
	t = conioxlib_msgbox("Tetris", str, btns, 1);
	(void) t;
	exit_1:
	;
}
/* --------------------------- Demo exit --------------------------- */
static int demo_exit(void)
{
	if (conioxlib_exittodos(DEMO_HELP) == 1)
	{
		return(1);
	}
	return(0);
}
/* Help file contents written to DEMO_HELP on startup: one entry per public
   function of the library, plus the Main, Mouse and Exit topics used by the
   dialogs. Every line must stay within 60 characters because conioxlib_help()
   truncates at 60, and topic names must stay within 15 characters because
   conioxlib_helpindex() truncates them at 15. */
static const char CONIOXLIB_FARDATA helptext[] =
	"@Main" "\r\n"
	"Welcome to the CONXLIB 2.0 demonstration program." "\r\n"
	"Pick an option with the arrow keys, hotkeys or the mouse." "\r\n"
	"Press F1 for help. ESC in the main menu acts as Exit." "\r\n"
	"@Mouse" "\r\n"
	"conioxlib_pollmouse() updates conioxlib_mousestatus:" "\r\n"
	"x, y, lclick, rclick, wheelx and wheely." "\r\n"
	"Call it once per frame before testing any control." "\r\n"
	"On Windows the cursor position is polled every frame." "\r\n"
	"@Exit" "\r\n"
	"Exit asks for confirmation through conioxlib_exittodos()." "\r\n"
	"No keeps running the demo, Yes ends the program and Shell" "\r\n"
	"spawns a DOS shell and comes back afterwards." "\r\n"
	"@getkey" "\r\n"
	"Prototype:" "\r\n"
	"unsigned int conioxlib_getkey(void);" "\r\n"
	"Reads the next key from the keyboard." "\r\n"
	"Parameters: none." "\r\n"
	"Plain keys arrive as their ASCII code and extended keys as" "\r\n"
	"(scan code << 8), so the KBD_ constants can be compared" "\r\n"
	"directly. It also drains the pending key captured by" "\r\n"
	"conioxlib_pollmouse() under Windows." "\r\n"
	"Example:" "\r\n"
	"  if (conioxlib_kbhit())" "\r\n"
	"  {" "\r\n"
	"      k = conioxlib_getkey();" "\r\n"
	"  }" "\r\n"
	"@kbhit" "\r\n"
	"Prototype:" "\r\n"
	"int conioxlib_kbhit(void);" "\r\n"
	"Returns nonzero when a key is waiting." "\r\n"
	"Parameters: none." "\r\n"
	"It sees both the keyboard buffer and the pending key kept" "\r\n"
	"by conioxlib_pollmouse() under Windows, so it is the safe" "\r\n"
	"replacement for the raw kbhit()." "\r\n"
	"Example:" "\r\n"
	"  while (!conioxlib_kbhit())" "\r\n"
	"  {" "\r\n"
	"      conioxlib_clock();" "\r\n"
	"  }" "\r\n"
	"@pollmouse" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_pollmouse(void);" "\r\n"
	"Updates conioxlib_mousestatus with the mouse state." "\r\n"
	"Parameters: none." "\r\n"
	"The fields are x, y, lclick, rclick, wheelx and wheely." "\r\n"
	"lclick and rclick are edges that stay set until polled;" "\r\n"
	"the wheel fields are deltas since the last call." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_pollmouse();" "\r\n"
	"  if (conioxlib_mousestatus.lclick)" "\r\n"
	"  {" "\r\n"
	"      /* handle the click */" "\r\n"
	"  }" "\r\n"
	"@box" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_box(unsigned short x1, unsigned short y1," "\r\n"
	"    unsigned short x2, unsigned short y2, unsigned int atr," "\r\n"
	"    const char *frame, const char *title," "\r\n"
	"    unsigned int flags);" "\r\n"
	"Draws a framed window from (x1,y1) to (x2,y2)." "\r\n"
	"Parameters: atr is the frame color; frame is one of A_NONE," "\r\n"
	"A_BLOCK, A_SINGLE, A_DOUBLE, S_SINGLE, S_DOUBLE, U_SINGLE" "\r\n"
	"or U_DOUBLE; title is painted centered on the top border;" "\r\n"
	"flags combines SHADOW, CLOSE and NOWINDOW." "\r\n"
	"Example:" "\r\n"
	"  box(3, 3, 36, 23, CONIOXLIB_ATTR(BLACK, CYAN)," "\r\n"
	"      A_DOUBLE, \"Files\", SHADOW | CLOSE);" "\r\n"
	"@cputsc" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_cputsc(unsigned short x1, unsigned short x2," "\r\n"
	"    unsigned short y, const char *str);" "\r\n"
	"Writes str centered between columns x1 and x2 on row y." "\r\n"
	"Parameters: x1 and x2 define the centering range, y is the" "\r\n"
	"destination row and str the text to write." "\r\n"
	"Example:" "\r\n"
	"  cputsc(20, 60, 9, \"Title\");" "\r\n"
	"@blocks" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_blocks(void);" "\r\n"
	"Draws the lock indicator panel of line 25." "\r\n"
	"Parameters: none." "\r\n"
	"It paints the CAPS, NUM and SCROLL mnemonics in columns" "\r\n"
	"60 to 74, upper case when the lock is active, and it is" "\r\n"
	"also refreshed by conioxlib_clock()." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_blocks();" "\r\n"
	"@clock" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_clock(void);" "\r\n"
	"Refreshes the clock of line 25." "\r\n"
	"Parameters: none." "\r\n"
	"It shows the time in columns 76 to 80 with the color of" "\r\n"
	"the active theme and also refreshes the lock indicators." "\r\n"
	"Call it inside every wait loop; the compatibility alias" "\r\n"
	"is conioxlib_watch()." "\r\n"
	"Example:" "\r\n"
	"  while (!conioxlib_kbhit())" "\r\n"
	"  {" "\r\n"
	"      conioxlib_watch();" "\r\n"
	"  }" "\r\n"
	"@getcountry" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_getcountry(struct conioxlib_scountry *c);" "\r\n"
	"Fills c with the DOS country information." "\r\n"
	"Parameters: c receives the date format, the currency" "\r\n"
	"symbol, the thousand, decimal, date and time separators," "\r\n"
	"the currency style and digits and the time format." "\r\n"
	"Example:" "\r\n"
	"  struct conioxlib_scountry c;" "\r\n"
	"  conioxlib_getcountry(&c);" "\r\n"
	"  printf(\"Currency: %s\", c.curr);" "\r\n"
	"@getdiskfree" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_getdiskfree_ext(" "\r\n"
	"    unsigned char drive, struct conioxlib_sdiskfree *df);" "\r\n"
	"Fills df with the free and total space of a drive." "\r\n"
	"Parameters: drive is 0 for the current drive, 1 for A, 2" "\r\n"
	"for B and so on; df receives the sectors per cluster, the" "\r\n"
	"bytes per sector and the free and total clusters." "\r\n"
	"Returns 0 on success and 1 on error. DOS uses the FAT32" "\r\n"
	"aware int 21h AX=7303h with the AH=36h fallback." "\r\n"
	"Example:" "\r\n"
	"  struct conioxlib_sdiskfree df;" "\r\n"
	"  if (conioxlib_getdiskfree_ext(3, &df) == 0)" "\r\n"
	"  {" "\r\n"
	"      printf(\"%lu KB free\"," "\r\n"
	"          (df.avail_clusters * df.sectors_per_cluster" "\r\n"
	"          * df.bytes_per_sector) / 1024UL);" "\r\n"
	"  }" "\r\n"
	"@filesize" "\r\n"
	"Prototype:" "\r\n"
	"unsigned long int conioxlib_filesize(const char *path);" "\r\n"
	"Returns the length of a file in bytes." "\r\n"
	"Parameters: path is the file to measure." "\r\n"
	"It returns 0xFFFFFFFFUL when the file cannot be opened or" "\r\n"
	"read." "\r\n"
	"Example:" "\r\n"
	"  fsz = conioxlib_filesize(\"CONXDEMO.C\");" "\r\n"
	"@setpal" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_setpal(unsigned char e);" "\r\n"
	"Changes the text mode DAC palette." "\r\n"
	"Parameters: e selects the palette: 0 restores the classic" "\r\n"
	"16 color palette and any other value applies the library" "\r\n"
	"palette used by the demo." "\r\n"
	"It only works under DOS; on Windows it does nothing" "\r\n"
	"because the console has no DAC registers." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_setpal(1);" "\r\n"
	"@vgaremap" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_vgaremap(unsigned char on);" "\r\n"
	"Remaps glyphs of the 8x16 VGA font (DOS only)." "\r\n"
	"Parameters: on enables the library mouse pointer when" "\r\n"
	"nonzero and restores the driver pointer when zero." "\r\n"
	"With 1 it reprograms glyphs 0x01 (mouse pointer), 0x10" "\r\n"
	"and 0x11 (solid triangles) and 0x18 and 0x19 (chunky" "\r\n"
	"arrows) so the scroll bars and the pointer look like" "\r\n"
	"Turbo Vision. It is a no-op on Windows and 32-bit Watcom." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_vgaremap(1);" "\r\n"
	"@push" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_push(void);" "\r\n"
	"Clears the screen with a push effect." "\r\n"
	"Parameters: none." "\r\n"
	"The screen is folded away zone by zone in six passes and" "\r\n"
	"any key aborts the effect." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_push();" "\r\n"
	"@bounce" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_bounce(void);" "\r\n"
	"Clears the screen with a bounce effect." "\r\n"
	"Parameters: none." "\r\n"
	"The screen is thrown down and bounces back up until it" "\r\n"
	"leaves the display; any key aborts the effect." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_bounce();" "\r\n"
	"@wipe" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_wipe(void);" "\r\n"
	"Clears the screen with a diagonal wipe." "\r\n"
	"Parameters: none." "\r\n"
	"The screen is erased from one corner to the opposite one" "\r\n"
	"without using global buffers; any key aborts it." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_wipe();" "\r\n"
	"@ckey" "\r\n"
	"Prototype:" "\r\n"
	"unsigned int conioxlib_ckey(void);" "\r\n"
	"Waits for a key and returns it." "\r\n"
	"Parameters: none." "\r\n"
	"While waiting it keeps the clock, the lock indicators and" "\r\n"
	"the mouse status up to date, so the dialogs stay alive." "\r\n"
	"Example:" "\r\n"
	"  k = conioxlib_ckey();" "\r\n"
	"  if (k == KBD_ESC)" "\r\n"
	"  {" "\r\n"
	"      /* cancelled */" "\r\n"
	"  }" "\r\n"
	"@getstr" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_getstr(unsigned short x, unsigned short y," "\r\n"
	"    unsigned char atr, char *str, char len);" "\r\n"
	"Reads a string from the keyboard at (x,y)." "\r\n"
	"Parameters: atr is the color of the field, str receives the" "\r\n"
	"text and is also the initial value shown, and len is the" "\r\n"
	"maximum number of characters accepted." "\r\n"
	"BACKSPACE erases and ENTER accepts; ESC restores the" "\r\n"
	"initial text." "\r\n"
	"Example:" "\r\n"
	"  char name[15];" "\r\n"
	"  strcpy(name, \"CONXLIB\");" "\r\n"
	"  conioxlib_getstr(10, 5, CONIOXLIB_ATTR(WHITE, BLUE)," "\r\n"
	"      name, 14);" "\r\n"
	"@getpw" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_getpw(unsigned short x, unsigned short y," "\r\n"
	"    unsigned char atr, char *pass, char len);" "\r\n"
	"Reads a password without echoing it." "\r\n"
	"Parameters: atr is the color of the field, pass receives" "\r\n"
	"the text and len is the maximum length." "\r\n"
	"Every typed character is shown as an asterisk." "\r\n"
	"Example:" "\r\n"
	"  char pass[10];" "\r\n"
	"  conioxlib_getpw(10, 5, CONIOXLIB_ATTR(WHITE, BLUE)," "\r\n"
	"      pass, 9);" "\r\n"
	"@grid" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_grid(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned char atr0," "\r\n"
	"    unsigned char atr1, char items[][16]," "\r\n"
	"    unsigned char noptions);" "\r\n"
	"Picks an option from a grid of four columns per row." "\r\n"
	"Parameters: atr0 is the normal color and atr1 the selected" "\r\n"
	"one, items holds the texts and noptions their number." "\r\n"
	"The arrow keys, the mouse wheel and the left button move" "\r\n"
	"the selection. Returns the index of the chosen item or 255" "\r\n"
	"when cancelled with ESC." "\r\n"
	"Example:" "\r\n"
	"  i = conioxlib_grid(41, 6, atr0, atr1, items, 9);" "\r\n"
	"@cputs_hot" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_cputs_hot(unsigned short x," "\r\n"
	"    unsigned short y, unsigned char atr," "\r\n"
	"    unsigned char atrhot, const char *s);" "\r\n"
	"Writes s at (x,y) highlighting its accelerator key." "\r\n"
	"Parameters: atr is the base color and atrhot the color of" "\r\n"
	"the highlighted letter. An ampersand before a letter" "\r\n"
	"marks it as the accelerator and is never drawn; with no" "\r\n"
	"ampersand the first letter is the accelerator." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_cputs_hot(10, 5, atr, atrhot, \"&File\");" "\r\n"
	"@buttons" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_buttons(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned char atr0," "\r\n"
	"    unsigned char atr1, unsigned char atrhot," "\r\n"
	"    char items[][11], char desc[][71]," "\r\n"
	"    unsigned char noptions, unsigned char separation);" "\r\n"
	"Draws a row of Turbo Vision style buttons." "\r\n"
	"Parameters: (x1,y1) is the top left corner, atr0 and atr1" "\r\n"
	"are the normal and pressed colors, atrhot the accelerator" "\r\n"
	"color, items the labels, desc the status line hints," "\r\n"
	"noptions their number and separation the gap between" "\r\n"
	"buttons. The chosen button flashes inverted like the" "\r\n"
	"Turbo Vision press feedback. Returns the index of the" "\r\n"
	"pressed button, 255 when F1 was pressed or 0 when" "\r\n"
	"cancelled with ESC." "\r\n"
	"Example:" "\r\n"
	"  i = conioxlib_buttons(28, 12, atr0, atr1, atrhot," "\r\n"
	"      btns, descs, 3, 2);" "\r\n"
	"@menu" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_menu(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned char atr0," "\r\n"
	"    unsigned char atr1, unsigned char atrhot," "\r\n"
	"    char items[][21], char desc[][71]," "\r\n"
	"    unsigned char noptions, unsigned char separation," "\r\n"
	"    unsigned char sel);" "\r\n"
	"Runs a vertical menu with status line hints." "\r\n"
	"Parameters: (x1,y1) is the top left corner, atr0 and atr1" "\r\n"
	"the normal and selected colors, atrhot the accelerator" "\r\n"
	"color, items the entries, desc the hints, noptions their" "\r\n"
	"number, separation the distance between rows and sel the" "\r\n"
	"initial selection. Items starting with '-' are" "\r\n"
	"non-selectable separators. Returns the chosen index," "\r\n"
	"CONIOXLIB_MENU_ESC, CONIOXLIB_MENU_HELP," "\r\n"
	"CONIOXLIB_MENU_LEFT or CONIOXLIB_MENU_RIGHT. A scroll bar" "\r\n"
	"appears when the items do not fit in one page." "\r\n"
	"Example:" "\r\n"
	"  opt = conioxlib_menu(5, 4, th.atr0, th.atr1," "\r\n"
	"      th.atrhot, menu, desc, 28, 1, opt);" "\r\n"
	"@menubar" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_menubar(unsigned short y," "\r\n"
	"    unsigned char atr0, unsigned char atr1," "\r\n"
	"    unsigned char atrhot, char items[][21]," "\r\n"
	"    char desc[][71], unsigned char noptions," "\r\n"
	"    unsigned char init);" "\r\n"
	"Runs a Borland style top menu bar on row y." "\r\n"
	"Parameters: atr0 and atr1 are the normal and selected" "\r\n"
	"colors, atrhot the accelerator color, items the labels," "\r\n"
	"desc the hints, noptions their number and init the item" "\r\n"
	"highlighted on entry. The horizontal wheel and the arrows" "\r\n"
	"move; ENTER or a click picks an item." "\r\n"
	"Returns the chosen index, CONIOXLIB_MENU_ESC" "\r\n"
	"or CONIOXLIB_MENU_HELP." "\r\n"
	"Example:" "\r\n"
	"  i = conioxlib_menubar(1, atr0, atr1, atrhot, bar," "\r\n"
	"      desc, 4, 0);" "\r\n"
	"@option" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_option(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned char atr0," "\r\n"
	"    unsigned char atr1, unsigned char atrhot," "\r\n"
	"    char items[][21], char desc[][71], char *sel," "\r\n"
	"    unsigned char noptions, unsigned char separation," "\r\n"
	"    unsigned char enabled);" "\r\n"
	"Runs a list of check boxes." "\r\n"
	"Parameters: sel holds one mark byte per item, a space for" "\r\n"
	"unchecked and the character \\xFE for checked; enabled" "\r\n"
	"must be nonzero to allow interaction. The space bar and" "\r\n"
	"the left button toggle the entries. Returns 255 when F1" "\r\n"
	"was pressed or 0 when the dialog is closed." "\r\n"
	"Example:" "\r\n"
	"  memset(sel, ' ', 9);" "\r\n"
	"  sel[0] = '\\xFE';" "\r\n"
	"  r = conioxlib_option(41, 6, atr0, atr1, atrhot," "\r\n"
	"      items, desc, sel, 9, 1, 1);" "\r\n"
	"@property" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_property(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned char atr0," "\r\n"
	"    unsigned char atr1, unsigned char atrhot," "\r\n"
	"    char items[][21], char desc[][71], char *sel," "\r\n"
	"    unsigned char noptions, unsigned char separation," "\r\n"
	"    unsigned char enabled);" "\r\n"
	"Runs a radio button list: exactly one item is marked." "\r\n"
	"Parameters: identical to conioxlib_option(), but only one" "\r\n"
	"entry holds the character \\xFE mark at a time." "\r\n"
	"Returns 255 when F1 was pressed or 0 on close." "\r\n"
	"Example:" "\r\n"
	"  sel[demo_theme] = '\\xFE';" "\r\n"
	"  r = conioxlib_property(41, 6, atr0, atr1, atrhot," "\r\n"
	"      names, desc, sel, 9, 1, 1);" "\r\n"
	"@background" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_background(unsigned char msg[]);" "\r\n"
	"Paints the application background with the colors of the" "\r\n"
	"active theme and writes msg centered on line 25." "\r\n"
	"Parameters: msg is the message for the status area; an" "\r\n"
	"empty string leaves the status area blank." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_background((unsigned char *) DEMO_TITLE);" "\r\n"
	"@hello" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_hello(char *name, unsigned char commercial);" "\r\n"
	"Saves the screen to a file and shows the about box." "\r\n"
	"Parameters: name is the file used to save the screen that" "\r\n"
	"conioxlib_bye() will restore; commercial selects between" "\r\n"
	"the shareware and the personal about text." "\r\n"
	"Under Turbo C it also installs the critical error" "\r\n"
	"handler restored by conioxlib_bye()." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_hello(\"CONXDEMO.TMP\", 0);" "\r\n"
	"@bye" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_bye(char *name, unsigned char e," "\r\n"
	"    unsigned char commercial);" "\r\n"
	"Restores the screen saved by conioxlib_hello() and ends" "\r\n"
	"the program." "\r\n"
	"Parameters: name is the same file given to hello(), e is" "\r\n"
	"the exit code and commercial selects the farewell text." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_bye(\"CONXDEMO.TMP\", 0, 0);" "\r\n"
	"@about" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_about(unsigned char commercial);" "\r\n"
	"Shows the credits box of the library." "\r\n"
	"Parameters: commercial selects between the shareware and" "\r\n"
	"the personal text." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_about(0);" "\r\n"
	"@exittodos" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_exittodos(char *name);" "\r\n"
	"Asks for confirmation before leaving to DOS." "\r\n"
	"Parameters: name is the help file shown by its F1 button." "\r\n"
	"Returns 1 when the user confirms the exit and 0 when he" "\r\n"
	"stays. The Shell answer runs conioxlib_shell() and also" "\r\n"
	"returns 0." "\r\n"
	"Example:" "\r\n"
	"  if (conioxlib_exittodos(DEMO_HELP) == 1)" "\r\n"
	"  {" "\r\n"
	"      exit(0);" "\r\n"
	"  }" "\r\n"
	"@screensaver" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_screensaver(void);" "\r\n"
	"Runs the block screen saver until a key or mouse event." "\r\n"
	"Parameters: none." "\r\n"
	"The screen is removed and put back block by block; any" "\r\n"
	"key, click or wheel movement wakes it up." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_screensaver();" "\r\n"
	"@shell" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_shell(unsigned short x1, unsigned short y1," "\r\n"
	"    unsigned short x2, unsigned short y2, char *command);" "\r\n"
	"Runs a DOS shell inside a window." "\r\n"
	"Parameters: the rectangle frames the shell output and" "\r\n"
	"command is the program to run; an empty command starts" "\r\n"
	"the interactive interpreter named by COMSPEC. Under" "\r\n"
	"Windows the shell takes the whole screen and the" "\r\n"
	"rectangle is ignored. Type EXIT inside the shell to" "\r\n"
	"come back; the screen is restored on return." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_shell(3, 2, 75, 22, \"\");" "\r\n"
	"@calendar" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_calendar(unsigned short x," "\r\n"
	"    unsigned short y, unsigned char atr0," "\r\n"
	"    unsigned char atr1, unsigned char atr2," "\r\n"
	"    unsigned char *day, unsigned char *month," "\r\n"
	"    unsigned int *year);" "\r\n"
	"Draws an interactive month calendar at (x,y)." "\r\n"
	"Parameters: atr0 is the color of the normal days, atr1" "\r\n"
	"the color of the selected day and atr2 the color of the" "\r\n"
	"Sundays; day, month and year receive the date picked" "\r\n"
	"with the cursors, PGUP/PGDW (month), HOME/END (year)" "\r\n"
	"or the mouse. Returns 1 when ENTER picks a day and 0" "\r\n"
	"when ESC cancels." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_calendar(30, 5, atr0, atr1, atr2," "\r\n"
	"      &day, &month, &year);" "\r\n"
	"@view" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_view(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned short x2," "\r\n"
	"    unsigned short y2, unsigned char atr0," "\r\n"
	"    unsigned char atr1, char buffer[][61]," "\r\n"
	"    unsigned char last, const char *title);" "\r\n"
	"Displays a scrollable text viewer." "\r\n"
	"Parameters: buffer holds the lines (60 usable columns)," "\r\n"
	"last is the number of lines minus one, atr0 the text" "\r\n"
	"color, atr1 the selected line color and title the frame" "\r\n"
	"title. The wheel, the arrows, PGUP, PGDN, HOME and END" "\r\n"
	"move; clicking a line jumps to it. Returns 255 when" "\r\n"
	"cancelled with ESC." "\r\n"
	"Example:" "\r\n"
	"  r = conioxlib_view(6, 3, 72, 21, atr0, atr1," "\r\n"
	"      helpbuf, n - 1, \"Help: getkey\");" "\r\n"
	"@help" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_help(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned short x2," "\r\n"
	"    unsigned short y2, unsigned char atr0," "\r\n"
	"    unsigned char atr1, char *name, char *topic," "\r\n"
	"    char buffer[][61]);" "\r\n"
	"Shows one topic of a help file." "\r\n"
	"Parameters: name is the help file, topic the topic to" "\r\n"
	"find (its header line is @topic) and buffer the work area" "\r\n"
	"for the text. Returns 1 when the topic was found and 0" "\r\n"
	"when it was not or the file is missing. Lines are cut at" "\r\n"
	"60 characters and the next @ line ends the entry." "\r\n"
	"Example:" "\r\n"
	"  r = conioxlib_help(7, 3, 69, 21, atr0, atr1," "\r\n"
	"      DEMO_HELP, \"getkey\", helpbuf);" "\r\n"
	"@helpindex" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_helpindex(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned char atr0," "\r\n"
	"    unsigned char atr1, char *name, char buffer[][16]);" "\r\n"
	"Lists every topic of a help file and opens the picked" "\r\n"
	"one." "\r\n"
	"Parameters: name is the help file and buffer a work area" "\r\n"
	"that receives the topic names. Returns the index of the" "\r\n"
	"topic shown or 255 when cancelled." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_helpindex(5, 3, atr0, atr1, DEMO_HELP," "\r\n"
	"      helpbuf);" "\r\n"
	"@filemultisel" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_filemultiselector(" "\r\n"
	"    unsigned char *sdisk, char *sdir," "\r\n"
	"    struct conioxlib_sfiles *files);" "\r\n"
	"Runs the file selector with multiple marks." "\r\n"
	"Parameters: sdisk receives the chosen drive letter, sdir" "\r\n"
	"the chosen directory and files the table with the name," "\r\n"
	"size, date, time, description and selected mark of every" "\r\n"
	"entry. F2 changes the mask, INS or SPACE mark a file, F3" "\r\n"
	"marks all, F4 unmarks all and ENTER enters a directory" "\r\n"
	"or accepts a drive. Returns 0 when accepted and 255 when" "\r\n"
	"cancelled." "\r\n"
	"Example:" "\r\n"
	"  disk = 'C';" "\r\n"
	"  if (conioxlib_filemultiselector(&disk, dir, files) == 0)" "\r\n"
	"  {" "\r\n"
	"      sprintf(str, \"%c:%s\", disk, dir);" "\r\n"
	"  }" "\r\n"
	"@fileselector" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_fileselector(" "\r\n"
	"    unsigned char *sdisk, char *sdir, char *name);" "\r\n"
	"Runs a compact picker for a single file: drives," "\r\n"
	"directories and names only, without sizes, dates or" "\r\n"
	"descriptions, so it fits the Open and Save as dialogs" "\r\n"
	"of the editor. Parameters: sdisk receives the chosen" "\r\n"
	"drive letter, sdir the chosen directory and name the" "\r\n"
	"chosen file name. Returns 1 when a file was chosen and" "\r\n"
	"0 when cancelled." "\r\n"
	"Example:" "\r\n"
	"  if (conioxlib_fileselector(&disk, dir, name) != 0)" "\r\n"
	"  {" "\r\n"
	"      sprintf(str, \"%c:%s%s\", disk, dir, name);" "\r\n"
	"  }" "\r\n"
	"@theme" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_theme(unsigned char id);" "\r\n"
	"Applies one of the nine visual themes." "\r\n"
	"Parameters: id is 0 Borland C++ 3.1, 1 Turbo C 2.0, 2 Turbo" "\r\n"
	"C 1.x, 3 Norton Utilities 9, 4 PC Tools, 5 Turbo Pascal" "\r\n"
	"7.0, 6 Norton Commander, 7 MS-DOS Edit and 8 Lotus 1-2-3." "\r\n"
	"Themes 0 and 5 share the real Turbo Vision 2.0 palette" "\r\n"
	"used by both Borland IDEs." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_theme(1);" "\r\n"
	"@gettheme" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_gettheme(struct conioxlib_stheme *t);" "\r\n"
	"Copies the active theme into t." "\r\n"
	"Parameters: t receives atr0, atr1, atrhot, atrstatus," "\r\n"
	"atrdialog, the frame string and the theme name." "\r\n"
	"Example:" "\r\n"
	"  struct conioxlib_stheme th;" "\r\n"
	"  conioxlib_gettheme(&th);" "\r\n"
	"  box(3, 3, 36, 23, th.atrdialog, th.frame," "\r\n"
	"      \"CONXLIB 2.0\", SHADOW | CLOSE);" "\r\n"
	"@savescr" "\r\n"
	"Prototype:" "\r\n"
	"char *conioxlib_savescr(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned short x2," "\r\n"
	"    unsigned short y2);" "\r\n"
	"Saves a screen region with malloc." "\r\n"
	"Parameters: the rectangle to save. Returns the buffer or 0" "\r\n"
	"when there is no memory. Free it with" "\r\n"
	"conioxlib_restorescr()." "\r\n"
	"Example:" "\r\n"
	"  oldb = conioxlib_savescr(3, 2, 75, 22);" "\r\n"
	"@restorescr" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_restorescr(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned short x2," "\r\n"
	"    unsigned short y2, char *buf);" "\r\n"
	"Restores a region saved by conioxlib_savescr() and frees" "\r\n"
	"the buffer." "\r\n"
	"Parameters: the same rectangle given on save and the" "\r\n"
	"buffer returned by conioxlib_savescr()." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_restorescr(3, 2, 75, 22, oldb);" "\r\n"
	"@status" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_status(const char *msg);" "\r\n"
	"Writes msg on the status line." "\r\n"
	"Parameters: msg is the text to show." "\r\n"
	"The status line lives in columns 1 to 59 of line 25; the" "\r\n"
	"lock indicators use 60 to 74 and the clock 76 to 80." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_status(\"Ready.\");" "\r\n"
	"@tooltip" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_tooltip(unsigned short x, unsigned short y," "\r\n"
	"    const char *msg, unsigned int ms);" "\r\n"
	"Shows a tooltip near (x,y) during ms milliseconds." "\r\n"
	"Parameters: msg is the text of the tip." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_tooltip(50, 12, \"Tooltip sample\", 1500);" "\r\n"
	"@progressbar" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_progressbar(unsigned short x," "\r\n"
	"    unsigned short y, unsigned short width," "\r\n"
	"    unsigned char atr, unsigned int value," "\r\n"
	"    unsigned int maxv);" "\r\n"
	"Draws an installer style progress ruler." "\r\n"
	"Parameters: (x,y) is the position, width the length in" "\r\n"
	"cells, atr the color, and value and maxv the proportion" "\r\n"
	"shown: the ruler spans width - 5 cells as a 0xB0 grid" "\r\n"
	"that fills up with solid 0xDB cells like the Turbo" "\r\n"
	"Vision gauges and the percentage number follows it" "\r\n"
	"on the right." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_progressbar(22, 13, 30, atr, done, total);" "\r\n"
	"@progress" "\r\n"
	"Prototype:" "\r\n"
	"void conioxlib_progress(const char *title," "\r\n"
	"    unsigned int value, unsigned int maxv);" "\r\n"
	"Shows an installer style gauge on the status line." "\r\n"
	"Parameters: title is the label drawn at column 1 and" "\r\n"
	"value and maxv the proportion. The gauge starts at" "\r\n"
	"column 30 with the status line color of the active" "\r\n"
	"theme and conioxlib_progressbar() ends it with the" "\r\n"
	"percentage number on the right." "\r\n"
	"Example:" "\r\n"
	"  for (i = 0; i <= 100; i++)" "\r\n"
	"  {" "\r\n"
	"      conioxlib_progress(\"Copying\", i, 100);" "\r\n"
	"      delay(50);" "\r\n"
	"  }" "\r\n"
	"@msgbox" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_msgbox(const char *title," "\r\n"
	"    const char *msg, char buttons[][11]," "\r\n"
	"    unsigned char nbuttons);" "\r\n"
	"Shows a message box with up to three buttons." "\r\n"
	"Parameters: title is the caption, msg the text, buttons" "\r\n"
	"the labels and nbuttons their number (1 to 3); passing 0" "\r\n"
	"as buttons uses the defaults OK and Cancel. Returns as" "\r\n"
	"conioxlib_buttons() does: the index of the pressed" "\r\n"
	"button, 255 when F1 was pressed or 0 when cancelled." "\r\n"
	"Example:" "\r\n"
	"  r = conioxlib_msgbox(\"Confirm\"," "\r\n"
	"      \"Exit the demo?\", btns, 2);" "\r\n"
	"@inputbox" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_inputbox(const char *title," "\r\n"
	"    const char *label, char *buf, char len);" "\r\n"
	"Asks for a string in a dialog box." "\r\n"
	"Parameters: title is the caption, label the text next to" "\r\n"
	"the field, buf receives the answer and is also the" "\r\n"
	"initial value, and len the maximum length. Returns 0" "\r\n"
	"when accepted with ENTER, 1 when cancelled and 255 when" "\r\n"
	"F1 was pressed." "\r\n"
	"Example:" "\r\n"
	"  r = conioxlib_inputbox(\"Open\", \"File:\", name, 60);" "\r\n"
	"@inputmask" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_inputmask(const char *title," "\r\n"
	"    const char *label, char *buf, char len," "\r\n"
	"    char mindigits);" "\r\n"
	"Asks for a digits-only string in a dialog box." "\r\n"
	"Parameters: like conioxlib_inputbox(), plus mindigits:" "\r\n"
	"the OK button only accepts the answer when at least that" "\r\n"
	"many digits were typed. Invalid characters are ignored" "\r\n"
	"as they are pressed. Returns 0 when accepted, 1 when" "\r\n"
	"cancelled and 255 when F1 was pressed." "\r\n"
	"Example:" "\r\n"
	"  r = conioxlib_inputmask(\"Port\", \"Value:\", buf," "\r\n"
	"      5, 2);" "\r\n"
	"@listbox" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_listbox(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned short x2," "\r\n"
	"    unsigned short y2, unsigned char atr0," "\r\n"
	"    unsigned char atr1, char items[][41]," "\r\n"
	"    unsigned char nitems, const char *title);" "\r\n"
	"Runs a scrollable list box with a scroll bar." "\r\n"
	"Parameters: atr0 is the normal color, atr1 the selected" "\r\n"
	"one, items the texts (up to 40 characters), nitems their" "\r\n"
	"number and title the caption. The arrows, PGUP, PGDN," "\r\n"
	"HOME, END and the wheel move; ENTER or a click chooses a" "\r\n"
	"row and a click on the bar jumps to that position." "\r\n"
	"Returns the index of the chosen item or 255 when" "\r\n"
	"cancelled with ESC." "\r\n"
	"Example:" "\r\n"
	"  i = conioxlib_listbox(30, 6, 55, 18, atr0, atr1," "\r\n"
	"      items, 26, \"Pick one\");" "\r\n"
	"@error" "\r\n"
	"Prototype (Turbo C DOS builds only):" "\r\n"
	"void interrupt error(unsigned int, ...);" "\r\n"
	"Critical error handler installed by conioxlib_hello()." "\r\n"
	"Parameters: the interrupt frame registers." "\r\n"
	"It keeps the CRITICAL ERROR flag set for conioxlib_shell()." "\r\n"
	"Do not call it directly; conioxlib_bye() restores" "\r\n"
	"the previous handler." "\r\n"
	"Example:" "\r\n"
	"  /* installed automatically by conioxlib_hello() */" "\r\n"
	"@shell_patch" "\r\n"
	"Prototype (Turbo C DOS builds only):" "\r\n"
	"void interrupt conioxlib_shell_patch(unsigned int, ...);" "\r\n"
	"Int 29h filter installed by conioxlib_shell() under Turbo C." "\r\n"
	"Parameters: the interrupt frame registers." "\r\n"
	"It redirects the CON output of the child process into the" "\r\n"
	"shell window. Do not call it directly." "\r\n"
	"Example:" "\r\n"
	"  /* installed automatically by conioxlib_shell() */" "\r\n"
	"@edit" "\r\n"
	"Prototype:" "\r\n"
	"unsigned char conioxlib_edit(unsigned short x1," "\r\n"
	"    unsigned short y1, unsigned short x2," "\r\n"
	"    unsigned short y2, unsigned char atr0," "\r\n"
	"    unsigned char atr1, const char *title," "\r\n"
	"    const char *filename);" "\r\n"
	"Embeds a text editor in a rectangle." "\r\n"
	"Parameters: atr0 is the text color, atr1 the color of the" "\r\n"
	"active line, title the caption and filename the file" "\r\n"
	"loaded at startup (an empty string starts empty). The" "\r\n"
	"File, Edit and Search menus open with F10 or a click;" "\r\n"
	"the mouse and the wheel work, PGUP/PGDW page and HOME/" "\r\n"
	"END jump, and Shift plus those keys marks a block that" "\r\n"
	"Cut, Copy, Paste and typing replace. File > Open picks" "\r\n"
	"the file with conioxlib_fileselector() and the undo" "\r\n"
	"and redo stacks keep four levels. Returns 0." "\r\n"
	"Example:" "\r\n"
	"  conioxlib_edit(1, 1, 80, 25, atr0, atr1," "\r\n"
	"      \"Editor\", \"NOTES.TXT\");" "\r\n";
/* ----------------------------------------------------------------------------------------------------------------- */
/* ------------------------------ Main -------------------------------- */
int main(void)
{
	unsigned char opt;
	struct conioxlib_stheme th;
	char (*demomenu)[21];
	char (*demodesc)[71];
	char *helpbuf;
	FILE *f;
	int done = 0;
	coniox_init(NULL);
	_setcursortype(_NOCURSOR);
	/* Write the help file with one entry per public function of the library */
	f = fopen(DEMO_HELP, "wb");
	if (f != 0)
	{
		fwrite(helptext, sizeof(helptext) - 1, 1, f);
		fclose(f);
	}
	conioxlib_hello("CONXDEMO.TMP", 0);
	/* The menu tables are copied once from the literals into heap buffers */
	demomenu = (char (*)[21]) demo_newtable(demooptions, DEMO_OPTIONS, 21);
	demodesc = (char (*)[71]) demo_newtable(demohints, DEMO_OPTIONS, 71);
	if ((demomenu == 0) || (demodesc == 0))
	{
		conioxlib_bye("CONXDEMO.TMP", 0, 0);
		return(1);
	}
	opt = 0;
	for (;;)
	{
		conioxlib_background((unsigned char *) DEMO_TITLE);
		/* Boxes and menu with the colors of the active theme; the menu starts at
		   row 4 so its 19 visible items stay inside the box */
		conioxlib_gettheme(&th);
		box(3, 3, 36, 23, th.atrdialog, th.frame, "CONXLIB 2.0", SHADOW | CLOSE);
		box(38, 3, 77, 23, th.atrdialog, th.frame, "Output", SHADOW);
		conioxlib_blocks();
		opt = conioxlib_menu(5, 4, th.atr0, th.atr1, th.atrhot, demomenu, demodesc, DEMO_OPTIONS, 1, opt);
		if (opt == CONIOXLIB_MENU_HELP)
		{
			/* The topic index reuses the same 256*61 work area size of the viewer pages */
			helpbuf = (char *) malloc(DEMO_HELPBUF);
			if (helpbuf != 0)
			{
				conioxlib_helpindex(5, 3, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), DEMO_HELP, (char (*)[16]) helpbuf);
				free(helpbuf);
			}
			continue;
		}
		if (opt == CONIOXLIB_MENU_ESC)
		{
			opt = 0;
		}
		demo_clear_output();
		switch (opt)
		{
			case 1: demo_windows(); break;
			case 2: demo_mouse(); break;
			case 3: demo_textinput(); break;
			case 4: demo_checkboxes(); break;
			case 5: demo_radiobuttons(); break;
			case 6: demo_grid(); break;
			case 7: demo_calendar(); break;
			case 8: demo_fileselector(); break;
			case 9: demo_viewer(); break;
			case 10: demo_screensaver(); break;
			case 11: demo_effects(); break;
			case 12: demo_buttons(); break;
			case 13: demo_shell(); break;
			case 14: demo_palette(); break;
			case 15: demo_colorattrs(); break;
			case 16: demo_menubar(); break;
			case 17: demo_about(); break;
			case 18: demo_msgbox(); break;
			case 19: demo_inputbox(); break;
			case 20: demo_inputmask(); break;
			case 21: demo_listbox(); break;
			case 22: demo_progress(); break;
			case 23: demo_themes(); break;
			case 24: demo_editor(); break;
			case 25: demo_statustip(); break;
			case 26: demo_tetris(); break;
			case 0:
			default:
				if (demo_exit())
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
