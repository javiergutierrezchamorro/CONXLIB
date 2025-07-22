#pragma once
#ifndef _CONIOXLIB_H_ 
#define _CONIOXLIB_H_

#define conioxlib_version (190)
#define conioxlib_build (__DATE__)

#ifdef __cplusplus
extern "C" {
#endif


#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	#include <windows.h>
	#include <WinUser.h>
#else
	//#include <bios.h>
	//#include <dir.h>
#endif
#include "conxlib.h"


#define A_NONE    "       "
#define A_BLOCK   "ÛÛßÜÛÛÛ"
#define A_SINGLE  "Ú¿ÄÄ³ÀÙ"
#define A_DOUBLE  "É»ÍÍºÈ¼"
#define U_SINGLE  "Ö·ÄÄºÓ½"
#define S_DOUBLE  U_SIMPLE
#define U_DOUBLE  "Õ¸ÍÍ³Ô¾"
#define S_SINGLE  U_DOUBLE
#define SHADOW    1
#define CLOSE     2
#define NOWINDOW	4

#define KBD_UP    0x4800
#define KBD_DOWN  0x5000
#define KBD_LEFT  0x4B00
#define KBD_RIGHT 0x4D00
#define KBD_PGUP  0x4900
#define KBD_PGDW  0x5100
#define KBD_ORG   0x4700
#define KBD_END   0x4F00
#define KBD_ENTER 0x1C0D
#define KBD_ESC   0x011B
#define KBD_F1    0x3B00
#define KBD_F2    0x3C00
#define KBD_F3    0x3D00
#define KBD_F4    0x3E00
#define KBD_F5    0x3F00
#define KBD_F6    0x4000
#define KBD_F7    0x4100
#define KBD_F8    0x4200
#define KBD_F9    0x4300
#define KBD_F10   0x4400

#define conioxlib_textsize(x1, y1, x2, y2)  ((((x2)-(x1)+1)*((y2)-(y1)+1))<<1)


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
	unsigned int date;					/* Formato de fecha */
	char curr[5];               /* S¡mbolo de moneda */
	char thsep[2];              /* Separador de miles */
	char dsep[2];               /* Separador decimal */
	char dtsep[2];              /* Separador de fecha */
	char tmsep[2];              /* Separador de hora */
	unsigned char currstyle[2]; /* Estilo de moneda */
	unsigned char digits;       /* D¡gitos significativos de la moneda */
	unsigned char time;         /* Formato de hora */
	unsigned long int convcase; /* Conversi¢n a may£sculas */
	unsigned char dasep;        /* Separador de datos */
	char reserved[10];          /* Reservado */
	unsigned int code;          /* C¢digo de p gina */
};


extern struct conioxlib_smousestatus conioxlib_mousestatus;


void conioxlib_pollmouse(void);


void getcountry(struct conioxlib_scountry *);
void setpal (unsigned char);
void empuja (void);
unsigned int ckey (void);
void watch (void);
void blocks (void);
void cputsc (unsigned char, unsigned char, unsigned char, char *);
void getstr (unsigned char, unsigned char, unsigned char, char *, char);
void getpw (unsigned char, unsigned char, unsigned char, char *, char);
unsigned char tabla (unsigned char, unsigned char, unsigned char, unsigned char, char [][16], unsigned char);
unsigned char buttons (unsigned char, unsigned char, unsigned char, unsigned char, char [][11], char [][71], unsigned char, unsigned char);
unsigned char menu (unsigned char, unsigned char, unsigned char, unsigned char, char [][21], char [][71], unsigned char, unsigned char);
unsigned char option (unsigned char, unsigned char, unsigned char, unsigned char, char [][21], char [][71], char *, unsigned char, unsigned char, unsigned char);
unsigned char property (unsigned char, unsigned char, unsigned char, unsigned char, char [][21], char [][71], char *, unsigned char, unsigned char, unsigned char);
void box (unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, char *, char *, unsigned char);
void hello (char *, unsigned char);
void bye (char *, unsigned char, unsigned char);
//void interrupt error (unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
void background (unsigned char msg[]);
void about (unsigned char);
unsigned char exittodos (char *);
void screensaver (void);
void rebota (void);
void shell (unsigned char, unsigned char, unsigned char, unsigned char, char *);
//void interrupt shell_patch (unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
void calendar (unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned int);
unsigned char view (unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, char [][61], unsigned char ultimo, char *);
unsigned char help (unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, char *, char *, char[][61]);
unsigned char helpindex (unsigned char, unsigned char, unsigned char, unsigned char, char *, char [][16]);
//unsigned char select (unsigned char *, char *, struct conioxlib_sfiles *);
int select_sort(const void *, const void *);
void select_puntua (char *, long int);

#endif
