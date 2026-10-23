#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <\bc\c\cars\cars.h>
#include <\bc\c\cars\conxlib.h>

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall casino (void)
{
	register unsigned int i, j, k;
	unsigned char oldx, oldy, carta1, carta2;
	char *oldb, *buf;
	long dinero;
	unsigned char jack1[21];
	char tmp[40];
  char men1[][21]={   "  Dices  ",
											"Black-Red",
											" Jackpot ",
											"Blackjack",
											"Continue"};
  char dmen1[][71]={  "Play the dices                                                      ",
											"Play the wheel black-red                                            ",
											"Play the jackpots                                                   ",
											"Play to blackjack                                                   ",
											"Continue moving without entering in the casino                      "};
	char but1[][11]={   "[Red]",
											"[Black]"};
	char dbut1[][71]={  "Bet for red                                                         ",
											"Bet for black                                                       "};
	char but2[][11]={   "[Stay]",
											"[Card]"};
	char dbut2[][71]={  "Stay playing                                                        ",
											"Get one card more                                                   "};

	oldx=wherex();
	oldy=wherey();
	oldb=malloc(textsize(1, 1, 80, 25));
	if (oldb==0)
		return;
	gettext(1, 1, 80, 25, oldb);
	i=whatis(objeto, player[0].x, player[0].y);
	box(20, 6, 60, 20, 16*7+15, A_DOUBLE, "Casino", SHADOW);
	gotoxy(21, 7);
	cputs("Welcome to the casino dear");
	gotoxy(21, 8);
	cprintf("%s we are pleased with you.", player[0].nombre);
	gotoxy(21, 9);
	cprintf("You have %s $", puntua(player[0].dinero));
	gotoxy(21, 10);
	cprintf("and %s $ in the bank", puntua(player[0].banco));
	textattr(16*7+0);
	cputsc(20, 60, 12, "Do you want to enter in the");
	cputsc(20, 60, 13, "casino?");
	do
	{
		i=menu(37, 15, 7*16+0, 2*16+15, men1, dmen1, 5, 1);
		if (i==255)
		{
			buf=malloc(16384);
			if (buf==0)
				return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Casino", buf);
			free(buf);
		}
	}
	while (i==255);
	if (i!=4)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
		cputsc(20, 60, 11, "How many money you want");
		cputsc(20, 60, 12, "to bet?");
		memset(tmp, 0, 40);
		getstr (30, 14, 16*1+15, tmp, 20);
		dinero=atol(tmp);
		if (player[0].dinero<=0)
		{
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cprintf("You try to bet %s $", puntua(dinero));
			gotoxy(21, 12);
			cprintf("but you only have %s.", puntua(player[0].dinero));
			gotoxy(21, 13);
			cputs("Sorry you are not allowed here.");
		}
		else if (dinero>player[0].dinero)
		{
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cprintf("You try to bet %s $", puntua(dinero));
			gotoxy(21, 12);
			cprintf("but you only have %s.", puntua(player[0].dinero));
			gotoxy(21, 13);
			cputs("5 $ deducted for burocracy.");
			player[0].dinero-=5;
		}
		else
		{
			/* Dice */
			if (i==0)
			{
				puttext(1, 1, 80, 25, oldb);
				i=random(6)+1;
				j=random(6)+1;
				box(20, 11, 60, 13, 16*7+15, A_DOUBLE, "Dice", SHADOW);
				gotoxy(35, 12);
				cprintf("%u ", i);
				delay(500);
				cprintf("+ %u ", j);
				delay(500);
				cprintf("= %u ", i+j);
				i=i+j;
				ckey();
				if ((i==7) || (i==11))
				{
					player[0].dinero+=(dinero<<1);
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You win!");
					gotoxy(21, 12);
					cprintf("You have earned %s $", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s $", puntua(player[0].dinero));
				}
				else
				{
					player[0].dinero-=dinero;
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You loose!");
					gotoxy(21, 12);
					cprintf("You have lost %s $", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s $", puntua(player[0].dinero));
				}
			}
			/* Black-Red */
			else if (i==1)
			{
				puttext(1, 1, 80, 25, oldb);
				box(20, 10, 60, 14, 16*7+15, A_DOUBLE, "Red-Black", SHADOW);
				gotoxy(35, 12);
				cputsc(21, 59, 11, "Which colour?");
				j=buttons(25, 13, 16*7+0, 16*2+15, but1, dbut1, 2, 23);
				i=random(100);
				puttext(1, 1, 80, 25, oldb);
				box(20, 11, 60, 13, 16*7+15, A_DOUBLE, "Red-Black", SHADOW);
				for (k=0; k!=10; k++)
				{
					gotoxy(38, 12);
					cputs(" Red ");
					delay(k*30);
					gotoxy(38, 12);
					cputs("Black");
					delay(k*30);
				}
				gotoxy(38, 12);
				i=i&1;
				if (i==0)
					cputs(" Red ");
				else
					cputs("Black");
				ckey();
				if (i==j)
				{
					player[0].dinero+=dinero;
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You win!");
					gotoxy(21, 12);
					cprintf("You have earned %s", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s", puntua(player[0].dinero));
				}
				else
				{
					player[0].dinero-=dinero;
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Casino", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You loose!");
					gotoxy(21, 12);
					cprintf("You have lost %s", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s", puntua(player[0].dinero));
				}
			}
			/* Jackpot */
			else if (i==2)
			{
				puttext(1, 1, 80, 25, oldb);
				box(20, 11, 60, 13, 16*7+15, A_DOUBLE, "Jackpot", SHADOW);
				memset(tmp, 0, 40);
				strcpy(tmp, "° ° °");
				gotoxy(38, 12);
				cputs(tmp);
				delay(500);
				tmp[0]=random(6)+49;
				gotoxy(38, 12);
				cputs(tmp);
				delay(500);
				tmp[2]=random(6)+49;
				gotoxy(38, 12);
				cputs(tmp);
				delay(500);
				tmp[4]=random(6)+49;
				gotoxy(38, 12);
				cputs(tmp);
				ckey();
				if ((tmp[0]==tmp[2]) && (tmp[2]==tmp[4]))
				{
					i=3;
					j=tmp[0];
				}
				else if (tmp[0]==tmp[2])
				{
					i=2;
					j=tmp[0];
				}
				else if (tmp[0]==tmp[4])
				{
					i=2;
					j=tmp[0];
				}
				else if (tmp[2]==tmp[4])
				{
					i=2;
					j=tmp[2];
				}
				else
				{
					i=1;
					j=1;
				}
        j-=48;
        /*tmp[0]-=48;
				tmp[2]-=48;
        tmp[4]-=48;*/
				if (i>1)
				{
					player[0].dinero+=dinero*(j+i);
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Jackpot", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You win!");
					gotoxy(21, 12);
					cprintf("You have earned %s", puntua(dinero*(j+i)));
					gotoxy(21, 13);
					cprintf("now you have %s", puntua(player[0].dinero));
				}
				else
				{
					player[0].dinero-=dinero;
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Jackpot", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You loose!");
					gotoxy(21, 12);
					cprintf("You have lost %s", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s", puntua(player[0].dinero));
				}
			}
			/* Blackjack */
			else if (i==3)
			{
				puttext(1, 1, 80, 25, oldb);
				box(10, 9, 68, 15, 16*7+15, A_DOUBLE, "Blackjack", SHADOW);
				j=0;
				carta1=0;
				do
				{
					jack1[j]=random(10)+1;
					carta1+=jack1[j];
					j++;
					textattr(16*7+0);
					gotoxy(11, 10);
					cputs("You:      ");
					for (i=0; i!=j; i++)
						cprintf("%2u ", jack1[i]);
					cprintf("(%2u) ", carta1);
					if (carta1==21)
					{
						gotoxy(59, 10);
						cputs("Blackjack");
						k=0;
					}
					else if (carta1>21)
					{
						gotoxy(59, 10);
						cputs("Pass");
						carta1=0;
						k=0;
					}
					else
						k=buttons(20, 14, 16*7+0, 16*2+15, but2, dbut2, 2, 30);
				}
				while(k!=0);
				if ((k==0) && (carta1>0) && (carta1<21))
				{
					gotoxy(59, 10);
					cputs("Stay");
				}
				k=0;
				j=0;
				gotoxy(20, 14);
				cputs("                                          ");
				carta2=0;
				do
				{
					jack1[j]=random(10)+1;
					carta2+=jack1[j];
					j++;
					textattr(16*7+0);
					gotoxy(11, 11);
					cputs("Croupier: ");
					for (i=0; i!=j; i++)
						cprintf("%2u ", jack1[i]);
					cprintf("(%2u) ", carta2);
					delay(1500);
					if (carta2==21)
					{
						gotoxy(59, 11);
						cputs("Blackjack");
						k=1;
					}
					else if (carta2>21)
					{
						gotoxy(59, 11);
						cputs("Pass");
						k=1;
						carta2=0;
					}
				}
				while((carta1>carta2) && (k==0));
				if ((k==0) && (carta2>0) && (carta2<21))
				{
					gotoxy(59, 11);
					cputs("Stay");
				}
				delay(1500);
				ckey();
				puttext(1, 1, 80, 25, oldb);
				if (carta1>carta2)
				{
					if (carta1==21)
						dinero*=3;
					player[0].dinero+=dinero;
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Blackjack", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You win!");
					gotoxy(21, 12);
					cprintf("You have earned %s", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s", puntua(player[0].dinero));
				}
				else
				{
					player[0].dinero-=dinero;
					box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Blackjack", SHADOW);
					textattr(16*7+0);
					gotoxy(21, 11);
					cputs("You loose!");
					gotoxy(21, 12);
					cprintf("You have lost %s", puntua(dinero));
					gotoxy(21, 13);
					cprintf("now you have %s", puntua(player[0].dinero));
				}
			}
		}
	}
	do
	{
		i=ckey();
		if (i==KBD_F1)
		{
			buf=malloc(16384);
			if (buf==0)
				return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Casino", buf);
			free(buf);
		}
	}
	while (i==KBD_F1);
	puttext(1, 1, 80, 25, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}
