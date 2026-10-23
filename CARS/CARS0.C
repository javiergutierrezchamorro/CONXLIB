#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <bios.h>
#include <string.h>
#include <time.h>
#include <\bc\c\cars\cars.h>
#include <\bc\c\cars\conxlib.h>

extern unsigned _stklen=8192;

struct strnhea trnhea;
struct sconfig config;
struct smaster master;
struct smapa mapa[50][20];
struct splayer player[MAX_PLAYER];
struct sobjeto objeto[MAX_OBJETO];

void main (unsigned char argc, char *argv[])
{
	randomize();
	hello("CARS.TMP", COMERCIAL);
	loadmaster();
  mainmenu();
  bye("CARS.TMP", 0, COMERCIAL);
}

/* ----------------------- Encripta una cadena ---------------------------- */
void _fastcall criptar (char *cad, unsigned int num)
{
	unsigned char *ptr;

	ptr=(unsigned char *) cad;
	for (ptr=(unsigned char *) (cad+num); ptr>(unsigned char *) cad; ptr--)
		*ptr=*ptr^170;
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

/* ------------------------- Puntua un long ------------------------------- */
void _fastcall loadmaster (void)
{
	FILE *f;

  textattr(7*16+0+128);
  gotoxy(1, 25);
  cputs("Loading Master...                                           ");
	f=fopen("CARS.MAS", "rb");
	if (f==0)
	{
		master.maxturnos=100;
		strcpy(master.password, "NEWMASTER");
		criptar(master.password, 9);
	}
	else
	{
		fread(&master, 1, sizeof(master), f);
		fclose(f);
	}
}

/* ------------------------- Puntua un long ------------------------------- */
void _fastcall savemaster (void)
{
	FILE *f;

  textattr(7*16+0+128);
  gotoxy(1, 25);
  cputs("Saving Master...                                            ");
	f=fopen("CARS.MAS", "wb");
	fwrite(&master, 1, sizeof(master), f);
	fclose(f);
}

/* ------------------------- Puntua un long ------------------------------- */
void _fastcall loadtrn (void)
{
	register unsigned int i;
	FILE *f;
	char tmp[80];

	textattr(7*16+0+128);
  gotoxy(1, 25);
  cputs("Loading Player...                                           ");
  sprintf(tmp, "%s.TRN", player[0].nombre);
  f=fopen(tmp, "rb");
  if (f==0)
  {
    /* Crear cabecera */
    strcpy(trnhea.mark, "\rCars Connection v2.00 Turn File.\r\n (c) Copyright 1997 by De Trans Software (D.T.S.)\r\n\26");
    strcpy(trnhea.registered, REGISTRATION);
    criptar(trnhea.registered, 49);
    /* Crear jugador */
    player[0].x=0;
		player[0].y=0;
    strcpy(player[0].password, "NONE");
		criptar(player[0].password, 4);
    player[0].nivel=1;
    player[0].turnos=master.maxturnos;
    player[0].totalvida=random(50)+50;
    player[0].llevovida=player[0].totalvida;
    player[0].totalcargo=50;
		player[0].llevocargo=0;
		player[0].tipocargo=0;
    player[0].dinero=500;
		player[0].banco=0;
		/* Crear mapa */
		memset(&mapa, 0, sizeof(mapa));
		/* Crear policias */
		memset(objeto, 0, sizeof(objeto));
    for (i=0; i!=10; i++)
    {
			objeto[i].x=random(50);
			objeto[i].y=random(20);
      sprintf(objeto[i].nombre, "Police %u", i+1);
      objeto[i].val1=random(2000);
      objeto[i].val2=objeto[i].val1*(10+random(4));
    }
    /* Crear tiendas */
    for (i=10; i!=30; i++)
    {
			objeto[i].x=random(50);
			objeto[i].y=random(20);
      sprintf(objeto[i].nombre, "Warehouse %u", i-9);
			objeto[i].val1=random(9)+1;
			objeto[i].val2=0;
			objeto[i].val3=0;
		}
		/* Crear bancos */
		for (i=30; i!=32; i++)
		{
			objeto[i].x=random(50);
			objeto[i].y=random(20);
			sprintf(objeto[i].nombre, "Bank %u", i-29);
			objeto[i].val1=random(9)+1;
			objeto[i].val2=random(1000)*10+100;
			objeto[i].val3=0;
		}
		/* Crear casinos */
		for (i=32; i!=34; i++)
    {
			objeto[i].x=random(50);
			objeto[i].y=random(20);
			sprintf(objeto[i].nombre, "Casino %u", i-31);
			objeto[i].val1=random(20)+40;
			objeto[i].val2=random(20)+40;
			objeto[i].val3=0;
		}
  }
  else
  {
		memset(&trnhea, 0, sizeof(trnhea));
		memset(&player[0], 0, sizeof(player[0]));
		memset(&mapa, 0, sizeof(mapa));
    memset(objeto, 0, sizeof(objeto));
		fread(&trnhea, 1, sizeof(trnhea), f);
		fread(&player[0], 1, sizeof(player[0]), f);
		fread(&mapa, 1, sizeof(mapa), f);
    fread(&objeto, 1, sizeof(objeto), f);
    fclose(f);
  }
}

/* ------------------------- Puntua un long ------------------------------- */
void _fastcall savetrn (void)
{
	FILE *f;
	char tmp[80];

	textattr(7*16+0+128);
  gotoxy(1, 25);
  cputs("Saving Player...                                            ");
  sprintf(tmp, "%s.TRN", player[0].nombre);
  f=fopen(tmp, "wb");
	fwrite(&trnhea, 1, sizeof(trnhea), f);
	fwrite(&player[0], 1, sizeof(player[0]), f);
	fwrite(&mapa, 1, sizeof(mapa), f);
  fwrite(&objeto, 1, sizeof(objeto), f);
  fclose(f);
}

/* ------------------------- Puntua un long ------------------------------- */
unsigned int _fastcall whatis (struct sobjeto objeto[MAX_OBJETO], char x, char y)
{
	register unsigned int i;

	for (i=0; i!=500; i++)
	{
		if ((objeto[i].x==x) && (objeto[i].y==y))
			return(i);
	}
	return(0);
}
