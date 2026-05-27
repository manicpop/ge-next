

/***************************************************************************
 *                                                                         *
 *   GECMDS.C                                                              *
 *                                                                         *
 *   Copyright (C) 1988, 89, 90, 91, 92 Michael B. Murdock                 *
 *                                                                         *
 *   This is the source for the Galactic Empire game module                *
 *                                                                         *
 *                                         M. Murdock     03/17/92         *
 *                                                                         *
 *   ge-next                                                               *
 *                                                                         *
 *   Copyright (C) 2024-2026 Anthony Schmidt, anthony@manicpop.org         *
 *                                                                         *
 *   https://manicpop.org/ge-next/  https://github.com/manicpop/ge-next    *
 *                                                                         *
 ***************************************************************************/

 /**************************************************************************
  * This program is free software; you can redistribute it and/or modify   *
  * under the terms of the GNU General Public License as published by the  *
  * Free Software Foundation; either version 2 of the License, or (at your *
  * option) any later version.                                             *
  *                                                                        *
  * This program is distributed in the hope that it will be useful,        *
  * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
  * General Public License for more details.                               *
  *                                                                        *
  * You should have received a copy of the GNU General Public License      *
  * along with this program; if not, write to the Free Software Foundation,*
  * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA            *
  *************************************************************************/

#ifdef PHARLAP

#include "gcomm.h"
#include "string.h"

#else
#include "stdio.h"
#include "ctype.h"
#include "dos.h"
#include "usracc.h"
#include "btvstf.h"
#include "stdlib.h"
#include "portable.h"
#include "dosface.h"

#endif

#include "math.h"
#include "majorbbs.h"

#include "gemain.h"

#define GECMDS 1

#include "geglobal.h"


/* LOCAL GLOBAL DEFS *****************************************************/


char *kwrd[NUMITEMS] = {"men",
			"mis",
			"tor",
			"ion",
			"flu",
			"foo",
			"fig",
			"dec",
			"tro",
			"zip",
			"jam",
			"min",
			"gol",
			"sp"};

char *item_name[NUMITEMS] = {	"men",
				"missiles",
				"torpedoes",
				"ion cannons",
				"flux pods",
				"food cases",
				"fighters",
				"decoys",
				"troops",
				"zippers",
				"jammers",
				"mines",
				"gold",
				"spies"};

struct upgdef {
	unsigned int	bit;
	int		namemsg;
	int		descmsg;
	int		priceidx;
};

static struct upgdef upgdefs[NUMUPGRADES] = {
	{ENHSCAN, UPGR1, UPGRD1, 0},
	{ARMOR, UPGR2, UPGRD2, 1},
	{ACCELBST, UPGR3, UPGRD3, 2},
	{SCANBST, UPGR4, UPGRD4, 3},
	{ENHLOCK, UPGR5, UPGRD5, 4},
	{DAMCTRL, UPGR6, UPGRD6, 5},
	{TPONDER, UPGR7, UPGRD7, 6},
	{NCORE, UPGR8, UPGRD8, 7}
};

void FUNC cmd_gehelp(void), cmd_cloak(void), cmd_impulse(void), cmd_phas(void),
	cmd_report(void), cmd_rotate(void), cmd_send(void),
	cmd_shields(void), cmd_warp(void), cmd_torp(void), cmd_missl(void),
	cmd_decoy(void), cmd_flux(void), cmd_set(void), cmd_orbit(void),
	cmd_transfer(void), cmd_admin(void), cmd_attack(void), cmd_geroster(void),
	cmd_buy(void), cmd_price(void), cmd_planet(void), cmd_maint(void),
	cmd_new(void), cmd_sell(void), cmd_sysop(void), cmd_rename(void),
	cmd_destruct(void), cmd_abort(void), cmd_jammer(void), cmd_mine(void),
	cmd_abandon(void), cmd_zipper(void), cmd_lock(void), cmd_navigate(void),
	cmd_who(void), cmd_clear(void), cmd_team(void), cmd_spy(void),
	cmd_jettison(void), cmd_stop(void);

#ifdef DATACMD
void FUNC cmd_data(void);
#endif

#define GECMDSIZ (sizeof(gecmds)/sizeof(struct cmd))

/* these must be in order for gesearch() to work */
struct	cmd	gecmds[]={
		/*	command	function	0=payers only */
		/*	-------	----------- 	------------- */
			{"?",	cmd_gehelp,	1},
			{"aba",	cmd_abandon,	0},
			{"abo",	cmd_abort,	1},
			{"adm",	cmd_admin,	0},
			{"att",	cmd_attack,	0},
			{"buy",	cmd_buy,	0},
			{"clo",	cmd_cloak,	1},
			{"cls",	cmd_clear,	0},
#ifdef DATACMD
			{"dat",	cmd_data,	0},
#endif
			{"dec",	cmd_decoy,	1},
			{"des",	cmd_destruct,	1},
			{"flu",	cmd_flux,	1},
			{"hel",	cmd_gehelp,	1},
			{"imp",	cmd_impulse,	1},
			{"jam",	cmd_jammer,	1},
			{"jet",	cmd_jettison,	1},
			{"loc",	cmd_lock,	1},
			{"mai",	cmd_maint,	1},
			{"min",	cmd_mine,	1},
			{"mis",	cmd_missl,	1},
			{"nav",	cmd_navigate,	1},
			{"new",	cmd_new,	1},
			{"orb",	cmd_orbit,	1},
			{"pha",	cmd_phas,	1},
			{"pla",	cmd_planet,	0},
			{"pri",	cmd_price,	0},
			{"ren",	cmd_rename,	0},
			{"rep",	cmd_report,	1},
			{"ros",	cmd_geroster,	1},
			{"rot",	cmd_rotate,	1},
			{"sca",	cmd_scan,	1},
			{"sel", cmd_sell,	1},
			{"sen",	cmd_send,	0},
			{"set",	cmd_set,	1},
			{"shi",	cmd_shields,	1},
			{"spy",	cmd_spy,	1},
			{"sto", cmd_stop,	1},
			{"sys",	cmd_sysop,	1},
			{"tea", cmd_team,	1},
			{"tor",	cmd_torp,	1},
			{"tra",	cmd_transfer,	0},
			{"war",	cmd_warp,	1},
			{"who",	cmd_who,	1},
			{"zip",	cmd_zipper,	1}
};


/* If you wish to add your own help commands to the list simply add the command
	name to the list and the name of the message in the MSG file. Also add the
	command to the list in the message HLPINDEX                Mike           */

struct hlpcmd	{
		char *command;
		int helptxt;
		};

struct hlpcmd gehlp[] = {
		{"abandon",			HLPABA},
		{"abort",			HLPABO},
		{"admin",			HLPADM},
		{"attack",			HLPATT},
		{"buy",				HLPBUY},
		{"cloak",			HLPCLO},
		{"cls",				HLPCLS},
		{"decoy",			HLPDEC},
		{"destruct",			HLPDES},
		{"help",			HLPHEL},
		{"hyper",			HLPHYP},
		{"impulse",			HLPIMP},
		{"jammer",			HLPJAM},
		{"jettison",			HLPJET},
		{"lock",			HLPLOC},
		{"maintenance",			HLPMAI},
		{"mine",			HLPMIN},
		{"missile",			HLPMIS},
		{"navigate",			HLPNAV},
		{"new",				HLPNEW},
		{"newprice",			HLPNEW2},
		{"orbit",			HLPORB},
		{"phaser",			HLPPHA},
		{"planet",			HLPPLA},
		{"price",			HLPPRI},
		{"rename",			HLPREN},
		{"report",			HLPREP},
		{"roster",			HLPROS},
		{"rotate",			HLPROT},
		{"scan",			HLPSCA},
		{"sell",			HLPSEL},
		{"send",			HLPSEN},
		{"set",				HLPSET},
		{"set2",			HLPSET2},
		{"shield",			HLPSHI},
		{"spy",				HLPSPY},
		{"stop",			HLPSTO},
		{"sys",				HLPSYS},
		{"team",			HLPTEA},
		{"torpedo",			HLPTOR},
		{"transfer",			HLPTRA},
		{"warp",			HLPWAR},
		{"who",				HLPWHO},
		{"x",				HLPEXT},
		{"zipper",			HLPZIP},

/* FYI: The above are commands and below are topics....       Mike       */

		{"battle",			HLPBATTL},
		{"battle2",			HLPBATT2},
		{"battle3",			HLPBATT3},
		{"battle4",			HLPBATT4},
		{"class",			HLPCLS1},
		{"cybertrons",			HLPCYBER},
		{"distress",			HLPDIST},
		{"flux",			HLPFLU},
		{"galaxy",			HLPGALXY},
		{"lydorians",			HLPLYDO},
		{"murdonians",			HLPMURD},
		{"moving",			HLPNAVIG},
		{"nebulas",			HLPNEB},
		{"planets",			HLPPLANT},
		{"planets2",			HLPPLAN2},
		{"planets3",			HLPPLAN3},
		{"sartens",			HLPSART},
		{"scoring",			HLPSCORE},
		{"starting",			HLPSTART},
		{"strategy",			HLPSTRAT},
		{"tryklons",			HLPTRYK},
		{"upgrades",			HLPUPG},
		{"vakories",			HLPVAKO},
		{"wormholes",			HLPWORM},
		{"zygorians",			HLPZYGO},

		{NULL,				0}
};

/**************************************************************************
** Search a sorted command table                                         **
**************************************************************************/

struct cmd *FUNC gesearch(char *ptr, struct cmd tab[], int len)
{
	int c;
	struct cmd *lo, *md, *hi;

	lo = &tab[0];
	hi = &tab[len-1];

	while (lo <= hi) {
#pragma warn -sig
		md = lo + ((hi - lo)/2L);
#pragma warn +sig
		if ((c = strncmp(ptr,md->command,3)) < 0)
			hi = md - 1;
		else if (c > 0)
			lo = md + 1;
		else
			return md;
	}
	return NULL;
}

/**************************************************************************
** Parse and dispatch a Galactic Empire command                          **
**************************************************************************/

void FUNC gwar(void)
{
	struct cmd *cmdptr;
	char *mv0ptr;

	if (margc == 0) {
		warnop();
	} else {
		logthis(spr("Input Ch# %d Margc %d", usrnum, margc));

		if (margc > 0) {
			sprintf(gechrbuf, "Margv[0] [%s]", margv[0]);
			logthis(gechrbuf);
		}
		if (margc > 1) {
			sprintf(gechrbuf, "Margv[1] [%s]", margv[1]);
			logthis(gechrbuf);
		}
		if (margc > 2) {
			sprintf(gechrbuf, "Margv[2] [%s]", margv[2]);
			logthis(gechrbuf);
		}
		if (margc > 3) {
			sprintf(gechrbuf, "Margv[3] [%s]", margv[3]);
			logthis(gechrbuf);
		}
		if (margc > 4) {
			sprintf(gechrbuf, "Margv[4] [%s]", margv[4]);
			logthis(gechrbuf);
		}

		for (mv0ptr = margv[0]; *mv0ptr != 0; mv0ptr++) {
			*mv0ptr = (char)tolower((unsigned char)*mv0ptr);
		}
		if (sameas(margv[0], ">")) {
#ifdef PHARLAP
			if (!hasmkey(PLAYKEY))
#else
			if (usrptr->class < PAYING)
#endif
			{
				prfmsg(NOCANDO);
				outprfge(FLT_NONE, usrnum);
			} else {
				cmd_send();
			}
			return;
		}
		if ((cmdptr = gesearch(margv[0], gecmds, GECMDSIZ)) != NULL) {
#ifdef PHARLAP
			if (!hasmkey(PLAYKEY) && cmdptr->cando == 0)
#else
			if (usrptr->class < PAYING && cmdptr->cando == 0)
#endif
			{
				prfmsg(NOCANDO);
				outprfge(FLT_NONE, usrnum);
			} else {
				(*(cmdptr->func))();
			}
		} else {
			prfmsg(INVCMD);
			outprfge(FLT_NONE, usrnum);
		}
	}
}

/**************************************************************************
** Blank line was input response                                         **
**************************************************************************/

void FUNC warnop(void)
{
	prfmsg(FORHELP);
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Display command and topic help                                        **
**************************************************************************/

void FUNC cmd_gehelp(void)
{
	int ndx, i, syshelp;
	char gechrbuf4[12], gechrbuf5[12], gechrbuf6[12], gechrbuf7[12];
	char gechrbuf8[12], gechrbuf9[12];

#ifdef PHARLAP
	if ((!syscmds) || (sysonly && !(hasmkey(SYSKEY))))
#else
	if ((!syscmds) || (sysonly && !(usrptr->flags & ISYSOP)))
#endif
		syshelp = FALSE;
	else
		syshelp = TRUE;

	setmbk(gehlpmb);

	if (margc < 2 || margc > 3) {
		prfmsg(HLPINDEX);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (sameto(margv[1], "version")) {
		prf(VERSION);
		prf("\r\r");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (sameto(margv[1], "sys") && syshelp == FALSE) {
		prfmsg(HLPINDEX);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (sameto(margv[1],"class")) {
		if (margc == 2) {
			prfmsg(HLPCLS1);
			for (i=0; i<cyb_class; ++i) {
				if (shipclass[i].max_type == CLASSTYPE_USER) {
					if (shipclass[i].max_tons >999999L)
						sprintf(gechrbuf,"%ldm",shipclass[i].max_tons/1000000L);
					else if (shipclass[i].max_tons >999)
						sprintf(gechrbuf,"%ldk",shipclass[i].max_tons/1000);
					else
						sprintf(gechrbuf,"%ld",shipclass[i].max_tons);

					if (shipclass[i].max_price >999999L)
						sprintf(gechrbuf2,"%ldm",shipclass[i].max_price/1000000L);
					else if (shipclass[i].max_price >999)
						sprintf(gechrbuf2,"%ldk",shipclass[i].max_price/1000);
					else
						sprintf(gechrbuf2,"%ld",shipclass[i].max_price);

					if (shipclass[i].scanrange >999999L)
						sprintf(gechrbuf3,"%ldm",shipclass[i].scanrange/1000000L);
					else if (shipclass[i].scanrange >999)
						sprintf(gechrbuf3,"%ldk",shipclass[i].scanrange/1000);
					else
						sprintf(gechrbuf3,"%ld",shipclass[i].scanrange);

					if (shipclass[i].max_warp == 0)
						strcpy(gechrbuf4,CLR_RED1 " N");
					else
						sprintf(gechrbuf4,"%s%2d",CLR_WHITE2,shipclass[i].max_warp);
					if (shipclass[i].max_shlds == 0)
						strcpy(gechrbuf5,CLR_RED1 " N");
					else
						sprintf(gechrbuf5,"%s%2d",CLR_WHITE2,shipclass[i].max_shlds);
					if (shipclass[i].max_phasr == 0)
						strcpy(gechrbuf6,CLR_RED1 " N");
					else
						sprintf(gechrbuf6,"%s%2d",CLR_WHITE2,shipclass[i].max_phasr);
					if (shipclass[i].max_torps == 0)
						strcpy(gechrbuf7,CLR_RED1 "N");
					else
						sprintf(gechrbuf7,"%s%1d",CLR_GREEN2,shipclass[i].max_torps);
					if (shipclass[i].max_missl == 0)
						strcpy(gechrbuf8,CLR_RED1 "N");
					else
						sprintf(gechrbuf8,"%s%1d",CLR_GREEN2,shipclass[i].max_missl);
					if (shipclass[i].max_accel >999)
						sprintf(gechrbuf9,"%s%2dk",CLR_WHITE2,shipclass[i].max_accel/1000);
					else
						sprintf(gechrbuf9,"%s%3d",CLR_WHITE2,shipclass[i].max_accel);
					prf("%s%2d %s%-24s %s%5s %s %s %s %s %s %s %s %s %s %s %s %4s %4s %4s %5d\r",
						CLR_CYAN2, i+1,
						CLR_CYAN1, shipclass[i].typename,
						CLR_YELLOW1, gechrbuf2,
						gechrbuf4,
						gechrbuf5,
						gechrbuf6,
						gechrbuf7,
						gechrbuf8,
						shipclass[i].has_mine ? CLR_GREEN2 "Y" : CLR_RED1 "N",
						shipclass[i].max_attk ? CLR_GREEN2 "Y" : CLR_RED1 "N",
						shipclass[i].has_decoy ? CLR_GREEN2 "Y" : CLR_RED1 "N",
						shipclass[i].has_jam ? CLR_GREEN2 "Y" : CLR_RED1 "N",
						shipclass[i].has_zip ? CLR_GREEN2 "Y" : CLR_RED1 "N",
						shipclass[i].max_cloak ? CLR_GREEN2 "Y" : CLR_RED1 "N",
						gechrbuf9,
						gechrbuf3,
						gechrbuf,
						shipclass[i].max_points
					);
				}
			}
			prfmsg(HLPCLS2);
			outprfge(FLT_NONE,usrnum);
			return;
		} else if (margc == 3) {
			i = atoi(margv[2])-1;
			if (i >= 0 && i < cyb_class && shipclass[i].max_type == CLASSTYPE_USER) {
				setmbk(geshmb);
				prfmsg(shipclass[i].hlpmsg);
				outprfge(FLT_NONE,usrnum);
				return;
			} else {
				prfmsg(HLPCLS3);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		} else {
			prfmsg(HLPINDEX);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	}

	ndx = 0;
	while (gehlp[ndx].command != NULL) {
		if (sameto(margv[1], gehlp[ndx].command)) {
			prfmsg(gehlp[ndx].helptxt);
			outprfge(FLT_NONE, usrnum);
			return;
		}
		++ndx;
	}

	prfmsg(HLPINDEX);
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Fire engines on impulse                                               **
**************************************************************************/

void FUNC cmd_impulse(void)
{
	unsigned deg;

	if (margc < 2 || margc > 3) {
		prfmsg(FORMAT, "IMPULSE");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->where == 1) {
		prfmsg(IMPULSE1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDNAVG);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (margc == 3)
		strcpy(gechrbuf, margv[2]);
	else
		strcpy(gechrbuf, "0");
	strcpy(gechrbuf2, gechrbuf);
	if (*gechrbuf == '@') {
		*gechrbuf = '+';
		deg = atoi(gechrbuf);
		if (deg > 359) {
			prfmsg(NUMOOR, 0, 359);
			outprfge(FLT_NONE, usrnum);
			return;
		}
	}

	if (valpcnt(margv[1], 0, 99)) {
		if (warsptr->helm == 0) {
			if (*gechrbuf2 != '@')
				if (!valdegree(gechrbuf))
					return;
			if (warsptr->where >= 10) {
				refresh(warsptr, usrnum);
				lock_sector(warsptr, usrnum, LOCKORB2);
				prfmsg(LEAVEORB);
				outprfge(FLT_SHIP, usrnum);
				warsptr->where = 0;
				warsptr->repair = 0;
			}
			if (*gechrbuf2 != '@') {
				if (warsptr->degrees != 0)
					deg = (unsigned)normal(warsptr->heading + (double)warsptr->degrees);
				else
					deg = warsptr->head2b;
			}
			prfmsg(ENGFIRE, deg);
			outprfge(FLT_NONE, usrnum);
			warsptr->speed2b = 1000.0 * ((double)warsptr->percent / 100.0);
			if (deg != warsptr->head2b)
				warsptr->head2b = (double)deg;
		} else {
			prfmsg(HLBROKE);
			if (warsptr->speed != 0)
				prfmsg(HLBROKE2);
			outprfge(FLT_NONE, usrnum);
		}
	}
}

/**************************************************************************
** Fire engines on warp drive                                            **
**************************************************************************/

void FUNC cmd_warp(void)
{
	unsigned deg;
	int speed, topspeed, cap, classcap, capwarn;

	if (margc < 2 || margc > 3) {
		prfmsg(FORMAT, "WARP");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (shipclass[warsptr->shpclass].max_warp == 0 && atoi(margv[1]) != 0) {
		prfmsg(WARP01);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->topspeed == 0 && atoi(margv[1]) != 0) {
		prfmsg(WARPSPD2);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDNAVG);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	speed = atoi(margv[1]);
	topspeed = warsptr->topspeed;
	cap = topspeed + (topspeed / 2);
	classcap = shipclass[warsptr->shpclass].max_warp +
		(shipclass[warsptr->shpclass].max_warp / 2);
	capwarn = FALSE;
	if (speed < 0) {
		prfmsg(FORMAT, "WARP");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (speed > classcap && speed > warsptr->speed / 1000) {
		prfmsg(WARP03);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	/* if requsted speed is over 150% cruising speed and over current speed */
	if (speed > cap && speed > warsptr->speed/1000) {
		/* if current speed is over 100% cruising speed */
		if (warsptr->speed/1000 > warsptr->topspeed) {
			/* if current speed is over 150% cruising speed, don't change */
			if (warsptr->speed/1000 > cap) {
				prfmsg(WARPSPD3);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			/* if not, change to 150% cruising speed */
			else {
				speed = cap;
				capwarn = TRUE;
				prfmsg(WARPSPD4,speed);
				outprfge(FLT_NONE,usrnum);
			}
		} else {
			prfmsg(WARP03);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	}

	if (margc == 3)
		strcpy(gechrbuf,margv[2]);
	else
		strcpy(gechrbuf,"0");
	strcpy(gechrbuf2,gechrbuf);
	if (*gechrbuf == '@') {
		*gechrbuf = '+';
		deg = atoi(gechrbuf);
		if (deg > 359) {
			prfmsg(NUMOOR,0,359);
			outprfge(FLT_NONE, usrnum);
			return;
		}
	}
	if (warsptr->helm == 0) {
		if (*gechrbuf2 != '@')
			if (!valdegree(gechrbuf))
				return;
		if (speed > topspeed && capwarn == FALSE) {
			if (topspeed < shipclass[warsptr->shpclass].max_warp)
				prfmsg(WARPSPD,topspeed);
			else
				prfmsg(WARP04,topspeed);
			outprfge(FLT_NONE,usrnum);
		}

		if (warsptr->where >= 10) {
			refresh(warsptr,usrnum);
			lock_sector(warsptr,usrnum,LOCKORB2);
			prfmsg(LEAVEORB);
			outprfge(FLT_SHIP,usrnum);
			warsptr->where = 0;
			warsptr->repair = 0;
		}

		if (*gechrbuf2 != '@') {
			if (warsptr->degrees != 0)
				deg = (unsigned)normal(warsptr->heading + (double)warsptr->degrees);
			else
				deg = warsptr->head2b;
		}
		prfmsg(ENGFIRE,deg);
		outprfge(FLT_NONE,usrnum);
		if (ship_accel(warsptr) >= 1000.0 && warsptr->speed < 1000.0 && speed != 0)
			warsptr->speed = 0.0;	/* no fractional warp speeds */
		warsptr->speed2b = 1000.0 * (float)speed;
		if (deg != warsptr->head2b)
			warsptr->head2b = (double)deg;
		} else {
			prfmsg(HLBROKE);
			if (warsptr->speed != 0)
				prfmsg(HLBROKE2);
			outprfge(FLT_NONE,usrnum);
		}
	}

/**************************************************************************
** Emergency stop                                                        **
**************************************************************************/

void FUNC cmd_stop(void)
{
	if (warsptr->helm < 0 && warsptr->speed > 0)
		prfmsg(EMERSTOP);
	else
		prfmsg(ENGFIRE, (unsigned)warsptr->head2b);

	outprfge(FLT_NONE, usrnum);
	warsptr->speed2b = 0;
}

/**************************************************************************
** Rotate ship                                                           **
**************************************************************************/

void FUNC cmd_rotate(void)
{
	unsigned deg;

	if (margc != 2) {
		prfmsg(FORMAT, "ROTATE");
		outprfge(FLT_NONE, usrnum);
	} else if (*margv[1] == '@') { /* turn absolute */
		if (warsptr->helm == 0) {
			*margv[1] = '+';
			deg = atoi(margv[1]);
			if (deg < 360) {
				if (deg == warsptr->heading) {
					prfmsg(NOWALRD, deg);
				} else {
					prfmsg(NOWTURN, deg);
					warsptr->head2b = (double)deg;
				}
				outprfge(FLT_NONE, usrnum);
			} else {
				prfmsg(NUMOOR, 0, 359);
				outprfge(FLT_NONE, usrnum);
				return;
			}
		} else {
			prfmsg(HLBROKE);
			if (warsptr->speed != 0)
				prfmsg(HLBROKE2);
			outprfge(FLT_NONE, usrnum);
		}
		} else if (valdegree(margv[1])) {
			if (warsptr->helm == 0) {
				if (warsptr->degrees == 0) {
				prfmsg(NOWALRD, (int)(warsptr->heading + .5));
			} else {
				deg = (unsigned)normal(warsptr->heading + (double)warsptr->degrees);
				prfmsg(NOWTURN, deg);
				warsptr->head2b = (double)deg;
			}
			outprfge(FLT_NONE, usrnum);
		} else {
			prfmsg(HLBROKE);
			if (warsptr->speed != 0)
				prfmsg(HLBROKE2);
				outprfge(FLT_NONE, usrnum);
			}
	}
}

/**************************************************************************
** Recharge engines                                                      **
**************************************************************************/

void FUNC cmd_flux(void)
{
	fluxstat(warsptr,usrnum,65535U);
}

/**************************************************************************
** Orbit a planet                                                        **
**************************************************************************/

void FUNC cmd_orbit(void)
{
	int got_plt;
	byte nebmask;

	if (margc != 2) {
		prfmsg(FORMAT,"ORBIT");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where == 1) {
		prfmsg(ORBIT4);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where >= 10) {
		prfmsg(ORBIT3);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	nebmask = (byte)innebula(coord1(warsptr->coord.xcoord),coord1(warsptr->coord.ycoord));
	plnum = (atoi(margv[1]));

	if (plnum <= MAXPLANETS && plnum > 0) {
		got_plt = getplanetdat(usrnum);
		if (nebmask) {
			if (!got_plt) {
				prfmsg(SCAN26);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (cdistance(&warsptr->coord,&plptr->coord)*10000.0 > (double)NEBRNG) {
				prfmsg(SCAN26);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}
		if (got_plt) {
			if (plptr->type == PLTYPE_WORM) {
				prfmsg(ORBIT0);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			distance = (unsigned)(cdistance(&warsptr->coord,&plptr->coord)*10000);
			if (distance <= 250) {
				lock_sector(warsptr,usrnum,LOCKORB1);
				if (strlen(plptr->name) == 0) {
					prfmsg(ORBIT1N,plnum);
				} else {
					prfmsg(ORBIT1,plnum,plptr->name);
				}
				warsptr->where = 10 + plnum;
				warsptr->speed = 0;
				warsptr->speed2b = 0;
			} else {
				prfmsg(ORBIT2);
			}
		} else {
			prfmsg(NOPLNT);
		}
	} else {
		prfmsg(NOPLNT);
	}
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Fire phasers                                                          **
**************************************************************************/

void FUNC cmd_phas(void)
{
	if (shipclass[warsptr->shpclass].max_phasr == 0) {
		prfmsg(PHASER0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->phasr < 0) {
		prfmsg(PHBROKE);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where == 1) {
		if (margc == 2) {
			if (valdegree(margv[1])) {
				if (warsptr->hypha == 0) {
					firehp(warsptr,usrnum);
				} else {
					prfmsg(HPWAIT);
					outprfge(FLT_NONE,usrnum);
				}
			}
		} else {
			prfmsg(FORMAT,"HYPER");
			outprfge(FLT_NONE,usrnum);
		}
	} else {
		if (margc == 2) {
			if (warsptr->phasr >= PMINFIRE) {
				if (valdegree(margv[1])) {
					warsptr->percent = 1;
					firep(warsptr,usrnum);
				}
			} else {
				prfmsg(PHANONE);
				outprfge(FLT_NONE,usrnum);
			}
		} else if (margc == 3) {
			if (warsptr->phasr >= PMINFIRE) {
				if (valpcnt(margv[2],0,5) && valdegree(margv[1])) {
					firep(warsptr,usrnum);
				}
			} else {
				prfmsg(PHANONE);
				outprfge(FLT_NONE,usrnum);
			}
		} else {
			prfmsg(FORMAT,"PHASER");
			outprfge(FLT_NONE,usrnum);
		}
	}
}

/**************************************************************************
** Fire torpedoes                                                        **
**************************************************************************/

void FUNC cmd_torp(void)
{
	int shpnum;
	WARSHP *wptr;
	byte nebmask;

	if (shipclass[warsptr->shpclass].max_torps == 0) {
		prfmsg(TORP3);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where == 1) {
		prfmsg(TORP2);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->cloak > 0 ) {
		prfmsg(PCLOKUP,"The torpedo launcher is");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_TORPEDO] == 0) {
		prfmsg(NOTORPS);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc < 2) {
		prfmsg(FORMAT,"TORPEDO");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	nebmask = (byte)innebula(coord1(warsptr->coord.xcoord),coord1(warsptr->coord.ycoord));

	shpnum = findshp(margv[1],1);

	if (shpnum < 0 && margv[1][0] == '@') {
		if (warsptr->lock >= 0 && warsptr->lock < nships && ingegame(warsptr->lock))
			prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrnum);
	} else if (shpnum == usrnum) {
		if (margv[1][0] == '@') {
			warsptr->lock = -1;
			warsptr->track_grace = 0;
			prfmsg(NOLOCK);
		} else {
			update_scantab(warsptr,usrnum);
			prfmsg(NOSHIP);
		}
		outprfge(FLT_NONE,usrnum);
	} else if (shpnum >= 0) {
		wptr = warshpoff(shpnum);
		if (!isvisible(warsptr,wptr)) {
			if (nebmask)
				prfmsg(SCAN27);
			else
				prfmsg(NOSHIP);
			outprfge(FLT_NONE,usrnum);
		} else {
			if (neutral(&warsptr->coord)) {
				zaphim(warsptr,usrnum);
				prfmsg(FRCTER);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (warsptr->shieldstat == SHIELDUP)
				shielddn(warsptr,usrnum);
			torp(warsptr,usrnum,shpnum);
		}
	} else {
		if (nebmask)
			prfmsg(SCAN27);
		else
			prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrnum);
	}

	if (warsptr->shieldstat == SHIELDUP) {
		shieldup(warsptr,usrnum);
	}
}

/**************************************************************************
** Fire missile                                                          **
**************************************************************************/

void FUNC cmd_missl(void)
{
	int shpnum;
	WARSHP *wptr;
	unsigned energy;
	byte nebmask;

	if (shipclass[warsptr->shpclass].max_missl == 0) {
		prfmsg(MISS01);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->cloak > 0) {
		prfmsg(PCLOKUP,"The missile launcher is");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_MISSILE] == 0) {
		prfmsg(NOMISSL);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc < 2) {
		prfmsg(FORMAT,"MISSILE");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 2)
		energy = 5000;
	else {
		if (atol(margv[2]) > 20000L)
			energy = 20000;
		else if (atol(margv[2]) < 2000L)
			energy = 2000;
		else
			energy = (unsigned)(atol(margv[2]));
	}

	nebmask = (byte)innebula(coord1(warsptr->coord.xcoord),coord1(warsptr->coord.ycoord));

	shpnum = findshp(margv[1],1);

	if (shpnum < 0 && margv[1][0] == '@') {
		if (warsptr->lock >= 0 && warsptr->lock < nships && ingegame(warsptr->lock))
			prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrnum);
	} else if (shpnum == usrnum) {
		if (margv[1][0] == '@') {
			warsptr->lock = -1;
			warsptr->track_grace = 0;
			prfmsg(NOLOCK);
		} else {
			update_scantab(warsptr,usrnum);
			prfmsg(NOSHIP);
		}
		outprfge(FLT_NONE,usrnum);
	} else if (shpnum >= 0) {
		wptr = warshpoff(shpnum);
		if (!isvisible(warsptr,wptr)) {
			if (nebmask)
				prfmsg(SCAN27);
			else
				prfmsg(NOSHIP);
			outprfge(FLT_NONE,usrnum);
		} else {
			if (neutral(&warsptr->coord)) {
				zaphim(warsptr,usrnum);
				prfmsg(FRCTER);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (warsptr->shieldstat == SHIELDUP)
				shielddn(warsptr,usrnum);
			misl(warsptr,usrnum,shpnum,energy,energy);
		}
	} else {
		if (nebmask)
			prfmsg(SCAN27);
		else
			prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrnum);
	}

	if (warsptr->shieldstat == SHIELDUP) {
		shieldup(warsptr,usrnum);
	}
}

/**************************************************************************
** Find a ship by letter or current lock                                 **
**************************************************************************/

int FUNC findshp(char *ptr, int type) /* 0 = this sector only, 1 = everywhere */
{
	char letter;
	int shpnum, i;
	double dist;
	WARSHP *wptr;

	shpnum = -1;
	if (ptr[0] == '@') {
		/* resolve current lock target, clearing stale locks as needed */
		if (warsptr->lock < 0 || warsptr->lock >= nships) {
			warsptr->lock = -1;
			warsptr->track_grace = 0;
			prfmsg(NOLOCK);
			return -1;
		}
		else {
			shpnum = warsptr->lock;
			if (!ingegame(shpnum)) {
				warsptr->lock = -1;
				warsptr->track_grace = 0;
				prfmsg(NOLOCK);
				return -1;
			}
			wptr=warshpoff(shpnum);
			dist = cdistance(&warsptr->coord,&(wptr->coord));
			if ((dist*10000) > (double)ship_scanrange(warsptr)) {
				warsptr->lock = -1;
				warsptr->track_grace = 0;
				prfmsg(NOLOCK);
				shpnum = -1;
			} else {
				shpnum = warsptr->lock;
			}
		}
	} else {
		/* otherwise resolve the current scan-table letter mapping */
		letter = (char)toupper((unsigned char)*ptr);
		update_scantab(warsptr,usrnum);
		for (i=0; i<NOSCANTAB; ++i) {
			if (scantab[usrnum].ship[i].flag && scantab[usrnum].ship[i].letter == letter) {
				shpnum = scantab[usrnum].ship[i].shipno;
				break;
			}
		}

		if (i >= NOSCANTAB)
			return -1;
	}

	if (!ingegame(shpnum)) {
		return -1;
	}

	wptr=warshpoff(shpnum);

	if (type == 0) {
		if (samesect(&(wptr->coord), &warsptr->coord)
			&& wptr->cloak < 10) {
			return shpnum;
		} else {
			return -1;
		}
	} else if (type == 1) {
		if (wptr->cloak < 10) {
			return shpnum;
		} else {
			return -1;
		}
	}
	return -1;
}

/**************************************************************************
** firing in sector NEUTRAL is a big no-no                               **
**************************************************************************/

void FUNC zaphim(WARSHP *ptr, int usrn)
{
	damstr(se100dam);
	prfmsg(ZAPHIM1, gechrbuf);
	outprfge(FLT_NONE, usrn);
	ptr->damage += se100dam;
}

/**************************************************************************
** Fire decoys                                                           **
**************************************************************************/

void FUNC cmd_decoy(void)
{
	int i;

	if (!shipclass[warsptr->shpclass].has_decoy) {
		prfmsg(DECOY0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where == 1) {
		prfmsg(DECOY1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->cloak > 0) {
		prfmsg(PCLOKUP,"The decoy launcher is");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDDECY);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_DECOYS] == 0) {
		prfmsg(NODECOYS);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->decload < 0) {
		prfmsg(DEBROKE);
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (warsptr->decload > 0) {
		prfmsg(DECOYREL);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	for (i=0; i<10;++i) {
		if (warsptr->decout[i] == 0) {
			--warsptr->items[I_DECOYS];
			warsptr->decout[i] = DECOYTIME;
			warsptr->decload = 1;
			prfmsg(DECFIRE);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	}
	prfmsg(DECMANY);
	outprfge(FLT_NONE,usrnum);
}


/**************************************************************************
** Launch Jammer                                                         **
**************************************************************************/

void FUNC cmd_jammer(void)
{
	if (warsptr->cloak > 0 ) {
		prfmsg(PCLOKUP,"The jammer launcher is");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!shipclass[warsptr->shpclass].has_jam) {
		prfmsg(JAMMER0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_JAMMERS] == 0) {
		prfmsg(JAMMER1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDJAMR);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->jamload > 0 ) {
		prfmsg(JAMMER6);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->jamload < 0) {
		prfmsg(JMBROKE);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	prfmsg(JAMMER2);
	outprfge(FLT_NONE,usrnum);
	jam(warsptr,usrnum);
}

void FUNC jam(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn, sev, scaled;
	double ddist;

	for (zothusn=0; zothusn < nships ; zothusn++) {
		wptr=warshpoff(zothusn);
		if (!ingegame(zothusn))
			continue;

		ddist = cdistance(&ptr->coord,&wptr->coord);
		ddist *= 10000;

		if (ddist > 50000.0)
			continue;

		if (wptr->upgrade & ENHSCAN)
			sev = 10 - (int)(ddist / 3000.0);
		else
			sev = 10 - (int)(ddist / 5000.0);
		if (sev < 1)
			sev = 1;
		if (sev > 10)
			sev = 10;

		if (wptr->jam_sev < (byte)sev) {
			/* closer jammer, raise severity and reset timer */
			wptr->jam_sev  = (byte)sev;
			wptr->jam_time = (byte)jamtime;
		}
		else {
			/* farther/equal jammer, small refresh */
			if (wptr->jam_sev > 0) {
				scaled = (jamtime * sev + wptr->jam_sev/2) / wptr->jam_sev;
				if (scaled < 1)
					scaled = 1;
				if (wptr->jam_time < scaled)
					wptr->jam_time = (byte)scaled;
			}
		}

		if (wptr->jam_time > 0 && usrn != zothusn) {
			prfmsg(JAMMER3);
			outprfge(FLT_NONE, zothusn);
		}
	}

	--ptr->items[I_JAMMERS];
	ptr->cantexit = FIRETICKS;
	ptr->jamload = 1;
}

/**************************************************************************
** Launch Zipper                                                         **
**************************************************************************/

void FUNC cmd_zipper(void)
{
	if (!shipclass[warsptr->shpclass].has_zip) {
		prfmsg(ZIPPER0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->cloak > 0 ) {
		prfmsg(PCLOKUP,"The zipper launcher is");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDZIPR);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_ZIPPERS] == 0) {
		prfmsg(ZIPPER1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->zipload > 0 ) {
		prfmsg(ZIPPER4);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->zipload < 0) {
		prfmsg(ZPBROKE);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	prfmsg(ZIPPER2);
	outprfge(FLT_NONE,usrnum);
	zip(warsptr);
}

void FUNC zip(WARSHP *ptr)
{
	MINE *mptr;
	int i;
	double ddist;

	for (i=0,mptr = mines; i<nummines;++mptr,++i) {
		if (mptr->channel != 255) {
			ddist = cdistance(&ptr->coord,&mptr->coord);
			ddist *= 10000;
			if (ddist < (double)ship_scanrange(ptr)) {
				mptr->timer = 1; /* set mine to explode next tick */
			}
		}
	}
	if (ptr->status == GESTAT_AUTO)
		prfmsg(ZIPPER3N,ptr->shipname);
	else if (ptr->shipname[0] == 0)
		prfmsg(ZIPPER3O,ptr->userid);
	else
		prfmsg(ZIPPER3,ptr->shipname);
	outrange(FLT_NONE,&ptr->coord);
	--ptr->items[I_ZIPPERS];
	ptr->cantexit = FIRETICKS;
	ptr->zipload = 1;
}

/**************************************************************************
** Launch Mine                                                           **
**************************************************************************/

void FUNC cmd_mine(void)
{
	int i;
	int mres;

	if (!shipclass[warsptr->shpclass].has_mine) {
		prfmsg(MINE0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->mineload < 0) {
		prfmsg(MNBROKE);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (neutral(&warsptr->coord)) {
		prfmsg(MINE7);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->cloak > 0 ) {
		prfmsg(PCLOKUP,"The mine launcher is");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDMINE);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_MINE] <= 0) {
		warsptr->items[I_MINE] = 0;
		prfmsg(MINE1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc != 2 ) {
		prfmsg(FORMAT,"MINE");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	i = atoi(margv[1]);

	if (i < 1 || i > 50) {
		prfmsg(FORMAT,"MINE");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	mres = laymine(warsptr,usrnum,i);
	if (mres == 1) {
		prfmsg(MINE3,i,i*TICKTIME);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (mres == 2)
		prfmsg(MINE8);
	else if (mres == 3)
		prfmsg(MINE9);
	else
		prfmsg(MINE2,usermines);
	outprfge(FLT_NONE,usrnum);
}


int FUNC laymine(WARSHP *ptr, int usrn, int timer)
{
	int i, cnt, slot;

	/* count up the number of mines this player has layed */
	cnt = 0;
	slot = -1;

	for (i=0; i<nummines;++i) {
		if (mines[i].channel == (byte)usrn)
			++cnt;
		else if (slot < 0 && mines[i].channel == 255)
			slot = i;
	}

	if (cnt >= usermines) {
		return 0;
	}

	if (slot < 0) {
		return 2;
	}

	if (ptr->mineload > 0) {
		return 3;
	}

	ptr->cantexit = FIRETICKS;
	mines[slot].channel = (byte)usrn;
	mines[slot].timer = timer;
	mines[slot].coord.xcoord = ptr->coord.xcoord;
	mines[slot].coord.ycoord = ptr->coord.ycoord;
	--ptr->items[I_MINE];
	ptr->mineload = 1;
	return 1;
}

/**************************************************************************
** Send a message                                                        **
**************************************************************************/

void FUNC cmd_send(void)
{
	int i;
	int validteam = FALSE;
	long val;
	char *msgptr;

	if (sameas(margv[0],">") && margc < 2) {
		prfmsg(FORMAT,"SEND");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!sameas(margv[0],">") && margc >= 2 && sameto(margv[1],"freq")) {
		if (margc == 2) {
			if (warsptr->freq == 0)
				prfmsg(MSGSNT4N);
			else
				prfmsg(MSGSNT4,warsptr->freq);
		} else if (margc == 3) {
			val = atol(margv[2]);
			if (val >= 0L && val <= 65535L) {
				warsptr->freq = (unsigned)val;
				if (warsptr->freq == 0)
					prfmsg(MSGSNT4N);
				else
					prfmsg(MSGSNT4,warsptr->freq);
			} else
				prfmsg(FORMAT,"SEND");
		} else
			prfmsg(FORMAT,"SEND");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if ((sameas(margv[0],">") && margc > 1) || margc > 2) {
		if (sameas(margv[0],">") || sameto(margv[1],"all")) {
			if (pfnlvl >= 2 && profon) {
				prfmsg(MSGPRF);
			} else {
				if (sameas(margv[0],">"))
					msgptr = margv[1];
				else
					msgptr = margv[2];
				rstrin();
				if (warsptr->shipname[0] == 0)
					prfmsg(MSGSNT1O,"Hailing",waruptr->userid,msgptr);
				else
					prfmsg(MSGSNT1,"Hailing",waruptr->userid,warsptr->shipname,msgptr);
				outwar(FLT_NONE,usrnum,0,0);
				prfmsg(MSGSNT2,"Hailing");
			}
		} else if (sameto(margv[1],"team")) {
			if (pfnlvl >= 2 && profon) {
				prfmsg(MSGPRF);
			} else {
				if (waruptr->teamcode > 0) {
					for (i=0;i<MAXTEAMS;++i) {
						if (teamtab[i].teamcode == waruptr->teamcode && teamtab[i].teamname[0] != '@') {
							validteam = TRUE;
							break;
						}
					}
				}
				if (!validteam) {
					if (waruptr->teamcode > 0) {
						waruptr->teamcode = 0;
						geudb(GEUPDATE,waruptr->userid,waruptr);
					}
					prfmsg(TEAMNOT);
				} else {
					rstrin();
					if (warsptr->shipname[0] == 0)
						prfmsg(MSGSNT1O,"Team",waruptr->userid,margv[2]);
					else
						prfmsg(MSGSNT1,"Team",waruptr->userid,warsptr->shipname,margv[2]);
					outwar(FLT_NONE,usrnum,waruptr->teamcode,2);
					prfmsg(MSGSNT2,"Team");
				}
			}
		} else if (sameto(margv[1],"encoded")) {
			if (pfnlvl >= 2 && profon) {
				prfmsg(MSGPRF);
			} else {
				if (warsptr->freq == 0)
					prfmsg(MSGSNT4N);
				else {
					rstrin();
					if (warsptr->shipname[0] == 0)
						prfmsg(MSGSNT1O,"Encoded",waruptr->userid,margv[2]);
					else
						prfmsg(MSGSNT1,"Encoded",waruptr->userid,warsptr->shipname,margv[2]);
					outwar(FLT_NONE,usrnum,warsptr->freq,1);
					prfmsg(MSGSNT3,warsptr->freq);
				}
			}
		} else {
			prfmsg(FORMAT,"SEND");
		}
	} else
		prfmsg(FORMAT,"SEND");
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Generate ships report                                                 **
**************************************************************************/

void FUNC cmd_report(void)
{
	WARSHP *ptr;
	char *tname;
	int max, pcnt, i, none, zothusn;
	double ddist;

	if (margc != 2 || (!sameas(margv[1],"nav") && !sameas(margv[1],"sys") &&
		!sameas(margv[1],"inv") && !sameas(margv[1],"acc") &&
		!sameas(margv[1],"ord") && !sameas(margv[1],"upg"))) {
		prfmsg(FORMAT,"REPORT");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	energy = (unsigned)warsptr->energy + .5;
	damage = (unsigned)warsptr->damage + .5;
	speed = ((unsigned)warsptr->speed + .5);
	heading = (int)(warsptr->heading + .5);

	if (warsptr->shipname[0] == 0)
		prfmsg(REP01O,shipclass[warsptr->shpclass].typename,showupg(warsptr),waruptr->userid);
	else
		prfmsg(REP01,shipclass[warsptr->shpclass].typename,showupg(warsptr),warsptr->shipname);
	prfmsg(DASHES);

	if (sameas(margv[1],"nav")) {
		prfmsg(REP35);
		if (warsptr->where == 1) {
			setsect(warsptr);
			if (innebula(xsect,ysect))
				prfmsg(REP02N,xsect, ysect);
			else
				prfmsg(REP02,xsect, ysect);
			prfmsg(REP03, showarp(warsptr->speed));
			prfmsg(REP04, heading);
		} else if (warsptr->where == 0) {
			setsect(warsptr);
			if (innebula(xsect,ysect))
				prfmsg(REP05N,xsect,ysect);
			else
				prfmsg(REP05,xsect,ysect);
			prfmsg(REP06, showarp(warsptr->speed));
			prfmsg(REP07, heading);
		} else if (warsptr->where >= 10) {
			setsect(warsptr);
			if (innebula(xsect,ysect))
				prfmsg(REP08N,warsptr->where - 10,xsect,ysect);
			else
				prfmsg(REP08,warsptr->where - 10,xsect,ysect);
		}

		setsect(warsptr);

		prfmsg(REP34,xsect,ysect,xcord,ycord);
		if (warsptr->status == GESTAT_AUTO)
			prfmsg(REP40,warsptr->status,warsptr->cybmine,warsptr->cybupdate);
	} else if (sameas(margv[1],"sys")) {
		prfmsg(REP36);
		prfmsg(REP09,energy);
		if (shipclass[warsptr->shpclass].max_shlds != 0) {
			if (warsptr->shieldstat == SHIELDUP) {
				if (warsptr->shieldtype < 10)
					prfmsg(REP10,warsptr->shieldtype);
				else
					prfmsg(REP10S,warsptr->shieldtype);
				charge(warsptr,&max,&pcnt);
				prfmsg(REP11B,pcnt);
			} else if (warsptr->shieldstat == SHIELDDN || warsptr->shieldstat == SHIELDDM) {
				if (warsptr->shieldtype < 10)
					prfmsg(REP11,warsptr->shieldtype);
				else
					prfmsg(REP11S,warsptr->shieldtype);
			}
		}
		if (shipclass[warsptr->shpclass].max_phasr != 0) {
			if (warsptr->phasr >= PMINFIRE) {
				if (warsptr->phasrtype < 10)
					prfmsg(REP23,warsptr->phasrtype);
				else
					prfmsg(REP23S,warsptr->phasrtype);
			} else if (warsptr->phasr >= 0) {
				if (warsptr->phasrtype < 10)
					prfmsg(REP29,warsptr->phasrtype);
				else
					prfmsg(REP29S,warsptr->phasrtype);
			} else {
				if (warsptr->phasrtype < 10)
					prfmsg(REP24,warsptr->phasrtype);
				else
					prfmsg(REP24S,warsptr->phasrtype);
			}
		}

		if (shipclass[warsptr->shpclass].max_cloak == 1) {
			if (warsptr->cloak > 0 && warsptr->cloak != 3)
				prfmsg(REP12);
			else
				prfmsg(REP13);
		}

		damstr(damage);

		prfmsg(REP14,gechrbuf);
		show_rep_sysdam(warsptr);

		if (shipclass[warsptr->shpclass].max_warp != 0) {
			if (warsptr->topspeed == 0)
				prfmsg(REP19);
			else if (warsptr->speed/1000 > warsptr->topspeed)
				prfmsg(REP20A);
			else if (warsptr->overspeed > 0)
				prfmsg(REP20,warsptr->topspeed);
		}

		if (warsptr->repair > 0)
			prfmsg(REP18A,repair_eta(warsptr));

	} else if (sameas(margv[1],"inv")) {
		prfmsg(REP38);

		for (i=0; i<NUMITEMS; ++i) {
			if (warsptr->items[i] > 0) {
				sprintf(gechrbuf,"%s%s%16lu",item_name[i],gedots(22-strlen(item_name[i])),warsptr->items[i]);
				gechrbuf[0] = (char)toupper((unsigned char)gechrbuf[0]);
				prf("%s\r",gechrbuf);
			}
		}

		sprintf(gechrbuf2,"%lu",calcweight(warsptr));
		prfmsg(REP37,gechrbuf2);
	} else if (sameas(margv[1],"acc")) {
		prfmsg(REP25);

		if (waruptr->planets == 0)
			prfmsg(REP26);
		else
			prfmsg(REP27,waruptr->planets);

		sprintf(gechrbuf,"%lu",waruptr->cash);
		prfmsg(REP28,gechrbuf);

		if (waruptr->score <= 0)
			sprintf(gechrbuf,"0");
		else
			sprintf(gechrbuf,"%lu",waruptr->score);

		prfmsg(REP30,gechrbuf);

		prfmsg(REP31,waruptr->kills,waruptr->ukills);

		prfmsg(REP32,warsptr->kills,warsptr->ukills);

		if (waruptr->teamcode > 0) {
			tname = teamname(waruptr);
			if (tname != NULL)
				prfmsg(REP33,tname);
		}
	} else if (sameas(margv[1],"ord")) {
		prfmsg(REP41);
		none = TRUE;
		prfmsg(REP42);
		for (i=0; i<MAXTORPS; ++i) {
			if (warsptr->ltorps[i].distance != 0) {
				none = FALSE;
				if (warsptr->ltorps[i].channel < nships) {
					ptr = warshpoff(warsptr->ltorps[i].channel);
					prf("\r ");
					if (warsptr->lock == warsptr->ltorps[i].channel)
						prf(" %s*%s%s%s*%s  ",CLR_RED1,CLR_BLUE2,username(ptr),CLR_RED1,CLR_WHITE2);
					else
						prf("  %s%s%s   ",CLR_BLUE2,username(ptr),CLR_WHITE2);
				}
				if (warsptr->ltorps[i].channel == 255)
					prf("\r  (destroyed)   ");
				if (warsptr->jam_sev <= (byte)2)
					prf("Dist: %u",warsptr->ltorps[i].distance);
				else
					prf("Dist: ?????");
			}
		}
		if (none == TRUE)
			prf("none\r");
		else
			prf("\r");
		none = TRUE;
		prfmsg(REP43);
		for (i=0; i<MAXMISSL; ++i) {
			if (warsptr->lmissl[i].distance != 0) {
				none = FALSE;
				if (warsptr->lmissl[i].channel < nships) {
					ptr = warshpoff(warsptr->lmissl[i].channel);
					prf("\r ");
					if (warsptr->lock == warsptr->lmissl[i].channel)
						prf(" %s*%s%s%s*%s  ",CLR_RED1,CLR_BLUE2,username(ptr),CLR_RED1,CLR_WHITE2);
					else
						prf("  %s%s%s   ",CLR_BLUE2,username(ptr),CLR_WHITE2);
				}
				if (warsptr->lmissl[i].channel == 255)
					prf("\r  (destroyed)   ");
				if (warsptr->jam_sev <= (byte)2)
					prf("Dist: %u",warsptr->lmissl[i].distance);
				else
					prf("Dist: ?????");
			}
		}
		if (none == TRUE)
			prf("none\r");
		else
			prf("\r");
		if (shipclass[warsptr->shpclass].max_torps != 0) {
			none = TRUE;
			prfmsg(REP44);
			for (zothusn = 0; zothusn < nships; zothusn++) {
				if (ingegame(zothusn)) {
					ptr = warshpoff(zothusn);
					for (i=0; i<MAXTORPS; ++i) {
						if (ptr->ltorps[i].distance != 0 && ptr->ltorps[i].channel == usrnum) {
							none = FALSE;
							prf("\r ");
							if (warsptr->lock == zothusn)
								prf(" %s*%s%s%s*%s  ",CLR_RED1,CLR_BLUE2,username(ptr),CLR_RED1,CLR_WHITE2);
							else
								prf("  %s%s%s   ",CLR_BLUE2,username(ptr),CLR_WHITE2);
							if (warsptr->jam_sev <= (byte)2)
								prf("Dist: %u",ptr->ltorps[i].distance);
							else
								prf("Dist: ?????");
						}
					}
				}
			}
			if (none == TRUE)
				prf("none\r");
			else
				prf("\r");
		}
		if (shipclass[warsptr->shpclass].max_missl != 0) {
			none = TRUE;
			prfmsg(REP45);
			for (zothusn = 0; zothusn < nships; zothusn++) {
				if (ingegame(zothusn)) {
					ptr = warshpoff(zothusn);
					for (i=0; i<MAXMISSL; ++i)
						if (ptr->lmissl[i].distance != 0 && ptr->lmissl[i].channel == usrnum) {
							none = FALSE;
							prf("\r ");
							if (warsptr->lock == zothusn)
								prf(" %s*%s%s%s*%s  ",CLR_RED1,CLR_BLUE2,username(ptr),CLR_RED1,CLR_WHITE2);
							else
								prf("  %s%s%s   ",CLR_BLUE2,username(ptr),CLR_WHITE2);
							if (warsptr->jam_sev <= (byte)2)
								prf("Dist: %u",ptr->lmissl[i].distance);
							else
								prf("Dist: ?????");

						}
				}
			}
			if (none == TRUE)
				prf("none\r");
			else
				prf("\r");
		}
		if (shipclass[warsptr->shpclass].has_mine != 0) {
			none = TRUE;
			prfmsg(REP46);
			for (i=0; i<nummines; ++i)
				if (mines[i].channel == (byte)usrnum) {
					none = FALSE;
					prf("\r ");
					ddist = cdistance(&warsptr->coord,&mines[i].coord);
					ddist *= 10000;
					bearing = cbearing(&warsptr->coord,&mines[i].coord,warsptr->heading);
					if (warsptr->jam_sev <= (byte)2)
						prf("%d %d  T:%2d  Br:%4d  Dist: %s",
							(int)mines[i].coord.xcoord,(int)mines[i].coord.ycoord,mines[i].timer,bearing,spr("%ld",(long)ddist));
					else
						prf("%d %d  T:%2d  Br:????  Dist: ?????",
							(int)mines[i].coord.xcoord,(int)mines[i].coord.ycoord,mines[i].timer);
				}
			if (none == TRUE)
				prf("none\r");
			else
				prf("\r");
		}
		if (shipclass[warsptr->shpclass].has_decoy != 0) {
			none = 0;
			for (i=0; i<10; ++i)
				if (warsptr->decout[i] != 0)
					++none;
			prfmsg(REP47);
			if (none == 0)
				prf("none\r");
			else
				prf("%d\r",none);
		}
	} else if (sameas(margv[1],"upg")) {
		none = TRUE;
		for (i = 0; i < NUMUPGRADES; ++i)
			if (warsptr->upgrade & upgdefs[i].bit) {
				if (none == TRUE)
					prfmsg(REP48);
				prfmsg(upgdefs[i].namemsg);
				if (upgdefs[i].bit == TPONDER) {
					if (warsptr->tponder == TPONHIGH)
						prf(" (HIGH)");
					else if (warsptr->tponder == TPONLOW)
						prf(" (LOW)");
					else
						prf(" (NORMAL)");
				}
				prf("\r");
				none = FALSE;
			}
		if (none == TRUE)
			prfmsg(REP49);
	}
	prfmsg(DASHES);
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Scan Command                                                          **
**************************************************************************/

/**************************************************************************
** Format a repair ETA string for REPORT SYS                             **
**************************************************************************/

static char *repdmg_eta(WARSHP *ptr, int steps, int active, int fuzz)
{
	int secs;

	if (ptr->repair > 0 || !(ptr->upgrade & DAMCTRL) || steps <= 0)
		return "";

	if (!active) {
		sprintf(gechrbuf3, "(ETA: TBD)");
		return gechrbuf3;
	}

	secs = ((steps + 1) / 2) * TICKTIME;
	secs += fuzz;
	if (secs < TICKTIME)
		secs = TICKTIME;
	sprintf(gechrbuf3, "(ETA: %d seconds)", secs);
	return gechrbuf3;
}

/**************************************************************************
** Show subsystem damage details for REPORT SYS                          **
**************************************************************************/

void FUNC show_rep_sysdam(WARSHP *ptr)
{
	int need;
	int active;
	int fuzz;
	int shfuzz;

	active = TRUE;
	fuzz = gernd();
	shfuzz = (fuzz % 5) - 2;
	fuzz = ((fuzz / 5) % 5) - 2;

	/* in order of how they're repaired */
	if (ptr->shieldstat == SHIELDDM) {
		need = 1 - ptr->shield;
		if (need <= 0)
			need = 1;
		need = (need + ptr->shieldtype - 1) / ptr->shieldtype;
		prfmsg(REP15,repdmg_eta(ptr,need,TRUE,shfuzz));
	}
	if (ptr->helm < 0) {
		prfmsg(REP16,repdmg_eta(ptr,-ptr->helm,active,fuzz));
		active = FALSE;
	}
	if (ptr->tactical < 0) {
		prfmsg(REP18,repdmg_eta(ptr,-ptr->tactical,active,fuzz));
		active = FALSE;
	}
	if (ptr->phasr < 0) {
		prfmsg(REP21,repdmg_eta(ptr,(int)ceil(-ptr->phasr),active,fuzz));
		active = FALSE;
	}
	if (ptr->torpcntl > 0) {
		prfmsg(REP22T,repdmg_eta(ptr,ptr->torpcntl,active,fuzz));
		active = FALSE;
	}
	if (ptr->mislcntl > 0) {
		prfmsg(REP22M,repdmg_eta(ptr,ptr->mislcntl,active,fuzz));
		active = FALSE;
	}
	if (ptr->cloak < 0) {
		prfmsg(REP17,repdmg_eta(ptr,-ptr->cloak,active,fuzz));
		active = FALSE;
	}
	if (ptr->jamload < 0) {
		prfmsg(REP22J,repdmg_eta(ptr,-ptr->jamload,active,fuzz));
		active = FALSE;
	}
	if (ptr->decload < 0) {
		prfmsg(REP22D,repdmg_eta(ptr,-ptr->decload,active,fuzz));
		active = FALSE;
	}
	if (ptr->zipload < 0) {
		prfmsg(REP22Z,repdmg_eta(ptr,-ptr->zipload,active,fuzz));
		active = FALSE;
	}
	if (ptr->mineload < 0) {
		prfmsg(REP22MN,repdmg_eta(ptr,-ptr->mineload,active,fuzz));
		active = FALSE;
	}
}

/**************************************************************************
** Load the planet currently being orbited                               **
**************************************************************************/

static int load_orbit_planet(int usrn)
{
	plnum = warsptr->where - 10;
	if (!getplanetdat(usrn)) {
		prfmsg(NOPLNT);
		outprfge(FLT_NONE, usrn);
		return FALSE;
	}
	return TRUE;
}


/**************************************************************************
** Take the shields up or down                                           **
**************************************************************************/

void FUNC cmd_shields(void)
{
	if (shipclass[warsptr->shpclass].max_shlds == 0) {
		prfmsg(SHIELD0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where == 1) {
		prfmsg(SHLD1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(SHDAMAG);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc != 2) {
		prfmsg(FORMAT,"SHIELD");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (sameas(margv[1],"up")) {
		if (warsptr->shieldstat == SHIELDDM) {
			prfmsg(SHNORPR);
			outprfge(FLT_NONE,usrnum);
			return;
		}
		if (fluxstat(warsptr,usrnum,SHENGUSE * warsptr->shieldtype) == 1) {
			shieldup(warsptr,usrnum);
			return;
		} else {
			prfmsg(SHNOPWR);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	} else if (sameas(margv[1],"down")) {
		shielddn(warsptr,usrnum);
		return;
	} else {
		prfmsg(FORMAT,"SHIELD");
		outprfge(FLT_NONE,usrnum);
		return;
	}
}

/**************************************************************************
** Turn cloaking on and off                                              **
**************************************************************************/

void FUNC cmd_cloak(void)
{
	if (shipclass[warsptr->shpclass].max_cloak == 0) {
		prfmsg(CLOK01);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc != 2) {
		prfmsg(FORMAT,"CLOAK");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->where == 1) {
		prfmsg(CLOK1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDCLOK);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (sameas(margv[1],"on")) {
		if (warsptr->destruct > (byte)0) {
			prfmsg(SELFD8);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		if (warsptr->repair > 0) {
			if (warsptr->where < 10) {
				prfmsg(CLOKNORP);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (!load_orbit_planet(usrnum))
				return;
			if (!sameas(plptr->userid,warsptr->userid)) {
				prfmsg(CLOKNORP);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}

		if (warsptr->cloak > 0) {
			prfmsg(CLOKCOM);
			outprfge(FLT_NONE,usrnum);
		} else if (warsptr->cloak == 0) {
			if (fluxstat(warsptr,usrnum,clenguse) == 1) {
				warsptr->cloak = 1;
				prfmsg(CLOKON);
				outprfge(FLT_NONE,usrnum);
				lock_simple(warsptr,usrnum,LOCKCLOK,0);
			} else {
				prfmsg(CLOKPWR);
				outprfge(FLT_NONE,usrnum);
			}
		} else if (warsptr->cloak < 0) {
			prfmsg(CLOKDAM);
			outprfge(FLT_NONE,usrnum);
		}
		return;
	} else if (sameas(margv[1],"off")) {
		if (warsptr->cloak <= 0) {
			prfmsg(CLOKDWN);
			outprfge(FLT_NONE,usrnum);
		} else if (warsptr->cloak > 0) {
			if (warsptr->cloak == 10) {
				warsptr->cloak = 3;
				assign_cybs(usrnum,1);	/* don't pull far away cybs if close ones around */
				prfmsg(CLOKOFF);
				outprfge(FLT_NONE,usrnum);
				prfmsg(CLOK2);
				outrange(FLT_NONE,&warsptr->coord);
				suddenappear(warsptr,usrnum);
			} else {
				warsptr->cloak = 3;
				assign_cybs(usrnum,1);	/* don't pull far away cybs if close ones around */
				prfmsg(CLOKOFF);
				outprfge(FLT_NONE,usrnum);
			}
		}
		return;
	}
	prfmsg(FORMAT,"CLOAK");
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Transfer goods between ship and planet                                **
**************************************************************************/

static void trans_down(int item)
{
	long amt;

	if (neutral(&warsptr->coord)) {
		prfmsg(TRANSFR2);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (trans_opt || sameas(plptr->userid,warsptr->userid)) {
		if (sameas("ALL",margv[2])) {
			if (warsptr->items[item] > (unsigned long)SLCAP)
				amt = SLCAP;
			else
				amt = (long)warsptr->items[item];
			if (amt == 0L) {
				/* user wants all down but there are none */
				sprintf(gechrbuf,"%lu",(unsigned long)amt);
				prfmsg(TRANSFR5,gechrbuf,item_name[item]);
				return;
			}
		} else
			amt = atol(margv[2]);

		if (amt == 0L) {
			/* user specified 0 or non-number */
			prfmsg(FORMAT,"TRANSFER");
			return;
		}

		if (amt < 0L) {
			unsigned long hold = (unsigned long)(-(amt + 1L)) + 1UL;
			unsigned long left;
			/* user wants all but specified amount */
			if (hold > warsptr->items[item]) {
				/* the amount to withhold is more than the total */
				prfmsg(TRANSFR1);
				return;
			}
			left = warsptr->items[item] - hold;
			if (left > (unsigned long)SLCAP)
				amt = SLCAP;
			else
				amt = (long)left;
		}

		if (warsptr->items[item] >= (unsigned long)amt) {
			unsigned long room;
			if (plptr->items[item].qty > ULCAP - (unsigned long)amt) {
				room = ULCAP - plptr->items[item].qty;
				if (room == 0UL) {
					prfmsg(TRANSFR6,item_name[item]);
					return;
				}
				amt = (long)room;
			}
			warsptr->items[item] -= (unsigned long)amt;
			plptr->items[item].qty += (unsigned long)amt;
			sprintf(gechrbuf,"%lu",(unsigned long)amt);
			prfmsg(TRANSFR5,gechrbuf,item_name[item]);
			setsect(warsptr); /* build PKEY */
			pkey.plnum = plnum;
			gesdb(GEUPDATE,&pkey,(GALSECT *)&planet);
			gepdb(GEUPDATE,warsptr->userid,warsptr->shipno,warsptr);
			return;
		} else {
			/* not enough on board */
			prfmsg(TRANSFR1);
		}
	} else {
		/* not owner */
		prfmsg(TRANSFR4);
	}
}

static void trans_up(int item)
{
	long amt;
	unsigned long room100;

	if (!load_orbit_planet(usrnum))
		return;

	/* you must own this planet or NOBODY must own it to xfer up */
	if (sameas(plptr->userid,warsptr->userid) || plptr->userid[0] == 0) {
		if (sameas("MAX",margv[2])) {
			room100 = ((unsigned long)shipclass[warsptr->shpclass].max_tons * 100UL) - cargo_weight100(warsptr);
			amt = (long)(room100 / (unsigned long)weight[item]);
			if (amt <= 0L) {
				/* user wants max but not even one will fit */
				prfmsg(TRANSUP6);
				return;
			}
		} else if (sameas("ALL",margv[2])) {
			if (plptr->items[item].qty > (unsigned long)SLCAP)
				amt = SLCAP;
			else
				amt = (long)plptr->items[item].qty;
			if (amt == 0L) {
				/* user wants all up but there are none */
				sprintf(gechrbuf,"%lu",(unsigned long)amt);
				prfmsg(TRANSUP5,gechrbuf,item_name[item]);
				return;
			}
		} else
			amt = atol(margv[2]);

		if (amt == 0L) {
			/* user specified 0 or non-number */
			prfmsg(FORMAT,"TRANSFER");
			return;
		}

		if (amt < 0L) {
			unsigned long hold = (unsigned long)(-(amt + 1L)) + 1UL;
			unsigned long left;
			/* user wants all but specified amount */
			if (hold > plptr->items[item].qty) {
				/* the amount to withhold is more than the total */
				prfmsg(TRANSUP1);
				return;
			}
			left = plptr->items[item].qty - hold;
			if (left > (unsigned long)SLCAP)
				amt = SLCAP;
			else
				amt = (long)left;
		}

		if (chkweight(warsptr,item,amt)) {
			if (plptr->items[item].qty >= (unsigned long)amt) {
				plptr->items[item].qty -= (unsigned long)amt;
				warsptr->items[item] += (unsigned long)amt;
				sprintf(gechrbuf,"%lu",(unsigned long)amt);
				prfmsg(TRANSUP5,gechrbuf,item_name[item]);
				setsect(warsptr); /* load PKEY */
				pkey.plnum = plnum;
				gesdb(GEUPDATE,&pkey,(GALSECT *)&planet);
				gepdb(GEUPDATE,warsptr->userid,warsptr->shipno,warsptr);
				return;
			} else {
				/* not enough on planet */
				prfmsg(TRANSUP1);
			}
		} else {
			/* not enough space */
			prfmsg(TRANSUP6);
		}
	} else {
		/* not owner */
		prfmsg(TRANSUP4);
	}
}

void FUNC cmd_transfer(void)
{
	int i;

	if (warsptr->where < 10) {
		prfmsg(TRANSFR3);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 4) {
		for (i=0; i < NUMITEMS; ++i) {
			if (sameto(kwrd[i],margv[3])) {
				if (sameto("u",margv[1]))
					trans_up(i);
				else if (sameto("d",margv[1]))
					trans_down(i);
				else
					prfmsg(FORMAT,"TRANSFER");
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}
	}

	prfmsg(FORMAT,"TRANSFER");
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** abandon a colony                                                      **
**************************************************************************/

void FUNC cmd_abandon(void)
{
	if (warsptr->where < 10) {
		prfmsg(ABAN01);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (sameas(plptr->userid,warsptr->userid)) {
		plptr->userid[0] = 0;
		plptr->password[0] = 0;
		plptr->teamcode = 0;
		if (waruptr->planets > 0)
			--waruptr->planets;
		geudb(GEUPDATE,waruptr->userid,waruptr);

		setsect(warsptr); /* build PKEY */
		pkey.plnum = plnum;
		gesdb(GEUPDATE,&pkey,(GALSECT *)&planet);

		prfmsg(ABAN02);
		outprfge(FLT_NONE,usrnum);
	} else {
		prfmsg(ADMIN2);
		outprfge(FLT_NONE,usrnum);
	}
}

/**************************************************************************
** establish a colony or administer it                                   **
**************************************************************************/

void FUNC cmd_admin(void)
{
	if (warsptr->where < 10) {
		prfmsg(ADMIN1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (plptr->userid[0] == 0) {
		if (waruptr->planets >= max_plnts) {
			prfmsg(ADMIN4,max_plnts);
			outprfge(FLT_NONE,usrnum);
			return;
		}
		prfmsg(ADMENU1);
		outprfge(FLT_NONE,usrnum);
		usrptr->substt = ADMENU1;
	}
	else
#ifdef PHARLAP
	if (sameas(plptr->userid,warsptr->userid) || (syscmds && !sysonly) || (sysonly && (hasmkey(SYSKEY))))
#else
	if (sameas(plptr->userid,warsptr->userid) || (syscmds && !sysonly) || (sysonly && (usrptr->flags&ISYSOP)))
#endif
	{
		prfmsg(ADMENU2);
		outprfge(FLT_NONE,usrnum);
		usrptr->substt = ADMENU2;
	}
	else {
		prfmsg(ADMIN2);
		outprfge(FLT_NONE,usrnum);
	}
}

/**************************************************************************
** Notify the owner about a planetary attack                             **
**************************************************************************/

static void call_4_help(int send_spy_mail, int won)
{
	if (instat(plptr->userid, gestt) && othusp->substt >= FIGHTSUB) {
		if (warsptr->shipname[0] == 0)
			prfmsg(ATTACK6O, plptr->name, xsect, ysect, warsptr->userid);
		else
			prfmsg(ATTACK6, plptr->name, xsect, ysect, warsptr->userid, warsptr->shipname);
		outprf(othusn);
		prfmsg(ATTACK7);
		outprfge(FLT_NONE, usrnum);
		clrprf();
	} else if (onsys(plptr->userid) && user[othusn].state != fse_state) {
		prfmsg(ATTACK6A);
		injoth();
		prfmsg(ATTACK7);
		outprfge(FLT_NONE, usrnum);
		clrprf();
	} else if (won == 0
		&& send_spy_mail
		&& gernd() % 6 == 0
		&& plptr->spyowner[0] != 0) {
		prfmsg(SPYM3, plptr->name, xsect, ysect, warsptr->userid);
		strcpy(mail.userid, plptr->spyowner);
		strcpy(mail.topic, "Intelligence Report");
		sendit();
		clrprf();
	} else if (won == 1
		&& plptr->spyowner[0] != 0) {
		prfmsg(SPYM4, plptr->name, xsect, ysect, warsptr->userid);
		strcpy(mail.userid, plptr->spyowner);
		strcpy(mail.topic, "Intelligence Report");
		sendit();
		clrprf();
	}
}

/**************************************************************************
** Transfer a conquered planet to the attacking player                   **
**************************************************************************/

static void wonplnt(void)
{
	char olduid[UIDSIZ];

	/* save old owner */
	strncpy(olduid, plptr->userid, UIDSIZ);
	olduid[UIDSIZ - 1] = 0;

	/* remove planet from old owner, if they still exist */
	if (olduid[0] && !sameas(olduid, warsptr->userid)) {
		setbtv(gebb5);
		if (qeqbtv(olduid, 0)) {
			gcrbtv(&tmpusr, 0);
			if (tmpusr.planets)
				--tmpusr.planets;
			geudb(GEUPDATE, tmpusr.userid, &tmpusr);
		}
	}

	/* assign planet to new owner */
	strncpy(plptr->userid, warsptr->userid, UIDSIZ);
	plptr->userid[UIDSIZ - 1] = 0;
	if (sameas(plptr->password, "team")) {
		plptr->password[0] = 0;
		plptr->teamcode = 0;
	}
	warsptr->hostile = 0;

	/* add planet to winner */
	++waruptr->planets;
	geudb(GEUPDATE, waruptr->userid, waruptr);
}

/**************************************************************************
** Attack helpers                                                       **
**************************************************************************/

static int attack_men(unsigned long num)
{
	double r;
	int won = 0;
	int ii;
	unsigned long i, j, left1, left2, kill1, kill2, ratio;

	/* take troops off ship */
	warsptr->items[I_TROOPS] -= num;
	sprintf(gechrbuf, "%ld", num);

	/* tell him they're gone*/
	prfmsg(ATTACKM1, gechrbuf);
	outprfge(FLT_NONE, usrnum);

	left1 = num;
	left2 = plptr->items[I_TROOPS].qty;

	kill1 = 0;
	kill2 = 0;

	if (plptr->items[I_FIGHTER].qty > 1) {
		prfmsg(ATTACKM7);
		i = ((unsigned long)(gernd() % 35)) + 9L;
		kill1 = i * (plptr->items[I_FIGHTER].qty);
		if (kill1 > left1)
			kill1 = left1;
		sprintf(gechrbuf, "%ld", kill1);
		prfmsg(ATTACKM8, gechrbuf);
	}

	/* figure out the proportion of this attack */
	if (left2 > 0)
		ratio = (left1 * 100UL) / left2;
	else {
		ratio = 0;
		if (plptr->items[(int)I_FIGHTER].qty <= 0L) {
			if (waruptr->planets < max_plnts)
				won = 1;
			else
				prfmsg(ADMIN4, waruptr->planets);
		}
	}

	/* this specifies the number of troops killed by ground troops */
	r = rndm(plattrt1) + .25;
	kill1 += (unsigned long)((double)left2 * r);

	if (ratio > 2L) {
		r = rndm(plattrt2) + .1;
		kill2 = (unsigned long)((double)left1 * r);
	}
	else {
		kill2 = 0L;
	}

	if (kill1 > left1)
		kill1 = left1;

	if (kill2 > left2)
		kill2 = left2;

	sprintf(gechrbuf, "%ld", kill1);
	sprintf(gechrbuf2, "%ld", kill2);

	prfmsg(ATTACKM2, gechrbuf2, gechrbuf);

	left1 -= kill1;
	left2 -= kill2;

	if (left2 > 0 && left2 < (left1 / 4) && !won) {
		if (waruptr->planets < max_plnts) {
			won = 1;
			sprintf(gechrbuf, "%ld", left2);
			prfmsg(ATTACKM3, gechrbuf);
		}
		else
			prfmsg(ADMIN4, waruptr->planets);
	}

	if (left1 > 0 && left1 < (left2 / 4)) {
		sprintf(gechrbuf, "%ld", left1);
		prfmsg(ATTACKM4, gechrbuf);
		plptr->items[I_TROOPS].qty += left1;
		left1 = 0;
	}

	/* randomly trash a few of the planets items */
	if (ratio > 2 && left1 > (left2 / 2)) {
		for (ii = 1; ii < NUMITEMS; ++ii) {
			j = (unsigned long)gernd() % 15;

			if (j > plptr->items[ii].qty)
				j = plptr->items[ii].qty;

			if (j >= 1) {
				sprintf(gechrbuf, "%ld", j);
				prfmsg(ATTACKM5, gechrbuf, item_name[ii]);
				plptr->items[ii].qty -= j;
			}
		}
	}

	if (left1 > 0) {
		warsptr->items[I_TROOPS] += left1;
		sprintf(gechrbuf, "%lu", left1);
		prfmsg(ATTACKM6, gechrbuf);
	}

	if (left1 > 0L && (left2 <= 0L && plptr->items[(int)I_FIGHTER].qty <= 0L) && !won) {
		if (waruptr->planets < max_plnts)
			won = 1;
		else
			prfmsg(ADMIN4, waruptr->planets);
	}

	plptr->items[I_TROOPS].qty = left2;

	outprfge(FLT_NONE, usrnum);
	clrprf();

	/* inform the player if he is not in game */
	if (ratio > 5L) /* big enough to let spy report on it */
		call_4_help(TRUE, won);
	else if (ratio > 1L)
		call_4_help(FALSE, won);

	/* dont mail him unless it's significant */
	if (ratio > 1L || won == 1) {
		mail.type = MESG02;
		strncpy(mail.userid, plptr->userid, UIDSIZ);
		mail.userid[UIDSIZ - 1] = 0;

		if (won == 1) {
			wonplnt();
			mail.type = MESG03;
		}

		strcpy(mail.name1, plptr->name);
		mail.class = MAIL_CLASS_DISTRESS;
		mail.int1 = sector.xsect;
		mail.int2 = sector.ysect;
		mail.long1 = num;
		if (warsptr->shipname[0] == 0) {
			mail.name2[0] = 0;
			if (mail.type == MESG02)
				mail.type = MESG02O;
			else if (mail.type == MESG03)
				mail.type = MESG03O;
		} else {
			strncpy(mail.name2, warsptr->shipname, sizeof(mail.name2) - 1);
			mail.name2[sizeof(mail.name2) - 1] = 0;
		}
		sprintf(mail.string1, "%s", warsptr->userid);

		mailit(1);
	}

	setsect(warsptr);
	pkey.plnum = plnum;
	gesdb(GEUPDATE, &pkey, (GALSECT *)&planet);
	return won;
}

static int attack_fig(unsigned long num)
{
	double r;
	int won = 0;
	int ii;
	unsigned long j, left1, left2, kill1, kill2, ratio;

	warsptr->items[I_FIGHTER] -= num;
	sprintf(gechrbuf, "%ld", num);
	prfmsg(ATTACKF1, gechrbuf);
	outprfge(FLT_NONE, usrnum);

	left1 = num;
	left2 = plptr->items[I_FIGHTER].qty;

	kill1 = 0;
	kill2 = 0;

	/* figure out the proportion of this attack */
	if (left2 > 0) {
		ratio = (left1 * 100UL) / left2;
	} else {
		ratio = 0;
	}

	/* If there are more than 500 troops - shoot down some fighters */
	if (left1 > 0 && plptr->items[I_TROOPS].qty > 500 && (gernd() % 5 - 1) > 0) {
		prfmsg(ATTACKF8);
		r = rndm(plattrf1) + .05;
		kill1 += (unsigned long)((double)left1 * r);
	}

	/* if there are fighters on the planet then they will shoot down some of the attackers */
	if (left2 > 0L) {
		/* this specifies the planet fighters' kill ratio -
		   at least 20% and as much as 120.0% */
		r = rndm(plattrf2) + .2; /* 1.0 */
		kill1 += (unsigned long)((double)left2 * r);

		/* if the ratio of attacker to attackee is at least 1 to 2 (2%)
		   then kill off some of the planets fighters - this makes it harder
			for the little marauders to eat away at your fighters */
		if (ratio > 1) {
			r = rndm(plattrf3) + .2;
			kill2 = (unsigned long)((double)left1 * r);
		} else {
			kill2 = 0L;
		}

		if (kill1 > left1)
			kill1 = left1;

		if (kill2 > left2)
			kill2 = left2;

		sprintf(gechrbuf, "%ld", kill2);
		sprintf(gechrbuf2, "%ld", kill1);

		prfmsg(ATTACKF2, gechrbuf, gechrbuf2);
		left1 -= kill1;
		left2 -= kill2;
	}

	/* must be at least 5% to do damage to items on the planet */
	if (ratio > 5) {
		for (ii = 0; ii < NUMITEMS; ++ii) {
			j = (unsigned long)gernd() % 15;

			if (j > plptr->items[ii].qty)
				j = plptr->items[ii].qty;

			if (j > 0 && ii != I_FIGHTER) {
				sprintf(gechrbuf, "%ld", j);
				prfmsg(ATTACKF5, gechrbuf, item_name[ii]);
				plptr->items[ii].qty -= j;
			}
		}
	}

	if (left1 > 0L && left2 <= 0L && plptr->items[I_TROOPS].qty < 5L) {
		prfmsg(ATTACKF7);
		if (waruptr->planets < max_plnts)
			won = 1;
		else
			prfmsg(ADMIN4, waruptr->planets);
	}

	if (left1 > 0L) {
		warsptr->items[I_FIGHTER] += left1;
		sprintf(gechrbuf, "%ld", left1);
		prfmsg(ATTACKF6, gechrbuf);
	}

	outprfge(FLT_NONE, usrnum);
	clrprf();

	if (ratio > 5) /* big enough to let spy report on it */
		call_4_help(TRUE, won);
	else if (ratio > 1)
		call_4_help(FALSE, won);

	plptr->items[I_FIGHTER].qty = left2;

	if (ratio > 2 || won == 1) {
		mail.class = MAIL_CLASS_DISTRESS;
		mail.type = MESG04;
		strncpy(mail.userid, plptr->userid, UIDSIZ);
		mail.userid[UIDSIZ - 1] = 0;

		if (won == 1) {
			wonplnt();
			mail.type = MESG05;
		}

		strcpy(mail.name1, plptr->name);
		mail.class = MAIL_CLASS_DISTRESS;
		mail.int1 = sector.xsect;
		mail.int2 = sector.ysect;
		mail.long1 = num;
		if (warsptr->shipname[0] == 0) {
			mail.name2[0] = 0;
			if (mail.type == MESG04)
				mail.type = MESG04O;
			else if (mail.type == MESG05)
				mail.type = MESG05O;
		} else {
			strncpy(mail.name2, warsptr->shipname, sizeof(mail.name2) - 1);
			mail.name2[sizeof(mail.name2) - 1] = 0;
		}
		sprintf(mail.string1, "%s", warsptr->userid);

		mailit(1);
	}

	setsect(warsptr);
	pkey.plnum = plnum;
	gesdb(GEUPDATE, &pkey, (GALSECT *)&planet);
	return won;
}

/**************************************************************************
** Attack Command                                                        **
**************************************************************************/

void FUNC cmd_attack(void)
{
	int won;
	unsigned long num;

	if (warsptr->where < 10) {
		prfmsg(ADMIN1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (sameas(plptr->userid,warsptr->userid)) {
		prfmsg(ATTACK0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (neutral(&warsptr->coord)) {
		zaphim(warsptr,usrnum);
		prfmsg(ATTKER);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 3) {
		if (sameto("tro",margv[2])) {
			if (sameas("ALL",margv[1]))
				num = warsptr->items[I_TROOPS];
			else
				num = atol(margv[1]);
			if (num > 0L && num <= warsptr->items[I_TROOPS]) {
				warsptr->hostile = warsptr->where;
				warsptr->cantexit = FIRETICKS;
				won = attack_men(num);
				if (won == 1) {
					prfmsg(ATTACK8);
					outprfge(FLT_NONE,usrnum);
				} else {
					prfmsg(ATTACK9);
					outprfge(FLT_NONE,usrnum);
				}
			} else {
				prfmsg(ATTACKM0);
				outprfge(FLT_NONE,usrnum);
			}
			return;
		} else if (sameto("fig",margv[2])) {
			if (shipclass[warsptr->shpclass].max_attk > 0) {
				if (sameas("ALL",margv[1]))
					num = warsptr->items[I_FIGHTER];
				else
					num = atol(margv[1]);
				if (num > 0 && num <= warsptr->items[I_FIGHTER]) {
					warsptr->hostile = warsptr->where;
					warsptr->cantexit = FIRETICKS;
					won = attack_fig(num);
					if (won == 1) {
						prfmsg(ATTACK8);
						outprfge(FLT_NONE,usrnum);
					} else {
						prfmsg(ATTACK9);
						outprfge(FLT_NONE,usrnum);
					}
				} else {
					prfmsg(ATTACKF0);
					outprfge(FLT_NONE,usrnum);
				}
				return;
			} else {
				prfmsg(ATTACK0A);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}
		prfmsg(FORMAT,"ATTACK");
	} else {
		prfmsg(FORMAT,"ATTACK");
	}
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Roster Command                                                        **
**************************************************************************/

void FUNC cmd_geroster(void)
{
	int i = 0;
	int j;
	int rank = 0;
	long target = 0;

	setbtv(gebb5);

	j = gemaxlist;

	if (margc == 2 && sameas(margv[1], "all")) {
		prfmsg(ROSTER1);
		outprfge(FLT_NONE, usrnum);
		j = 200;
	} else {
		prfmsg(ROSTER2, gemaxlist);
		outprfge(FLT_NONE, usrnum);
	}

	if (usaptr->userid[0] != 0 && usaptr->userid[0] != '@' && qeqbtv(usaptr->userid, 0))
		target = absbtv();

	if (qhibtv(1)) {
		do {
			gcrbtv(&tmpusr, 1);
			logthis(spr("ROS:Got %s Score %lu", tmpusr.userid, tmpusr.score));
			if ((tmpusr.score > 0 || j == 200) && tmpusr.userid[0] != '@') {
				++i;
				sprintf(gechrbuf, "%11lu", tmpusr.score);
				sprintf(gechrbuf2, " %10.2fm", ((float)tmpusr.population) / 100.0);
				prf("%-29s%s%6u%6u%4u%s\r", tmpusr.userid, gechrbuf, tmpusr.kills, tmpusr.ukills, tmpusr.planets, gechrbuf2);
				if (target != 0 && absbtv() == target)
					rank = i;
				if (i % 5 == 0)
					outprfge(FLT_NONE, usrnum);
			}
		} while (qprbtv() && i < j);
		if (i % 5 != 0)
			outprfge(FLT_NONE, usrnum);
		if (rank != 0) {
			prfmsg(ROSTER3, rank);
			outprfge(FLT_NONE, usrnum);
		}
	} else {
		logthis("No one in roster yet");
	}
}

/**************************************************************************
** Planet command                                                        **
**************************************************************************/

void FUNC cmd_planet(void)
{
	int page = 1;
	int stepper = 0;
	int inc;

	if (margc > 1) {
		page = atoi(margv[1]);
		if (page < 1 || page > ((max_plnts + 19) / 20) || margc > 2) {
			prfmsg(FORMAT, "PLANET");
			outprfge(FLT_NONE, usrnum);
			return;
		}
	}

	inc = (page - 1) * 20;

	setbtv(gebb2);

	if (qlobtv(0)) {
		if (qeqbtv(warsptr->userid, 1)) {
			prfmsg(PLAMSG1);
			do {
				gcrbtv(&planet, 1);
				if (sameas(planet.userid, warsptr->userid)) {
					if (stepper >= inc) {
						prf("%-24s %6d %6d  %6d\r", planet.name, planet.xsect, planet.ysect, planet.plnum);
					}
					++stepper;
					if (stepper > inc && ((stepper - inc) % 5) == 0) /* cat five lines then print */
						outprfge(FLT_NONE, usrnum);
					if (stepper >= inc + 20) {
						prfmsg(PLAMSG3, page + 1);
						outprfge(FLT_NONE, usrnum);
						return;
					}
				} else {
					break;
				}
			} while (qnxbtv());
			if (stepper > inc && ((stepper - inc) % 5) != 0)
				outprfge(FLT_NONE, usrnum);
		} else {
			prfmsg(PLAMSG2);
			outprfge(FLT_NONE, usrnum);
		}
	} else {
		prfmsg(PLAMSG2);
		outprfge(FLT_NONE, usrnum);
	}
}

/**************************************************************************
** Sell goods helper                                                     **
**************************************************************************/

static void sell(int item)
{
	unsigned long amt, gross, fee, net;
	byte toorich, toomuch;
	long req;

	toorich = FALSE;
	toomuch = FALSE;

	if (sameas("ALL", margv[1])) {
		if (warsptr->items[item] == 0L) {
			prfmsg(SELL5, item_name[item]);
			return;
		}
		amt = warsptr->items[item];
	} else {
		req = atol(margv[1]);
		if (req == 0L) {
			prfmsg(FORMAT, "SELL");
			return;
		}
		if (req < 0L) {
			unsigned long hold = (unsigned long)(-(req + 1L)) + 1UL;
			if (hold > warsptr->items[item]) {
				prfmsg(SELL3, item_name[item]);
				return;
			}
			amt = warsptr->items[item] - hold;
		} else {
			amt = (unsigned long)req;
		}
	}

	if (amt == 0UL) {
		prfmsg(FORMAT, "SELL");
		return;
	}
	if (warsptr->items[item] >= amt) {
		if (amt > ULCAP / (unsigned long)baseprice[item]) {
			amt = ULCAP / (unsigned long)baseprice[item];
			toomuch = TRUE;
		}

		gross = (unsigned long)baseprice[item] * amt;
		fee = 1UL + (gross / 1000UL);
		if (gross <= fee)
			net = 0UL;
		else
			net = gross - fee;

		if (waruptr->cash > ULCAP - net) {
			unsigned long room, lo, hi, mid;

			toorich = TRUE;
			room = ULCAP - waruptr->cash;
			lo = 0UL;
			hi = amt;

			while (lo < hi) {
				mid = lo + ((hi - lo + 1UL) >> 1);
				gross = (unsigned long)baseprice[item] * mid;
				fee = 1UL + (gross / 1000UL);
				if (gross <= fee || gross - fee <= room)
					lo = mid;
				else
					hi = mid - 1UL;
			}

			if (lo == 0UL) {
				prfmsg(TOORICH, spr("%lu", ULCAP));
				return;
			}

			amt = lo;
			gross = (unsigned long)baseprice[item] * amt;
			fee = 1UL + (gross / 1000UL);
			if (gross <= fee)
				net = 0UL;
			else
				net = gross - fee;
		}

		warsptr->items[item] -= amt;
		waruptr->cash += net;

		gepdb(GEUPDATE, warsptr->userid, warsptr->shipno, warsptr);
		geudb(GEUPDATE, waruptr->userid, waruptr);

		sprintf(gechrbuf, "%lu", amt);
		sprintf(gechrbuf2, "%lu", net);
		sprintf(gechrbuf3, "%lu", fee);

		if (toomuch == TRUE && toorich == FALSE)
			prfmsg(TOOMUCH);
		if (toorich == TRUE)
			prfmsg(TOORICH, spr("%lu", ULCAP));

		prfmsg(SELL2, gechrbuf3, gechrbuf2, gechrbuf, item_name[item]);
		return;
	} else {
		prfmsg(SELL3, item_name[item]);
	}
}

/**************************************************************************
** Sell goods                                                            **
**************************************************************************/

void FUNC cmd_sell(void)
{
	int i;

	if (warsptr->where < 10) {
		prfmsg(SELL1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (!neutral(&warsptr->coord)) {
		prfmsg(SELL1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (waruptr->factions[gcnum] > 100) { /* don't buy from jerks */
		prfmsg(NOZYG, (int)((waruptr->factions[gcnum] - 61) / 40));
		outprfge(FLT_NONE, usrnum);
		return;
	}

	plnum = warsptr->where - 10;

	if (!load_orbit_planet(usrnum))
		return;

	if (neutral(&warsptr->coord) && plnum == 1) { /* must be Zygor */
		if (margc == 3) {
			for (i = 0; i < NUMITEMS; ++i) {
				if (sameto(kwrd[i], margv[2])) {
					sell(i);
					outprfge(FLT_NONE, usrnum);
					return;
				}
			}
		}
	}

	prfmsg(FORMAT, "SELL");
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Buy-side helpers                                                      **
**************************************************************************/

static unsigned long amt4sale(int item)
{
	unsigned long forsale = 0;

	if (sameas(plptr->userid, warsptr->userid))
		forsale = plptr->items[item].qty;
	else if (plptr->items[item].qty > plptr->items[item].reserve && plptr->items[item].sell == 'Y')
		forsale = plptr->items[item].qty - plptr->items[item].reserve;

	return forsale;
}

/**************************************************************************
** Calculate the purchase price for a planet item                        **
**************************************************************************/

static long price(unsigned item, unsigned long amt)
{
	long tot;

	if (sameas(plptr->userid, warsptr->userid))
		tot = ((long)baseprice[item]) * amt;
	else
		tot = ((long)plptr->items[item].markup2a) * amt;

	return tot;
}

/**************************************************************************
** Buy goods helper                                                      **
**************************************************************************/

static void buy(int item)
{
	unsigned long amt, avail, tot, ptot;
	unsigned long room100;

	if (plptr->userid[0] != 0) {
		if ((amt = atol(margv[1])) > 0 || sameas("MAX", margv[1]) || sameas("ALL", margv[1])) {
			if (sameas(plptr->userid, warsptr->userid) || plptr->items[item].sell == 'Y') {
				if (sameas("MAX", margv[1])) {
					room100 = ((unsigned long)shipclass[warsptr->shpclass].max_tons * 100UL) - cargo_weight100(warsptr);
					amt = room100 / (unsigned long)weight[item];
				}
				if (sameas("ALL", margv[1]))
					amt = amt4sale(item);
				if ((sameas(plptr->userid, warsptr->userid) && amt > SLCAP / baseprice[item])
					|| (!sameas(plptr->userid, warsptr->userid)
						&& plptr->items[item].markup2a > 0
						&& amt > SLCAP / (long)plptr->items[item].markup2a)) {
					prfmsg(TOOMUCH);
					return;
				}
				if (chkweight(warsptr, item, amt)) {
					avail = amt4sale(item);
					if (avail > 0 && avail >= amt) {
						if ((tot = price(item, amt)) <= waruptr->cash) {
							if (strncmp(margv[0], "buy", 3) == 0) {
								if (!neutral(&warsptr->coord)) {
									plptr->items[item].qty -= amt;
									plptr->items[item].sold2a += amt;
									if (plptr->cash > ULCAP - tot) {
										ptot = ULCAP - plptr->cash;
										plptr->cash += ptot;
									} else {
										plptr->cash += tot;
									}
									setsect(warsptr);
									pkey.plnum = plnum;
									gesdb(GEUPDATE, &pkey, (GALSECT *)&planet);
								}
								warsptr->items[item] += amt;
								waruptr->cash -= tot;
								sprintf(gechrbuf, "%lu", tot);

								if (sameas(plptr->userid, warsptr->userid)) {
									sprintf(gechrbuf2, "%lu", amt);
									prfmsg(BUY9, gechrbuf2, item_name[item], baseprice[item], gechrbuf);
								} else {
									sprintf(gechrbuf2, "%lu", amt);
									prfmsg(BUY9, gechrbuf2, item_name[item], plptr->items[item].markup2a, gechrbuf);
								}
								return;
							} else {
								sprintf(gechrbuf, "%lu", tot);
								sprintf(gechrbuf2, "%lu", amt);
								if (sameas(plptr->userid, warsptr->userid)) {
									prfmsg(PRICE1, gechrbuf2, item_name[item], baseprice[item], gechrbuf);
								} else {
									prfmsg(PRICE1, gechrbuf2, item_name[item], plptr->items[item].markup2a, gechrbuf);
								}
							}
						} else {
							sprintf(gechrbuf, "%lu", tot);
							sprintf(gechrbuf2, "%lu", amt);
							prfmsg(BUY2, gechrbuf, gechrbuf2, item_name[item]);
						}
					} else {
						sprintf(gechrbuf, "%lu", avail);
						prfmsg(BUY3, gechrbuf, item_name[item]);
					}
				} else {
					if (strncmp(margv[0], "buy", 3) == 0)
						prfmsg(BUY8);
					else {
						sprintf(gechrbuf2, "%lu", amt);
						sprintf(gechrbuf, "%lu", price(item, amt));
						if (sameas(plptr->userid, warsptr->userid)) {
							prfmsg(PRICE2, gechrbuf2, item_name[item], baseprice[item], gechrbuf);
						} else {
							prfmsg(PRICE2, gechrbuf2, item_name[item], plptr->items[item].markup2a, gechrbuf);
						}
					}
				}
			} else {
				prfmsg(BUY4, item_name[item]);
			}
		} else {
			prfmsg(FORMAT, "BUY");
		}
	} else {
		prfmsg(BUY7);
	}
}

/**************************************************************************
** Buy goods                                                             **
**************************************************************************/

void FUNC cmd_buy(void)
{
	int i;

	if (warsptr->where < 10) {
		prfmsg(BUY1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;
	fixplanetteam();

	if (neutral(&warsptr->coord) && plnum == 1 && waruptr->factions[gcnum] > 100) { /* if Zygor, don't sell to jerks */
		prfmsg(NOZYG, (int)((waruptr->factions[gcnum] - 61) / 40));
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (sameas(plptr->password, "team")
		&& plptr->teamcode > 0
		&& plptr->teamcode != waruptr->teamcode) {
		prfmsg(BUYPAS3);
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (sameas(plptr->password, "team")
		&& plptr->teamcode > 0
		&& plptr->teamcode == waruptr->teamcode) {
		prfmsg(BUYPAS4);
		outprfge(FLT_NONE, usrnum);
	} else if (!sameas(plptr->password, "none") && margc > 1 && margc < 4) {
		prfmsg(BUYPAS1);
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (!sameas(plptr->password, "none")
		&& !sameas(plptr->password, margv[3])) {
		prfmsg(BUYPAS2);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (margc > 2) {
		for (i = 0; i < NUMITEMS; ++i) {
			if (sameto(kwrd[i], margv[2])) {
				buy(i);
				outprfge(FLT_NONE, usrnum);
				return;
			}
		}
	}
	prfmsg(FORMAT, "BUY");
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Price goods                                                           **
**************************************************************************/

void FUNC cmd_price(void)
{
	int i;

	if (warsptr->where < 10) {
		prfmsg(BUY1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (neutral(&warsptr->coord) && plnum == 1 && waruptr->factions[gcnum] > 100) { /* if Zygor, don't price to jerks */
		prfmsg(NOZYG, (int)((waruptr->factions[gcnum] - 61) / 40));
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (margc == 3) {
		for (i = 0; i < NUMITEMS; ++i) {
			if (sameto(kwrd[i], margv[2])) {
				buy(i);
				outprfge(FLT_NONE, usrnum);
				return;
			}
		}
	}
	prfmsg(FORMAT, "PRICE");
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Maintenance and repair                                                **
**************************************************************************/

void FUNC cmd_maint(void)
{
	unsigned price;

	if (warsptr->where < 10) {
		prfmsg(MAINT1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (!sameas(plptr->password, "none") && margc < 2) {
		prfmsg(MAINT2);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (!sameas(plptr->password, "none")
		&& !sameas(plptr->password, margv[1])) {
		prfmsg(MAINT3);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (plptr->userid[0] == 0 || plptr->items[I_MEN].qty < 25000L) {
		prfmsg(MAINT8);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (neutral(&warsptr->coord) && chkitm(usrnum))
		warsptr->cantexit = 0;

	if (warsptr->cantexit > 0) {
		prfmsg(MAINT9);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->cloak > 0 && !sameas(plptr->userid, warsptr->userid)) {
		prfmsg(MAINT12);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->repair > 0) {
		prfmsg(MAINT11);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	price = 200;

	if (neutral(&warsptr->coord)) {
		if (plnum == 1 || plnum == 2) {
			price = 2500;
		} else {
			prfmsg(MAINT4);
			outprfge(FLT_NONE, usrnum);
			return;
		}
	}

	if (waruptr->cash >= price) {
		if (!repair_needed(warsptr))
			prfmsg(MAINT13);
		else {
			warsptr->repair = 1;
			waruptr->cash -= price;
			prfmsg(MAINT5, repair_eta(warsptr));
		}
	} else {
		prfmsg(MAINT6);
	}

	outprfge(FLT_NONE, usrnum);
}


/**************************************************************************
** New ship or goods command                                             **
**************************************************************************/

static int upg_allowed(WARSHP *ptr, unsigned int loadout, int idx)
{
	if (!(loadout & upgdefs[idx].bit))
		return FALSE;
	if (upgdefs[idx].bit == TPONDER && shipclass[ptr->shpclass].noclaim == 0)
		return FALSE;
	return TRUE;
}

static long upg_price(WARSHP *ptr, int idx)
{
	return((long)(((double)upgrprice[upgdefs[idx].priceidx] *
		shipclass[ptr->shpclass].damfact) / 100.0));
}

void FUNC cmd_new(void)
{
	int type, ctype, upidx, i;
	unsigned int loadout;
	unsigned int upbit;
	byte tpmode;
	long delta, credit, fee;
	long price;

	if ((margc != 3 && !(margc == 4 && sameas(margv[1], "upgrade"))) &&
		!(margc == 2 && sameas(margv[1], "upgrade"))) {
		prfmsg(FORMAT, "NEW");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->where < 10) {
		prfmsg(NEW1);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (neutral(&warsptr->coord) && plnum == 1) { /* must be Zygor */
		if (waruptr->factions[gcnum] > 100) { /* don't sell to jerks */
			prfmsg(NOZYG, (int)((waruptr->factions[gcnum] - 61) / 40));
			outprfge(FLT_NONE, usrnum);
			return;
		}
		if (sameas(margv[1], "upgrade")) {
			loadout = shipclass[warsptr->shpclass].loadout;
			if (loadout == 0) {
				prfmsg(UPGRNO, shipclass[warsptr->shpclass].typename);
				outprfge(FLT_NONE, usrnum);
				return;
			}
			if (margc == 3) {
				type = atoi(margv[2]);
				if (type < 1 || type > NUMUPGRADES) {
					prfmsg(NEW12);
				} else {
					upidx = type - 1;
					upbit = upgdefs[upidx].bit;
					if (!upg_allowed(warsptr, loadout, upidx)) {
						prfmsg(NEW20);
					} else if (warsptr->upgrade & upbit) {
						prfmsg(NEW21);
					} else {
						price = upg_price(warsptr, upidx);
						if (price <= waruptr->cash) {
							waruptr->cash -= price;
							warsptr->upgrade |= upbit;
							if (upbit == TPONDER) {
								warsptr->tponder = TPONNORM;
								prfmsg(NEW22, l2as(price));
								outprfge(FLT_NONE, usrnum);
								prfmsg(TPOND4);
								outprfge(FLT_NONE, usrnum);
								prfmsg(TPOND5);
							} else {
								prfmsg(NEW22, l2as(price));
							}
						} else {
							prfmsg(NEW23);
						}
					}
				}
				outprfge(FLT_NONE, usrnum);
				return;
			}
			if (margc == 4) {
				type = atoi(margv[2]);
				if (type != 7) {
					prfmsg(NEW12);
					outprfge(FLT_NONE, usrnum);
					return;
				}
				if (!upg_allowed(warsptr, loadout, 6)) {
					prfmsg(NEW20);
					outprfge(FLT_NONE, usrnum);
					return;
				}
				if (sameas(margv[3], "high"))
					tpmode = TPONHIGH;
				else if (sameas(margv[3], "normal"))
					tpmode = TPONNORM;
				else if (sameas(margv[3], "low"))
					tpmode = TPONLOW;
				else {
					prfmsg(TPOND5);
					outprfge(FLT_NONE, usrnum);
					return;
				}
				if (!(warsptr->upgrade & TPONDER)) {
					price = upg_price(warsptr, 6);
					if (price <= waruptr->cash) {
						waruptr->cash -= price;
						warsptr->upgrade |= TPONDER;
						warsptr->tponder = tpmode;
						prfmsg(NEW22, l2as(price));
						outprfge(FLT_NONE, usrnum);
					} else {
						prfmsg(NEW23);
						outprfge(FLT_NONE, usrnum);
						return;
					}
				} else {
					warsptr->tponder = tpmode;
				}
				if (warsptr->tponder == TPONHIGH)
					prfmsg(TPOND1);
				else if (warsptr->tponder == TPONLOW)
					prfmsg(TPOND3);
				else
					prfmsg(TPOND2);
				outprfge(FLT_NONE, usrnum);
				return;
			}
			prfmsg(UPGRHEAD, shipclass[warsptr->shpclass].typename);
			outprfge(FLT_NONE, usrnum);
			for (i = 0; i < NUMUPGRADES; ++i) {
				if (!upg_allowed(warsptr, loadout, i))
					continue;
				prfmsg(upgdefs[i].namemsg);
				if (warsptr->upgrade & upgdefs[i].bit)
					prfmsg(upgdefs[i].descmsg, i + 1, "purchased");
				else
					prfmsg(upgdefs[i].descmsg, i + 1, l2as(upg_price(warsptr, i)));
				outprfge(FLT_NONE, usrnum);
			}
			return;
		}
		if (sameas(margv[1], "ship")) {
			type = atoi(margv[2]) - 1;
			if (type >= 0 && type < cyb_class && shipclass[type].max_type == CLASSTYPE_USER) {
				if (waruptr->noships < maxships) {
					if (shipclass[type].max_price <= waruptr->cash) {
						waruptr->cash -= shipclass[type].max_price;
						prfmsg(NEW3, shipclass[type].typename);
						initshp(waruptr->userid, type);
						gepdb(GEADD, tmpshp.userid, tmpshp.shipno, &tmpshp);
					} else {
						prfmsg(NEW4, shipclass[type].typename);
					}
				} else {
					prfmsg(NEW16, waruptr->noships);
				}
			} else {
				prfmsg(NEW2);
			}
		} else if (sameas(margv[1], "shield")) {
			type = atoi(margv[2]);
			if (type <= 0 || type > TOPSHIELD) {
				prfmsg(NEW12);
			} else if (warsptr->shieldtype == type) {
				prfmsg(NEW15S, type);
			} else if (shipclass[warsptr->shpclass].max_shlds >= type) {
				ctype = warsptr->shieldtype;
				delta = (shieldprice[ctype - 1] - (shieldprice[ctype - 1] / 3L));

				if (delta > 0) {
					prfmsg(NEW19, l2as(delta));
					outprfge(FLT_NONE, usrnum);
				} else {
					delta = 0;
				}

				delta = shieldprice[type - 1] - delta;
				credit = 0;

				if (delta < 0) {
					credit = (delta * -1);
					fee = credit / 50L;
					credit = credit - fee;
					if (credit < 0)
						credit = 0;
					if (waruptr->cash > ULCAP - credit) {
						credit = ULCAP - waruptr->cash;
						prfmsg(TOORICH, spr("%lu", ULCAP));
					}
					prfmsg(NEW18, l2as(fee), l2as(credit));
					outprfge(FLT_NONE, usrnum);
					delta = 0;
				}

				if (delta < 1000 && delta > 0) {
					prfmsg(NEW17);
					outprfge(FLT_NONE, usrnum);
					delta = 1000;
				}

				if (delta <= waruptr->cash) {
					waruptr->cash -= delta;
					waruptr->cash += credit;
					warsptr->shieldtype = type;
					if (delta == 0) {
						prfmsg(NEW13, type);
					} else {
						prfmsg(NEW7, l2as(delta), type);
					}
				} else {
					prfmsg(NEW8, type);
				}
			} else {
				prfmsg(NEW6, type);
			}
		} else if (sameas(margv[1], "phaser")) {
			type = atoi(margv[2]);
			if (type <= 0 || type > TOPPHASOR) {
				prfmsg(NEW12);
			} else if (warsptr->phasrtype == type) {
				prfmsg(NEW15P, type);
			} else if (shipclass[warsptr->shpclass].max_phasr >= type) {
				ctype = warsptr->phasrtype;
				delta = (phaserprice[ctype - 1] - (phaserprice[ctype - 1] / 3L));

				if (delta > 0) {
					prfmsg(NEW29, l2as(delta));
					outprfge(FLT_NONE, usrnum);
				} else {
					delta = 0;
				}

				delta = phaserprice[type - 1] - delta;
				credit = 0;

				if (delta < 0) {
					credit = (delta * -1);
					fee = credit / 50L;
					credit = credit - fee;
					if (credit < 0)
						credit = 0;
					if (waruptr->cash > ULCAP - credit) {
						credit = ULCAP - waruptr->cash;
						prfmsg(TOORICH, spr("%lu", ULCAP));
					}
					prfmsg(NEW28, l2as(fee), l2as(credit));
					outprfge(FLT_NONE, usrnum);
					delta = 0;
				}

				if (delta < 1000 && delta > 0) {
					prfmsg(NEW17);
					outprfge(FLT_NONE, usrnum);
					delta = 1000;
				}

				if (delta <= waruptr->cash) {
					waruptr->cash -= delta;
					waruptr->cash += credit;
					warsptr->phasrtype = type;
					if (delta == 0) {
						prfmsg(NEW14, type);
					} else {
						prfmsg(NEW10, l2as(delta), type);
					}
				} else {
					prfmsg(NEW11, type);
				}
			} else {
				prfmsg(NEW9, type);
			}
		} else {
			prfmsg(FORMAT, "NEW");
		}
	} else {
		prfmsg(NEW5);
	}
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** SYSOP commands                                                        **
**************************************************************************/

void FUNC cmd_sysop(void)
{
	int i, j;
	unsigned long amt;
	int gotone;
	int count, class, clscnt;
	WARSHP *ptr;

#ifdef PHARLAP
	if ((!syscmds) || (sysonly && !(hasmkey(SYSKEY)))) {
#else
	if ((!syscmds) || (sysonly && !(usrptr->flags&ISYSOP))) {
#endif
		prfmsg(INVCMD);
		outprfge(FLT_NONE, usrnum);
		return;
	}
	if (sameas("factions",margv[1]) && margc == 2) {
		for (i = 0; i < 8; ++i) {
			prfmsg(FACNAME0 + i);
			prf("... %d\r",waruptr->factions[i]);
		}
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (sameas("help",margv[1]) && margc == 2) {
		setmbk(gehlpmb);
		prfmsg(gehlp[37].helptxt);
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (sameas("nebseed",margv[1]) && margc == 2) {
		prf("\rNebula seed: %s\r",spr("%lu",nebseed));
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (sameas("fillslots",margv[1]) && margc == 2) {
		count = 0;
		for (j = nterms; j < nships; ++j) {
			ptr = warshpoff(j);
			if (ptr->status == GESTAT_AVAIL) {
				clscnt = j - nterms;
				class = -1;
				for (i = 0; i < tot_classes; ++i) {
					if (shipclass[i].max_type == CLASSTYPE_CYBORG ||
						shipclass[i].max_type == CLASSTYPE_DROID) {
						if (clscnt < shipclass[i].tot_to_create) {
							class = i;
							break;
						}
						clscnt -= shipclass[i].tot_to_create;
					}
				}
				if (class > -1 && shipclass[class].init_func != NULL) {
					(*(shipclass[class].init_func))(ptr, j, class);
					++count;
				}
			}
		}
		prf("%d NPC slots filled.\r",count);
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (sameas("get",margv[1]) && margc == 4) {
		if (margc == 4) {
			for (i=0; i < NUMITEMS; ++i) {
				if (sameto(kwrd[i],margv[3])) {
					if ((amt = atol(margv[2])) > 0) {
						warsptr->items[i] += amt;
						sprintf(gechrbuf,"%ld",amt);
						prfmsg(SYSGET,gechrbuf,item_name[i]);
						outprfge(FLT_NONE, usrnum);
						return;
					}
				}
			}
		}
	} else if (sameas("kill",margv[1]) && margc == 3) {
		rstrin();
		gotone = FALSE;
		for (othusn=0; othusn < nships; othusn++) {
			if (sameto(margv[2],warshpoff(othusn)->userid)) {
				warshpoff(othusn)->damage = 101;
				prfmsg(SYSKILL,warshpoff(othusn)->userid);
				outprfge(FLT_NONE, usrnum);
				gotone = TRUE;
			}
		}
		if (!gotone) {
			prfmsg(SYSKILN);
			outprfge(FLT_NONE, usrnum);
		}
		return;
	} else if (sameas("cash",margv[1]) && margc == 3) {
		long delta;
		unsigned long amount;

		delta = atol(margv[2]);
		if (delta < 0L) {
			amount = (unsigned long)(-(delta + 1L)) + 1UL;
			if (amount > waruptr->cash)
				waruptr->cash = 0;
			else
				waruptr->cash -= amount;
		} else {
			amount = (unsigned long)delta;
			if (amount > ULCAP - waruptr->cash)
				waruptr->cash = ULCAP;
			else
				waruptr->cash += amount;
		}
		sprintf(gechrbuf,"%ld",delta);
		prfmsg(SYSCASH,gechrbuf);
		outprfge(FLT_NONE, usrnum);
		return;
	} else if (sameas("midnight",margv[1]) && margc == 2) {
		gemidnighta();
		return;
	} else if (sameas("teamdump",margv[1]) && margc == 2) {
		prfmsg(SYSTEAM);
		for (i = 0; i < MAXTEAMS; ++i) {
			if (teamtab[i].teamcode == 0)
				continue;
			sprintf(gechrbuf,"|%5ld|%-30s|%5d|%10u|%-10s|%-10s|\r\n",
				teamtab[i].teamcode,
				teamtab[i].teamname,
				teamtab[i].teamcount,
				teamtab[i].teamdeldate,
				teamtab[i].password,
				teamtab[i].secret);
			prf(gechrbuf);
			outprfge(FLT_NONE, usrnum);
		}
		return;
	} else if (sameas("goto",margv[1]) && margc == 4) {
		if (margc == 4) {
			i = atoi(margv[2]);
			j = atoi(margv[3]);
			if (abs(i) <= univmax && abs(j) <= univmax) {
				setsect(warsptr);
				if (warsptr->where != 1)
					warsptr->where = 0;
				warsptr->hostile = 0;
				warsptr->coord.xcoord = (double)i + .5;
				warsptr->coord.ycoord = (double)j + .5;

				plnum = 0;
				getplanetdat(usrnum);
				prfmsg(MOVE1,
						(innebula(xsect,ysect) ? CLR_GREEN2 "nebula" : "sector"),
						xsect, ysect,
						(innebula(i,j) ? CLR_GREEN2 "nebula" : "sector"),
						i, j);
				outprfge(FLT_NONE, usrnum);
				return;
			}
		}
	} else if (sameas("class",margv[1]) && margc == 3) {
		i = atoi(margv[2]) - 1;
		if (VALID_SHPCLASS(i) && shipclass[i].max_type != CLASSTYPE_NONE) {
			warsptr->shpclass = i;
			warsptr->topspeed = shipclass[warsptr->shpclass].max_warp;
			prfmsg(SYSCLS,shipclass[warsptr->shpclass].typename);
			outprfge(FLT_NONE, usrnum);
			return;
		}
	} else if (sameas("shieldtype",margv[1]) && margc == 3) {
		i = atoi(margv[2]);
		if (i >= 0 && i < 255) {
			warsptr->shieldtype = i;
			prfmsg(NEW7,"0",warsptr->shieldtype);
			outprfge(FLT_NONE, usrnum);
			return;
		}
	} else if (sameas("phasertype",margv[1]) && margc == 3) {
		i = atoi(margv[2]);
		if (i >= 0 && i < 255) {
			warsptr->phasrtype = i;
			prfmsg(NEW10,"0",warsptr->phasrtype);
			outprfge(FLT_NONE, usrnum);
			return;
		}
	} else if (sameas("maint",margv[1])) {
		if (margc >= 3 && sameas("now",margv[2])) {
			fullrepair(warsptr);
			prfmsg(MAINT7);
			outprfge(FLT_NONE,usrnum);
			return;
		} else {
			if (warsptr->cantexit > 0) {
				prfmsg(MAINT9);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (!repair_needed(warsptr))
				prfmsg(MAINT13);
			else {
				warsptr->repair = 1;
				prfmsg(MAINT5,repair_eta(warsptr));
			}
		}
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas("unjam",margv[1])) {
		warsptr->jam_sev = (byte)0;
		warsptr->jam_time = (byte)0;
		prfmsg(JAMMER5);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas("list",margv[1])) {
		if (margc == 2)
			i = 0;
		else
			i = atoi(margv[2]);
		if (i < 0)
			i = 0;

		prfmsg(SYSLST);
		if (nships > i+50)
			j = i+50;
		else
			j = nships;

		for (; i<j; ++i) {
			ptr = warshpoff(i);
			if (ptr->status != GESTAT_AVAIL) {
				setsect(ptr);
				prf("%3d %-20s %5d %5d %6d %4d %7d %5d %5d %4d\r",i,username(ptr),xsect,ysect,(int)(ptr->damage),(int)(ptr->tick),(int)(ptr->cybmine),(int)(ptr->holdcourse),(int)(ptr->cybupdate),(int)(ptr->lastfired));
			}
		}
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas("classlist",margv[1])) {
		prfmsg(SYSCLL);
		for (i=0; i<tot_classes; ++i) {
			if (shipclass[i].max_type != CLASSTYPE_NONE) {
				prf("%3d %-34s %5d %10d \r",i+1,
					shipclass[i].typename,
					shipclass[i].cybs_can_att,
					shipclass[i].noclaim);
			}
		}
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas("cybpause",margv[1]) && margc == 3) {
		i = atoi(margv[2]);
		prfmsg(SYSCYB,i);
		outprfge(FLT_NONE,usrnum);
		cybhaltflg = i;
		return;
	} else if (sameas("multiply",margv[1]) && (margc > 1 && margc < 4)) {
		if (warsptr->where < 10) {
			prfmsg(ADMIN1);
			outprfge(FLT_NONE,usrnum);
			return;
		}
		if (!load_orbit_planet(usrnum))
			return;
		if (plptr->items[0].qty > 0 && plptr->userid[0] != 0) {
			if (margc == 3)
				j = atoi(margv[2]);
			else
				j = 1;
			if (j > 50)
				j = 50;			/* be nice to system resources */
			for (i=0; i<j; ++i) {
				multiply(TRUE);
			}
			gesdb(GEUPDATE,(PKEY *)&planet,(GALSECT *)&planet);
			prfmsg(SYSPOP,j);
			outprfge(FLT_NONE,usrnum);
			return;
		}
		prfmsg(SYSNP);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas("orbit",margv[1]) && (margc == 3)) {
		if (warsptr->where >= 10) {
			prfmsg(ORBIT3);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		plnum = (atoi(margv[2]));

		if (plnum <= MAXPLANETS && plnum > 0) {
			i = getplanetdat(usrnum);
			if (i) {
				if (plptr->type == PLTYPE_WORM) {
					prfmsg(ORBIT0);
					outprfge(FLT_NONE,usrnum);
					return;
				}
				if (strlen(plptr->name) == 0) {
					prfmsg(ORBIT1N,plnum);
				} else {
					prfmsg(ORBIT1,plnum,plptr->name);
				}
				outprfge(FLT_NONE,usrnum);
				sprintf(gechrbuf,"%lu (%d)",plptr->timestamp >> 4,(int)plptr->timestamp & 0xF);
				prf("Planet timestamp: %s\r",gechrbuf);
				outprfge(FLT_NONE,usrnum);
				warsptr->where = 10 + plnum;
				warsptr->speed = 0;
				warsptr->speed2b = 0;
				return;
			} else {
				prfmsg(NOPLNT);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		} else {
			prfmsg(NOPLNT);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	} else if (sameas("assigncybs",margv[1]) && margc == 2) {
		assign_cybs(usrnum,0);
		prfmsg(SYSACY);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas("fill",margv[1])) {
		if (margc == 2)
			amt = ULCAP;
		if (margc == 3)
			amt = atol(margv[2]);
		for (i = 0; i < NUMITEMS; ++i)
			warsptr->items[i] = amt;
		return;
	} else {
		prfmsg(FORMAT,"SYS");
		outprfge(FLT_NONE,usrnum);
	}
}

/**************************************************************************
** Rename the ship command..                                             **
**************************************************************************/

void FUNC cmd_rename(void)
{
	if (margc >= 2) {
		rstrin();
		strncpy(warsptr->shipname,margv[1],19);
		warsptr->shipname[19] = 0;
		if (warsptr->shipname[0] == 0)
			prfmsg(RENAME1O);
		else
			prfmsg(RENAME1,warsptr->shipname);
	} else {
		warsptr->shipname[0] = 0;
		prfmsg(RENAME1O);
	}
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Self Destruct                                                         **
**************************************************************************/

void FUNC cmd_destruct(void)
{
	if (!neutral(&warsptr->coord)) {
		prfmsg(SELFD1);
		outprfge(FLT_NONE, usrnum);
		if (warsptr->cloak > 0 && warsptr->cloak != 3) {
			warsptr->cloak = 3;
			prfmsg(CLOKOFF);
			outprfge(FLT_NONE, usrnum);
		}
		warsptr->destruct = (byte)COUNTDOWN;
		return;
	}
	prfmsg(SELFD1A);
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Abort Self Destruct                                                   **
**************************************************************************/

void FUNC cmd_abort(void)
{
	if (warsptr->destruct > (byte)0) {
		if (warsptr->destruct < 10) {
			if (warsptr->shipname[0] == 0)
				prfmsg(SELFD4AO, warsptr->userid);
			else
				prfmsg(SELFD4A, warsptr->shipname);
			outrange(FLT_NONE, &warsptr->coord);
		}
		prfmsg(SELFD4);
		warsptr->destruct = (byte)0;
	} else {
		prfmsg(SELFD5);
	}
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Lock command...                                                       **
**************************************************************************/

void FUNC cmd_lock(void)
{
	int shpnum;
	int oldlock;
	char lockltr;
	byte nebmask;
	WARSHP *wptr;

	if (margc == 1) {
		if (warsptr->lock >= 0 && warsptr->lock < nships && ingegame(warsptr->lock)) {
			prfmsg(LOCK04, shpltr(warsptr->lock, usrnum));
			outprfge(FLT_NONE, warsptr->lock);
		}
		warsptr->lock = -1;
		warsptr->track_grace = 0;
		prfmsg(LOCK01);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (margv[1][0] == '@') {
		prfmsg(NOSHIP);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->cloak > 0) {
		prfmsg(PCLOKUP, "Scanner lock is");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (warsptr->jam_sev > (byte)7 || (warsptr->jam_sev > (byte)2 && gernd() % (9 - (int)warsptr->jam_sev) == 0)) {
		prfmsg(JAMMER4W);
		outprfge(FLT_NONE, usrnum);
		return;
	}

	nebmask = (byte)innebula(coord1(warsptr->coord.xcoord), coord1(warsptr->coord.ycoord));
	oldlock = warsptr->lock;
	shpnum = findshp(margv[1], 1);

	if (shpnum >= 0) {
		wptr = warshpoff(shpnum);
		if (!isvisible(warsptr, wptr)) {
			if (nebmask)
				prfmsg(SCAN27);
			else
				prfmsg(NOSHIP);
			outprfge(FLT_NONE, usrnum);
			return;
		}
		warsptr->lock = shpnum;
		warsptr->track_grace = LOCKGRACE;
		if (warshpoff(shpnum)->status == GESTAT_AUTO
			&& shipclass[warshpoff(shpnum)->shpclass].max_type == CLASSTYPE_CYBORG
			&& warshpoff(shpnum)->cybmine == 255) {
			warshpoff(shpnum)->cybmine = usrnum;	/* engage user */
			warshpoff(shpnum)->track_grace = CYBGRACE;
			warshpoff(shpnum)->tick = 2;		/* do it fast */
			warshpoff(shpnum)->npcmsg = 255;	/* reset annoy msg tracking */
		}
		if (warshpoff(shpnum)->status == GESTAT_USER) {
			if (warshpoff(shpnum)->shipname[0] == 0)
				prfmsg(LOCK02O, username(warshpoff(shpnum)));
			else
				prfmsg(LOCK02, warshpoff(shpnum)->shipname, username(warshpoff(shpnum)));
		} else {
			prfmsg(LOCK02N, warshpoff(shpnum)->shipname, username(warshpoff(shpnum)));
		}
		outprfge(FLT_NONE, usrnum);
		if (oldlock != shpnum) {
			lockltr = shpltr(shpnum, usrnum);
			if (lockltr == '?')
				prfmsg(LOCK03N);
			else
				prfmsg(LOCK03, lockltr);
			outprfge(FLT_NONE, shpnum);
		}
	} else {
		if (nebmask)
			prfmsg(SCAN27);
		else
			prfmsg(NOSHIP);
		outprfge(FLT_NONE, usrnum);
	}
}

/**************************************************************************
** navigate command...                                                   **
**************************************************************************/

void FUNC cmd_navigate(void)
{
	COORD tmp;
	int x, y;

	if (margc == 3) {
		x = atoi(margv[1]);
		y = atoi(margv[2]);
	} else if (margc == 1) {
		x = 0;
		y = 0;
	} else {
		prfmsg(FORMAT,"NAVIGATE");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (abs(x) > univmax || abs(y) > univmax) {
		prfmsg(FORMAT,"NAVIGATE");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	tmp.xcoord = x;
	tmp.ycoord = y;

	tmp.xcoord += .5;
	tmp.ycoord += .5;

	ddistance = cdistance(&(warsptr->coord), &tmp) * 10000;
	bearing = cbearing(&(warsptr->coord), &tmp, warsptr->heading);

	sprintf(gechrbuf,"NAV from X:%f Y:%f",warsptr->coord.xcoord,warsptr->coord.ycoord);
	logthis(gechrbuf);

	sprintf(gechrbuf,"NAV to X:%f Y:%f",tmp.xcoord,tmp.ycoord);
	logthis(gechrbuf);

	sprintf(gechrbuf,"Dist: %f, Bearing: %d",ddistance,bearing);
	logthis(gechrbuf);

	prfmsg(NAV01,x,y,bearing,spr("%ld",(long)ddistance));
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** who command...                                                        **
**************************************************************************/

void FUNC cmd_who(void)
{
	int zothusn;
	WARSHP *wptr;

	for (zothusn=0; zothusn < nterms; zothusn++)
		if (ingegame(zothusn)) {
			wptr = warshpoff(zothusn);
			if (wptr->status == GESTAT_USER)
				prf("%s ", username(wptr));
		}
	prf("\r");
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Set Command                                                           **
**************************************************************************/

void FUNC cmd_set(void)
{
	int invalid = FALSE;
	byte msgfilter;

	if (margc < 2 || margc > 4) {
		prfmsg(FORMAT,"SET");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (sameas(margv[1],"scan")) {
		if (margc != 3)
			invalid = TRUE;
		else if (sameas(margv[2],"simple"))
			waruptr->options[SCANOPTS] = SIMPLE;
		else if (sameas(margv[2],"full"))
			waruptr->options[SCANOPTS] = FULL;
		else if (sameas(margv[2],"fullnames"))
			waruptr->options[SCANOPTS] = FULLNAMES;
		else if (sameas(margv[2],"fullextra"))
			waruptr->options[SCANOPTS] = FULLEXTRA;
		else if (sameas(margv[2],"nomap"))
			waruptr->options[SCANOPTS] = NOMAP;
		else
			invalid = TRUE;
	} else if (sameas(margv[1],"display")) {
		msgfilter = waruptr->options[MSG_FILTER];
		if (margc != 4)
			invalid = TRUE;
		else if (sameas(margv[2],"cybs")) {
			msgfilter &= ~MSGF_CYBS_MASK;
			if (sameas(margv[3],"on"))
				msgfilter |= 0x00;
			else if (sameas(margv[3],"battle"))
				msgfilter |= 0x01;
			else if (sameas(margv[3],"approach"))
				msgfilter |= 0x02;
			else if (sameas(margv[3],"off"))
				msgfilter |= 0x03;
			else
				invalid = TRUE;
		} else if (sameas(margv[2],"distress")) {
			msgfilter &= ~MSGF_DISTRESS;
			if (sameas(margv[3],"on"))
				msgfilter |= 0x00;
			else if (sameas(margv[3],"off"))
				msgfilter |= MSGF_DISTRESS;
			else
				invalid = TRUE;
		} else if (sameas(margv[2],"beacon")) {
			msgfilter &= ~MSGF_BEACON;
			if (sameas(margv[3],"on"))
				msgfilter |= 0x00;
			else if (sameas(margv[3],"off"))
				msgfilter |= MSGF_BEACON;
			else
				invalid = TRUE;
		} else if (sameas(margv[2],"hail")) {
			msgfilter &= ~MSGF_HAIL;
			if (sameas(margv[3],"on"))
				msgfilter |= 0x00;
			else if (sameas(margv[3],"off"))
				msgfilter |= MSGF_HAIL;
			else
				invalid = TRUE;
		} else if (sameas(margv[2],"entry")) {
			msgfilter &= ~MSGF_ENTRY_MASK;
			if (sameas(margv[3],"on"))
				msgfilter |= 0x00;
			else if (sameas(margv[3],"reduced"))
				msgfilter |= 0x20;
			else if (sameas(margv[3],"off"))
				msgfilter |= 0x40;
			else
				invalid = TRUE;
		} else if (sameas(margv[2],"ship")) {
			msgfilter &= ~MSGF_SHIP;
			if (sameas(margv[3],"on"))
				msgfilter |= 0x00;
			else if (sameas(margv[3],"off"))
				msgfilter |= MSGF_SHIP;
			else
				invalid = TRUE;
		} else
			invalid = TRUE;
		if (!invalid)
			waruptr->options[MSG_FILTER] = msgfilter;
	} else if (sameas(margv[1],"?")) {
		invalid = 2;
		if (waruptr->options[SCANOPTS] == SIMPLE)
			prfmsg(SETOPT,"scan","simple");
		else if (waruptr->options[SCANOPTS] == FULL)
			prfmsg(SETOPT,"scan","full");
		else if (waruptr->options[SCANOPTS] == FULLNAMES)
			prfmsg(SETOPT,"scan","fullnames");
		else if (waruptr->options[SCANOPTS] == FULLEXTRA)
			prfmsg(SETOPT,"scan","fullextra");
		else if (waruptr->options[SCANOPTS] == NOMAP)
			prfmsg(SETOPT,"scan","nomap");
		else
			prfmsg(SETOPT,"scan","undefined");
		switch (waruptr->options[MSG_FILTER] & MSGF_CYBS_MASK) {
			case 0:
				prfmsg(SETOPT2,"display","cybs","on");
				break;
			case 1:
				prfmsg(SETOPT2,"display","cybs","battle");
				break;
			case 2:
				prfmsg(SETOPT2,"display","cybs","approach");
				break;
			default:
				prfmsg(SETOPT2,"display","cybs","off");
				break;
		}
		if (waruptr->options[MSG_FILTER] & MSGF_DISTRESS)
			prfmsg(SETOPT2,"display","distress","off");
		else
			prfmsg(SETOPT2,"display","distress","on");
		if (waruptr->options[MSG_FILTER] & MSGF_BEACON)
			prfmsg(SETOPT2,"display","beacon","off");
		else
			prfmsg(SETOPT2,"display","beacon","on");
		if (waruptr->options[MSG_FILTER] & MSGF_HAIL)
			prfmsg(SETOPT2,"display","hail","off");
		else
			prfmsg(SETOPT2,"display","hail","on");
		switch (waruptr->options[MSG_FILTER] & MSGF_ENTRY_MASK) {
			case 0x00:
				prfmsg(SETOPT2,"display","entry","on");
				break;
			case 0x20:
				prfmsg(SETOPT2,"display","entry","reduced");
				break;
			default:
				prfmsg(SETOPT2,"display","entry","off");
				break;
		}
		if (waruptr->options[MSG_FILTER] & MSGF_SHIP)
			prfmsg(SETOPT2,"display","ship","off");
		else
			prfmsg(SETOPT2,"display","ship","on");
		prf("\r\r");
	} else
		invalid = TRUE;

	if (invalid == TRUE)
		prfmsg(FORMAT,"SET");
	if (invalid == FALSE) {
		if (margc == 4)
			prfmsg(SETOPT2,margv[1],margv[2],margv[3]);
		else
			prfmsg(SETOPT,margv[1],margv[2]);
		prf("\r\r");
	}
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Team Command                                                          **
**   Team join nnnnnn pppppppppp                                         **
**   Team score                                                          **
**   Team unjoin                                                         **
**   Team start nnnnnn ssssssssss pppppppppp <team name>                 **
**        nnnnnn is teamcode                                             **
**        ssssssssss is the secret password for the founder              **
**        pppppppppp is the public password for team members             **
**   Team members                                                        **
**   Team kick ssssssssss userid                                         **
**   Team newpass ssssssssss pppppppppp                                  **
**   Team newname ssssssssss <team name>                                 **
**                                                                       **
**   Add logic to compare if the team member got kicked off and tell     **
**   him in a mail message. nice                                         **
**                                                                       **
**************************************************************************/

void FUNC cmd_team(void)
{
	int i, j, next;
	unsigned int tmcount[MAXTEAMS];
	unsigned long tmscore[MAXTEAMS];
	int tmflag[MAXTEAMS];
	char oldteamname[31];
	unsigned int olddel;
	long highscore;
	int highpos;
	TEAM tmp;
	int temptab[MAXTEAMS];
	if (margc < 2) {
		prfmsg(FORMAT,"TEAM");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (sameas(margv[1],"join")) {
		if (waruptr->teamcode > 0) {
			for (i=0; i<MAXTEAMS; ++i) {
				if (waruptr->teamcode == teamtab[i].teamcode
					&& teamtab[i].teamname[0] != '@')
					break;
			}
			if (i >= MAXTEAMS) {
				waruptr->teamcode = 0;
				geudb(GEUPDATE,waruptr->userid,waruptr);
			}
		}

		/* got enough parameters */
		if (margc != 4) {
			prfmsg(FORMAT,"TEAM");
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* verify that the teamcode is valid */
		strcpy(gechrbuf,margv[2]);

		if (strlen(gechrbuf) != 5) {
			prfmsg(FORMAT,"TEAM");
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* are they all digits */
		for (i=0; i<5; ++i) {
			if (gechrbuf[i] < '0' || gechrbuf[i] > '9') {
				prfmsg(FORMAT,"TEAM");
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}

		tmp.teamcode = atol(gechrbuf);

		if (waruptr->teamcode > 0) {
			if (waruptr->teamcode == tmp.teamcode)
				prfmsg(TEAMALR2);
			else
				prfmsg(TEAMALRD);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* verify that this is an actual team */
		for (i=0; i<MAXTEAMS; ++i) {
			if (tmp.teamcode == teamtab[i].teamcode
				&& teamtab[i].teamname[0] != '@') {
				break;
			}
		}

		if (i >= MAXTEAMS) {
			prfmsg(TEAMBAD);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* verify that the password is correct */
		if (!sameas(teamtab[i].password,margv[3])) {
			prfmsg(TEAMBADP);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* verify that the team has room */
		if (teamtab[i].teamcount >= team_max) {
			prfmsg(TEAMBIG);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* add the teamcode to this user's record */
		waruptr->teamcode = tmp.teamcode;
		geudb(GEUPDATE,waruptr->userid,waruptr);

		/* update the team count */
		teamtab[i].teamcount++;

		/* update the disk copy of team database */
		update_team_tab();

		/* tell user that team has been joined */
		prfmsg(TEAMJOIN,teamtab[i].teamname);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas(margv[1],"score")) {
		/* sort the table */
		highpos = 0;
		setbtv(gebb5);

		for (i=0; i < MAXTEAMS; ++i) {
			temptab[i] = 0;
			tmcount[i] = 0;
			tmscore[i] = 0L;
			tmflag[i] = 0; /* flag the records for sorting */
		}

		if (qlobtv(0)) {
			do {
				gcrbtv(&tmpusr,0);

				if (tmpusr.teamcode > 0) {
					for (i=0; i<MAXTEAMS; ++i) {
						if (teamtab[i].teamcode == tmpusr.teamcode
							&& teamtab[i].teamname[0] != '@') {
							++tmcount[i];
							tmscore[i] += (tmpusr.plscore + tmpusr.klscore);
							break;
						}
					}
				}
			} while (qnxbtv());
		}

		for (i=0; i < MAXTEAMS; ++i) {
			if (tmcount[i] > 0)
				tmscore[i] = (tmscore[i] / (long)tmcount[i]) + ((long)tmcount[i] * (long)teambonus);
		}

		/* sort the records */
		for (i=0; i < MAXTEAMS; ++i) {
			highscore = 0;
			highpos = 0;
			for (j=0; j < MAXTEAMS; ++j) {
				if (tmscore[j] >= highscore && tmflag[j] != 1) {
					highscore = tmscore[j];
					highpos = j;
				}
			}
			tmflag[highpos] = 1; /* take it out of the running */
			temptab[i] = highpos;
		}

		prfmsg(TEAMHDR);
		for (i=0; i<MAXTEAMS; ++i) {
			j = temptab[i];
			if (teamtab[j].teamcode > 0
				&& teamtab[j].teamname[0] != '@') {
				prf("%-6s %-38s%7u %15lu\r",
					spr("%ld",teamtab[j].teamcode),
					teamtab[j].teamname,
					tmcount[j],
					tmscore[j]);
				outprfge(FLT_NONE,usrnum);
			}
		}
		prfmsg(TEAMTLR);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas(margv[1],"unjoin")) {
		if (waruptr->teamcode > 0) {
			for (i=0; i<MAXTEAMS; ++i) {
				if (waruptr->teamcode == teamtab[i].teamcode
					&& teamtab[i].teamname[0] != '@')
					break;
			}
			if (i >= MAXTEAMS) {
				waruptr->teamcode = 0;
				geudb(GEUPDATE,waruptr->userid,waruptr);
			}
		}

		if (waruptr->teamcode > 0) {
			/* verify that this is still a good team */
			for (i=0; i<MAXTEAMS; ++i) {
				if (waruptr->teamcode == teamtab[i].teamcode
					&& teamtab[i].teamname[0] != '@') {
					prfmsg(TEAMUNJN,teamname(waruptr));
					outprfge(FLT_NONE,usrnum);
					waruptr->teamcode = 0;
					geudb(GEUPDATE,waruptr->userid,waruptr);

					/* update the team count */
					teamtab[i].teamcount--;
					if (teamtab[i].teamcount > 65000U) /* roll over */
						teamtab[i].teamcount = 0;

					if (teamtab[i].teamcount == 0) {
						strcpy(teamtab[i].teamname,"@DELETED@");
						teamtab[i].teamdeldate = cofdat(today());
						teamtab[i].password[0] = 0;
						teamtab[i].secret[0] = 0;
					}

					/* update the disk copy of team database */
					update_team_tab();

					return;
				}
			}
			prfmsg(TEAMNOT);
			outprfge(FLT_NONE,usrnum);
		} else {
			prfmsg(TEAMNOT);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	} else if (sameas(margv[1],"start")) {
		if (waruptr->teamcode > 0) {
			for (i=0; i<MAXTEAMS; ++i) {
				if (waruptr->teamcode == teamtab[i].teamcode
					&& teamtab[i].teamname[0] != '@')
					break;
			}
			if (i >= MAXTEAMS) {
				waruptr->teamcode = 0;
				geudb(GEUPDATE,waruptr->userid,waruptr);
			}
		}

		if (waruptr->teamcode > 0) {
			prfmsg(TEAMALRD);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		if (margc < 6) {
			prfmsg(FORMAT,"TEAM");
			outprfge(FLT_NONE,usrnum);
			return;
		}

		olddel = 65535U;
		for (next=0; next<MAXTEAMS; ++next) {
			if (teamtab[next].teamcode == 0)
				break;
		}

		if (next >= MAXTEAMS) {
			next = -1;
			for (i=0; i<MAXTEAMS; ++i) {
				if (teamtab[i].teamname[0] == '@' && teamtab[i].teamdeldate <= olddel) {
					olddel = teamtab[i].teamdeldate;
					next = i;
				}
			}

			if (next < 0) {
				prfmsg(TOOMANY,MAXTEAMS);
				outprf(usrnum);
				return;
			}
		}

		/* verify that the teamcode is valid */
		strcpy(gechrbuf,margv[2]);
		if (strlen(gechrbuf) != 5) {
			prfmsg(TEAMBAD);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* are they all digits */
		for (i=0; i<5; ++i) {
			if (gechrbuf[i] < '0' || gechrbuf[i] > '9') {
				prfmsg(TEAMBAD);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}

		tmp.teamcode = atol(gechrbuf);

		if (tmp.teamcode <= 0) {
			prfmsg(TEAMBAD);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* save off the passwords before we rstrin */
		strncpy(tmp.secret,margv[3],10);
		strncpy(tmp.password,margv[4],10);
		tmp.secret[10] = 0;
		tmp.password[10] = 0;

		rstrin();
		strncpy(tmp.teamname, margv[5], 30);
		tmp.teamname[30] = 0;
		if (strlen(tmp.teamname) < 5) {
			prfmsg(TEAMBNAM);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		if (!((tmp.teamname[0] >= 'A' && tmp.teamname[0] <= 'Z')
			|| (tmp.teamname[0] >= 'a' && tmp.teamname[0] <= 'z'))) {
			prfmsg(TEAMNINV);
			outprfge(FLT_NONE,usrnum);
			return;
		}
		tmp.teamcount = 1;

		/* check to see that this team does not already exist */
		for (i=0; i<MAXTEAMS; ++i) {
			if (tmp.teamcode == teamtab[i].teamcode) {
				if (teamtab[i].teamname[0] == '@')
					prfmsg(TEAMDEAC);
				else
					prfmsg(TEAMEXST);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (teamtab[i].teamname[0] != '@'
				&& sameas(tmp.teamname,teamtab[i].teamname)) {
				prfmsg(TEAMEXST);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}

		/* add the team to the team database */
		teamtab[next].teamcode = tmp.teamcode;
		strncpy(teamtab[next].teamname, tmp.teamname, 30);
		teamtab[next].teamname[30] = 0;
		teamtab[next].teamcount = tmp.teamcount;
		teamtab[next].teamdeldate = 0;
		strncpy(teamtab[next].password, tmp.password, 10);
		teamtab[next].password[10] = 0;
		strncpy(teamtab[next].secret, tmp.secret, 10);
		teamtab[next].secret[10] = 0;

		/* add the teamcode to this users record */
		waruptr->teamcode = tmp.teamcode;
		geudb(GEUPDATE,waruptr->userid,waruptr);

		/* update the disk copy of team database */
		update_team_tab();

		/* tell user that team has been created */
		prfmsg(TEAMCRT,tmp.teamname,gechrbuf,tmp.password,tmp.secret);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (sameas(margv[1],"members")) {
		if (waruptr->teamcode > 0) {
			for (i=0; i<MAXTEAMS; ++i) {
				if (waruptr->teamcode == teamtab[i].teamcode
					&& teamtab[i].teamname[0] != '@')
					break;
			}
			if (i >= MAXTEAMS) {
				waruptr->teamcode = 0;
				geudb(GEUPDATE,waruptr->userid,waruptr);
			}
		}

		if (waruptr->teamcode == 0) {
			prfmsg(TEAMNOT);
			outprfge(FLT_NONE,usrnum);
			return;
		}

		setbtv(gebb5);

		j = team_max;
		i = 0;

		if (qeqbtv(&waruptr->teamcode,2)) {
			prfmsg(TEAMMHDR);

			do {
				gcrbtv(&tmpusr,2);
				if (tmpusr.teamcode == waruptr->teamcode) {
					if (i == 0)
						prf("%s",tmpusr.userid);
					else
						prf(", %s",tmpusr.userid);

					outprfge(FLT_NONE,usrnum);
					++i;
				} else {
					break;
				}
			} while (qnxbtv() && i < j);

			prf("\r");
			outprfge(FLT_NONE,usrnum);
			return;
		} else {
			waruptr->teamcode = 0;
			geudb(GEUPDATE,waruptr->userid,waruptr);
			prfmsg(TEAMNOT);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	} else if (sameas(margv[1],"kick")) {
		if (margc < 4) {
			prfmsg(FORMAT,"TEAM");
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* locate this players team in the list */
		for (i=0; i<MAXTEAMS; ++i) {
			if (waruptr->teamcode == teamtab[i].teamcode
				&& teamtab[i].teamname[0] != '@') {
				strncpy(oldteamname,teamtab[i].teamname,30);
				oldteamname[30] = 0;

				/* check to see that the passwords match */
				if (sameas(margv[2],teamtab[i].secret)) {
					/* check to see if its a valid userid */
					rstrin();
					strncpy(tmpusr.userid,margv[3],UIDSIZ);
					tmpusr.userid[UIDSIZ - 1] = 0;
					makhdl(tmpusr.userid);
					if (geudb(GELOOKUP,tmpusr.userid,&tmpusr)) {
						gcrbtv(&tmpusr,0);
						/* check if the player is currently on this team */
						if (tmpusr.teamcode == teamtab[i].teamcode) {
							/* reset the teamcode */
							tmpusr.teamcode = 0;
							/* re-write the users record */
							geudb(GEUPDATE,tmpusr.userid,&tmpusr);

							/* update the team count */
							teamtab[i].teamcount--;
							if (teamtab[i].teamcount > 65000U) /* roll over */
								teamtab[i].teamcount = 0;

							if (teamtab[i].teamcount == 0) {
								strcpy(teamtab[i].teamname,"@DELETED@");
								teamtab[i].teamdeldate = cofdat(today());
								teamtab[i].password[0] = 0;
								teamtab[i].secret[0] = 0;
							}

							/* update the disk copy of team database */
							update_team_tab();

							/* send the guy mail telling him he got kicked off the team */
							clrprf();
							prfmsg(TEAMKYOU,oldteamname,warsptr->userid);
							strcpy(mail.userid,tmpusr.userid);
							strcpy(mail.topic,"Team Membership Revoked");
							sendit();
							clrprf();

							/* tell this user it is done */
							prfmsg(TEAMKICK,tmpusr.userid);
							outprfge(FLT_NONE,usrnum);
							return;
						} else {
							prfmsg(TEAMNTM);
							outprfge(FLT_NONE,usrnum);
						}
					} else {
						prfmsg(TEAMNFND);
						outprfge(FLT_NONE,usrnum);
					}
				} else {
					prfmsg(TEAMBDSC);
					outprfge(FLT_NONE,usrnum);
				}
				return;
			}
		}

		if (waruptr->teamcode > 0) {
			waruptr->teamcode = 0;
			geudb(GEUPDATE,waruptr->userid,waruptr);
		}
		prfmsg(TEAMNOT);
		outprfge(FLT_NONE,usrnum);
	} else if (sameas(margv[1],"newpass")) {
		if (margc < 4) {
			prfmsg(FORMAT,"TEAM");
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* locate this players team in the list */
		for (i=0; i<MAXTEAMS; ++i) {
			if (waruptr->teamcode == teamtab[i].teamcode
				&& teamtab[i].teamname[0] != '@') {
				/* check to see that the passwords match */
				if (sameas(margv[2],teamtab[i].secret)) {
					/* get the password - make sure it is less than 10 char */
					if (strlen(margv[3]) > 10) {
						prfmsg(TEAMBPSS);
						outprfge(FLT_NONE,usrnum);
						return;
					}
					strcpy(teamtab[i].password,margv[3]);
					prfmsg(TEAMNPSS,teamtab[i].password);
					outprfge(FLT_NONE,usrnum);
					update_team_tab();
					return;
				} else {
					prfmsg(TEAMBDSC);
					outprfge(FLT_NONE,usrnum);
				}
				return;
			}
		}

		if (waruptr->teamcode > 0) {
			waruptr->teamcode = 0;
			geudb(GEUPDATE,waruptr->userid,waruptr);
		}
		prfmsg(TEAMNOT);
		outprfge(FLT_NONE,usrnum);
	} else if (sameas(margv[1],"newname")) {
		if (margc < 4) {
			prfmsg(FORMAT,"TEAM");
			outprfge(FLT_NONE,usrnum);
			return;
		}

		/* locate this players team in the list */
		for (i=0; i<MAXTEAMS; ++i) {
			if (waruptr->teamcode == teamtab[i].teamcode
				&& teamtab[i].teamname[0] != '@') {
				/* check to see that the passwords match */
				if (sameas(margv[2],teamtab[i].secret)) {
					/* get the new teamname - make sure it is at least 5 char long */
					rstrin();
					if (strlen(margv[3]) < 5) {
						prfmsg(TEAMBNAM);
						outprfge(FLT_NONE,usrnum);
						return;
					}
					if (!((margv[3][0] >= 'A' && margv[3][0] <= 'Z')
						|| (margv[3][0] >= 'a' && margv[3][0] <= 'z'))) {
						prfmsg(TEAMNINV);
						outprfge(FLT_NONE,usrnum);
						return;
					}
					for (j=0; j<MAXTEAMS; ++j) {
						if (j != i
							&& teamtab[j].teamname[0] != '@'
							&& sameas(margv[3],teamtab[j].teamname)) {
							prfmsg(TEAMEXST);
							outprfge(FLT_NONE,usrnum);
							return;
						}
					}
					strncpy(teamtab[i].teamname,margv[3],30);
					teamtab[i].teamname[30] = 0;
					prfmsg(TEAMNNAM,teamtab[i].teamname);
					outprfge(FLT_NONE,usrnum);
					update_team_tab();
					return;
				} else {
					prfmsg(TEAMBDSC);
					outprfge(FLT_NONE,usrnum);
				}
				return;
			}
		}

		if (waruptr->teamcode > 0) {
			waruptr->teamcode = 0;
			geudb(GEUPDATE,waruptr->userid,waruptr);
		}
		prfmsg(TEAMNOT);
		outprfge(FLT_NONE,usrnum);
	}
	prfmsg(FORMAT,"TEAM");
	outprfge(FLT_NONE,usrnum);
}

char *FUNC teamname(WARUSR *ptr)
{
	int i;

	for (i=0; i<MAXTEAMS; ++i) {
		if (ptr->teamcode == teamtab[i].teamcode
			&& teamtab[i].teamname[0] != '@') {
			return teamtab[i].teamname;
		}
	}
	return NULL;
}

void FUNC cmd_clear(void)
{
	prf("\33[2J\33[0;0H");
	outprfge(FLT_NONE,usrnum);
}

#ifdef DATACMD

/**************************************************************************
** DATA scan helpers                                                     **
**************************************************************************/

static void scan_data1(void)
{
	SCANTAB *sptr;
	WARSHP *wptr;
	int i, j;
	char mask[] = {" %c %d %d %d %d %s %d %d %s %d/%s%s\r"};

	prf("DataScan: Range: %s\r",spr("%6ld",shipclass[warsptr->shpclass].scanrange));

	update_scantab(warsptr, usrnum);

	sptr = &scantab[usrnum];

	prf("Shp Xsect Ysect Xcoord Ycoord Distance Bearing Heading Speed Class\r");

	setsect(warsptr);

	prf(mask,'*',xsect,ysect,xcord,ycord,"0",0,
		(int)warsptr->heading,showarp(warsptr->speed),
		warsptr->shpclass,shipclass[warsptr->shpclass].typename,showupg(warsptr));

	if (warsptr->jam_sev > (byte)2) {
		prf("** Jammed **\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	for (i=0; i<NOSCANTAB; ++i) {
		if (sptr->ship[i].flag == 1) {
			wptr = warshpoff(sptr->ship[i].shipno);

			setsect(wptr);

			j = wptr->shpclass;
			prf(mask,sptr->ship[i].letter,xsect,ysect,xcord,ycord,spr("%ld",(long)(sptr->ship[i].dist)),
				sptr->ship[i].bearing,sptr->ship[i].heading,showarp(sptr->ship[i].speed),
				j,shipclass[j].typename,showupg(wptr));
		}
	}

	outprfge(FLT_NONE,usrnum);
}

static void scan_data2(void)
{
	unsigned i, x, y;

	refresh(warsptr, usrnum);

	setsect(warsptr);
	prf("Datascan: Sector X:%u Y:%u\r",xsect,ysect);

	getsector(&warsptr->coord);

	prf("NPlnts = %d\r",sector.numplan);
	for (i=0; i < sector.numplan; ++i) {
		if (sector.ptab[i].coord.xcoord != 0) {
			x = coord2(sector.ptab[i].coord.xcoord);
			y = coord2(sector.ptab[i].coord.ycoord);
			prf("Pl#%d: Xcoord:%d, Ycoord:%d, Type:%d\r",i+1,x,y,sector.ptab[i].type);
		}
	}
}

void FUNC cmd_data(void)
{
	int i, j;

	if (margc != 3) {
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!sameas(margv[1],"qazwsx")) {
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (sameas(margv[2],"report")) {
		prf("UD1:%s,%u,%u,%u*\r",
			waruptr->userid,
			waruptr->noships,
			waruptr->kills,
			waruptr->planets);
		sprintf(gechrbuf,"%lu",waruptr->score);
		sprintf(gechrbuf2,"%lu",waruptr->cash);
		sprintf(gechrbuf3,"%lu",waruptr->population);
		prf("UD2:%s,%s,%s*\r",gechrbuf,gechrbuf2,gechrbuf3);
		outprfge(FLT_NONE,usrnum);

		setsect(warsptr);

		prf("SD1:%s,%d*\r",
			(warsptr->status == GESTAT_USER && warsptr->shipname[0] == 0 ? warsptr->userid : warsptr->shipname),
			warsptr->shpclass);

		prf("SD2:%s,%s,%d,%d,%d,%d,%s,%s*\r",
			spr("%ld",(long)warsptr->heading),
			spr("%ld",(long)warsptr->speed),
			xsect,ysect,xcord,ycord,
			spr("%ld",(long)warsptr->damage),
			spr("%ld",(long)warsptr->energy));

		prf("SD3:%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*\r",
			spr("%ld",(long)warsptr->phasr),
			warsptr->phasrtype,
			warsptr->kills,
			warsptr->lastfired,
			warsptr->shieldtype,
			warsptr->shieldstat,
			warsptr->shield,
			warsptr->cloak,
			warsptr->tactical,
			warsptr->helm,
			warsptr->where);

		prf("SD4:");
		for (i=0; i<MAXTORPS; ++i)
			prf("T%d:%u-%u,",i,warsptr->ltorps[i].channel,warsptr->ltorps[i].distance);
		prf("*\r");

		prf("SD5:");
		for (i=0; i<MAXMISSL; ++i)
			prf("M%d:%u-%u,",i,warsptr->lmissl[i].channel,warsptr->lmissl[i].distance);
		prf("*\r");

		prf("SD6:");
		for (i=0; i<NUMITEMS; ++i)
			prf("I%d:%s,",i,spr("%ld",warsptr->items[i]));
		prf("*\r");

		j = 0;
		for (i=0; i<MAXDECOY; ++i)
			if (warsptr->decout[i] > 0)
				++j;

		prf("SD7:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*\r",
			j,
			(int)warsptr->jam_time,
			warsptr->kills,
			warsptr->freq,
			warsptr->hostile,
			warsptr->cantexit,
			warsptr->repair,
			warsptr->hypha,
			(int)warsptr->torpcntl,
			(int)warsptr->mislcntl,
			warsptr->destruct,
			warsptr->status);

		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[2],"scan")) {
		scan_data1();
		return;
	}
	if (sameas(margv[2],"sector")) {
		scan_data2();
		return;
	}

	prfmsg(INVCMD);
	outprfge(FLT_NONE,usrnum);
}

#endif

char *FUNC gedots(int numdots)
{
	int i;
	static char dotbuf[41];

	if (numdots > 40)
		numdots = 40;

	for (i=0; i<numdots; ++i)
		dotbuf[i] = '.';
	dotbuf[numdots] = 0;
	return dotbuf;
}

/**************************************************************************
** Spy Command                                                           **
**************************************************************************/

void FUNC cmd_spy(void)
{
	if (warsptr->where < 10) {
		prfmsg(ADMIN1);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!load_orbit_planet(usrnum))
		return;

	if (sameas(plptr->userid, warsptr->userid)) {
		prfmsg(SPY0);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (neutral(&warsptr->coord)) {
		prfmsg(SPY0C);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->items[I_SPY] > 0) {
		warsptr->items[I_SPY]--;
		prfmsg(SPYM1);
		outprfge(FLT_NONE,usrnum);
		strcpy(plptr->spyowner,warsptr->userid);

		setsect(warsptr); /* build PKEY */
		pkey.plnum = plnum;
		gesdb(GEUPDATE, &pkey, (GALSECT *)&planet);

		return;
	} else {
		prfmsg(SPYM0);
	}
	outprfge(FLT_NONE,usrnum);
}

/**************************************************************************
** Jettison helper                                                       **
**************************************************************************/

static void jettison(int item)
{
	unsigned long amt;
	long req;
	int defuse = FALSE;

	if (item == I_MEN || item == I_TROOPS || item == I_SPY) {
		prfmsg(JETT2,item_name[item]);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (item == I_TORPEDO || item == I_MINE)
		defuse = TRUE;

	if (sameas("ALL",margv[1]) > 0L) {
		amt = warsptr->items[item];
		warsptr->items[item] = 0;
		if (defuse == TRUE)
			prfmsg(JETT3D,spr("%lu",amt),item_name[item]);
		else
			prfmsg(JETT3,spr("%lu",amt),item_name[item]);
		outprfge(FLT_NONE,usrnum);
		gepdb(GEUPDATE,warsptr->userid,warsptr->shipno,warsptr);
		return;
	} else if ((req = atol(margv[1])) > 0L) {
		amt = (unsigned long)req;
		if (warsptr->items[item] >= amt) {
			warsptr->items[item] -= amt;
			if (defuse == TRUE)
				prfmsg(JETT3D,spr("%lu",amt),item_name[item]);
			else
				prfmsg(JETT3,spr("%lu",amt),item_name[item]);
			outprfge(FLT_NONE,usrnum);
			gepdb(GEUPDATE,warsptr->userid,warsptr->shipno,warsptr);
			return;
		} else {
			prfmsg(JETT1);
			outprfge(FLT_NONE,usrnum);
		}
	} else {
		prfmsg(FORMAT,"JETTISON");
		outprfge(FLT_NONE,usrnum);
	}
}

/**************************************************************************
** Jettison Command                                                      **
**************************************************************************/

void FUNC cmd_jettison(void)
{
	int i;

	if (margc == 3) {
		for (i=0; i < NUMITEMS; ++i) {
			if (sameto(kwrd[i],margv[2])) {
				jettison(i);
				return;
			}
		}
	}

	prfmsg(FORMAT,"JETTISON");
	outprfge(FLT_NONE,usrnum);
}
