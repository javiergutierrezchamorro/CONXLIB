extern unsigned char comercial;
extern unsigned int set386, memems, sonido, totalconf, fileconf, attribconf, cancelconf, method, autosave, stacker, slack, fat, disk, sector;
extern char *buf;
extern struct sfiles *files;
extern unsigned long int llevototal, total, llevoparcial, parcial;
extern char masc[13], tray[80], dir[80];
extern char fprg[80], fhlp[80], fcfg[80], freg[80], ftmp[80];

void _fastcall entorno (void);
void _fastcall starte (void);
void _pascal wipee (char *);
void _fastcall comandos (char *, unsigned char, char *);
void _fastcall startc (void);
void _pascal wipec (char *);
char * pascal puntua (long int);
unsigned char _fastcall loadconfig (char *);
void _fastcall saveconfig (char *);
void _fastcall shareware (char *, unsigned char *);
