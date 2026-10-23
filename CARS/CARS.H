#if !defined(__CARS_DEF_)
#define __CARS_DEF_
#define COMERCIAL     1
#define REGISTRATION  "Registered to De Trans Software (D.T.S.)"
#define MAX_PLAYER    10
#define MAX_OBJETO    500

struct strnhea
{
	char mark[100];
	char registered[50];
};

struct smapa
{
	unsigned char conocido;
};

struct sobjeto
{
	char x;             /* Posici¢n X */
	char y;             /* Posici¢n Y */
  char nombre[16];    /* Nombre */
  long int val1;      /* Valor 1 */
  long int val2;      /* Valor 2 */
  long int val3;      /* Valor 3 */
};

struct splayer
{
	char x;
	char y;
  char nombre[16];
	char password[16];
	unsigned char nivel;
	unsigned char turnos;
	unsigned int totalvida;
	unsigned int llevovida;
	unsigned int totalcargo;
	unsigned int llevocargo;
  unsigned char tipocargo;
	unsigned long int dinero;
	unsigned long int banco;
};

struct smaster
{
	unsigned char maxturnos;
  char password[16];
};

struct sconfig
{
	unsigned char directvideo;
};

extern struct strnhea trnhea;
extern struct sconfig config;
extern struct smaster master;
extern struct smapa mapa[50][20];
extern struct splayer player[MAX_PLAYER];
extern struct sobjeto objeto[MAX_OBJETO];

/* General */
void _fastcall criptar (char *, unsigned int);
char * pascal puntua (long int);
void _fastcall loadmaster (void);
void _fastcall savemaster (void);
unsigned char _fastcall loadtrn (void);
void _fastcall savetrn (void);
unsigned int _fastcall whatis (struct sobjeto [MAX_OBJETO], char, char);

/* Player */
void _fastcall mainmenu (void);
void _fastcall login (void);
void _fastcall gamemenu (void);
void _fastcall move (void);
void _fastcall drawpanel (void);
void _fastcall drawstatus (struct splayer);
void _fastcall drawinfo (char *);
void _fastcall drawmap (void);
void _fastcall drawinventory (void);

/* Warehouse */
void _fastcall warehouse (void);
/* Bank */
void _fastcall bank (void);
/* Casino */
void _fastcall casino (void);
#endif
