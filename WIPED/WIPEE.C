#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <bios.h>
#include <string.h>
#include <dir.h>
#include <io.h>
#include <\tc\c\wiped\wiped.h>
#include <\tc\c\wiped\conxlib.h>

extern unsigned _stklen=8192;

unsigned char comercial;
unsigned int set386=0, memems=0, sonido=1, totalconf=1, fileconf=0, attribconf=1, cancelconf=1, method=3, autosave=1, stacker=0, slack=1, fat=0, disk, sector;
char *buf;
struct sfiles *files;
unsigned long int llevototal, total, llevoparcial, parcial;
char masc[13], tray[80], dir[80];
char fprg[80], fhlp[80], fcfg[80], freg[80], ftmp[80];

void main (unsigned char argc, char *argv[])
{
	register unsigned char i;
	char drive[5], dir[80], name[9], ext[5], tmp[80];

	strcpy(fprg, argv[0]);
  strcpy(fhlp, argv[0]);
  *strchr(fhlp, '.')=0;
  strcat(fhlp, ".HLP");
  strcpy(fcfg, argv[0]);
	*strchr(fcfg, '.')=0;
	strcat(fcfg, ".CFG");
  strcpy(freg, argv[0]);
	*strchr(freg, '.')=0;
	strcat(freg, ".REG");
	 strcpy(ftmp, argv[0]);
	*strchr(ftmp, '.')=0;
	strcat(ftmp, ".TMP");
	shareware(freg, &comercial);

	if (argc<2)
		entorno();
	else
	{
		loadconfig(fcfg);
		for (i=1; i<argc; i++)
		{
			strupr(argv[i]);
      if (strcmp(argv[i], "/386+")==0)
        set386=1;
      else if (strcmp(argv[i], "/386-")==0)
        set386=0;
      else if (strcmp(argv[i], "/EMS+")==0)
        memems=1;
      else if (strcmp(argv[i], "/EMS-")==0)
        memems=0;
      else if (strcmp(argv[i], "/DIRECT+")==0)
        directvideo=1;
      else if (strcmp(argv[i], "/DIRECT-")==0)
        directvideo=0;
      else if (strcmp(argv[i], "/SOUND+")==0)
        sonido=1;
      else if (strcmp(argv[i], "/SOUND-")==0)
				sonido=0;
			else if (strcmp(argv[i], "/STACKER+")==0)
				stacker=1;
			else if (strcmp(argv[i], "/STACKER-")==0)
				stacker=0;
			else if (strcmp(argv[i], "/SLACK+")==0)
				slack=1;
			else if (strcmp(argv[i], "/SLACK-")==0)
				slack=0;
			else if (strcmp(argv[i], "/FAT+")==0)
				fat=1;
			else if (strcmp(argv[i], "/FAT-")==0)
				fat=0;
      else if (strcmp(argv[i], "/TOTAL+")==0)
        totalconf=1;
      else if (strcmp(argv[i], "/TOTAL-")==0)
        totalconf=0;
      else if (strcmp(argv[i], "/ATTRIB+")==0)
        attribconf=1;
      else if (strcmp(argv[i], "/ATTRIB-")==0)
        attribconf=0;
      else if (strcmp(argv[i], "/FILE+")==0)
        fileconf=1;
      else if (strcmp(argv[i], "/FILE-")==0)
        fileconf=0;
      else if (strcmp(argv[i], "/CANCEL+")==0)
        cancelconf=1;
      else if (strcmp(argv[i], "/CANCEL-")==0)
        cancelconf=0;
      else if (strcmp(argv[i], "/SAVE+")==0)
        autosave=1;
      else if (strcmp(argv[i], "/SAVE-")==0)
        autosave=0;
      else if (strcmp(argv[i], "/STANDARD")==0)
        method=1;
      else if (strcmp(argv[i], "/FAST")==0)
        method=2;
      else if (strcmp(argv[i], "/SECURE")==0)
        method=3;
      else if (strcmp(argv[i], "/SAFE")==0)
        method=4;
      else if (strcmp(argv[i], "/NCSA")==0)
        method=0;
		}
		if (argc>1)
		{
			strcpy(tmp, argv[1]);
			fnsplit(tmp, drive, dir, name, ext);
			sprintf(tray, "%s%s", drive, dir);
			sprintf(masc, "%s%s", name, ext);
		}
    comandos(argv[0], comercial, argv[argc-1]);
	}
}

/* ----------------------- Principal con entrono -------------------------- */
void _fastcall entorno (void)
{
	register unsigned int i;
	unsigned char olddisk;
	unsigned char oldx, oldy;
	char *oldb;
	char olddir[80];
  char but1[][11]={   "[Yes]", "[No]"};
  char dbut1[][71]={  "WIPE will save the actual configuration                             ",
											"WIPE will use the actual configuration but will not save it         "};
	char but2[][11]={   "[Ok]"};
	char dbut2[][71]={  "Close window                                                        "};
	char but3[][11]={		"[Yes]", "[No]"};
	char dbut3[][71]={	"WIPE will use the existing configuration file                       ",
											"WIPE will not use the existing configuration file                   "};
	char pro1[][21]={   "Standard",
											"Fast    ",
											"Safe    ",
											"Secure  ",
											"NCSA    "};
	char dpro1[][71]={  "WIPE will use the DOS standard deletion method                      ",
											"WIPE will use the fast wipe method                                  ",
                      "WIPE will use the safe wipe method                                  ",
											"WIPE will use the secure wipe method                                ",
											"WIPE will use the NCSA wipe method                                  "};
  char spro1[5];
	char opt1[][21]={   "Use 386 instructions",
											"Use EMS memory      ",
											"Use direct output   ",
											"Enable sound        ",
											"Duplicator tolerance",
                      "Clear slack         ",
                      "Clear FAT           ",
											"Total confirmation  ",
											"Attrib confirmation ",
											"File confirmation   ",
											"Cancel confirmation ",
											"Autosave options    "};
	char dopt1[][71]={  "WIPE will use 386 or later features if available                    ",
											"WIPE will use EMS memory if available                               ",
                      "WIPE will perform direct screen output                              ",
											"WIPE will enable system sounds                                      ",
                      "WIPE will use Stacker/Super Stor/Double Space compatibility         ",
                      "WIPE will clear the slack of each file                              ",
                      "WIPE will clear the FAT                                             ",
											"WIPE will ask you before deleting files                             ",
											"WIPE will ask you before deleting an attributed file                ",
											"WIPE will ask you before deleteing each file                        ",
											"WIPE will ask you after cancelling the process                      ",
                      "WIPE will save automatically the configuration                      "};
  char sopt1[12];
  hello(ftmp, comercial);
  oldb=malloc(textsize(20, 4, 62, 21));
	if (oldb==0)
		return;
	oldx=wherex();
	oldy=wherey();
  gettext(20, 4, 62, 21, oldb);

	i=loadconfig(fcfg);
  if (set386)
		sopt1[0]='X';
  else
		sopt1[0]=' ';
  if (memems)
		sopt1[1]='X';
  else
		sopt1[1]=' ';
  if (directvideo)
		sopt1[2]='X';
  else
		sopt1[2]=' ';
  if (sonido)
		sopt1[3]='X';
  else
		sopt1[3]=' ';
  if (stacker)
    sopt1[4]='X';
  else
    sopt1[4]=' ';
  if (slack)
    sopt1[5]='X';
  else
    sopt1[5]=' ';
  if (fat)
    sopt1[6]='X';
  else
    sopt1[6]=' ';
  if (totalconf)
    sopt1[7]='X';
  else
    sopt1[7]=' ';
	if (attribconf)
    sopt1[8]='X';
  else
    sopt1[8]=' ';
  if (fileconf)
    sopt1[9]='X';
  else
    sopt1[9]=' ';
  if (cancelconf)
    sopt1[10]='X';
  else
    sopt1[10]=' ';
  if (autosave)
    sopt1[11]='X';
	else
    sopt1[11]=' ';
	memset(spro1, ' ', 5);
	spro1[method]='ù';
	if (i!=0)
	{
		box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Configuration", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 11, "Do you have an existing configuration");
		cputsc(20, 60, 12, "file, do you want to use it?");
		do
		{
			i=buttons(30, 14, 7*16+0, 2*16+15, but3, dbut3, 2, 19);
			if (i==255)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Existing", buf);
				free(buf);
			}
		}
		while(i==255);
    puttext(20, 4, 62, 21, oldb);
	}
	else
		i=1;
	if (i==1)
	{
		box(20, 4, 60, 20, 16*7+15, A_DOUBLE, "Configure Options", SHADOW);
		textattr(16*7+0);
    cputsc(20, 60, 5, "Please set default WIPE");
    cputsc(20, 60, 6, "options");
		if (comercial)
		{
			do
			{
				i=option(29, 8, 7*16+0, 2*16+15, opt1, dopt1, sopt1, 12, 1, 1);
				if (i==255)
				{
					buf=malloc(16384);
					if (buf==0)
						return;
					help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Options", buf);
					free(buf);
				}
			}
			while (i==255);
		}
		else
		{
			textattr(16*7+8);
			gotoxy(29, 10);
			cputs("[ ] Use 386 instructions");
			gotoxy(29, 11);
			cputs("[ ] Use EMS memory");
			do
			{
				i=option(29, 10, 7*16+0, 2*16+15, opt1+2, dopt1+2, sopt1+2, 10, 1, 1);
				if (i==255)
				{
					buf=malloc(16384);
					if (buf==0)
						return;
					help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Options", buf);
					free(buf);
				 }
			}
			while(i==255);
		}
    puttext(20, 4, 62, 21, oldb);
		if (sopt1[0]==' ')
			set386=0;
		else
			set386=1;
		if (sopt1[1]==' ')
			memems=0;
		else
			memems=1;
		if (sopt1[2]==' ')
			directvideo=0;
		else
			directvideo=1;
		if (sopt1[3]==' ')
			sonido=0;
		else
			sonido=1;
		if (sopt1[4]==' ')
      stacker=0;
		else
      stacker=1;
    if (sopt1[5]==' ')
      slack=0;
		else
      slack=1;
    if (sopt1[6]==' ')
      fat=0;
		else
      fat=1;
    if (sopt1[7]==' ')
			totalconf=0;
		else
			totalconf=1;
    if (sopt1[8]==' ')
			fileconf=0;
		else
			fileconf=1;
    if (sopt1[9]==' ')
			attribconf=0;
		else
			attribconf=1;
    if (sopt1[10]==' ')
			cancelconf=0;
		else
			cancelconf=1;
    if (sopt1[11]==' ')
			autosave=0;
		else
			autosave=1;
		box(20, 8, 60, 17, 16*7+15, A_DOUBLE, "Configure Method", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 9, "Which method of destruction do you");
		cputsc(20, 60, 10, "want to use?");
		if (comercial)
		{
			do
			{
				i=property(35, 12, 7*16+0, 2*16+15, pro1, dpro1, spro1, 5, 1, 1);
				if (i==255)
				{
					buf=malloc(16384);
					if (buf==0)
						return;
					help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Methods", buf);
					free(buf);
				}
			}
			while(i==255);
		}
		else
		{
			gotoxy(35, 16);
			textattr(16*7+8);
			cputs("( ) NCSA");
			do
			{
				i=property(35, 12, 7*16+0, 2*16+15, pro1, dpro1, spro1, 4, 1, 1);
				if (i==255)
				{
					buf=malloc(16384);
					if (buf==0)
						return;
					help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Methods", buf);
					free(buf);
				}
			}
			while(i==255);
		}
    puttext(20, 4, 62, 21, oldb);
		for (i=0; i!=5; i++)
		{
			if (spro1[i]=='ù')
			{
				method=i;
				break;
			}
		}
  }
	if (autosave==0)
  {
		box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Save", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 11, "Do you want to save the current");
		cputsc(20, 60, 12, "configuration?");
		do
		{
			i=buttons(30, 14, 7*16+0, 2*16+15, but1, dbut1, 2, 19);
			if (i==255)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Save", buf);
				free(buf);
			}
		}
		while(i==255);
    puttext(20, 6, 62, 21, oldb);
		if (i==0)
			saveconfig(fcfg);
	}
	else
		saveconfig(fcfg);
	free(oldb);
	if ((comercial==0) && (method==4))
	{
		method=3;
		box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Shareware", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 11, "In the shareware version NCSA method");
		cputsc(20, 60, 12, "is not available. Please register");
		do
		{
			i=buttons(38, 14, 7*16+0, 2*16+15, but2, dbut2, 1, 0);
			if (i==255)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Shareware", buf);
				free(buf);
			}
		}
		while(i==255);
    puttext(20, 4, 62, 21, oldb);
	}
	files=malloc(sizeof(struct sfiles)*220);
	if (files==0)
		return;
	again:
	do
	{
		i=select(&disk, dir, files);
		if (i==255)
		{
			buf=malloc(16384);
			if (buf==0)
				return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Select", buf);
			free(buf);
		}
	}
	while(i==255);
	olddisk=getdisk();
	getcurdir(0, olddir);
	setdisk(disk-'A');
	chdir(dir);
	buf=malloc(16384);
	if (buf==0)
	{
		free(oldb);
		free(files);
		return;
	}
	starte();
  i=exittodos(fhlp);
	if (i==1)
		goto again;
	free(buf);
	setdisk(olddisk);
	chdir(olddir);
	free(files);
	gotoxy(oldx, oldy);
  bye(ftmp, 0, comercial);
}

/* ------------------- Inicializa el dibujo de Wipear --------------------- */
void _fastcall starte (void)
{
	register unsigned int i, j;
	FILE *f;
	char prog[4]={			'|', '/', '-', '\\'};
	struct dfree dfree;
	char barra[51];
	char tmp[80];
	char but1[][11]={		"[Proceed]", "[Abort]"};
	char dbut1[][71]={	"WIPE will proceed destroying selected files                         ",
											"WIPE will abort the desturction of the selected files               "};
	llevototal=0;
	llevoparcial=0;
	parcial=0;
	total=0;
	j=0;
	for (i=0; i!=200; i++)
	{
		if (files[i].selected)
		{
			f=fopen(files[i].name, "rb");
			if (f!=0)
			{
				total+=filelength(fileno(f));
				fclose(f);
				j++;
			}
		}
	}
	if (j==0)
		return;
	getdfree(0, &dfree);
	sector=dfree.df_bsec;
	if (totalconf==1)
	{
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "Total", SHADOW);
		textattr(16*7+0);
		sprintf(tmp, "You are going to delete %s files", puntua(j));
		cputsc(20, 60, 10, tmp);
		sprintf(tmp, "Ocupping %s bytes", puntua(total));
		cputsc(20, 60, 11, tmp);
		sprintf(tmp, "From the directory: %c:%s", disk, dir);
		cputsc(20, 60, 12, tmp);
		cputsc(20, 60, 13, "Do you want to continue?");
		do
		{
			i=buttons(28, 15, 7*16+0, 2*16+15, but1, dbut1, 2, 19);
			if (i==255)
				help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Total", buf);
		}
		while(i==255);
		if (i==1)
		{
			background(0);
			return;
		}
	}
	if (stacker==0)
		memset(buf, 0, 16384);
	else
	{
		randomize();
		for (i=0; i!=16384; i++)
			buf[i]=random(256);
	}
	box(1, 2, 78, 23, 16*7+15, A_DOUBLE, "Wipping", SHADOW | CLOSE);
	box(3, 3, 76, 6, 16*7+15, A_SINGLE, "Information", 0);
	textattr(16*7+0);
	cputsc(4, 75, 4, "Wipping files from the directory:");
	sprintf(tmp, "%c:%s", disk, dir);
	cputsc(4, 75, 5, tmp);
	box(3, 7, 76, 11, 16*7+15, A_SINGLE, "Total", 0);
	box(3, 12, 76, 16, 16*7+15, A_SINGLE, "Partial", 0);
	box(3, 17, 76, 22, 16*7+15, A_SINGLE, "Process", 0);
	textattr(16*7+0);
	window(4, 8, 74, 10);
	memset(barra, 0, 51);
	memset(barra, '°', 50);
	gotoxy(11, 1);
	cputs(barra);
	gotoxy(34, 2);
	cprintf("%3u%%", llevototal*100/total);
	gotoxy(20, 3);
	cprintf("%11s of ", puntua(0));
	cprintf("%11s", puntua(total));
	for (i=0; i!=200; i++)
	{
		if (files[i].selected)
			wipee(files[i].name);
	}
	window(4, 18, 75, 21);
	if (fat)
	{
		textattr(16*2+15);
		for (i=0; i!=64; i++)
		{
			gotoxy(1, 4);
			cprintf("Clearing FAT %c       ", prog[(i>>2)&3]);
			sprintf(tmp, "WIPED%03u.TMP", i);
			f=fopen(tmp, "wb");
			fclose(f);
		}
		for (i=0; i!=64; i++)
		{
			gotoxy(1, 4);
			cprintf("Clearing FAT %c       ", prog[(i>>2)&3]);
			sprintf(tmp, "WIPED%03u.TMP", i);
			remove(tmp);
		}
		textattr(7*16+0);
		cputs("\r\n");
	}
	textattr(7*16+0);
	gotoxy(1, 4);
	cprintf("Done");
	if (sonido)
	{
		sound(2000);
		delay(150);
		sound(1000);
		delay(150);
		sound(3000);
		delay(150);
		nosound();
	}
	bioskey(0);
	window(1, 1, 80, 25);
}

/* --------------------------- Wipear ficheros ---------------------------- */
void _pascal wipee (char *nombre)
{
  register unsigned int escritos, tecla;
	FILE *f;
	union REGS r;
	struct SREGS s;
	char barra[51];
	char but1[][11]={   "[Yes]", "[No]", "[All]"};
	char dbut1[][71]={  "WIPE will wipe this file                                            ",
											"WIPE will skip this file                                            ",
											"WIPE will wipe this file and the rest of files                      "};
	char but2[][11]={   "[Abort]", "[Continue]"};
	char dbut2[][71]={  "WIPE will abort the wipping process                                 ",
											"WIPE will continue the wipping process                              "};
	window(1, 1, 80, 25);
	textattr(16*7+0);
	strupr(nombre);
	f=fopen(nombre, "rb");
	parcial=filelength(fileno(f));
	fclose(f);
	window(4, 8, 75, 10);
	gotoxy(34, 2);
	window(4, 13, 75, 15);
	memset(barra, 0, 51);
	memset(barra, '°', 50);
	gotoxy(11, 1);
	cputs(barra);
	gotoxy(34, 2);
	cprintf("%3u%%", 0);
	gotoxy(20, 3);
	cprintf("%11s of ", puntua(0));
	cprintf("%11s", puntua(parcial));
	window(4, 18, 75, 21);
	textattr(16*2+15);
	gotoxy(1, 4);
	cprintf("Wipping: %13s", nombre);
	textattr(16*7+0);
  r.x.ax=0x4300;
	s.ds=FP_SEG(nombre);
	r.x.dx=FP_OFF(nombre);
	int86x(0x21, &r, &r, &s);
  window(1, 1, 80, 25);
	if (fileconf==1)
	{
		gettext(20, 9, 62, 18, buf);
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "File", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 10, "You are going to delete file");
		sprintf(barra, "%s (%s bytes)", nombre, puntua(parcial));
		cputsc(20, 60, 11, barra);
		sprintf(barra, "From the directory: %c:%s", disk, dir);
		cputsc(20, 60, 12, barra);
		cputsc(20, 60, 13, "Do you want to continue?");
		do
		{
			escritos=buttons(28, 15, 7*16+0, 2*16+15, but1, dbut1, 3, 9);
			if (escritos==255)
				help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "File", buf);
		}
		while(escritos==255);
		puttext(20, 9, 62, 18, buf);
		memset(buf, 0, 16384);
		if (escritos==1)
			return;
		else if (escritos==2)
			fileconf=0;
	}
  else if (attribconf==1)
	{
    if (r.x.cx & (FA_SYSTEM | FA_HIDDEN | FA_RDONLY))
    {
      gettext(20, 9, 62, 18, buf);
			box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "Attrib", SHADOW);
			textattr(16*7+0);
			cputsc(20, 60, 10, "You are going to delete file");
			sprintf(barra, "%s (%s bytes)", nombre, puntua(parcial));
			cputsc(20, 60, 11, barra);
			sprintf(barra, "From the directory: %c:%s", disk, dir);
			cputsc(20, 60, 12, barra);
			cputsc(20, 60, 13, "Do you want to continue?");
			do
			{
				escritos=buttons(28, 15, 7*16+0, 2*16+15, but1, dbut1, 3, 9);
				if (escritos==255)
					help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Attrib", buf);
			}
			while(escritos==255);
			puttext(20, 9, 62, 18, buf);
      memset(buf, 0, 16384);
      if (escritos==1)
        return;
      else if (escritos==2)
        attribconf=0;
    }
		r.x.ax=0x4301;
		r.x.cx=0;
		s.ds=FP_SEG(nombre);
		r.x.dx=FP_OFF(nombre);
		int86x(0x21, &r, &r, &s);
	}
	/* Si tiene 0 bytes s¢lo borrarlo */
	if (parcial==0)
		remove(nombre);
	else if (method==0)
	{
		llevoparcial=parcial;
		llevototal+=llevoparcial;
		window(4, 8, 74, 10);
		memset(barra, 0, 51);
		memset(barra, '°', 50);
		memset(barra, 'Û', (unsigned int) (llevototal*50/total));
		gotoxy(11, 1);
		cputs(barra);
		gotoxy(34, 2);
		cprintf("%3u%%", llevototal*100/total);
		gotoxy(20, 3);
		cprintf("%11s of ", puntua(llevototal));
		cprintf("%11s", puntua(total));
		window(4, 13, 74, 15);
		memset(barra, 0, 51);
		memset(barra, '°', 50);
		memset(barra, 'Û', (unsigned int) (llevoparcial*50/parcial));
		gotoxy(11, 1);
		cputs(barra);
		gotoxy(34, 2);
		cprintf("%3u%%", llevoparcial*100/parcial);
		gotoxy(20, 3);
		cprintf("%11s of ", puntua(llevoparcial));
		cprintf("%11s", puntua(parcial));
		remove(nombre);
		window(4, 18, 75, 21);
		gotoxy(1, 4);
		cprintf("Wipped:  %13s\r\n", nombre);
	}
	else if (method==1)
	{
		llevoparcial=parcial;
		llevototal+=llevoparcial;
		window(4, 8, 74, 10);
		memset(barra, 0, 51);
		memset(barra, '°', 50);
		memset(barra, 'Û', (unsigned int) (llevototal*50/total));
		gotoxy(11, 1);
		cputs(barra);
		gotoxy(34, 2);
		cprintf("%3u%%", llevototal*100/total);
		gotoxy(20, 3);
		cprintf("%11s of ", puntua(llevototal));
		cprintf("%11s", puntua(total));
		window(4, 13, 74, 15);
		memset(barra, 0, 51);
		memset(barra, '°', 50);
		memset(barra, 'Û', (unsigned int) (llevoparcial*50/parcial));
		gotoxy(11, 1);
		cputs(barra);
		gotoxy(34, 2);
		cprintf("%3u%%", llevoparcial*100/parcial);
		gotoxy(20, 3);
		cprintf("%11s of ", puntua(llevoparcial));
		cprintf("%11s", puntua(parcial));
		f=fopen(nombre, "wb");
		fclose(f);
		remove(nombre);
		window(4, 18, 75, 21);
		gotoxy(1, 4);
		cprintf("Wipped:  %13s\r\n", nombre);
	}
	else if (method==2)
	{
		llevoparcial=parcial;
		llevototal+=llevoparcial;
		window(4, 8, 74, 10);
		memset(barra, 0, 51);
		memset(barra, '°', 50);
		memset(barra, 'Û', (unsigned int) (llevototal*50/total));
		gotoxy(11, 1);
		cputs(barra);
		gotoxy(34, 2);
		cprintf("%3u%%", llevototal*100/total);
		gotoxy(20, 3);
		cprintf("%11s of ", puntua(llevototal));
		cprintf("%11s", puntua(total));
		window(4, 13, 74, 15);
		memset(barra, 0, 51);
		memset(barra, '°', 50);
		memset(barra, 'Û', (unsigned int) (llevoparcial*50/parcial));
		gotoxy(11, 1);
		cputs(barra);
		gotoxy(34, 2);
		cprintf("%3u%%", llevoparcial*100/parcial);
		gotoxy(20, 3);
		cprintf("%11s of ", puntua(llevoparcial));
		cprintf("%11s", puntua(parcial));
		f=fopen(nombre, "wb");
		if (parcial<1024)
			fwrite(buf, 1, (unsigned int) parcial, f);
		else
			fwrite(buf, 1, 1024, f);
		fclose(f);
		remove(nombre);
		window(4, 18, 75, 21);
		gotoxy(1, 4);
		cprintf("Wipped:  %13s\r\n", nombre);
	}
	else if (method==3)
	{
		f=fopen(nombre, "wb");
		llevoparcial=0;
		do
		{
			if ((parcial-llevoparcial)<16384)
				escritos=fwrite(buf, 1, (unsigned int) (parcial-llevoparcial), f);
			else
				escritos=fwrite(buf, 1, 16384, f);
			llevoparcial+=escritos;
			llevototal+=escritos;
			window(4, 8, 74, 10);
			memset(barra, 0, 51);
			memset(barra, '°', 50);
			memset(barra, 'Û', (unsigned int) (llevototal*50/total));
			gotoxy(11, 1);
			cputs(barra);
			gotoxy(34, 2);
			cprintf("%3u%%", llevototal*100/total);
			gotoxy(20, 3);
			cprintf("%11s of ", puntua(llevototal));
			cprintf("%11s", puntua(total));
			window(4, 13, 74, 15);
			memset(barra, 0, 51);
			memset(barra, '°', 50);
			memset(barra, 'Û', (unsigned int) (llevoparcial*50/parcial));
			gotoxy(11, 1);
			cputs(barra);
			gotoxy(34, 2);
			cprintf("%3u%%", llevoparcial*100/parcial);
			gotoxy(20, 3);
			cprintf("%11s of ", puntua(llevoparcial));
			cprintf("%11s", puntua(parcial));
		}
		while (llevoparcial<parcial);
    fclose(f);
    remove(nombre);
		window(4, 18, 75, 21);
		gotoxy(1, 4);
		cprintf("Wipped:  %13s\r\n", nombre);
	}
	else if (method==4)
	{
		f=fopen(nombre, "wb+");
		llevoparcial=0;
		rewind(f);
		do
		{
			if ((parcial-llevoparcial)<16384)
			{
				memset(buf, 1, (unsigned int) (parcial-llevoparcial));
				escritos=fwrite(buf, 1, (unsigned int) (parcial-llevoparcial), f);
				fseek(f, -escritos, 1);
				memset(buf, 1, (unsigned int) (parcial-llevoparcial));
				escritos=fwrite(buf, 0, (unsigned int) (parcial-llevoparcial), f);
				fseek(f, -escritos, 1);
				memset(buf, 1, (unsigned int) (parcial-llevoparcial));
				escritos=fwrite(buf, 2, (unsigned int) (parcial-llevoparcial), f);
				fseek(f, -escritos, 1);
			}
			else
			{
				memset(buf, 1, 16384);
				escritos=fwrite(buf, 1, 16384, f);
				fseek(f, -escritos, 1);
				memset(buf, 1, 16384);
				escritos=fwrite(buf, 0, 16384, f);
				fseek(f, -escritos, 1);
				memset(buf, 1, 16384);
				escritos=fwrite(buf, 2, 16384, f);
				fseek(f, -escritos, 1);
			}
			llevoparcial+=escritos;
			llevototal+=escritos;
			window(4, 8, 74, 10);
			memset(barra, 0, 51);
			memset(barra, '°', 50);
			memset(barra, 'Û', (unsigned int) (llevototal*50/total));
			gotoxy(11, 1);
			cputs(barra);
			gotoxy(34, 2);
			cprintf("%3u%%", llevototal*100/total);
			gotoxy(20, 3);
			cprintf("%11s of ", puntua(llevototal));
			cprintf("%11s", puntua(total));
			window(4, 13, 74, 15);
			memset(barra, 0, 51);
			memset(barra, '°', 50);
			memset(barra, 'Û', (unsigned int) (llevoparcial*50/parcial));
			gotoxy(11, 1);
			cputs(barra);
			gotoxy(34, 2);
			cprintf("%3u%%", llevoparcial*100/parcial);
			gotoxy(20, 3);
			cprintf("%11s of ", puntua(llevoparcial));
			cprintf("%11s", puntua(parcial));
		}
		while (llevoparcial<parcial);
		if (slack)
			fwrite(buf, 1, sector, f);
		fclose(f);
    remove(nombre);
		window(4, 18, 75, 21);
		gotoxy(1, 4);
		cprintf("Wipped:  %13s\r\n", nombre);
	}
	window(1, 1, 80, 25);
	tecla=bioskey(1);
	if (tecla!=0)
		tecla=bioskey(0);
	if (tecla==0x011B)
	{
		if (cancelconf==1)
		{
			gettext(20, 10, 62, 16, buf);
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Cancel", SHADOW);
			textattr(16*7+0);
			cputsc(20, 60, 11, "Do you want to cancel current");
			cputsc(20, 60, 12, "operation?");
			do
			{
				escritos=buttons(28, 15, 7*16+0, 2*16+15, but2, dbut2, 2, 19);
				if (escritos==255)
					help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Cancel", buf);
			}
			while(escritos==255);
			puttext(20, 10, 62, 16, buf);
			memset(buf, 0, 16384);
		}
		else
			escritos=0;
		if (escritos==0)
		{
			for (escritos=0; escritos!=200; escritos++)
				files[escritos].selected=0;
		}
	}
	watch();
}
