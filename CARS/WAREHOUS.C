#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <\bc\c\cars\cars.h>
#include <\bc\c\cars\conxlib.h>


/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall warehouse (void)
{
	register unsigned int i, j, k;
	unsigned char oldx, oldy, carta1, carta2;
	char *oldb, *buf;
	long kg;
	unsigned char jack1[21];
	char tmp[40];
	char men1[][21]={   "  Lists  ",
											"   Buy   ",
											"   Sell  ",
                      "         ",
											"Continue"};
	char dmen1[][71]={  "List of all the warehouses                                          ",
											"Buy material                                                        ",
                      "Sell material                                                       ",
                      "                                                                    ",
                      "Continue moving without entering in the warehouse                   "};

	oldx=wherex();
	oldy=wherey();
	oldb=malloc(textsize(1, 1, 80, 25));
	if (oldb==0)
		return;
	gettext(1, 1, 80, 25, oldb);
	i=whatis(objeto, player[0].x, player[0].y);
	box(20, 6, 60, 20, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
	gotoxy(21, 7);
	cputs("This warehouse is dedicated to");
	gotoxy(21, 8);
	cputs("classes: ");
  if (objeto[i].val1&1)
    cputs("I ");
	if (objeto[i].val1&2)
		cputs("II ");
	if (objeto[i].val1&4)
		cputs("III ");
	gotoxy(21, 9);
	cprintf("You can sell up to %u Kg of class ", player[0].llevocargo);
	if (player[0].tipocargo==1)
		cputs("I");
	if (player[0].tipocargo==2)
		cputs("II");
	if (player[0].tipocargo==3)
		cputs("III");
	gotoxy(21, 10);
	cprintf("You can buy up to %u Kg of class ", player[0].totalcargo-player[0].llevocargo);
	if (player[0].tipocargo==1)
		cputs("I");
	if (player[0].tipocargo==2)
		cputs("II");
	if (player[0].tipocargo==3)
		cputs("III");
	gotoxy(21, 10);
	textattr(16*7+0);
	cputsc(20, 60, 12, "Do you want to enter in the");
	cputsc(20, 60, 13, "warehouse?");
	do
	{
		i=menu(37, 15, 7*16+0, 2*16+15, men1, dmen1, 5, 1);
		if (i==255)
		{
			buf=malloc(16384);
			if (buf==0)
				return;
			help (7, 3, 69, 21, 16*3+15, 16*3+0, "CARS.HLP", "Warehouse", buf);
			free(buf);
		}
	}
	while (i==255);
	if ((i!=0) && (i!=4))
	{
		j=whatis(objeto, player[0].x, player[0].y);
		if (((objeto[j].val1&1) && (player[0].tipocargo==1)) ||
				((objeto[j].val1&2) && (player[0].tipocargo==2)) ||
				((objeto[j].val1&4) && (player[0].tipocargo==3)))
		{
		}
		else
		{
			puttext(1, 1, 80, 25, oldb);
			box(20, 11, 60, 14, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
			cputsc(20, 60, 12, "You are not allowed to sell or");
			cputsc(20, 60, 13, "buy in this warehouse");
			ckey();
			i=4;
		}
	}
	/* List */
	if (i==0)
	{
		puttext(1, 1, 80, 25, oldb);
		box(5, 2, 74, 24, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
		window(6, 3, 73, 23);
		gotoxy(1, 1);
		for (j=0; j!=300; j++)
		{
			if (memcmp(objeto[j].nombre, "Warehouse", 9)==0)
			{
				if (objeto[j].val1&1)
					cprintf("Name: %15s; Class: 1; Sell/Buy: (%2u/%2u); Coord: (%02u, %02u)\r\n", objeto[j].nombre, (int) objeto[j].val3, (int) objeto[j].val2, (int) objeto[j].x, (int) objeto[j].y );
				else if (objeto[j].val1&2)
					cprintf("Name: %15s; Class: 2; Sell/Buy: (%2u/%2u); Coord: (%02u, %02u)\r\n", objeto[j].nombre, (int) objeto[j].val3, (int) objeto[j].val2, (int) objeto[j].x, (int) objeto[j].y );
				else if (objeto[j].val1&4)
					cprintf("Name: %15s; Class: 3; Sell/Buy: (%2u/%2u); Coord: (%02u, %02u)\r\n",  objeto[j].nombre, (int) objeto[j].val3, (int) objeto[j].val2, (int) objeto[j].x, (int) objeto[j].y );
			}
		}
		cputs("------------------------- End of the list -------------------------");
		window(1, 1, 80, 25);
		ckey();
	}
	/* Buy */
	else if (i==1)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
		gotoxy(21, 10);
		cprintf("We want %s $ per Kg.", puntua(objeto[j].val2));
		cputsc(20, 60, 12, "How many Kg you want");
		cputsc(20, 60, 13, "to buy?");
		memset(tmp, 0, 40);
		getstr (30, 15, 16*1+15, tmp, 20);
		kg=atol(tmp);
		if (kg>(player[0].totalcargo-player[0].llevocargo))
		{
			puttext(1, 1, 80, 25, oldb);
			/* No puede comprar */
			ckey();
		}
		else
		{
			puttext(1, 1, 80, 25, oldb);
			player[0].llevocargo+=kg;
			player[0].dinero-=objeto[j].val2*kg;
			if (objeto[j].val1&1)
				player[0].tipocargo=1;
			else if (objeto[j].val1&2)
				player[0].tipocargo=2;
			else if (objeto[j].val1&4)
				player[0].tipocargo=3;
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cputs("You have bought!");
			gotoxy(21, 12);
			cprintf("You have paid %s $", puntua(objeto[j].val2*kg));
			gotoxy(21, 13);
			cprintf("now you have %s $", puntua(player[0].dinero));
			ckey();
		}
	}
	/* Sell */
	else if (i==2)
	{
		puttext(1, 1, 80, 25, oldb);
		box(20, 9, 60, 16, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
		gotoxy(21, 10);
		cprintf("We pay %s $ per Kg.", puntua(objeto[j].val3));
		cputsc(20, 60, 12, "How many Kg you want");
		cputsc(20, 60, 13, "to sell?");
		memset(tmp, 0, 40);
		getstr (30, 15, 16*1+15, tmp, 20);
		kg=atol(tmp);
		if (kg>player[0].llevocargo)
		{
			puttext(1, 1, 80, 25, oldb);
			/* No puede vender */
			ckey();
		}
		else
		{
			puttext(1, 1, 80, 25, oldb);
			player[0].llevocargo-=kg;
			player[0].dinero+=objeto[j].val3*kg;
			if (player[0].llevocargo==0)
				player[0].tipocargo=0;
			box(20, 10, 60, 15, 16*7+15, A_DOUBLE, "Warehouse", SHADOW);
			textattr(16*7+0);
			gotoxy(21, 11);
			cputs("You have sold!");
			gotoxy(21, 12);
			cprintf("You have got %s $", puntua(objeto[j].val3*kg));
			gotoxy(21, 13);
			cprintf("now you have %s $", puntua(player[0].dinero));
			ckey();
		}
	}
	puttext(1, 1, 80, 25, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}