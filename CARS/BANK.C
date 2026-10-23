#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <\bc\c\cars\cars.h>
#include <\bc\c\cars\conxlib.h>

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall bank (void)
{
	register unsigned int i;
	unsigned char oldx, oldy;
	char *oldb, *buf;
	long dinero;
	char tmp[40];
	char men1[][21]={   "Deposit ",
											"Withdraw",
											"  Rob   ",
											"Destroy ",
                      "Continue"};
	char dmen1[][71]={  "Makes a deposit in the bank at the current interest                 ",
											"Retires money from the bank at the current penalty                  ",
											"Robs the bank                                                       ",
											"Destroys the bank                                                   ",
											"Continue moving without entering in the bank                        "};

	oldx=wherex();
	oldy=wherey();
	oldb=malloc(textsize(1, 1, 80, 25));
	if (oldb==0)
		return;
	gettext(1, 1, 80, 25, oldb);
	i=whatis(objeto, player[0].x, player[0].y);
	box(20, 6, 60, 20, 16*7+15, A_DOUBLE, "Bank", SHADOW);
	gotoxy(21, 7);
	cprintf("Current interest is: %u%% (Deposit)", (unsigned int) objeto[i].val1);
	gotoxy(21, 8);
	cprintf("Current interest is: %u%% (Withdraw)", 15);
	gotoxy(21, 9);
	cprintf("You have %s $", puntua(player[0].dinero));
	gotoxy(21, 10);
	cprintf("and %s $ in our bank", puntua(player[0].banco));
	textattr(16*7+0);
	cputsc(20, 60, 12, "Do you want to enter in the");
	cputsc(20, 60, 13, "bank?");
  do
	{
		i=menu(37, 15, 7*16+0, 2*16+15, men1, dmen1, 5, 1);
		if (i==255)
    {
      buf=malloc(16384);
      if (buf==0)
				return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Bank", buf);
			free(buf);
		}
	}
	while (i==255);
	/* Deposit */
	if (i==0)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
		cputsc(20, 60, 11, "How many money you want");
		cputsc(20, 60, 12, "to deposit?");
		memset(tmp, 0, 40);
		getstr (30, 14, 16*1+15, tmp, 20);
		dinero=atol(tmp);
		if (dinero>player[0].dinero)
		{
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cprintf("You try to deposit %s $", puntua(dinero));
			gotoxy(21, 12);
			cprintf("but you only have %s.", puntua(player[0].dinero));
			gotoxy(21, 13);
			cputs("5 $ deducted for burocracy.");
			player[0].dinero-=5;
		}
		else
		{
			player[0].banco+=dinero;
			player[0].dinero-=dinero;
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cprintf("You have diposited %s $", puntua(dinero));
			gotoxy(21, 12);
			cprintf("You have %s $ of cash money", puntua(player[0].dinero));
			gotoxy(21, 13);
			cprintf("and %s $ in our bank", puntua(player[0].banco));
		}
		do
		{
			i=ckey();
			if (i==KBD_F1)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Bank", buf);
				free(buf);
			}
		}
		while (i==KBD_F1);
	}
	/* Withdraw */
	else if (i==1)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
		cputsc(20, 60, 11, "How many money you want");
		cputsc(20, 60, 12, "to withdraw?");
		memset(tmp, 0, 40);
		getstr (30, 14, 16*1+15, tmp, 20);
		dinero=atol(tmp);
		if (dinero>player[0].banco)
		{
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cprintf("You try to withdraw %s $", puntua(dinero));
			gotoxy(21, 12);
			cprintf("but you only have %s.", puntua(player[0].dinero));
			gotoxy(21, 13);
			cputs("5 $ deducted for burocracy.");
			player[0].banco-=5;
		}
		else
		{
			player[0].banco-=dinero;
			player[0].dinero+=dinero-((15*dinero)/100);
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cprintf("You have withdrawed %s $", puntua(dinero));
			gotoxy(21, 12);
			cprintf("You have %s $ of cash money", puntua(player[0].dinero));
			gotoxy(21, 13);
			cprintf("and %s $ in our bank", puntua(player[0].banco));
		}
		do
		{
			i=ckey();
			if (i==KBD_F1)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Bank", buf);
				free(buf);
			}
		}
		while (i==KBD_F1);
	}
	/* Rob */
	else if (i==2)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 9, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
		textattr(16*7+0);
		i=random(100);
		if (i<=5)
		{
			i=whatis(objeto, player[0].x, player[0].y);
			cputsc(21, 59, 10, "Congratulations, you have");
			cputsc(21, 59, 11, "just robbed the bank!");
			sprintf(tmp, "Your reward is %s $", puntua(objeto[i].val2));
			cputsc(21, 59, 12, tmp);
			cputsc(21, 59, 13, "The insurance will pay all");
			cputsc(21, 59, 14, "the acounts.");
			player[0].dinero+=objeto[i].val2;
			player[0].banco=0;
			objeto[i].val2=0;
			objeto[i].val1+=5;
		}
		else
		{
			cputsc(21, 59, 10, "You try to rob the bank; due");
			cputsc(21, 59, 11, "to the high security level for an");
			cputsc(21, 59, 12, "incompetente as you, you have been");
			cputsc(21, 59, 13, "discovered. Jail fiance have been");
			cputsc(21, 59, 14, "payed, you can return to street.");
			player[0].llevovida=player[0].llevovida>>1;
			player[0].dinero=player[0].dinero>>1;
			player[0].llevocargo=0;
			player[0].banco=0;
		}
		do
		{
			i=ckey();
			if (i==KBD_F1)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Bank", buf);
				free(buf);
			}
		}
		while (i==KBD_F1);
	}
	/* Destroy */
	else if (i==3)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 9, 60, 15, 16*7+15, A_DOUBLE, "Bank", SHADOW);
		textattr(16*7+0);
		i=random(100);
		if (i<=5)
		{
			cputsc(21, 59, 10, "Congratulations, you have");
			cputsc(21, 59, 11, "destroyed the bank!");
			cputsc(21, 59, 12, "No reward for your action.");
			i=random(100);
			if (i<=10)
			{
				i=whatis(objeto, player[0].x, player[0].y);
				cputsc(21, 59, 13, "The bank is not insured,");
				cputsc(21, 59, 14, "the company has broken.");
				memset(objeto[i].nombre, 0, 16);
			}
			else
			{
				i=whatis(objeto, player[0].x, player[0].y);
				cputsc(21, 59, 13, "The insurance will pay");
				cputsc(21, 59, 14, "all damages.");
			}
		}
		else
		{
			cputsc(21, 59, 10, "You try to destroy the bank; due");
			cputsc(21, 59, 11, "to the high security level for an");
			cputsc(21, 59, 12, "incompetente as you, you have been");
			cputsc(21, 59, 13, "discovered. Jail fiance have been");
			cputsc(21, 59, 14, "payed, you can return to street.");
			player[0].dinero=player[0].dinero>>2;
		}
		do
		{
			i=ckey();
			if (i==KBD_F1)
			{
				buf=malloc(16384);
				if (buf==0)
					return;
				help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Bank", buf);
				free(buf);
			}
		}
		while (i==KBD_F1);
	}
	else if (i==4)
	{
	}
  puttext(1, 1, 80, 25, oldb);
  free(oldb);
	gotoxy(oldx, oldy);
}
