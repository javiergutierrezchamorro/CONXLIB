#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <bios.h>
#include <alloc.h>
#include <dir.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <\tc\c\wiped\wiped.h>
#include <\tc\c\wiped\conxlib.h>

/* ----------------------- Principal con entrono -------------------------- */
void _fastcall comandos (char *ruta, unsigned char comercial, char *mask)
{
  char drive[5], name[13], ext[5], dir[80], olddir[80];
  unsigned char disk, olddisk;

  fnsplit(ruta, drive, dir, name, ext);
	textattr(3);
	cputs("\r\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ\r\n");
  cputs(" °ÛÛÛÛÛ   °ÛÛÛÛÛÛ  °ÛÛÛÛÛÛ       (c) Copyright 1994-1997 ");
	textattr(11);
	cputs("De Trans Software\r\n");
	textattr(3);
  cputs(" °Û   °Û    °ÛÛ    °Û                          WIPE Version 4.0\r\n");
	cputs(" °Û   °Û    °ÛÛ    °ÛÛÛÛÛÛ    ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\r\n");
  cputs(" °Û   °Û    °ÛÛ         °Û                Destroys files that will be\r\n");
  cputs(" °ÛÛÛÛÛ  Ü  °ÛÛ Ü  °ÛÛÛÛÛÛ Ü                     unrecoverable.\r\n");
	textattr(3);
	cputs("ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ\r\n");
  if (comercial)
    cputs("Registered version ");
  else
    cputs("Shareware version ");
  textattr(7);
	cprintf("[DOS v%u.%02u; %uK/%uK]", _osmajor, _osminor, biosmemory(), (unsigned int) (farcoreleft()>>10));
	textattr(11);
	cprintf("\r\nType %s /? if you need help and %s /* for internal information.\r\n", name, name);
  strupr(mask);
  fnsplit(mask, drive, dir, name, ext);
  disk=drive[0];
  textattr(15);
  cprintf("Destroying %s\r\n", mask);
	olddisk=getdisk();
	getcurdir(0, olddir);
  setdisk(disk-'A');
  chdir(dir);
  buf=malloc(16384);
	if (buf==0)
		return;
  startc();
	free(buf);
	setdisk(olddisk);
	chdir(olddir);
}

/* ------------------- Inicializa el dibujo de Wipear --------------------- */
void _fastcall startc (void)
{
/*	register unsigned int i, j;
	FILE *f;
  struct ffblk ffblk;
	char tmp[80];

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
	if (totalconf==1)
	{
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "Total Confirmation", SHADOW);
		textattr(16*7+0);
		sprintf(tmp, "You are going to delete %s files", puntua(j));
		cputsc(20, 60, 10, tmp);
		sprintf(tmp, "Ocupping %s bytes", puntua(total));
		cputsc(20, 60, 11, tmp);
		sprintf(tmp, "From the directory: %c:%s", disk, dir);
		cputsc(20, 60, 12, tmp);
		cputsc(20, 60, 13, "Do you want to continue?");
		i=buttons(28, 15, 7*16+0, 2*16+15, but1, dbut1, 2, 19);
		if (i==1)
		{
			background(0);
			return;
		}
	}
	memset(buf, 0, 16384);
	box(1, 2, 78, 23, 16*7+15, A_DOUBLE, "Wipping", SHADOW | CLOSE);
	box(3, 3, 76, 6, 16*7+15, A_SINGLE, "Information", 0);
	textattr(16*7+0);
	cputsc(4, 75, 4, "Wipping files from the directory:");
	sprintf(tmp, "%c:%s", disk, dir);
	cputsc(4, 75, 5, tmp);
	box(3, 7, 76, 11, 16*7+15, A_SINGLE, "Total", 0);
	box(3, 12, 76, 16, 16*7+15, A_SINGLE, "Partial", 0);
	box(3, 17, 76, 22, 16*7+15, A_SINGLE, "Process", 0);
	for (i=0; i!=200; i++)
	{
		if (files[i].selected)
			wipee(files[i].name);
	}
	window(4, 18, 75, 21);
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
	delay(1000);
	window(1, 1, 80, 25);*/
}

/* --------------------------- Wipear ficheros ---------------------------- */
void _pascal wipec (char *nombre)
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
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "File Confirmation", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 10, "You are going to delete file");
		sprintf(barra, "%s (%s bytes)", nombre, puntua(parcial));
		cputsc(20, 60, 11, barra);
		sprintf(barra, "From the directory: %c:%s", disk, dir);
		cputsc(20, 60, 12, barra);
		cputsc(20, 60, 13, "Do you want to continue?");
		escritos=buttons(28, 15, 7*16+0, 2*16+15, but1, dbut1, 3, 9);
		puttext(20, 9, 62, 18, buf);
		memset(buf, 0, 16384);
		if (escritos==1)
			return;
		else if (escritos==2)
			fileconf=0;
	}
	else if ((attribconf==1) && (r.x.cx & (FA_SYSTEM | FA_HIDDEN | FA_RDONLY)))
  {
		gettext(20, 9, 62, 18, buf);
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "Attribute Confirmation", SHADOW);
		textattr(16*7+0);
		cputsc(20, 60, 10, "You are going to delete file");
		sprintf(barra, "%s (%s bytes)", nombre, puntua(parcial));
		cputsc(20, 60, 11, barra);
		sprintf(barra, "From the directory: %c:%s", disk, dir);
		cputsc(20, 60, 12, barra);
		cputsc(20, 60, 13, "Do you want to continue?");
		escritos=buttons(28, 15, 7*16+0, 2*16+15, but1, dbut1, 3, 9);
		puttext(20, 9, 62, 18, buf);
		memset(buf, 0, 16384);
		if (escritos==1)
			return;
		else if (escritos==2)
			attribconf=0;
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
		remove(nombre);
	}
	else if (method==1)
	{
		f=fopen(nombre, "wb");
		fclose(f);
		llevoparcial=parcial;
		llevototal+=llevoparcial;
		remove(nombre);
	}
	else if (method==2)
	{
		f=fopen(nombre, "wb");
		if (parcial<1024)
			fwrite(buf, 1, (unsigned int) parcial, f);
		else
			fwrite(buf, 1, 1024, f);
		fclose(f);
		llevoparcial=parcial;
		llevototal+=llevoparcial;
		remove(nombre);
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
  tecla=bioskey(1);
  if (tecla!=0)
    tecla=bioskey(0);
  if (tecla==0x011B)
	{
		if (cancelconf==1)
		{
			gettext(20, 10, 62, 16, buf);
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Cancel Confirmation", SHADOW);
			textattr(16*7+0);
			cputsc(20, 60, 11, "Do you want to cancel current");
			cputsc(20, 60, 12, "operation?");
			escritos=buttons(28, 15, 7*16+0, 2*16+15, but2, dbut2, 2, 19);
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
	window(1, 1, 80, 25);
	watch();
}

/* ------------------------- Puntua un long ------------------------------- */
char * pascal puntua (long int n)
{
	register unsigned int i, j=0, k=0;
	static char tmp[15], res[20];

	memset(tmp, 0, 15);
	memset(res, 0, 20);
	ltoa(n, tmp, 10);
	if (n<1000)
		return(tmp);
	strrev(tmp);
	for (i=0; i!=strlen(tmp); i++)
	{
		if (j==3)
		{
			j=0;
			res[k]=',';
			k++;
		}
		res[k]=tmp[i];
		j++;
		k++;
	}
	strrev(res);
	return(res);
}

/* ----------------------- Principal con entrono -------------------------- */
unsigned char _fastcall loadconfig (char *nombre)
{
	FILE *f;
	char tmp[80];

  f=fopen(nombre, "rb");
	if (f!=0)
	{
		fgets(tmp, 79, f);
		fgets(tmp, 79, f);
    fscanf(f, "%u\r\n", &set386);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &memems);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &directvideo);
		fgets(tmp, 79, f);
    fscanf(f, "%u\r\n", &sonido);
		fgets(tmp, 79, f);
    fscanf(f, "%u\r\n", &stacker);
		fgets(tmp, 79, f);
    fscanf(f, "%u\r\n", &slack);
		fgets(tmp, 79, f);
    fscanf(f, "%u\r\n", &fat);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &totalconf);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &attribconf);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &fileconf);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &cancelconf);
		fgets(tmp, 79, f);
		fscanf(f, "%u\r\n", &method);
		fgets(tmp, 79, f);
    fscanf(f, "%u\r\n", &autosave);
		fclose(f);
		return(1);
	}
	else
		return(0);
}

/* ----------------------- Principal con entrono -------------------------- */
void _fastcall saveconfig (char *nombre)
{
  FILE *f;

	f=fopen(nombre, "wb");
  if (f==0)
    return;
  fputs("; WIPE for DOS v4.0 Configuration file by De Trans Software (D.T.S.)\r\n", f);
	fputs("; Use 386 instructions\r\n", f);
  fprintf(f, "%u\r\n", set386);
	fputs("; Use EMS memory\r\n", f);
	fprintf(f, "%u\r\n", memems);
	fputs("; Direct screen output\r\n", f);
	fprintf(f, "%u\r\n", directvideo);
	fputs("; Enable sounds\r\n", f);
  fprintf(f, "%u\r\n", sonido);
  fputs("; Enable Stacker/Super Sotr/Double Space compatibility\r\n", f);
  fprintf(f, "%u\r\n", stacker);
  fputs("; Clear file slack\r\n", f);
  fprintf(f, "%u\r\n", slack);
  fputs("; Clear FAT\r\n", f);
  fprintf(f, "%u\r\n", fat);
	fputs("; Ask before start destroying\r\n", f);
  fprintf(f, "%u\r\n", totalconf);
	fputs("; Ask before destroy an attributed file\r\n", f);
  fprintf(f, "%u\r\n", attribconf);
	fputs("; Ask before destroy one file\r\n", f);
  fprintf(f, "%u\r\n", fileconf);
	fputs("; Ask before cancel operation\r\n", f);
  fprintf(f, "%u\r\n", cancelconf);
  fputs("; Destruction method\r\n", f);
  fprintf(f, "%u\r\n", method);
	fputs("; Autosave configuration\r\n", f);
  fprintf(f, "%u\r\n", autosave);
  fclose(f);
}

/* -------- Peque¤os inconvenientes para que la gente se registre --------- */
void _fastcall shareware (char *nombre, unsigned char *comercial)
{
	register unsigned int i;
	unsigned char oldx, oldy;
	FILE *f;
	unsigned char *buf, *ptr, *oldb;
	long int checksum;

	buf=malloc(8192);
	if (buf==0)
		return;
  f=fopen(nombre, "rb");
	checksum=0;
	if (f!=0)
	{
		memset(buf, 0, 8192);
		fread(buf, 1, 8192, f);
		fclose(f);
		ptr=buf;
		for (i=0; i!=8192; i++)
		{
			checksum+=(*ptr);
			ptr++;
		}
	}
	if (checksum==1029523L)
		*comercial=1;
	else
	{
		*comercial=0;
		randomize();
		i=random(10);
		if (i==5)
		{
    	_setcursortype(_NOCURSOR);
			oldb=malloc(4000);
			if (oldb==0)
				return;
			oldx=wherex();
			oldy=wherey();
			gettext(1, 1, 80, 25, oldb);
			background(1);
			help (7, 3, 69, 21, 16*3+15, 16*3+0, fhlp, "Shareware", buf);
			for (i=10; i>0; i--)
			{
				sound(1000);
				delay(250);
				nosound();
				delay(750);
			}
			puttext(1, 1, 80, 25, oldb);
			free(oldb);
			gotoxy(oldx, oldy);
			textattr(7);
			_setcursortype(_NORMALCURSOR);
		}
	}
	helpindex(8, 3, 16*3+0, 16*2+7, fhlp, buf);
	free(buf);
}
