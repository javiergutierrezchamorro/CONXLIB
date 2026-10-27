/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coniox.h"
#include "conxlib.h"

/* ----------------------------------------------------------------------------------------------------------------- */
#define DEMO_OPTIONS	26
#define DEMO_TITLE	"CONXLIB DEMO 2.0               (c) Copyright 1997-2026 Javier Gutierrez Chamorro"
#define DEMO_HELP	"CONXDEMO.HLP"
#define DEMO_HOT	CONIOXLIB_ATTR(RED, LIGHTGRAY)
#define DEMO_HOT_SEL	CONIOXLIB_ATTR(RED, GREEN)

#if ((__DOS__) || (__MSDOS__)) && !defined(__DJGPP__)
	#if defined(__TURBOC__)
		extern unsigned _stklen = 16384U;
	#elif defined(__WATCOMC__)
		unsigned __stack = 16384U;
	#endif
#endif

static struct conioxlib_sfiles files[220];
static char helpbuf[256][61];
static char demomenu[DEMO_OPTIONS][21] = {	"&Exit",
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
											"S&tatus/tooltip"};
static char demodesc[DEMO_OPTIONS][71] = {	"Leave the demo: conioxlib_exittodos() confirms and bye restores       ",
											"All box() frame styles: double, single, rounded, solid                ",
											"Live conioxlib_mousestatus: position, buttons and wheel               ",
											"conioxlib_getstr() and getpw(): field editors with cursor keys        ",
											"conioxlib_option(): toggle marks with Space or Ins keys               ",
											"conioxlib_property(): only one of the marks stays active              ",
											"conioxlib_grid(): grid picker returning the selected cell             ",
											"conioxlib_calendar(): month view with highlighted weekends            ",
											"conioxlib_select(): drives, folders, files and marked picks           ",
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
											"conioxlib_theme(): Borland, Turbo C, Norton, PC Tools                 ",
											"conioxlib_edit(): simple Borland-style text editor                    ",
											"conioxlib_status and tooltip without touching clock/locks             "};
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
static char *demotext[] = {	"CONXLIB 2.0 - Retro console library over CONIOX 5.03",
							"",
							"Accelerators: mark a letter with & in the item text.",
							"Press that letter to activate the option. atrhot sets",
							"the colour of the hot letter (passed to menu/buttons).",
							"",
							"Works with Borland Turbo C, Watcom C (16 and 32 bits),",
							"DJGPP, and Microsoft Visual C++ for Windows 32/64 bits.",
							0};

/* ------------------ Limpia la ventana de salida ------------------------- */
static void demo_clear_output(void)
{
	textattr(CONIOXLIB_ATTR(BLACK, LIGHTGRAY));
	window(39, 4, 76, 22);
	clrscr();
	window(1, 1, 80, 25);
}

/* --------------------- Muestra los estilos de box() --------------------- */
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

/* ---------------- Estado del raton en vivo con pollmouse() --------------- */
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

/* -------------- Entrada de texto con getstr() y getpw() ----------------- */
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
	conioxlib_getpw(41, 12, CONIOXLIB_ATTR(WHITE, BLUE), pass, 8);
	textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
	gotoxy(41, 14);
	cprintf("String: [%s]", str);
	gotoxy(41, 15);
	cprintf("Password: [%s]", pass);
	t = conioxlib_ckey();
	(void) t;
}

/* ------------------ Casillas de marcar con option() --------------------- */
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

/* ------------------ Botones de radio con property() --------------------- */
static void demo_radiobuttons(void)
{
	unsigned char propsel[5];
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
		cprintf("Choice: %s", optmenu[i]);
	}
	t = conioxlib_ckey();
	(void) t;
}

/* -------------------- Tabla de celdas con grid() ------------------------- */
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

/* ---------------------- Calendario con calendar() ------------------------ */
static void demo_calendar(void)
{
	conioxlib_calendar(21, 5, CONIOXLIB_ATTR(BLACK, CYAN), CONIOXLIB_ATTR(WHITE, BLUE), CONIOXLIB_ATTR(LIGHTRED, CYAN), 0, 0, 0);
}

/* ----------- Selector de ficheros, disco y tamano de fichero ------------- */
static void demo_fileselector(void)
{
	char dir[80];
	char str[80];
	unsigned char disk;
	unsigned int i;
	unsigned int t;
	struct conioxlib_sdiskfree df;
	unsigned long int fsz;

	strcpy(dir, "\\");
	disk = 'C';
	if (conioxlib_select(&disk, dir, files) == 0)
	{
		box(20, 7, 60, 16, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_DOUBLE, "Selection", SHADOW);
		sprintf(str, "%c:%s", disk, dir);
		textattr(CONIOXLIB_ATTR(WHITE, LIGHTGRAY));
		cputsc(20, 60, 9, str);
		i = 0;
		while ((i < 220) && ((files[i].selected == 0) || (files[i].size[0] == '\xF0')))
		{
			i++;
		}
		gotoxy(22, 11);
		if (i < 220)
		{
			cprintf("Marked: %s", files[i].name);
		}
		else
		{
			cputs("Marked: <none>");
		}
		/* Espacio libre de la unidad elegida con getdiskfree_ext() */
		gotoxy(22, 13);
		if (conioxlib_getdiskfree_ext((unsigned char) (disk - 'A' + 1), &df) == 0)
		{
			cprintf("Free: %lu KB of %lu KB", (df.avail_clusters * df.sectors_per_cluster * df.bytes_per_sector) / 1024UL, (df.total_clusters * df.sectors_per_cluster * df.bytes_per_sector) / 1024UL);
		}
		else
		{
			cputs("Free: <not available>");
		}
		/* Tamano del fichero marcado con filesize() */
		gotoxy(22, 14);
		if (i < 220)
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
}

/* ------------------------ Visor de texto con view() ---------------------- */
static void demo_viewer(void)
{
	unsigned int i;
	unsigned int ultimo;

	i = 0;
	while (demotext[i] != 0)
	{
		memset(helpbuf[i], 0, 61);
		strncpy(helpbuf[i], demotext[i], 60);
		i++;
	}
	ultimo = i - 1;
	conioxlib_view(6, 3, 72, 21, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), helpbuf, (unsigned char) ultimo, "CONXLIB overview");
}

/* -------------------- Salvapantallas con screensaver() ------------------- */
static void demo_screensaver(void)
{
	conioxlib_screensaver();
}

/* -------------------- Efectos push, bounce y wipe ------------------------ */
static void demo_effects(void)
{
	conioxlib_push();
	conioxlib_bounce();
	conioxlib_wipe();
}

/* ------------------- Barra de botones con buttons() ---------------------- */
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
	i = conioxlib_buttons(40, 12, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), CONIOXLIB_ATTR(WHITE, GREEN), DEMO_HOT, btnmenu, btndesc, 4, 1);
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

/* -------------------------- Salida a DOS con shell() ---------------------- */
static void demo_shell(void)
{
	conioxlib_shell(3, 2, 75, 22, "");
}

/* ---------------- Paleta VGA con setpal() y vgaremap() ------------------- */
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
	/* vgaremap(): triangulos y flechas gruesas en la fuente 8x16 (solo DOS);
	   en Windows la consola ya trae los caracteres de CP437 */
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

/* --------------- Todos los colores con CONIOXLIB_ATTR -------------------- */
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

/* -------------- Barra de menus y desplegables estilo IDE ---------------- */
static void demo_menubar(void)
{
	static char baritems[10][21] = {"&File", "&Edit", "&Search", "&Run", "&Compile", "&Debug", "&Project", "&Options", "&Window", "&Help"};
	static char bardesc[10][71] = {
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
	static char fileitems[11][21] = {
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
	static char filedesc[11][71] = {
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
	static char edititems[5][21] = {
		"Cu&t           ",
		"&Copy          ",
		"&Paste         ",
		"-",
		"Cl&ear         "};
	static char editdesc[5][71] = {
		"Cut selection to clipboard                                        ",
		"Copy selection to clipboard                                       ",
		"Paste from clipboard                                              ",
		"                                                                  ",
		"Clear the current selection                                       "};
	unsigned char bar, sub;
	char *oldscr;
	int donebar;
	unsigned int i;
	unsigned int t;
	struct conioxlib_stheme th;

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
	/* Redibujar la barra con los colores del tema activo */
	conioxlib_gettheme(&th);
	bar = conioxlib_menubar(1, th.atr0, th.atr1, th.atrhot, baritems, bardesc, 10, 0);
	donebar = 0;
	while ((bar != CONIOXLIB_MENU_ESC) && (!donebar))
	{
		if (oldscr != 0)
		{
			puttext(1, 1, 80, 25, oldscr);
		}
		textattr(th.atr0);
		gotoxy(1, 1);
		for (i = 0; i < 80; i++)
		{
			putch(' ');
		}
		{
			int bi, pos, k, visible;

			pos = 1;
			for (bi = 0; bi < 10; bi++)
			{
				/* Anchura de celda = longitud visible (sin el '&') + 2 */
				visible = 0;
				for (k = 0; baritems[bi][k] != 0; k++)
				{
					if (baritems[bi][k] != '&')
					{
						visible++;
					}
				}
				if (bi == (int) bar)
				{
					gotoxy(pos, 1);
					textattr(th.atr1);
					putch(' ');
					conioxlib_cputs_hot((unsigned short) (pos + 1), 1, th.atr1, th.atrhot, baritems[bi]);
					textattr(th.atr1);
					putch(' ');
				}
				else
				{
					gotoxy(pos, 1);
					textattr(th.atr0);
					putch(' ');
					conioxlib_cputs_hot((unsigned short) (pos + 1), 1, th.atr0, th.atrhot, baritems[bi]);
					textattr(th.atr0);
					putch(' ');
				}
				pos += visible + 2;
			}
		}
		if (bar == 0)
		{
			box(1, 2, 17, 14, CONIOXLIB_ATTR(BLACK, LIGHTGRAY), A_SINGLE, "", SHADOW);
			sub = conioxlib_menu(2, 3, th.atr0, th.atr1, th.atrhot, fileitems, filedesc, 11, 1, 0);
		}
		else if (bar == 1)
		{
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
			cprintf("Selected: %s", baritems[bar]);
			t = conioxlib_ckey();
			(void) t;
			break;
		}
		window(1, 1, 80, 25);
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
			continue;
		}
		if (sub == CONIOXLIB_MENU_RIGHT)
		{
			bar++;
			if (bar > 9)
			{
				bar = 0;
			}
			continue;
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
			cprintf("File / %s", fileitems[sub]);
		}
		else if (bar == 1)
		{
			cprintf("Edit / %s", edititems[sub]);
		}
		t = conioxlib_ckey();
		(void) t;
		donebar = 1;
	}
	if ((bar == CONIOXLIB_MENU_ESC) && (oldscr != 0))
	{
		puttext(1, 1, 80, 25, oldscr);
	}
	if (oldscr != 0)
	{
		free(oldscr);
	}
}

/* ------------- Entorno y creditos con getcountry() y about() ------------- */
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


/* -------------------- Caja de mensajes con msgbox() ---------------------- */
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

/* --------------------- Caja de entrada con inputbox() -------------------- */
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

/* -------------------- Campo numerico con inputmask() --------------------- */
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

/* ------------------ Lista con scroll con listbox() ----------------------- */
static void demo_listbox(void)
{
	static char items[26][41] = {
		"Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot",
		"Golf", "Hotel", "India", "Juliet", "Kilo", "Lima",
		"Mike", "November", "Oscar", "Papa", "Quebec", "Romeo",
		"Sierra", "Tango", "Uniform", "Victor", "Whiskey", "X-ray",
		"Yankee", "Zulu"
	};
	unsigned char r;
	unsigned int t;

	/* 26 items y solo 12 filas visibles: obliga a probar el scroll, la rueda
	   y la barra lateral del listbox */
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
}

/* ---------------- Progreso con progress() y progressbar() --------------- */
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

/* ------------------------- Temas visuales ------------------------------- */
/* Muestra los cinco temas como botones de radio con property(); al elegir uno
   se aplica con conioxlib_theme() y la demo entera cambia de colores (las
   cajas y menus de main() ya usan gettheme()). */
static void demo_themes(void)
{
	static unsigned char demo_theme = 0;
	static char themenames[5][21] = {"Borland C++ 3.1", "Turbo C 2.0", "Turbo C 1.x", "Norton Utilities", "PC Tools"};
	static char themedesc[5][71] = {"Black on gray, green highlight and blue dialogs ", "Same colors with single frame dialogs           ", "Monochrome with black background                ", "Yellow on blue with double frames               ", "White on cyan, PC Tools look                    "};
	unsigned char sel[5];
	unsigned int i;
	unsigned int t;
	unsigned char r;
	struct conioxlib_stheme th;

	conioxlib_gettheme(&th);
	box(39, 5, 76, 16, th.atrdialog, th.frame, "Themes", SHADOW);
	memset(sel, ' ', 5);
	sel[demo_theme] = '\xFE';
	r = conioxlib_property(41, 7, th.atr0, th.atr1, th.atrhot, themenames, themedesc, (char *) sel, 5, 1, 1);
	(void) r;
	for (i = 0; i < 5; i++)
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
	gotoxy(41, 13);
	cprintf("Theme applied: %-16s", th.name);
	gotoxy(41, 14);
	cputs("Main menu and boxes follow it now.");
	t = conioxlib_ckey();
	(void) t;
}

/* --------------------- Editor de texto con edit() ------------------------ */
static void demo_editor(void)
{
	conioxlib_edit("");
}

/* --------------------- Linea de estado y tooltip ------------------------- */
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

/* --------------------------- Salida de la demo --------------------------- */
static int demo_exit(void)
{
	if (conioxlib_exittodos(DEMO_HELP) == 0)
	{
		return(1);
	}
	return(0);
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* ------------------------------ Principal -------------------------------- */
int main(void)
{
	unsigned char opt;
	struct conioxlib_stheme th;
	FILE *f;
	int done = 0;

	coniox_init(NULL);
	_setcursortype(_NOCURSOR);

	f = fopen(DEMO_HELP, "wb");
	if (f != 0)
	{
		fprintf(f, "@Main\r\n");
		fprintf(f, "Welcome to the CONXLIB 2.0 demonstration program.\r\n");
		fprintf(f, "Pick an option with the arrow keys, hotkeys or the mouse.\r\n");
		fprintf(f, "Press F1 for help. ESC in the main menu acts as Exit.\r\n");
		fprintf(f, "@Mouse\r\n");
		fprintf(f, "conioxlib_pollmouse() updates conioxlib_mousestatus.\r\n");
		fprintf(f, "On Windows the cursor position is polled every frame.\r\n");
		fprintf(f, "@Exit\r\n");
		fprintf(f, "Exit asks for confirmation through conioxlib_exittodos().\r\n");
		fclose(f);
	}

	conioxlib_hello("CONXDEMO.TMP", 0);
	opt = 0;
	for (;;)
	{
		conioxlib_background((unsigned char *) DEMO_TITLE);
		/* Cajas y menu con los colores del tema activo; el menu arranca en la
		   fila 4 para que sus 19 items visibles queden dentro de la caja */
		conioxlib_gettheme(&th);
		box(3, 3, 36, 23, th.atrdialog, th.frame, "CONXLIB 2.0", SHADOW | CLOSE);
		box(38, 3, 77, 23, th.atrdialog, th.frame, "Output", SHADOW);
		conioxlib_blocks();
		opt = conioxlib_menu(5, 4, th.atr0, th.atr1, th.atrhot, demomenu, demodesc, DEMO_OPTIONS, 1, opt);
		if (opt == CONIOXLIB_MENU_HELP)
		{
			conioxlib_helpindex(5, 3, CONIOXLIB_ATTR(WHITE, CYAN), CONIOXLIB_ATTR(BLACK, CYAN), DEMO_HELP, (char (*)[16]) helpbuf);
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
	remove(DEMO_HELP);
	conioxlib_bye("CONXDEMO.TMP", 0, 0);
	return(0);
}
