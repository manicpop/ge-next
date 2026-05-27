
/***************************************************************************
 *                                                                         *
 *   GEDROIDS.C                                                            *
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
#include "math.h"
#include "portable.h"
#include "dosface.h"
#endif

#include "majorbbs.h"

#include "gemain.h"


#define GEDROIDS 1

#include "geglobal.h"

/* LOCAL GLOBAL DEFS ****************************************************/

char	droidname[UIDSIZ];
double	dr_topspeed;

static void droid_act_1(WARSHP *ptr, int usrn);
static void droid_act_2(WARSHP *ptr, int usrn);
static void droid_act_3(WARSHP *ptr, int usrn);
static void droid_act_4(WARSHP *ptr, int usrn);
static void droid_act_5(WARSHP *ptr, int usrn);
static void droid_act_6(WARSHP *ptr, int usrn);

/**************************************************************************
** Droid Zygor Loadout Function                                          **
**************************************************************************/

static void droid_zyg_loadout(WARSHP *ptr)
{
	ptr->items[I_MISSILE] = (gernd() % 50) + 10;
	ptr->items[I_TORPEDO] = (gernd() % 50) + 10;
	ptr->items[I_IONCANNON] = (gernd() % 20) + 10;
	ptr->items[I_FIGHTER] = (gernd() % 100) + 20;
	ptr->items[I_FLUXPOD] = (gernd() % 40) + 10;
	ptr->items[I_DECOYS] = (gernd() % 100) + 10;
	ptr->items[I_JAMMERS] = (gernd() % 100) + 10;
	ptr->items[I_ZIPPERS] = (gernd() % 100) + 10;
	ptr->items[I_MINE] = (gernd() % 100) + 10;
	ptr->items[I_GOLD] = (gernd() % cyb_gold) + 1000;
}

/**************************************************************************
** Droid Init Function                                                   **
**************************************************************************/

void FUNC droid_init(WARSHP *ptr, int usrn, int class)
{
	WARSHP *wptr;
	int zothusn;
	double ddist;

	if (usrn < 0 || usrn >= nships) {
		logthis(spr("DROID_INIT:bad usrn [%d]",usrn));
		return;
	}

	strncpy(droidname, "@Droid-", UIDSIZ);
	sprintf(&droidname[7], "%d", usrn);

	waruptr = warusroff(usrn);
	warsptr = warshpoff(usrn);

	logthis(spr("GE:INF:Adding %s user", droidname));

	initusr(droidname);

	memcpy(waruptr, &tmpusr, sizeof(WARUSR));	/* make it the current user */

	/* make me a Ship */
	logthis(spr("GE:INF:Adding %s ship - %d", droidname, class));

	initshp(droidname, class);

	memcpy(ptr, &tmpshp, sizeof(WARSHP));	/* make is the current ship */
	strncpy(ptr->shipname, shipclass[class].npcprefx, sizeof(ptr->shipname) - 1);
	ptr->shipname[sizeof(ptr->shipname) - 1] = 0;
	sprintf(gechrbuf, "%u", usrn * usrn + gernd() % (2 * usrn + 1) + 1000);
	strncat(ptr->shipname, gechrbuf,
		sizeof(ptr->shipname) - strlen(ptr->shipname) - 1);

	waruptr->kills = 0;	/* new droid so clear this */

	if (shipclass[ptr->shpclass].loadout == 1) {	/* Garbage Scows stay close to 0 0 */
		ptr->coord.xcoord = rndm((double)univmax/6);
		ptr->coord.ycoord = rndm((double)univmax/6);
	} else if (shipclass[ptr->shpclass].loadout == 4) {	/* SCTs, a little more room */
		ptr->coord.xcoord = rndm((double)univmax/4);
		ptr->coord.ycoord = rndm((double)univmax/4);
	} else if (shipclass[ptr->shpclass].loadout == 2 ||
		shipclass[ptr->shpclass].loadout == 6) {	/* TGs, GCFs */
		ptr->coord.xcoord = rndm((double)univmax/2);
		ptr->coord.ycoord = rndm((double)univmax/2);
	} else if (shipclass[ptr->shpclass].loadout == 5 &&
		univmax > 30) {	/* SDDs, halfway between neut and barrier */
		ptr->coord.xcoord = ((double)univmax/2.0)+(rndm(29.9)-14.8);
		ptr->coord.ycoord = ((double)univmax/2.0)+(rndm(29.9)-14.8);
	} else {
		ptr->coord.xcoord = rndm((double)univmax);
		ptr->coord.ycoord = rndm((double)univmax);
	}
	if (gernd() % 2 == 0)	/* put in all quadrants */
		ptr->coord.xcoord *= -1.0;
	if (gernd() % 2 == 0)
		ptr->coord.ycoord *= -1.0;


	if (shipclass[ptr->shpclass].loadout == 2) {	/* Murdonian Transport */
		ptr->items[I_MISSILE] = (gernd() % 20) + 10;
		ptr->items[I_TORPEDO] = (gernd() % 20) + 10;
		ptr->items[I_IONCANNON] = (gernd() % 25) + 5;
		ptr->items[I_FLUXPOD] = (gernd() % 30) + 10;
		ptr->items[I_DECOYS] = (gernd() % 75) + 10;
		ptr->items[I_JAMMERS] = (gernd() % 75) + 10;
		ptr->items[I_MINE] = (gernd() % 75) + 10;
		ptr->items[I_GOLD] = (gernd() % 500) + 100;
	} else if (shipclass[ptr->shpclass].loadout == 6) {	/* Galactic Command Freighter */
		droid_zyg_loadout(ptr);
	} else {
		ptr->items[I_FLUXPOD] = (gernd() % 10) + 10;
		if (shipclass[ptr->shpclass].max_torps)
			ptr->items[I_TORPEDO] = (gernd() % 20) + 10;
		if (shipclass[ptr->shpclass].max_missl)
			ptr->items[I_MISSILE] = (gernd() % 20) + 10;
		if (shipclass[ptr->shpclass].has_decoy)
			ptr->items[I_DECOYS] = (gernd() % 20) + 10;
		if (shipclass[ptr->shpclass].has_jam)
			ptr->items[I_JAMMERS] = (gernd() % 20) + 10;
		if (shipclass[ptr->shpclass].has_mine)
			ptr->items[I_MINE] = (gernd() % 20) + 10;
		if (shipclass[ptr->shpclass].has_zip)
			ptr->items[I_ZIPPERS] = (gernd() % 20) + 10;
		if (shipclass[ptr->shpclass].loadout == 4)	/* Sarten Civil Transport */
			ptr->items[I_GOLD] = (gernd() % 200) + 100;
	}

	npc_cruise(ptr, usrn, 0);
	ptr->status = GESTAT_AUTO;
	ptr->tick = (CYBTICKTIME + gernd() % CYBTICKTIME) * 5;

	if (shipclass[class].max_phasr > 1)
		ptr->phasrtype = (gernd() % shipclass[class].max_phasr) + 1;
	else
		ptr->phasrtype = 1;
	if (shipclass[class].max_shlds > 1)
		ptr->shieldtype = (gernd() % shipclass[class].max_shlds) + 1;
	else
		ptr->shieldtype = 1;

	ptr->shield = 40 + (ptr->shieldtype * 10);
	ptr->phasr = 100;
	ptr->npcmsg = 0;

	for (zothusn = 0; zothusn < nterms; zothusn++) {
		wptr = warshpoff(zothusn);
		if (ingegame(zothusn) && wptr->jam_sev <= (byte)2) {
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if (ddist > shipclass[wptr->shpclass].scanrange
				|| innebula(coord1(wptr->coord.xcoord), coord1(wptr->coord.ycoord))) {
				bearing = cbearing(&wptr->coord, &ptr->coord, wptr->heading);
				prfmsg(DROIDNEW, bearing);
				outprfge(FLT_SHIP, zothusn);
				} else {
					setsect(ptr);
					prfmsg(DROIDNW2, xsect, ysect);
					outprfge(FLT_SHIP, zothusn);
				}
			}
		}
	}

/**************************************************************************
** Droid Annoy Function                                                  **
**************************************************************************/

/* ptr to sender , usrn = reciever */
static void droid_annoy(WARSHP *ptr, int usrn)
{
	int base, sel, interval;

	/* skip NPCs entirely */
	if (usrn >= nterms)
		return;

	base = DRBASEM + ((ptr->shpclass - dr_class) * 12);
	interval = 10 + shipclass[ptr->shpclass].tough_factor;	/* tougher npcs have fewer ticks */
	sel = 0;

	/* display friend or foe msg if not engaged with that user */
	if (ptr->cybmine == 255) {
		if (ptr->npcmsg == 0)			/* if starting fresh or coming back from cybmine set */
			ptr->npcmsg = interval;
		if (ptr->npcmsg > interval * 4)		/* cycle through msgs instead of rnd pick */
			ptr->npcmsg = 1;
		if (ptr->npcmsg % interval == 0)
			sel = base + (ptr->npcmsg / interval);
		if (sel != 0) {
			if (sel + 4 >= DRLASTM) {
				geshocst(0, "GE:BAD DROID MSG FF");
				logthis(spr("droid_annoy:bad msg ff usrn [%d]", usrn));
				return;
			}
			if (warusroff(usrn)->factions[shipclass[ptr->shpclass].faction] > 50) {
				prfmsg(sel + 4, ptr->shipname);
				outprfge(FLT_BEACON, usrn);
			} else {
				prfmsg(sel, ptr->shipname);
				outprfge(FLT_NONE, usrn);
			}
		}
	} else {
		/* npcmsg 255 means the ship was just hit, say ouch */
		if (ptr->npcmsg == 255) {
			sel = base + (gernd() % 4) + 9;
			if (sel >= DRLASTM) {
				geshocst(0, "GE:BAD DROID MSG BTL");
				logthis(spr("droid_annoy:bad msg btl usrn [%d]", usrn));
				return;
			}
			prfmsg(sel, ptr->shipname);
			outprfge(FLT_NONE, usrn);
			ptr->npcmsg = 0;
		}
	}
}

/**************************************************************************
** Droid Distress Function                                               **
**************************************************************************/

static void droid_distress(WARSHP *ptr, int usrn)
{
	if (ptr->distress >= nships || !ingegame(ptr->distress))
		ptr->distress = 255;
	if (ptr->distress != ptr->cybmine && ptr->cybmine < nships) {
		setsect(ptr);
		prfmsg(DRDISMSG, ptr->shipname, shipclass[ptr->shpclass].typename,
			username(warshpoff(ptr->cybmine)), xsect, ysect);
		outwar(FLT_DISTRESS, usrn, 0, 0);
		ptr->distress = ptr->cybmine;
	}
}

/**************************************************************************
** Droid Check State Function                                            **
**************************************************************************/

static void droid_check_state(WARSHP *ptr, int usrn)
{
	if (ptr->speed < 1000.0) {
		ptr->where = 0;
		if (ptr->shieldstat != SHIELDDM)
			shieldup(ptr, usrn);
	} else {
		ptr->where = 1;
		if (ptr->shieldstat == SHIELDUP)
			shielddn(ptr, usrn);
	}
}

/**************************************************************************
** Not Claimed Droid Function                                            **
**************************************************************************/

static int notclaimed_d(int drtype, int usrn)
{
	WARSHP *wptr;
	int zothusn;

	for (zothusn = nterms; zothusn < nships; zothusn++) {
		wptr = warshpoff(zothusn);
		if (wptr->status == GESTAT_AUTO && shipclass[wptr->shpclass].loadout == drtype && wptr->cybmine == (byte)usrn)
			return FALSE;
	}
	return TRUE;
}

/**************************************************************************
** Droid Phaser Function                                                 **
**************************************************************************/

static void droid_phaser(WARSHP *ptr, int usrn, WARSHP *wptr)
{
	if (shipclass[ptr->shpclass].max_phasr > 0 && !neutral(&ptr->coord) &&
		!neutral(&wptr->coord) && isvisible(ptr, wptr) &&
		gernd() % (4 - (shipclass[ptr->shpclass].tough_factor / 2)) == 0) {
		ptr->degrees = cbearing(&ptr->coord, &wptr->coord, ptr->heading);
		if (wptr->where == 1 && ptr->where == 1 && ptr->hypha == 0 && ptr->phasr >= 0)
			firehp(ptr, usrn);
		else if (ptr->where == 0 && (wptr->where == 0 || (wptr->where == 1 &&
			shipclass[ptr->shpclass].max_phasr >= phatowrp)) && ptr->phasr >= PMINFIRE) {
			ptr->percent = 2;
			firep(ptr, usrn);
		}
	}
}

/**************************************************************************
** Droid Torpedo Function                                                **
**************************************************************************/

static void droid_torp(WARSHP *ptr, int usrn, WARSHP *wptr, int zothusn)
{
	int i, tden;

	if (!neutral(&ptr->coord) && !neutral(&wptr->coord) && isvisible(ptr, wptr) &&
		ptr->where == 0 && wptr->where == 0 &&
		shipclass[ptr->shpclass].max_torps && gernd() % 2 == 0) {
		tden = 5 - shipclass[ptr->shpclass].tough_factor;
		if (tden < 1)
			tden = 1;
		for (i = 0; i < shipclass[ptr->shpclass].max_torps; ++i) {
			if (ptr->items[I_TORPEDO] > 0 && gernd() % tden == 0)
				torp(ptr, usrn, zothusn);
		}
	}
}

/**************************************************************************
** Droid Lives Function                                                  **
**************************************************************************/

void FUNC droid_lives(WARSHP *ptr, int usrn)
{
	if (usrn < 0 || usrn >= nships) {
		logthis(spr("DROID_LIVES:bad usrn [%d]", usrn));
		return;
	}

	sprintf(&droidname[7], "%d", usrn + 1);

	if (ptr->damage >= 100.0)
		return;

	/* reset the ticker to 255 to cause it to recalc */
	ptr->tick = 255;

	if (ptr->holdcourse > 0)
		--(ptr->holdcourse);

	/* if we're engaged with another ship, skip the below */
	if (ptr->cybmine != 255)
		ptr->cybupdate = 0;
	/* if we have a cybupdate value, we must have hit telezip() */
	if (ptr->cybupdate > 1)
		--(ptr->cybupdate);
	else if (ptr->cybupdate == 1) {
		/* once we get some distance from the barrier, cruise */
		npc_cruise(ptr, usrn, 0);
		ptr->cybupdate = 0;
	}

	/* save off the topspeed in 1000's */
	/* if no warp, top speed is impulse 99 */
	if (ptr->topspeed == 0 && shipclass[ptr->shpclass].max_accel > 0)
		dr_topspeed = 990;
	else
		dr_topspeed = (double)ptr->topspeed * 1000.0;

	if (ptr->tactical < 0) {
		ptr->cybmine = 255;
		ptr->holdcourse = 0;
		npc_cruise(ptr, usrn, 0);
		droid_check_state(ptr, usrn);
		droid_distress(ptr, usrn);	/* clear only */
		ptr->energy = 50000L;
		ptr->tick = (CYBTICKTIME + gernd() % CYBTICKTIME) * 5;
		return;
	}

	if (shipclass[ptr->shpclass].loadout == 2)
		droid_act_2(ptr, usrn);	/* Murdonian Transport */
	else if (shipclass[ptr->shpclass].loadout == 3)
		droid_act_3(ptr, usrn);	/* Vakory Survey Drone */
	else if (shipclass[ptr->shpclass].loadout == 4)
		droid_act_4(ptr, usrn);	/* Sarten Civil Transport */
	else if (shipclass[ptr->shpclass].loadout == 5)
		droid_act_5(ptr, usrn);	/* Sarten Defense Drone */
	else if (shipclass[ptr->shpclass].loadout == 6)
		droid_act_6(ptr, usrn);	/* Galactic Command Freighter */
	else
		droid_act_1(ptr, usrn);	/* Lydorian Garbage Scow */

	ptr->energy = 50000L;

	if (ptr->tick == 255) {
		if (ptr->cybmine == 255)	/* if just cruising around don't get back to me for some time */
			ptr->tick = (CYBTICKTIME + gernd() % CYBTICKTIME) * 5;
		else if (ptr->cybmine >= nterms)	/* if going after a fellow NPC, medium speed */
			ptr->tick = (CYBTICKTIME + gernd() % CYBTICKTIME) * 3 -
				shipclass[ptr->shpclass].tough_factor;
		else
			ptr->tick = CYBTICKTIME +
				gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
	}
}

/**************************************************************************
** Lydorian Garbage Scow                                                 **
** Impulse only, no phasers, no projectiles                              **
**************************************************************************/

static void droid_act_1(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn;
	double ddist;

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		if (ptr->cybmine == 255) {
			/* look at user ships only */
			for (zothusn = 0; zothusn < nterms; zothusn++) {
				wptr = warshpoff(zothusn);
				/* hail users in area */
				if (ingegame(zothusn) && wptr->status == GESTAT_USER && isvisible(ptr, wptr)) {
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					if (ddist < (double)shipclass[wptr->shpclass].scanrange &&
						ddist < (double)shipclass[ptr->shpclass].scanrange) {
						ptr->tick = CYBTICKTIME +
							gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
						droid_annoy(ptr, zothusn);
					}
				}
			}
			++ptr->npcmsg;
		}
		if (ptr->cybmine < nships && ingegame(ptr->cybmine)) {
			zothusn = ptr->cybmine;
			wptr = warshpoff(zothusn);
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if ((ddist < (double)shipclass[ptr->shpclass].scanrange && isvisible(ptr, wptr)) ||
				ptr->cantexit > 0) {
				/* if still in range, flee */
				if (ptr->helm >= 0) {
					ptr->speed2b = dr_topspeed;
					ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) + 180.0);
				}
				droid_annoy(ptr, zothusn);
			} else {
				npc_cruise(ptr, usrn, 0);	/* phew we're safe */
			}
		} else {
			ptr->cybmine = 255;
		}
	} else {
		npc_cruise(ptr, usrn, 3);
	}
	droid_check_state(ptr, usrn);
	droid_distress(ptr, usrn);
}

/**************************************************************************
** Murdonian Transport                                                   **
** Warp, phasers, no projectiles                                         **
**************************************************************************/

static void droid_act_2(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn, around;
	double ddist;

	around = FALSE;

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		if (ptr->cybmine == 255) {
			/* look at user ships only */
			for (zothusn = 0; zothusn < nterms; zothusn++) {
				wptr = warshpoff(zothusn);
				/* hail users in area */
				if (ingegame(zothusn) && wptr->status == GESTAT_USER && isvisible(ptr, wptr)) {
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					if (ddist < (double)shipclass[wptr->shpclass].scanrange &&
						ddist < (double)shipclass[ptr->shpclass].scanrange) {
						around = TRUE;	/* if user around but not hostile go to impulse */
						if (ptr->holdcourse == 0) {
							if (ptr->helm >= 0)
								ptr->speed2b = ((gernd() % 85) + 15) * 10;
							ptr->holdcourse = gernd() % 15 + 5;
						}
						ptr->tick = CYBTICKTIME +
							gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
						droid_annoy(ptr, zothusn);
					}
				}
			}
			++ptr->npcmsg;
			/* get back to it if no one's around */
			if (around == FALSE && ptr->holdcourse == 0)
				npc_cruise(ptr, usrn, 0);
		}
		if (ptr->cybmine < nships && ingegame(ptr->cybmine)) {
			ptr->holdcourse = 0;
			zothusn = ptr->cybmine;
			wptr = warshpoff(zothusn);
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if ((ddist < (double)shipclass[ptr->shpclass].scanrange && isvisible(ptr, wptr)) ||
				ptr->cantexit > 0) {
				if (isvisible(ptr, wptr))
					droid_annoy(ptr, zothusn);
				/* if still in range, flee and attack */
				if (ptr->helm >= 0) {
					if (ptr->damage > 50)
						npc_cruise(ptr, usrn, 4);
					else
						ptr->speed2b = 990.0;
					if (!isvisible(ptr, wptr))
						ptr->head2b = rndm(359.9);
					else
						ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) +
							180.0 + (rand() % 51 - 25));
				}
				if (ddist < 30000)
					droid_phaser(ptr, usrn, wptr);
			} else {
				npc_cruise(ptr, usrn, 0);	/* phew we're safe */
			}
		} else {
			ptr->cybmine = 255;
		}
	} else {
		npc_cruise(ptr, usrn, 3);
	}
	droid_check_state(ptr, usrn);
	droid_distress(ptr, usrn);
}

/**************************************************************************
** Vakory Survey Drone                                                   **
** Warp, phasers, torps, jammers, mines                                  **
**************************************************************************/

static void droid_act_3(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn;
	double ddist;

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		if (ptr->cybmine == 255) {
			/* look at user ships only */
			for (zothusn = 0; zothusn < nterms; zothusn++) {
				wptr = warshpoff(zothusn);
				/* hail users in area */
				if (ingegame(zothusn) && wptr->status == GESTAT_USER && isvisible(ptr, wptr)) {
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					if (ddist < (double)shipclass[wptr->shpclass].scanrange &&
						ddist < (double)shipclass[ptr->shpclass].scanrange) {
						ptr->tick = CYBTICKTIME +
							gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
						droid_annoy(ptr, zothusn);
					}
				}
			}
			++ptr->npcmsg;
		}
		if (ptr->cybmine < nships && ingegame(ptr->cybmine)) {
			zothusn = ptr->cybmine;
			wptr = warshpoff(zothusn);
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if ((ddist < (double)shipclass[ptr->shpclass].scanrange && isvisible(ptr, wptr)) ||
				ptr->cantexit > 0) {
				if (isvisible(ptr, wptr))
					droid_annoy(ptr, zothusn);
				if (ptr->holdcourse == 0) {
					if (ptr->damage < 67) {
						if (ptr->helm >= 0)
							ptr->speed2b = ((gernd() % 99) + 1) * 10;
					} else {
						if (ptr->helm >= 0)
							npc_cruise(ptr, usrn, 4);
						if (ptr->items[I_MINE] > 0 && shipclass[ptr->shpclass].has_mine &&
							ptr->mineload == 0 && !neutral(&ptr->coord) &&
							gernd() % 3 == 0 &&
							(ptr->helm >= 0 || ptr->speed >= 1000.0))
							laymine(ptr, usrn, 10);
					}
					if (ptr->helm >= 0) {
						if (!isvisible(ptr, wptr))
							ptr->head2b = rndm(359.9);
						else
							ptr->head2b = normal(vector(&ptr->coord, &(wptr->coord)) +
								180.0 + (rand() % 51 - 25));
					}
					ptr->holdcourse = gernd() % 15 + 10;
				}
				if (ddist < 30000 && !neutral(&ptr->coord) && isvisible(ptr, wptr)) {
					droid_phaser(ptr, usrn, wptr);
					droid_torp(ptr, usrn, wptr, zothusn);
					if (ptr->items[I_JAMMERS] > 0 && shipclass[ptr->shpclass].has_jam &&
						ptr->jamload == 0 && ptr->holdcourse == 1)
						jam(ptr, usrn);
				}
			} else {
				npc_cruise(ptr, usrn, 0);	/* phew we're safe */
			}
		} else {
			ptr->cybmine = 255;
		}
	} else {
		npc_cruise(ptr, usrn, 3);
	}
	droid_check_state(ptr, usrn);
	droid_distress(ptr, usrn);
	}

/**************************************************************************
** Sarten Civil Transport                                                **
** Warp, phasers, decoys, jammers                                        **
**************************************************************************/

static void droid_act_4(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn;
	double ddist;

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		if (ptr->cybmine == 255) {
			/* look at user ships only */
			for (zothusn = 0; zothusn < nterms; zothusn++) {
				wptr = warshpoff(zothusn);
				/* hail users in area */
				if (ingegame(zothusn) && wptr->status == GESTAT_USER && isvisible(ptr, wptr)) {
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					if (ddist < (double)shipclass[wptr->shpclass].scanrange &&
						ddist < (double)shipclass[ptr->shpclass].scanrange) {
						ptr->tick = CYBTICKTIME +
							gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
						droid_annoy(ptr, zothusn);
					}
				}
			}
			++ptr->npcmsg;
		}
		if (ptr->cybmine < nships && ingegame(ptr->cybmine)) {
			zothusn = ptr->cybmine;
			wptr = warshpoff(zothusn);
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if ((ddist < (double)shipclass[ptr->shpclass].scanrange && isvisible(ptr, wptr)) ||
				ptr->cantexit > 0) {
				if (isvisible(ptr, wptr))
					droid_annoy(ptr, zothusn);
				if (ptr->holdcourse == 0) {
					if (ptr->helm >= 0) {
						if (!isvisible(ptr, wptr))
							ptr->head2b = rndm(359.9);
						else
							ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) +
								180.0 + (rand() % 51 - 25));
						if (!isvisible(ptr, wptr) &&
							innebula(coord1(ptr->coord.xcoord), coord1(ptr->coord.ycoord)))
							ptr->speed2b = 990;
						else if (gernd() % 2 == 0)
							npc_cruise(ptr, usrn, 4);
						else
							ptr->speed2b = 990;
					}
					ptr->holdcourse = gernd() % 15 + 10;
				}
				if (ddist < 30000 && !neutral(&ptr->coord) && isvisible(ptr, wptr)) {
					droid_phaser(ptr, usrn, wptr);
					if (ptr->where == 0 && ptr->items[I_DECOYS] > 0 &&
						shipclass[ptr->shpclass].has_decoy)
						npc_lay_decoys(ptr);
					if (ptr->holdcourse == 1 && ptr->items[I_JAMMERS] > 0 &&
						shipclass[ptr->shpclass].has_jam && ptr->jamload == 0)
						jam(ptr, usrn);
				}
			} else {
				npc_cruise(ptr, usrn, 0);	/* phew we're safe */
			}
		} else {
			ptr->cybmine = 255;
		}
	} else {
		npc_cruise(ptr, usrn, 3);
	}
	droid_check_state(ptr, usrn);
	droid_distress(ptr, usrn);
}

/**************************************************************************
** Sarten Defense Drone                                                  **
** Warp, phasers, torps, mines, zippers                                  **
**************************************************************************/

static void droid_act_5(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn, lta, low_ship, pickcyb;
	double ddist, low_dist;

	low_dist = 999999999.0;
	low_ship = -1;

	lta = shipclass[ptr->shpclass].lowest_to_attk - 1;

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		if (ptr->cybmine == 255) {
			if (cattkd > 0 && (cattkd >= 10 || gernd() % ((10 - cattkd) * 600) == 0))
				pickcyb = nships;	/* should we go after cybs? sure, why not */
			else
				pickcyb = nterms;	/* only go after users we don't like, if any */
			for (zothusn = 0; zothusn < pickcyb; zothusn++) {
				if (usrn != zothusn && ingegame(zothusn)) {
					wptr = warshpoff(zothusn);
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					/* hail any users if close */
					if (zothusn < nterms && wptr->status == GESTAT_USER &&
						ddist < (double)shipclass[wptr->shpclass].scanrange &&
						ddist < (double)shipclass[ptr->shpclass].scanrange &&
						ddist < 15000.0 && isvisible(ptr, wptr)) {
						ptr->tick = CYBTICKTIME +
							gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
						droid_annoy(ptr, zothusn);
					}
					if ((shipclass[wptr->shpclass].max_type == CLASSTYPE_CYBORG &&
						wptr->cybmine == 255) ||
						(wptr->status == GESTAT_USER && isvisible(ptr, wptr) &&
						lta <= wptr->shpclass &&
						warusroff(zothusn)->factions[shipclass[ptr->shpclass].faction] > 50 &&
						notclaimed_d(shipclass[ptr->shpclass].loadout, zothusn))) {
						if (ddist < low_dist) {
							low_dist = ddist;
							low_ship = zothusn;
						}
					}
				}
			}
			++ptr->npcmsg;
			if (low_ship >= 0 && low_ship < nships)
				ptr->cybmine = low_ship;
		}
		/* if still not tracking */
		if (ptr->cybmine == 255 && ptr->holdcourse == 0) {
			npc_cruise(ptr, usrn, 0);
			ptr->holdcourse = gernd() % 5 + 5;
		}
		if (ptr->cybmine < nships && ingegame(ptr->cybmine)) {
			zothusn = ptr->cybmine;
			wptr = warshpoff(zothusn);
			if (!isvisible(ptr, wptr) && ptr->holdcourse == 0) {
				if (innebula(coord1(ptr->coord.xcoord), coord1(ptr->coord.ycoord))) {
					if (ptr->helm >= 0) {
						ptr->head2b = vector(&ptr->coord, &(wptr->coord));
						npc_cruise(ptr, usrn, 4);
					}
					return;
				}
				ptr->holdcourse = gernd() % 5 + 5;
				npc_cruise(ptr, usrn, 0);	/* let them cruise */

				/* if the guy is cloaked then give up after awhile */
				if (gernd() % 10 == 0)
					ptr->cybmine = 255;
				return;
			}
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if (ddist < (double)shipclass[wptr->shpclass].scanrange &&
				ddist < (double)shipclass[ptr->shpclass].scanrange)
				droid_annoy(ptr, zothusn);
			if (ptr->holdcourse == 0) {
				if (ptr->helm >= 0) {
					if (ddist > 50000.0) {
						ptr->head2b = vector(&ptr->coord, &(wptr->coord));
						npc_cruise(ptr, usrn, 4);
					} else if (ddist > 20000.0) {
						ptr->head2b = vector(&ptr->coord, &(wptr->coord));
						if (dr_topspeed >= 10000) {
							if (ptr->speed < 1000) {
								ptr->speed2b = 0.0;
								ptr->speed = ptr->speed2b;
							}
							ptr->speed2b = 10000;
						} else {
							npc_cruise(ptr, usrn, 4);
						}
					} else if (ddist > 5000.0) {
						ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) +
							(rand() % 51 - 25));
						ptr->speed2b = 990.0;
					} else {
						ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) +
							(rand() % 51 - 25) + 180.0);
						ptr->speed2b = ((int)(rndm(350.0) + 150.0) / 10) * 10;
					}
				}
			}
			if (ddist < 30000) {
				droid_phaser(ptr, usrn, wptr);
				droid_torp(ptr, usrn, wptr, zothusn);
			}
			if (ddist < 3000 && !neutral(&ptr->coord) &&
				ptr->items[I_MINE] > 0 && shipclass[ptr->shpclass].has_mine &&
				ptr->mineload == 0 && ptr->holdcourse == 0 &&
				gernd() % 3 == 0) {
				if (ptr->helm >= 0 || ptr->speed >= 1000.0) {
					laymine(ptr, usrn, 10);
					ptr->holdcourse = 10;
					if (ptr->helm >= 0) {
						ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) + 180.0);
						if (ptr->speed < 1000 && dr_topspeed >= 1000) {
							ptr->speed2b = 0.0;
							ptr->speed = ptr->speed2b;
						}
						ptr->speed2b = (double)(ptr->topspeed / 2) * 1000;
					}
				}
			}
			if (ptr->items[I_ZIPPERS] > 0 && shipclass[ptr->shpclass].has_zip &&
				ptr->zipload == 0 && wptr->minesnear == TRUE &&
				gernd() % 3 == 0) {
				zip(ptr);
				wptr->minesnear = FALSE;
			}
		} else {
			ptr->cybmine = 255;
		}
	} else {
		npc_cruise(ptr, usrn, 3);
	}
	droid_check_state(ptr, usrn);
}

/**************************************************************************
** Galactic Command Freighter                                            **
** Warp, phasers, torps, decoys, jammers, zippers                        **
**************************************************************************/

static void droid_act_6(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn, setship;
	COORD neutsect;
	double ddist;

	/* GCFs are impervious to helm damage, otherwise this would throw off their
	   scripted behavior */
	if (ptr->helm < 0)
		ptr->helm = 0;

	setship = TRUE;

	neutsect.xcoord = 0.50001;
	neutsect.ycoord = 0.50001;

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		if (ptr->cybmine == 255) {
			/* one ship should be traveling to/from neutral zone */
			/* use npcstate to coordinate the Zygor/repair/resupply cycle */
			for (zothusn = nterms; zothusn < nships; zothusn++) {	/* are any GCFs already doing it */
				wptr = warshpoff(zothusn);
				if (ingegame(zothusn) && wptr->status == GESTAT_AUTO &&
					shipclass[wptr->shpclass].loadout == shipclass[ptr->shpclass].loadout &&
					wptr->npcstate != 0) {
					setship = FALSE;
					break;
				}
			}
			if (ptr->npcstate == 9) {	/* if i just did it, put me at the back of the line */
				ptr->npcstate = 0;
				ptr->tick = CYBTICKTIME * 10;
			}
			if (setship == TRUE || ptr->damage > 50)	/* if i'm picked, or if i'm damaged, go to Zygor */
				ptr->npcstate = 1;
			/* look at user ships only */
			for (zothusn = 0; zothusn < nterms; zothusn++) {
				wptr = warshpoff(zothusn);
				/* hail users in area */
				if (ingegame(zothusn) && wptr->status == GESTAT_USER && isvisible(ptr, wptr)) {
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					if (ddist < (double)shipclass[wptr->shpclass].scanrange &&
						ddist < (double)shipclass[ptr->shpclass].scanrange) {
						ptr->tick = CYBTICKTIME +
							gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
						if (!neutral(&wptr->coord))	/* don't advertise Zygor if user is already there */
							droid_annoy(ptr, zothusn);
					}
				}
			}
			++ptr->npcmsg;
			if (ptr->npcstate != 0) {
				if (ptr->npcstate == 1 || ptr->npcstate == 2 ||
					(neutral(&ptr->coord) && ptr->npcstate < 7)) {	/* go to Zygor */
					if (ptr->npcstate < 3)
						ptr->head2b = normal(vector(&ptr->coord, &neutsect));
					if (ptr->npcstate == 1 && cdistance(&ptr->coord, &neutsect) < 8)
						ptr->npcstate = 2;	/* keep cybs from picking off ships right outside neutral zone */
					if (cdistance(&ptr->coord, &neutsect) > 1.5)
						npc_cruise(ptr, usrn, 4);
					else if (cdistance(&ptr->coord, &neutsect) > .1)
						ptr->speed2b = 990;
					else if (cdistance(&ptr->coord, &neutsect) > .025)
						ptr->speed2b = 250;
					else if (ptr->npcstate == 1 || ptr->npcstate == 2) {
						ptr->speed2b = 0;
						ptr->speed = ptr->speed2b;
						if (ptr->damage > 3.0)
							ptr->damage -= 3.0;	/* do maintenance and stay until done */
						else {
							ptr->npcstate = 3;
							ptr->damage = 0.0;
						}
					} else if (ptr->npcstate > 2 && ptr->npcstate < 6) {
						++ptr->npcstate;	/* hang out a little longer */
					}
					if (ptr->npcstate == 6) {
						droid_zyg_loadout(ptr);	/* reset ship contents */
						ptr->head2b = rndm(359.9);
						npc_cruise(ptr, usrn, 4);
						ptr->npcstate = 7;
					}
				} else if (ptr->npcstate == 7 &&
					cdistance(&ptr->coord, &neutsect) > 15) {
					ptr->npcstate = 8;	/* get a little distance */
				} else if (ptr->npcstate == 8) {	/* go the other way for a bit then let another ship do it */
					if (cdistance(&ptr->coord, &neutsect) < univmax / 3) {
						ptr->head2b = normal(vector(&ptr->coord, &neutsect) + 180.0);
						npc_cruise(ptr, usrn, 4);
					} else {
						npc_cruise(ptr, usrn, 1);
						ptr->npcstate = 9;	/* done */
					}
				}
			}
		}
		if (ptr->cybmine < nships && ingegame(ptr->cybmine)) {
			zothusn = ptr->cybmine;
			wptr = warshpoff(zothusn);
			ddist = cdistance(&ptr->coord, &wptr->coord);
			ddist *= 10000;
			if ((ddist < (double)shipclass[ptr->shpclass].scanrange && isvisible(ptr, wptr)) ||
				ptr->cantexit > 0) {
				if (isvisible(ptr, wptr))
					droid_annoy(ptr, zothusn);
				if (ptr->holdcourse == 0) {
					if (!isvisible(ptr, wptr))
						ptr->head2b = rndm(359.9);
					else
						ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) +
							(rand() % 71 - 35) + 180);
					if (ptr->damage < 33)
						ptr->speed2b = 990.0;
					else
						npc_cruise(ptr, usrn, 4);
					ptr->holdcourse = gernd() % 15 + 10;
				}
				if (ddist < 30000 && !neutral(&ptr->coord) && isvisible(ptr, wptr)) {
					droid_phaser(ptr, usrn, wptr);
					droid_torp(ptr, usrn, wptr, zothusn);
					if (ptr->where == 0 && ptr->items[I_DECOYS] > 0 &&
						shipclass[ptr->shpclass].has_decoy)
						npc_lay_decoys(ptr);
					if (ptr->holdcourse == 1 && ptr->items[I_JAMMERS] > 0 &&
						shipclass[ptr->shpclass].has_jam && ptr->jamload == 0)
						jam(ptr, usrn);
					if (ptr->items[I_ZIPPERS] > 0 && shipclass[ptr->shpclass].has_zip &&
						ptr->zipload == 0 && wptr->minesnear == TRUE &&
						gernd() % 3 == 0) {
						zip(ptr);
						wptr->minesnear = FALSE;
					}
				}
			} else {
				npc_cruise(ptr, usrn, 0);	/* phew we're safe */
			}
		} else {
			ptr->cybmine = 255;
		}
	} else {
		npc_cruise(ptr, usrn, 3);
	}
	droid_check_state(ptr, usrn);
	droid_distress(ptr, usrn);
}

/**************************************************************************
** Droid Won Function                                                    **
**************************************************************************/

void FUNC droid_won(WARSHP *ptr, int usrn, WARSHP *wptr)
{
	wptr = wptr;
	npc_cruise(ptr, usrn, 0);
}

/**************************************************************************
** Droid Died Function                                                   **
**************************************************************************/


void FUNC droid_died(WARSHP *ptr, int usrn, WARSHP *wptr)
{
	usrn = usrn;
	wptr = wptr;
	ptr->status = GESTAT_AVAIL;
	logthis(spr("GE:INF:%s Died!", ptr->userid));
}
