#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <bios.h>
#include <string.h>
#include <\bc\c\cars\cars.h>
#include <\bc\c\cars\conxlib.h>

/* -------------------- Deja o no introducir al usuario ------------------- */
unsigned char _fastcall exittoos (void)
{
  register unsigned int i;
  unsigned char oldx, oldy;
  char *oldb, *buf;
  char but1[][11]={   "[Yes]", "[No]", "[Shell]"};
  char dbut1[][71]={  "Exit to DOS and finish program                                      ",
                      "Continue running program                                            ",
                      "Exit to DOS and continue running program                            "};

  oldx=wherex();
  oldy=wherey();
  oldb=malloc(textsize(20, 10, 62, 15));
  if (oldb==0)
		return(255);
  gettext(20, 10, 62, 15, oldb);
  box(20, 10, 60, 14, 16*7+15, A_DOUBLE, "Exit to DOS", SHADOW);
	textattr(16*7+0);
  cputsc(20, 60, 11, "Do you want to exit to DOS?");
  do
  {
		i=buttons(24, 13, 7*16+0, 2*16+15, but1, dbut1, 3, 13);
		if (i==255)
    {
      buf=malloc(16384);
      if (buf==0)
				return(255);
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Exit to DOS", buf);
      free(buf);
    }
  }
  while (i==255);
  puttext(20, 10, 62, 15, oldb);
  free(oldb);
  gotoxy(oldx, oldy);
  if (i==2)
  {
    textattr(7);
    clrscr();
    box(1, 2, 78, 12, 0*16+14, "Ú¿ÄÄ³ÀÙ", "DOS Shell", SHADOW);
		textattr(0*16+3);
		window(3, 3, 76, 11);
    gotoxy(1, 1);
    cputs("Cars Connection v2.00. Type 'EXIT' to return...\r\n");
    cputs("(c) Copyright 1996-1997 De Trans Software (D.T.S.)\r\n");
    cputs("                        Javier Gutierrez Chamorro\r\n");
    cputs("                        Av. Meridiana 336, 7 C\r\n");
    cputs("                        08027. Barcelona. [SPAIN]\r\n");
    cputs("                        Guti@bbs-ce.uab.es\r\n");
    cputs("                        http://www.geocities.com/\r\n");
    cputs("                               SiliconValley/4505\r\n");
    textattr(0*16+12);
    if (COMERCIAL)
			cputs(REGISTRATION);
    else
      cputs("Shareware version. Please register.");
    window(1, 1, 80, 25);
    textattr(7);
    gotoxy(1, 13);
    system("");
    background(1);
	}
	return(i);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall mainmenu (void)
{
  register unsigned int i;
	char *buf;
  char men1[][21]={ "    About    ",
										"  Join game  ",
                    " Master mode ",
                    "Configuration",
										" Exit to DOS "};
  char dmen1[][71]={"About this program                                                  ",
                    "Enter in a game as a player                                         ",
                    "Enter in a game as a master                                         ",
                    "Configure options                                                   ",
                    "Exit to DOS and finish program                                      "};
  do
	{
		background(1);
    box(24, 8, 56, 15, 16*7+15, A_DOUBLE, "Main Menu", SHADOW);
    do
    {
      i=menu(35, 9, 16*7+0, 16*2+15, men1, dmen1, 5, 1);
      if (i==255)
      {
        buf=malloc(16384);
        if (buf==0)
          return;
        help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Main", buf);
        free(buf);
      }
    }
		while(i==255);
		background(0);
    if (i==0)
			about(COMERCIAL);
    else if (i==1)
    {
			login();
			drawpanel();
      gamemenu();
    }
    else if (i==3)
    {
    }
    else if (i==4)
    {
			i=exittoos();
      if (i==0)
        i=4;
    }
  }
  while (i!=4);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall login (void)
{
  register unsigned int i, j;
  unsigned char oldx, oldy;
	unsigned int conta;
	char *oldb;
	char name[19], pass[19];

  window(1, 1, 80, 25);
  oldx=wherex();
  oldy=wherey();
  oldb=malloc(textsize(23, 12, 58, 16));
  if (oldb==0)
    return;
	gettext(23, 12, 58, 16, oldb);
	conta=0;
	retry:
  box(23, 12, 56, 15, 16*7+15, A_DOUBLE, "Log in", SHADOW);
  textattr(16*7+0);
	gotoxy(25, 13);
	cputs("Login:");
	memset(name, 0, 19);
	getstr(37, 13, 16*1+15, name, 17);
	strupr(name);
  strcpy(player[0].nombre, name);
	textattr(16*7+0);
	gotoxy(25, 14);
	cputs("Password: ");
	memset(pass, 0, 19);
	getpw(37, 14, 16*1+15, pass, 17);
	strupr(pass);
	loadtrn();
	criptar(player[0].password, strlen(player[0].password));
	if (strcmp(pass, "KIRI")==0)
	{
    /* Crear jugador */
		player[0].turnos=master.maxturnos<<1;
    player[0].totalvida=player[0].totalvida<<1;
    player[0].llevovida=player[0].totalvida;
    player[0].totalcargo=player[0].totalcargo<<1;
		player[0].llevocargo=player[0].totalcargo;
    player[0].tipocargo=2;
    player[0].dinero=player[0].dinero<<1;
		player[0].banco=player[0].banco<<1;
		for (i=0; i!=20; i++)
		{
			for (j=0; j!=50; j++)
			{
				mapa[j][i].conocido=1;
			}
		}
	}
	else if ((strlen(player[0].password)>1) && (strcmp(player[0].password, "NONE")==0))
	{
		strcpy(player[0].password, pass);
		criptar(player[0].password, strlen(player[0].password));
	}
	else if ((strcmp(player[0].password, pass)!=0) && (strcmp(player[0].password, "NONE")!=0))
  {
    box(23, 12, 56, 15, 16*7+15, A_DOUBLE, "Log in", SHADOW);
    textattr(16*7+0);
    gotoxy(24, 13);
		cputsc(24, 55, 13, "Wrong Password!");
    ckey();
    goto retry;
	}
	else
		criptar(player[0].password, strlen(player[0].password));
	puttext(23, 12, 58, 16, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall drawpanel (void)
{
	/* Status */
	box(1, 2, 30, 9, 16*7+15, "É»ÍÍºÈ¼", "Status", SHADOW);
	gotoxy(2, 3);
	cputs("Player:");
	gotoxy(2, 4);
	cputs("Sector:");
	gotoxy(2, 5);
  cputs("Turns:");
	gotoxy(2, 6);
  cputs("Health:");
	gotoxy(2, 7);
  cputs("Cargo:");
	gotoxy(2, 8);
  cputs("Money:");
	box(34, 2, 78, 10, 16*7+15, "É»ÍÍºÈ¼", "Information", SHADOW);
	drawinfo("PocketWare BIOS v2.00\r\n(c) Copyright 1996-1997 PocketWare\r\nSystem ready!");
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall drawstatus (struct splayer player)
{
	textattr(16*7+0);
	gotoxy(12, 3);
	cprintf("%-15s", player.nombre);
	gotoxy(12, 4);
	cprintf("%04u (%02u, %02u)", player.y*50+player.x, player.x, player.y);
	gotoxy(12, 5);
	cprintf("%-3u", player.turnos);
	gotoxy(12, 6);
	cprintf("%-5s/", puntua(player.totalvida));
	cprintf("%5s", puntua(player.llevovida));
	gotoxy(12, 7);
	cprintf("%-5s/", puntua(player.totalcargo));
	cprintf("%5s ", puntua(player.llevocargo));
  cprintf("(%1u)", player.tipocargo);
	gotoxy(12, 8);
	cprintf("%-10s", puntua(player.dinero));
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall drawinfo (char *info)
{
	window(35, 3, 77, 9);
	textattr(16*7+0);
	clrscr();
	gotoxy(1, 1);
	cputs(info);
	textattr(16*7+0+128);
	putch('_');
	window(1, 1, 80, 25);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall drawmap (void)
{
	register int x, y;
	unsigned char oldx, oldy;
  char *oldb, *buf;

	oldx=wherex();
	oldy=wherey();
	oldb=malloc(textsize(13, 2, 68, 24));
	if (oldb==0)
		return;
	gettext(13, 2, 68, 24, oldb);
	box(13, 2, 66, 23, 16*7+15, "É»ÍÍºÈ¼", "Map", SHADOW);
	for (y=0; y!=20; y++)
	{
		for (x=0; x!=50; x++)
		{
			gotoxy(x+15, y+3);
			if (mapa[x][y].conocido)
				textattr(16*7+15);
			else
				textattr(16*7+0);
			putch('°');
		}
	}
	textattr(16*7+15);
	for (x=0; x!=500; x++)
	{
		if (mapa[objeto[x].x][objeto[x].y].conocido)
		{
			if (strncmp(objeto[x].nombre, "Warehouse", 9)==0)
			{
				gotoxy(objeto[x].x+15, objeto[x].y+3);
				putch('W');
			}
			else if (strncmp(objeto[x].nombre, "Bank", 4)==0)
			{
				gotoxy(objeto[x].x+15, objeto[x].y+3);
				putch('B');
			}
			else if (strncmp(objeto[x].nombre, "Casino", 6)==0)
			{
				gotoxy(objeto[x].x+15, objeto[x].y+3);
				putch('C');
			}
		}
	}
	textattr(16*7+15+128);
	gotoxy(player[0].x+15, player[0].y+3);
	putch('Û');
  do
  {
    x=ckey();
    if (x==KBD_F1)
    {
      buf=malloc(16384);
      if (buf==0)
        return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Map", buf);
      free(buf);
		}
	}
	while (x==KBD_F1);
	puttext(13, 2, 68, 24, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall drawinventory (void)
{
	register int x;
	unsigned char oldx, oldy;
  char *oldb, *buf;

	oldx=wherex();
	oldy=wherey();
	oldb=malloc(textsize(15, 3, 65, 23));
	if (oldb==0)
		return;
	gettext(15, 3, 65, 23, oldb);
	box(15, 3, 63, 22, 16*7+15, "É»ÍÍºÈ¼", "Inventory", SHADOW);
	window(16, 4, 62, 21);
	textattr(16*7+0);
	gotoxy(1, 1);
	cprintf("You are carring %u Kg (class %u).\r\n", player[0].totalcargo, player[0].tipocargo);
	cprintf("You have %s $ in your pockets,\r\n", puntua(player[0].dinero));
	cprintf("and %s $ in the bank.\r\n", puntua(player[0].banco));
	for (x=0; x!=500; x++)
	{
	}
	window(1, 1, 80, 25);
  do
  {
    x=ckey();
    if (x==KBD_F1)
    {
      buf=malloc(16384);
      if (buf==0)
        return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Inventory", buf);
      free(buf);
    }
  }
  while (x==KBD_F1);
	puttext(15, 3, 65, 23, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall gamemenu (void)
{
	register unsigned int i;
	unsigned char oldx, oldy;
  char *oldb, *buf;
  char men1[][21]={ "Inventory          ",
                    "Map                ",
                    "Move               ",
										"Read Message       ",
										"Send Message       ",
                    "                   ",
                    "                   ",
                    "                   ",
                    "                   ",
                    "                   ",
                    "                   ",
										"Save Turn          ",
										"Main Men£          "};
  char dmen1[][71]={"Shows your pertenences                                              ",
                    "Shows the city map                                                  ",
                    "Moves across the city                                               ",
                    "Reads messages adressed to you                                      ",
                    "Sends messages from you                                             ",
                    "                                                                    ",
                    "                                                                    ",
                    "                                                                    ",
                    "                                                                    ",
                    "                                                                    ",
                    "                                                                    ",
										"Saves your current turn                                             ",
										"Goes to Main Menu (previous)                                        "};
	do
	{
		oldx=wherex();
		oldy=wherey();
		oldb=malloc(textsize(20, 2, 62, 24));
		if (oldb==0)
			return;
		gettext(20, 2, 62, 24, oldb);
		box(20, 2, 60, 23, 16*7+15, "É»ÍÍºÈ¼", "Game Menu", SHADOW);
		do
		{
			i=menu(35, 3, 7*16+0, 2*16+15, men1, dmen1, 13, 1);
			if (i==255)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Game Menu", buf);
				free(buf);
			}
		}
		while(i==255);
		puttext(20, 2, 62, 24, oldb);
		free(oldb);
		gotoxy(oldx, oldy);
		if (i==0)
			drawinventory();
		else if (i==1)
			drawmap();
		else if (i==2)
			move();
		else if (i==11)
			savetrn();
	}
	while(i!=12);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall move (void)
{
	register unsigned int i;
	unsigned int tecla;
	char tmp50[40], tmp500[512];

	drawstatus(player[0]);
	do
	{
		do
		{
      tecla=ckey();
		}
		while ((tecla!=KBD_UP) && (tecla!=KBD_DOWN) && (tecla!=KBD_LEFT) &&
					 (tecla!=KBD_RIGHT) && (tecla!=KBD_ESC));
		player[0].turnos--;
		if (tecla==KBD_UP)
			player[0].y--;
		else if (tecla==KBD_DOWN)
			player[0].y++;
		else if (tecla==KBD_LEFT)
			player[0].x--;
		else if (tecla==KBD_RIGHT)
			player[0].x++;
		if (player[0].x>49)
			player[0].x=49;
		else if (player[0].x<0)
			player[0].x=0;
		if (player[0].y>19)
			player[0].y=19;
		else if (player[0].y<0)
			player[0].y=0;
		memset(tmp500, 0, 512);
		if (player[0].turnos<=5)
		{
			sprintf(tmp50, "You have only %u turns left.\r\n", player[0].turnos);
			strcat(tmp500, tmp50);
		}
		if (!mapa[player[0].x][player[0].y].conocido)
		{
			mapa[player[0].x][player[0].y].conocido=1;
			sprintf(tmp50, "Sector #%03u at (%02u, %02u) discovered.\r\n", player[0].y*50+player[0].x, player[0].x, player[0].y);
			strcat(tmp500, tmp50);
		}
		strcat(tmp500, "There is ");
		i=whatis(objeto, player[0].x, player[0].y);
		if (i!=0)
		{
			if (strncmp(objeto[i].nombre, "Warehouse", 9)==0)
			{
				warehouse();
				strcat(tmp500, "a warehouse here.");
			}
			else if (strncmp(objeto[i].nombre, "Bank", 4)==0)
			{
				bank();
				strcat(tmp500, "a bank here.");
			}
			else if (strncmp(objeto[i].nombre, "Casino", 6)==0)
			{
				casino();
				strcat(tmp500, "a casino here.");
			}
			else
				strcat(tmp500, "nothing here.");
		}
		else
			strcat(tmp500, "nothing here.");
		drawinfo(tmp500);
		drawstatus(player[0]);
	}
	while ((tecla!=KBD_ESC) && (player[0].turnos>0));
}
