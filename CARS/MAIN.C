#include <stdio.h>
#include <stdlib.h>
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
