
/***************************************************************************
 *                                                                         *
 *   GEFUNCS.C                                                             *
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
#include "stdio.h"

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

#define GEFUNCS 1

#include "geglobal.h"

/* LOCAL GLOBAL DEFS *****************************************************/

static long	deathdeduct;		/* death penalty amount to report after kill processing */
static void	shieldhitmsg(int shmsg, int usrn);	/* map shieldhit() result codes to output messages */
static void	pick_letter(SCANTAB *ptr);

/**************************************************************************
** Lockon helper for torp and misl                                       **
**************************************************************************/

int FUNC lockon(WARSHP *ptr, int type, int ship, int usrn)
{
	WARSHP *wptr;
	double dist, speed, fact = 0.0;

	if (type == 0 && ptr->torpcntl > 0) {
		prfmsg(TRBROKE);
		outprfge(FLT_NONE,usrn);
		return 0;
	}

	if (type == 1 && ptr->mislcntl > 0) {
		prfmsg(MIBROKE);
		outprfge(FLT_NONE,usrn);
		return 0;
	}

	if (ptr->jam_sev > (byte)2) {
		prfmsg(JAMMER4W);
		outprfge(FLT_NONE,usrn);
		return 0;
	}

	wptr = warshpoff(ship);

	if (neutral(&(wptr->coord))) {
		prfmsg(FCNONO);
		outprfge(FLT_NONE,usrn);
		return 0;
	}

	dist = cdistance(&ptr->coord,&(wptr->coord));
	if (wptr->cloak < 10 && (dist * 10000.0) < (double)ship_scanrange(ptr)) {
		speed = ptr->speed + wptr->speed;

		if (type == 0) { /* torpedo */
			if (wptr->speed > 999)
				fact = 0.0;
			else {
				fact = (1.2 - (speed / 5000));
				fact *= ((5.0 - dist) / tor_fact);
			}
		}

		if (type == 1) /* missile */
			fact = ((5.0 - dist) / mis_fact);

		if (fact > .7) {
			if (wptr->status == GESTAT_AUTO) {	/* if npc... */
				wptr->cybmine = usrn;	/* engage user */
				wptr->track_grace = CYBGRACE;
				wptr->tick = 2;		/* do it fast */
				wptr->npcmsg = 255;	/* reset annoy msg tracking */
			}
			return 1;
		} else {
			prfmsg(FCNOLOCK,shpltr(usrn,ship));
			outprfge(FLT_NONE,usrn);
			return 0;
		}
	} else {
		prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrn);
		return 0;
	}
}

/**************************************************************************
** Check whether a user's pending-entry buffer is empty                  **
**************************************************************************/

static int entrypend_empty(int usrn)
{
	int i;
	byte *pendptr;

	/* each user owns a fixed-width slice of the shared pending-entry buffer */
	pendptr = entrypend + (usrn * entrybytes);

	for (i = 0; i < entrybytes; ++i) {
		if (pendptr[i] != 0)
			return FALSE;
	}

	return TRUE;
}

/**************************************************************************
** Count outstanding ship repair and maintenance steps                   **
**************************************************************************/

static int maint_steps(WARSHP *ptr)
{
	int steps;

	steps = 0;
	if (ptr->helm < 0)
		steps += -ptr->helm;
	if (ptr->tactical < 0)
		steps += -ptr->tactical;
	if (ptr->phasr < 0)
		steps += -ptr->phasr;
	if (ptr->torpcntl > 0)
		steps += ptr->torpcntl;
	if (ptr->mislcntl > 0)
		steps += ptr->mislcntl;
	if (ptr->cloak < 0)
		steps += -ptr->cloak;
	if (ptr->jamload < 0)
		steps += -ptr->jamload;
	if (ptr->decload < 0)
		steps += -ptr->decload;
	if (ptr->zipload < 0)
		steps += -ptr->zipload;
	if (ptr->mineload < 0)
		steps += -ptr->mineload;
	if (ptr->topspeed == 0 && shipclass[ptr->shpclass].max_warp > 0)
		++steps;
	if (ptr->overspeed > 0)
		++steps;

	return steps;
}

/**************************************************************************
** Count repair steps needed to restore damaged shields                  **
**************************************************************************/

static int shield_steps(WARSHP *ptr)
{
	int need;

	if (ptr->shieldstat != SHIELDDM || ptr->shieldtype <= 0)
		return 0;

	need = 1 - ptr->shield;
	if (need <= 0)
		need = 1;

	return (need + ptr->shieldtype - 1) / ptr->shieldtype;
}

/**************************************************************************
** Check whether a ship needs any kind of maintenance or repair          **
**************************************************************************/

int FUNC repair_needed(WARSHP *ptr)
{
	if (ptr->damage > 0.0)
		return TRUE;
	if (shield_steps(ptr) > 0)
		return TRUE;
	if (maint_steps(ptr) > 0)
		return TRUE;
	return FALSE;
}

/**************************************************************************
** Estimate the remaining repair time for a ship already in maintenance  **
**************************************************************************/

unsigned FUNC repair_eta(WARSHP *ptr)
{
	int hull_ticks, sys_ticks, sh_ticks, sysrate, ticks;
	double hullrate;

	if (!ptr->repair)
		return 0;

	hullrate = 3.0;
	sysrate = 5;
	hull_ticks = 0;
	if (ptr->damage > 0.0)
		hull_ticks = (int)((ptr->damage / hullrate) + 0.999999);
	sys_ticks = (maint_steps(ptr) + sysrate - 1) / sysrate;
	sh_ticks = (shield_steps(ptr) + sysrate - 1) / sysrate;

	ticks = hull_ticks;
	if (sys_ticks > ticks)
		ticks = sys_ticks;
	if (sh_ticks > ticks)
		ticks = sh_ticks;

	return (unsigned)(ticks * TICKTIME);
}

/**************************************************************************
** Repair ship completely, instantly (SYSOP and ge-arena only)           **
**************************************************************************/

void FUNC fullrepair(WARSHP *ptr)
{
	ptr->repair = 0;
	ptr->damage = 0.0;
	if (ptr->phasr < 1)
		ptr->phasr = 0;
	ptr->tactical = 0;
	ptr->helm = 0;
	if (ptr->cloak < 1)
		ptr->cloak = 0;
	ptr->torpcntl = 0;
	ptr->mislcntl = 0;
	ptr->zipload = 0;
	ptr->jamload = 0;
	ptr->decload = 0;
	ptr->mineload = 0;
	ptr->torps_fired = 0;
	ptr->missl_fired = 0;

	if (ptr->shieldstat == SHIELDDM)
		ptr->shieldstat = SHIELDDN;
	if (ptr->shield < 1)
		ptr->shield = 0;
	if (shipclass[ptr->shpclass].max_warp > 0)
		ptr->topspeed = shipclass[ptr->shpclass].max_warp;
	ptr->overspeed = 0;
}

/**************************************************************************
** Spend maintenance steps repairing systems in priority order           **
**************************************************************************/

static void repair_systems(WARSHP *ptr, int usrn, int steps, int domaint)
{
	int fix;

	while (steps > 0) {
		if (ptr->helm < 0) {
			fix = (steps < -ptr->helm) ? steps : -ptr->helm;
			ptr->helm += fix;
			steps -= fix;
			if (ptr->helm == 0) {
				prfmsg(HLREPR);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->tactical < 0) {
			fix = (steps < -ptr->tactical) ? steps : -ptr->tactical;
			ptr->tactical += fix;
			steps -= fix;
			if (ptr->tactical == 0) {
				prfmsg(TAREPR);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->phasr < 0) {
			fix = (steps < -ptr->phasr) ? steps : -ptr->phasr;
			ptr->phasr += fix;
			steps -= fix;
			if (ptr->phasr == 0) {
				prfmsg(PHREPR);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->torpcntl > 0) {
			fix = (steps < ptr->torpcntl) ? steps : ptr->torpcntl;
			ptr->torpcntl -= fix;
			steps -= fix;
			if (ptr->torpcntl == 0) {
				prfmsg(FCREPRT);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->mislcntl > 0) {
			fix = (steps < ptr->mislcntl) ? steps : ptr->mislcntl;
			ptr->mislcntl -= fix;
			steps -= fix;
			if (ptr->mislcntl == 0) {
				prfmsg(FCREPRM);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->cloak < 0) {
			fix = (steps < -ptr->cloak) ? steps : -ptr->cloak;
			ptr->cloak += fix;
			steps -= fix;
			if (ptr->cloak == 0) {
				prfmsg(CLREPR);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->jamload < 0) {
			fix = (steps < -ptr->jamload) ? steps : -ptr->jamload;
			ptr->jamload += fix;
			steps -= fix;
			if (ptr->jamload == 0) {
				prfmsg(REPRJ);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->decload < 0) {
			fix = (steps < -ptr->decload) ? steps : -ptr->decload;
			ptr->decload += fix;
			steps -= fix;
			if (ptr->decload == 0) {
				prfmsg(REPRD);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->zipload < 0) {
			fix = (steps < -ptr->zipload) ? steps : -ptr->zipload;
			ptr->zipload += fix;
			steps -= fix;
			if (ptr->zipload == 0) {
				prfmsg(REPRZ);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (ptr->mineload < 0) {
			fix = (steps < -ptr->mineload) ? steps : -ptr->mineload;
			ptr->mineload += fix;
			steps -= fix;
			if (ptr->mineload == 0) {
				prfmsg(REPRMN);
				outprfge(FLT_NONE,usrn);
			}
			continue;
		}

		if (domaint && shipclass[ptr->shpclass].max_warp > 0
			&& (ptr->topspeed != shipclass[ptr->shpclass].max_warp || ptr->overspeed > 0)) {
			ptr->topspeed = shipclass[ptr->shpclass].max_warp;
			ptr->overspeed = 0;
			prfmsg(REPWARP);
			outprfge(FLT_NONE,usrn);
			--steps;
			continue;
		}

		break;
	}
}

/**************************************************************************
** Notify enhanced-lock observers about a simple ship event             **
**************************************************************************/

void FUNC lock_simple(WARSHP *ptr, int usrn, int msg, int skipsame)
{
	WARSHP *lptr;
	int i;
	int px, py, lx, ly;
	char letter;

	if (skipsame) {
		px = coord1(ptr->coord.xcoord);
		py = coord1(ptr->coord.ycoord);
	}

	for (i = 0; i < nterms; ++i) {
		if (!ingegame(i) || i == usrn)
			continue;

		lptr = warshpoff(i);
		if (!(lptr->upgrade & ENHLOCK) || lptr->lock != usrn)
			continue;

		if (skipsame) {
			lx = coord1(lptr->coord.xcoord);
			ly = coord1(lptr->coord.ycoord);
			if (lx == px && ly == py)
				continue;
		}

		letter = shpltr(i,usrn);
		if (letter == '?')
			continue;

		prfmsg(msg,letter);
		outprfge(FLT_NONE,i);
	}
}

/**************************************************************************
** Notify enhanced-lock observers about a ship event and sector         **
**************************************************************************/

void FUNC lock_sector(WARSHP *ptr, int usrn, int msg)
{
	WARSHP *lptr;
	int i;
	int sx, sy;
	char letter;

	sx = coord1(ptr->coord.xcoord);
	sy = coord1(ptr->coord.ycoord);

	for (i = 0; i < nterms; ++i) {
		if (!ingegame(i) || i == usrn)
			continue;

		lptr = warshpoff(i);
		if (!(lptr->upgrade & ENHLOCK) || lptr->lock != usrn)
			continue;

		letter = shpltr(i,usrn);
		if (letter == '?')
			continue;

		prfmsg(msg,letter,sx,sy);
		outprfge(FLT_NONE,i);
	}
}

/**************************************************************************
** Notify enhanced-lock observers about a projectile fired at a target   **
**************************************************************************/

void FUNC lock_proj(int usrn, int target, int msg, int msgn)
{
	WARSHP *lptr;
	int i;
	char sletter, tletter;

	for (i = 0; i < nterms; ++i) {
		if (!ingegame(i) || i == usrn || i == target)
			continue;

		lptr = warshpoff(i);
		if (!(lptr->upgrade & ENHLOCK) || lptr->lock != usrn)
			continue;

		sletter = shpltr(i,usrn);
		if (sletter == '?')
			continue;

		tletter = shpltr(i,target);
		if (tletter == '?')
			prfmsg(msgn,sletter);
		else
			prfmsg(msg,sletter,tletter);
		outprfge(FLT_NONE,i);
	}
}

/**************************************************************************
** Return a ship's effective acceleration rate                          **
**************************************************************************/

double FUNC ship_accel(WARSHP *ptr)
{
	double accelrate;

	accelrate = (double)shipclass[ptr->shpclass].max_accel;
	if (ptr->upgrade & ACCELBST)
		accelrate *= 2.0;

	return accelrate;
}

/**************************************************************************
** Return a ship's effective scan range                                 **
**************************************************************************/

long FUNC ship_scanrange(WARSHP *ptr)
{
	long range;

	range = shipclass[ptr->shpclass].scanrange;
	if (ptr->upgrade & SCANBST)
		range *= 2L;

	return range;
}

/**************************************************************************
** Shared NPC cruise-state helper                                        **
**************************************************************************/

void FUNC npc_cruise(WARSHP *ptr, int usrn, int call)
{
	/* 0 = drop pursuits and random, 1 = random, 2 = top speed short hold,
	3 = top speed long hold, 4 top speed no hold */

	if (call == 0) {
		ptr->cybmine = (byte)255;
		ptr->npcmsg = 255;
		ptr->distress = (byte)255;
	}

	if (shipclass[ptr->shpclass].max_accel == 0)	/* bases don't do any of the below */
		return;

	if (call == 2)
		ptr->holdcourse = gernd() % 10 + 6;
	if (call == 3)
		ptr->holdcourse = gernd() % 18 + 12;

	if (ptr->helm < 0)
		return;

	if (call < 2)
		ptr->head2b = rndm(359.9);

	if (ptr->topspeed == 0) {
		if (call > 1)
			ptr->speed2b = 990;
		else
			ptr->speed2b = ((gernd() % 99) + 1) * 10;
	} else {
		if (ptr->speed < 1000) {	/* quick jump to warp, no fractional warp speeds */
			hyperspace(ptr, usrn, 1);
			if (shipclass[ptr->shpclass].max_accel <= ptr->topspeed * 1000)
				ptr->speed = shipclass[ptr->shpclass].max_accel;
			else
				ptr->speed = ptr->topspeed * 1000;
		}
		if (cyb_fast(ptr)) {
			ptr->speed2b = (double)ptr->topspeed * 1000;
			ptr->speed = ptr->speed2b;
		}
		if (call > 1)
			ptr->speed2b = (double)ptr->topspeed * 1000;
		if (call < 2) {
			if (ptr->topspeed >= 10) /* don't go faster than warp 10 if cruising */
				ptr->speed2b = ((gernd() % 10) + 1) * 1000;
			else
				ptr->speed2b = ((gernd() % ptr->topspeed) + 1) * 1000;
		}
	}
}

/**************************************************************************
** Shared NPC decoy deployment helper                                    **
**************************************************************************/

void FUNC npc_lay_decoys(WARSHP *ptr)
{
	int i;

	if (!shipclass[ptr->shpclass].has_decoy)
		return;
	if (ptr->items[I_DECOYS] == 0)
		return;
	if (ptr->decload != 0)
		return;

	/* send out a decoy */
	for (i = 0; i < 3; ++i)
		if (ptr->decout[i] == 0 && gernd() % (50 * (i + 1)) == 0 &&
			ptr->items[I_DECOYS] > 0) {
			--ptr->items[I_DECOYS];
			ptr->decout[i] = DECOYTIME;
			ptr->decload = 1;
			return;
		}
}

/**************************************************************************
** Fire standard phasers at targets inside the current firing arc        **
**************************************************************************/

void FUNC firep(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	WARUSR *uptr;
	WARUSR *wuptr;
	unsigned deg;
	double factor;
	byte src_neb, targ_neb, nebmask, underone;
	int hitone, fired, locksent, shmsg;

	hitone = FALSE;
	fired = FALSE;
	locksent = FALSE;
	uptr = warusroff(usrn);

	if (ptr->cloak > 0) {
		prfmsg(PCLOKUP,"The phasers are");
		outprfge(FLT_NONE,usrn);
		return;
	}

	if (ptr->damage >= 100.0) { /* no firing in the brief period between going over 100 and blowing up */
		prfmsg(RNDPHSR);
		outprfge(FLT_NONE,usrn);
		return;
	}

	if (ptr->shieldstat == SHIELDUP && ptr->status == GESTAT_USER)
		shielddn(ptr,usrn);

	if (ptr->phasr < PMINFIRE) {
		prfmsg(PHANONE);
		outprfge(FLT_NONE,usrn);
		return;
	}

	if (neutral(&ptr->coord)) {
		zaphim(ptr,usrn);
		prfmsg(FRCTER);
		outprfge(FLT_NONE,usrn);
		return;
	}

	/* current phaser aim is ship heading offset by the requested degrees */
	deg = (unsigned)(normal(ptr->heading + (double)ptr->degrees) + 0.5);
	if (ptr->status == GESTAT_USER) {
		prfmsg(PFIRED,(int)ptr->phasr,ptr->percent);
		outprfge(FLT_NONE,usrn);
		fired = TRUE;
		lock_simple(ptr,usrn,LOCKPHA,0);
		locksent = TRUE;
	}

	src_neb = (byte)innebula(coord1(ptr->coord.xcoord),coord1(ptr->coord.ycoord));
	for (othusn = 0; othusn < nships; ++othusn) {
		wptr = warshpoff(othusn);
		if (ptr->status == GESTAT_USER && wptr->status == GESTAT_USER)
			wuptr = warusroff(othusn);
		ddistance = cdistance(&ptr->coord,&wptr->coord) * 10000;
		/* coarse filter: live targets, valid-space compatibility, and max phaser reach */
		if (ingegame(othusn) && (wptr->where != 1 || ptr->phasrtype >= phatowrp) && ddistance < 100000.0) {
			/* reject self */
			if (othusn != usrn
				/* reject if recipient and firer are same faction, unless firer is a user */
				&& (shipclass[wptr->shpclass].faction != shipclass[ptr->shpclass].faction
					|| shipclass[ptr->shpclass].faction == 0)
				/* if user, reject shots on distressed ships unless they're in distress from you or you're locked */
				&& (ptr->status == GESTAT_AUTO || wptr->distress == 255 || wptr->distress == usrn || ptr->lock == othusn)
				/* reject if on the same team unless you've locked onto your teammate (naughty!) */
				&& (ptr->status != GESTAT_USER || wptr->status != GESTAT_USER || uptr->teamcode == 0
					|| wuptr->teamcode != uptr->teamcode || ptr->lock == othusn)) {
				heading = (unsigned)(vector(&ptr->coord,&wptr->coord) + 0.5);
				/* phasers only affect targets that fall inside the current firing arc */
				if (smallest(heading,deg) < ptr->percent + PHABIAS) {
					factor = pdamage(ptr,ddistance,ptr->percent);
					factor *= 0.5 + (double)ptr->phasrtype / 2.0;
					factor = ton_fact(wptr,factor);

					/* lower it for hyper */
					if (wptr->where == 1)
						factor = factor / 2.0;

					if (factor > 0.0) {
						if (neutral(&wptr->coord)) {
							prfmsg(PDEFNEUT,username(wptr));
							outprfge(FLT_NONE,usrn);
						}
						else {
							if (ptr->status == GESTAT_AUTO && factor < 0.5)
								continue;
							/* NPCs delay their public fire message until a real hit is confirmed */
							if (fired == FALSE) {
								prfmsg(PFIRED,(int)ptr->phasr,ptr->percent);
								outprfge(FLT_NONE,usrn);
								fired = TRUE;
								if (locksent == FALSE) {
									lock_simple(ptr,usrn,LOCKPHA,0);
									locksent = TRUE;
								}
							}
							/* sub-point hits still count as contact even though they do no hull damage */
							underone = (factor < 1.0);
							if (underone == TRUE)	/* hit, but no damage */
								factor = 0.0;
							hitone = TRUE;
							/* prioritize user hits over npcs so users get credit */
							if (wptr->damage < 100.0
								|| (wptr->lastfired >= 0 && wptr->lastfired < nships && warshpoff(wptr->lastfired)->status == GESTAT_AUTO
									&& ptr->status == GESTAT_USER))
								wptr->lastfired = usrn;
							/* glancing or shielded hits only set half FIRETICKS; real hull hits set the full delay */
							if (underone || wptr->shieldstat == SHIELDUP) {
								if (wptr->cantexit < FIRETICKS / 2)
									wptr->cantexit = FIRETICKS / 2;
								if (ptr->cantexit < FIRETICKS / 2)
									ptr->cantexit = FIRETICKS / 2;
							}
							else {
								wptr->cantexit = FIRETICKS;
								ptr->cantexit = FIRETICKS;
							}
							if (wptr->status == GESTAT_AUTO) {	/* if npc... */
								wptr->cybmine = usrn;	/* engage user */
								wptr->track_grace = CYBGRACE; /* retain this ship as cybmine even if it disappears briefly */
								wptr->tick = 2;		/* do it fast */
								wptr->npcmsg = 255;	/* reset annoy msg tracking */
							}
							/* nebulae and cloaks change the visible hit messaging without changing the damage path */
							targ_neb = (byte)innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
							nebmask = (byte)((src_neb || targ_neb) && !(src_neb && targ_neb && ddistance < (double)NEBRNG));
							/* unshielded targets take hull damage; shielded targets only get shield effects/messages */
							if (wptr->shieldstat != SHIELDUP) {
								damstr((int)factor);
								if (nebmask)
									prfmsg(PHITHIMN,gechrbuf,coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
								else if (wptr->cloak == 10)
									prfmsg(PHITHIMC,gechrbuf);
								else if (wptr->status == GESTAT_AUTO)
									prfmsg(PHITNPC,gechrbuf,username(wptr));
								else
									prfmsg(PHITHIM,gechrbuf,username(wptr));
								outprfge(FLT_NONE,usrn);
								if (nebmask) {
									bearing = (int)(cbearing(&wptr->coord,&ptr->coord,wptr->heading) + .5);
									prfmsg(PHITYOUN,bearing,gechrbuf);
								}
								else if (ptr->status == GESTAT_AUTO)
									prfmsg(PNPCHIT,username(ptr),gechrbuf);
								else
									prfmsg(PHITYOU,username(ptr),gechrbuf);
								outprfge(FLT_NONE,othusn);
								/* cap npc-on-npc phasers so big ships don't get one shot kills */
								if (ptr->status == GESTAT_AUTO && wptr->status == GESTAT_AUTO
									&& factor >= (double)((shipclass[ptr->shpclass].tough_factor + 1) * 5 + 5))
									wptr->damage += (double)((shipclass[ptr->shpclass].tough_factor + 1) * 5 + (gernd() % 5) + 1);
								else
									wptr->damage += factor;
								/* faction dislike scales with actual hull damage dealt */
								set_dislike(uptr,shipclass[wptr->shpclass].faction,(int)factor);
								randamage(wptr,othusn,factor); /*assess any random damage */
							}
							else {
								shmsg = shieldhit(wptr,(int)factor); /* modify the damage */
								/* shielded hits still aggravate the faction, but only by a small fixed amount */
								set_dislike(uptr,shipclass[wptr->shpclass].faction,2);
								if (nebmask)
									prfmsg(PDEFLECN,coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
								else if (wptr->cloak == 10)
									prfmsg(PDEFLECC);
								else if (wptr->status == GESTAT_AUTO)
									prfmsg(PDEFLNPC,username(wptr));
								else
									prfmsg(PDEFLECT,username(wptr));
								outprfge(FLT_NONE,usrn);
								if (nebmask) {
									bearing = (int)(cbearing(&wptr->coord,&ptr->coord,wptr->heading) + .5);
									prfmsg(PHITDEFN,bearing,(factor < 1.0) ? "<1" : spr("%d",(int)factor));
								}
								else if (ptr->status == GESTAT_AUTO)
									prfmsg(PNPCDEF,username(ptr),(factor < 1.0) ? "<1" : spr("%d",(int)factor));
								else
									prfmsg(PHITDEF,username(ptr),(factor < 1.0) ? "<1" : spr("%d",(int)factor));
								outprfge(FLT_NONE,othusn);
								shieldhitmsg(shmsg,othusn);
							}
						}
					}
				}
			}
		}
	}

	if (hitone == TRUE || ptr->status == GESTAT_USER)	/* if NPC, don't actually fire unless doing damage */
		ptr->phasr = 0;
}

/**************************************************************************
** Fire hyper-phasers at hyperspace targets within the beam width        **
**************************************************************************/

void FUNC firehp(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	WARUSR *uptr;
	WARUSR *wuptr;
	unsigned deg;
	double factor;
	byte src_neb, targ_neb, nebmask;

	uptr = warusroff(usrn);

	if (ptr->damage >= 100.0) {
		prfmsg(RNDPHSR);
		outprfge(FLT_NONE,usrn);
		return;
	}

	if (neutral(&ptr->coord)) {
		zaphim(ptr,usrn);
		prfmsg(FRCTER);
		outprfge(FLT_NONE,usrn);
		return;
	}

	if (fluxstat(ptr,usrn,HPFIRAMT) == 1) {
		/* hyper-phasers fire along a fixed beam centered on the requested offset */
		deg = (unsigned)(normal(ptr->heading + (double)ptr->degrees) + 0.5);
		prfmsg(HPFIRED,deg);
		outprfge(FLT_NONE,usrn);
		lock_simple(ptr,usrn,LOCKHPHA,0);
		src_neb = (byte)innebula(coord1(ptr->coord.xcoord),coord1(ptr->coord.ycoord));
		/* unlike standard phasers, firing always spends the flux and the one-shot hyperspace cooldown */
		ptr->energy -= HPFIRAMT;
		ptr->hypha = 1;
		for (othusn = 0; othusn < nships; ++othusn) {
			wptr = warshpoff(othusn);
			if (ptr->status == GESTAT_USER && wptr->status == GESTAT_USER)
				wuptr = warusroff(othusn);
			ddistance = cdistance(&ptr->coord,&wptr->coord) * 10000;
			/* hyper-phasers only affect live hyperspace targets within the broad search radius */
			if (ingegame(othusn) && wptr->where == 1 && ddistance < 100000.0) {
				if (othusn != usrn
					&& (shipclass[wptr->shpclass].faction != shipclass[ptr->shpclass].faction
						|| shipclass[ptr->shpclass].faction == 0)
					&& (ptr->status == GESTAT_AUTO || wptr->distress == 255 || wptr->distress == usrn || ptr->lock == othusn)
					&& (ptr->status != GESTAT_USER || wptr->status != GESTAT_USER || uptr->teamcode == 0
						|| wuptr->teamcode != uptr->teamcode || ptr->lock == othusn)) {
					heading = (unsigned)(vector(&ptr->coord,&wptr->coord) + 0.5);
					/* hyper-phasers use a fixed beam width rather than the configurable phaser spread */
					if (smallest(heading,deg) < HPBEAMW) {
						/* once a target is in the beam, hyper-phasers still require a normal scan-quality solution */
						if (ddistance < (double)ship_scanrange(ptr)) {
							factor = pdamage(ptr,ddistance,0);
							factor *= 0.5 + (double)ptr->phasrtype / 2.0;
							factor = ton_fact(wptr,factor);

							if (factor > 0.0) {
								if (neutral(&wptr->coord)) {
									prfmsg(PDEFNEUT,username(wptr));
									outprfge(FLT_NONE,usrn);
								}
								else {
									if (factor < 1.0)	/* hit, but no damage */
										factor = 0.0;
									damstr((int)factor);
									targ_neb = (byte)innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
									/* nebulae still mask the messaging even though the hit occurs entirely in hyperspace */
									nebmask = (byte)((src_neb || targ_neb) && !(src_neb && targ_neb && ddistance < (double)NEBRNG));

									if (nebmask)
										prfmsg(HPHITNEB,gechrbuf,coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
									else if (wptr->status == GESTAT_AUTO)
										prfmsg(HPHITN,gechrbuf,username(wptr));
									else
										prfmsg(HPHITM,gechrbuf,username(wptr));
									outprfge(FLT_NONE,usrn);
									if (nebmask) {
										bearing = (int)(cbearing(&wptr->coord,&ptr->coord,wptr->heading) + .5);
										prfmsg(HPHITUN,bearing,gechrbuf);
									}
									else if (ptr->status == GESTAT_AUTO)
										prfmsg(HPNHITU,username(ptr),gechrbuf);
									else
										prfmsg(HPHITU,username(ptr),gechrbuf);
									outprfge(FLT_NONE,othusn);
									/* prioritize user hits over npcs so users get credit */
									if (wptr->damage < 100.0
										|| (wptr->lastfired >= 0 && wptr->lastfired < nships && warshpoff(wptr->lastfired)->status == GESTAT_AUTO
											&& ptr->status == GESTAT_USER))
										wptr->lastfired = usrn;
									/* cap npc-on-npc phasers so big ships don't get one shot kills */
									if (ptr->status == GESTAT_AUTO && wptr->status == GESTAT_AUTO
										&& factor >= (double)((shipclass[ptr->shpclass].tough_factor + 1) * 5 + 5))
										wptr->damage += (double)((shipclass[ptr->shpclass].tough_factor + 1) * 5 + (gernd() % 5) + 1);
									else
										wptr->damage += factor;
									set_dislike(uptr,shipclass[wptr->shpclass].faction,(int)factor);
									if (wptr->status == GESTAT_AUTO) {	/* if npc... */
										wptr->cybmine = usrn;	/* engage user */
										wptr->track_grace = CYBGRACE; /* retain this ship as cybmine even if it disappears briefly */
										wptr->tick = 2;		/* do it fast */
										wptr->npcmsg = 255;	/* reset annoy msg tracking */
									}
									/* any real hyper-phaser hit imposes the full exit delay on both ships */
									wptr->cantexit = FIRETICKS;
									ptr->cantexit = FIRETICKS;
									randamage(wptr,othusn,factor); /* assess any random damage */
								}
							}
						}
					}
				}
			}
		}
	}
	else {
		prfmsg(HNOFIRP);
		outprfge(FLT_NONE,usrn);
	}
}

/**************************************************************************
** Launch a torpedo at a locked target                                   **
**************************************************************************/

int FUNC torp(WARSHP *ptr, int usrn, int shpnum)
{
	WARSHP *wptr;
	WARSHP *optr;
	byte others[MAXTORPS], ocount, found;
	int i, oi, slot;

	if (ptr->damage >= 100.0) {
		prfmsg(RNDTORP);
		outprfge(FLT_NONE,usrn);
		return 0;
	}

	if (lockon(ptr,0,shpnum,usrn) == 1) {
		wptr = warshpoff(shpnum);
		slot = -1;
		/* incoming torpedoes are tracked on the target ship, one slot per live inbound round */
		for (i = 0; i < MAXTORPS; ++i) {
			if (wptr->ltorps[i].distance == 0) {
				slot = i;
				break;
			}
		}
		if (slot < 0) {
			prfmsg(TORMANY,MAXTORPS);
			ocount = 0;
			for (i = 0; i < MAXTORPS; ++i) {
				if (wptr->ltorps[i].distance > 0
					&& wptr->ltorps[i].channel != (byte)usrn
					&& wptr->ltorps[i].channel < nships
					&& ingegame((int)wptr->ltorps[i].channel)) {
					found = 0;
					for (oi = 0; oi < (int)ocount; ++oi) {
						if (others[oi] == wptr->ltorps[i].channel) {
							found = 1;
							break;
						}
					}
					if (!found && ocount < MAXTORPS)
						others[ocount++] = wptr->ltorps[i].channel;
				}
			}
			if (ocount > 0) {
				gechrbuf[0] = 0;
				for (oi = 0; oi < (int)ocount; ++oi) {
					optr = warshpoff((int)others[oi]);
					if (oi > 0)
						strcat(gechrbuf,", ");
					strcat(gechrbuf,username(optr));
				}
				strcat(gechrbuf,(ocount > 1) ? " are" : " is");
				prfmsg(TORMANY2,gechrbuf);
			}
			outprfge(FLT_NONE,usrn);
			return 0;
		}
		if (ptr->torps_fired >= shipclass[ptr->shpclass].max_torps) {
			/* all launchers are reloading */
			if (ptr->status != GESTAT_AUTO) {
				if (shipclass[ptr->shpclass].max_torps == 1)
					prfmsg(TORPRELS);
				else
					prfmsg(TORPRELM);
				outprfge(FLT_NONE,usrn);
			}
			return 0;
		}
		prfmsg(TFIRE1);
		outprfge(FLT_NONE,usrn);
		--ptr->items[I_TORPEDO];
		++ptr->torps_fired;
		lock_proj(usrn,shpnum,LOCKTOR,LOCKTORN);
		prfmsg(TFIRE2,shpltr(shpnum,usrn));
		outprfge(FLT_NONE,shpnum);
		/* store the initial travel distance plus a small offset for some reason */
		wptr->ltorps[slot].distance = (unsigned)(cdistance(&ptr->coord,&(wptr->coord))*10000);
		wptr->ltorps[slot].distance += 20;	/* why? */
		wptr->ltorps[slot].channel = (byte)usrn;
		wptr->cantexit = FIRETICKS;
		ptr->cantexit = FIRETICKS;
		return 1;
	}
	return 0;
}

/**************************************************************************
** Launch a missile at a locked target                                   **
**************************************************************************/

int FUNC misl(WARSHP *ptr, int usrnum, int shpnum, unsigned energy, unsigned eng_flu)
{
	WARSHP *wptr;
	WARSHP *optr;
	byte others[MAXMISSL], ocount, found;
	int i, oi, slot;

	if (ptr->damage >= 100.0) {
		prfmsg(RNDMISL);
		outprfge(FLT_NONE,usrnum);
		return 0;
	}

	if (lockon(ptr,1,shpnum,usrnum) == 1) {
		wptr = warshpoff(shpnum);
		slot = -1;
		/* incoming missiles are tracked on the target ship, one slot per live inbound round */
		for (i = 0; i < MAXMISSL; ++i) {
			if (wptr->lmissl[i].distance == 0) {
				slot = i;
				break;
			}
		}
		if (slot < 0) {
			prfmsg(MISMANY,MAXMISSL);
			ocount = 0;
			for (i = 0; i < MAXMISSL; ++i) {
				if (wptr->lmissl[i].distance > 0
					&& wptr->lmissl[i].channel != (byte)usrnum
					&& wptr->lmissl[i].channel < nships
					&& ingegame((int)wptr->lmissl[i].channel)) {
					found = 0;
					for (oi = 0; oi < (int)ocount; ++oi) {
						if (others[oi] == wptr->lmissl[i].channel) {
							found = 1;
							break;
						}
					}
					if (!found && ocount < MAXMISSL)
						others[ocount++] = wptr->lmissl[i].channel;
				}
			}
			if (ocount > 0) {
				gechrbuf[0] = 0;
				for (oi = 0; oi < (int)ocount; ++oi) {
					optr = warshpoff((int)others[oi]);
					if (oi > 0)
						strcat(gechrbuf,", ");
					strcat(gechrbuf,username(optr));
				}
				strcat(gechrbuf,(ocount > 1) ? " are" : " is");
				prfmsg(MISMANY2,gechrbuf);
			}
			outprfge(FLT_NONE,usrnum);
			return 0;
		}
		if (ptr->missl_fired >= shipclass[ptr->shpclass].max_missl) {
			if (ptr->status != GESTAT_AUTO) {
				if (shipclass[ptr->shpclass].max_missl == 1)
					prfmsg(MISSRELS);
				else
					prfmsg(MISSRELM);
				outprfge(FLT_NONE,usrnum);
			}
			return 0;
		}
		if (fluxstat(ptr,usrnum,eng_flu) == 0) {
			prfmsg(MISSHRT);
			outprfge(FLT_NONE,usrnum);
			return 0;
		}
		prfmsg(MFIRE1,energy);
		outprfge(FLT_NONE,usrnum);
		--ptr->items[I_MISSILE];
		++ptr->missl_fired;
		ptr->energy -= eng_flu;
		lock_proj(usrnum,shpnum,LOCKMIS,LOCKMISN);
		prfmsg(MFIRE2,shpltr(shpnum,usrnum));
		outprfge(FLT_NONE,shpnum);
		/* store the initial travel distance plus a small offset, along with the missile's payload energy */
		wptr->lmissl[slot].distance = (unsigned)(cdistance(&ptr->coord,&(wptr->coord))*10000);
		wptr->lmissl[slot].distance += 20;
		wptr->lmissl[slot].channel = (byte)usrnum;
		wptr->lmissl[slot].energy = energy;
		wptr->cantexit = FIRETICKS;
		ptr->cantexit = FIRETICKS;
		return 1;
	}
	return 0;
}


/**************************************************************************
** Look up the ships this player has                                     **
**************************************************************************/

void FUNC lookupshp(void)
{
	int noships = 0;

	/* get the user record from MPOGEUSR.dat */
	if (!(geudb(GELOOKUP,usaptr->userid, waruptr))) {
		/* Not found.... Better make up something */
		initusr(usaptr->userid); /* create his account */
		if (!geudb(GEADD,tmpusr.userid,&tmpusr))
			geshocst(0,spr("GE:ERR:User Add Fail %s",usaptr->userid));
		memcpy(waruptr,&tmpusr,sizeof(WARUSR));	/* make it the current user */
	}
	else {
		/* Got it! ... Dang are we lucky */
		if (!geudb(GEGET,usaptr->userid, waruptr)) {
			/* lookup succeeded but the fetch failed; rebuild a usable in-memory user record */
			geshocst(0,spr("GE:ERR:User Get Fail %s",usaptr->userid));
			initusr(usaptr->userid); /* create his account */
			if (!geudb(GEADD,tmpusr.userid,&tmpusr))
				geshocst(0,spr("GE:ERR:User Add Fail %s",usaptr->userid));
			memcpy(waruptr,&tmpusr,sizeof(WARUSR));	/* make it the current user */
		}
	}

	setbtv(gebb1);

	/* don't count if no ships at all, or no ships for this user */
	if (qlobtv(0) && gepdb(GELOOKUPNAME, usaptr->userid, 0, warsptr)) {
		/* get a total for user ship count */
		do {
			gcrbtv(warsptr,0);
			if (!sameas(usaptr->userid, warsptr->userid))
				break;
			if (!valid_user_ship(warsptr))
				continue;
			noships++;
		} while (qnxbtv());

		waruptr->noships = noships;
		if (!gepdb(GELOOKUPNAME, usaptr->userid, 0, warsptr))
			geshocst(0,spr("GE:ERR:Ship Cursor Reset Fail %s",usaptr->userid));
	}

	if (noships == 0) {
		initshp(usaptr->userid,0); /* give the dude a class 1 ship */
		if (!gepdb(GEADD,tmpshp.userid,tmpshp.shipno,&tmpshp))
			geshocst(0,spr("GE:ERR:Ship Add Fail %s",usaptr->userid));
		memcpy(warsptr,&tmpshp,sizeof(WARSHP));	/* make is the current ship */
		waruptr->noships = 1;
		prfmsg(FIRSTIME);
		outprfge(FLT_NONE,usrnum);
	}
	else if (noships > 1) {
		findships(0, 0);
		prfmsg(FLEET3);
		usrptr->substt = CHOOSESH;
		outprfge(FLT_NONE,usrnum);
		return;
	}
	else if (noships == 1) {
		setbtv(gebb1);
		/* verify the single-ship list result before trusting scantab[0] */
		if (findships(0,1) != 1 || scantab[usrnum].ship[0].shipno == 0) {
			geshocst(0,spr("GE:ERR:Ship List Fail %s",usaptr->userid));
			initshp(usaptr->userid,0); /* give the dude a class 1 ship */
			if (!gepdb(GEADD,tmpshp.userid,tmpshp.shipno,&tmpshp))
				geshocst(0,spr("GE:ERR:Ship Add Fail %s",usaptr->userid));
			memcpy(warsptr,&tmpshp,sizeof(WARSHP));	/* make is the current ship */
			prfmsg(FIRSTIME);
			outprfge(FLT_NONE,usrnum);
			tossingegame(); /* into the game you go bud! */
			return;
		}

		if (gepdb(GEGET,usaptr->userid,scantab[usrnum].ship[0].shipno,warsptr)) {
			if (!valid_user_ship(warsptr)) {
				geshocst(0,spr("GE:DBG:Ship Load Bad %s",usaptr->userid));
				initshp(usaptr->userid,0); /* give the dude a class 1 ship */
				if (!gepdb(GEADD,tmpshp.userid,tmpshp.shipno,&tmpshp))
					geshocst(0,spr("GE:ERR:Ship Add Fail %s",usaptr->userid));
				memcpy(warsptr,&tmpshp,sizeof(WARSHP));	/* make is the current ship */
				prfmsg(FIRSTIME);
				outprfge(FLT_NONE,usrnum);
			}
			tossingegame(); /* into the game you go bud! */
			return;
		}
		else {
			/* somehow lost the ship... make one anyway */
			geshocst(0,spr("GE:DBG:Ship Load Err %s",usaptr->userid));
			initshp(usaptr->userid,0); /* give the dude a class 1 ship */
			if (!gepdb(GEADD,tmpshp.userid,tmpshp.shipno,&tmpshp))
				geshocst(0,spr("GE:ERR:Ship Add Fail %s",usaptr->userid));
			memcpy(warsptr,&tmpshp,sizeof(WARSHP));	/* make is the current ship */
			prfmsg(FIRSTIME);
			outprfge(FLT_NONE,usrnum);
		}
	}
	tossingegame();
}

/**************************************************************************
** Broadcast a ship's visible appearance in its current sector          **
**************************************************************************/

void FUNC suddenappear(WARSHP *ptr, int usrn)
{
	if (ptr->shipname[0] == 0)
		prfmsg(ENTWARNO,ptr->userid);
	else
		prfmsg(ENTWAR,ptr->shipname);
	outsect(FLT_NONE,&ptr->coord,usrn);
}

/**************************************************************************
** Check whether entrant is within the recipient's entry-message range  **
**************************************************************************/

static int entryinrng(int entrant, int recipient)
{
	WARSHP *eptr, *rptr;
	double dist;

	if (!ingegame(recipient))
		return FALSE;

	eptr = warshpoff(entrant);
	rptr = warshpoff(recipient);
	dist = cdistance(&eptr->coord,&rptr->coord) * 10000.0;
	return dist <= (double)ship_scanrange(rptr);
}

/**************************************************************************
** Send the formatted entry announcement for one ship to one recipient  **
**************************************************************************/

static void send_entrymsg(int entrant, int recipient)
{
	WARSHP *eptr;
	WARUSR *euptr;

	eptr = warshpoff(entrant);
	euptr = warusroff(entrant);

	if (eptr->shipname[0] == 0)
		prfmsg(ANNOUNO,euptr->userid,shipclass[eptr->shpclass].typename,showupg(eptr));
	else
		prfmsg(ANNOUN,shipclass[eptr->shpclass].typename,showupg(eptr),eptr->shipname,euptr->userid);
	outprfge(FLT_ENTRY,recipient);
}

/**************************************************************************
** Send the formatted exit announcement for one ship to one recipient   **
**************************************************************************/

static void send_exitmsg(int entrant, int recipient)
{
	WARSHP *eptr;
	WARUSR *euptr;

	eptr = warshpoff(entrant);
	euptr = warusroff(entrant);

	if (eptr->shipname[0] == 0)
		prfmsg(PEACEONO,euptr->userid);
	else
		prfmsg(PEACEOUT,euptr->userid,eptr->shipname);
	outprfge(FLT_ENTRY,recipient);
}

/**************************************************************************
** Clear all deferred entry/exit notification state for one user        **
**************************************************************************/

static void clear_entrymsg(int usrn)
{
	entrytab[usrn].active = 0;
	entrytab[usrn].ticks = 0;
	setmem(entrysent + (usrn * entrybytes), entrybytes, 0);
	setmem(entrypend + (usrn * entrybytes), entrybytes, 0);
}

/**************************************************************************
** Start deferred entry notifications for a newly entering user         **
**************************************************************************/

void FUNC start_entrymsg(int usrn)
{
	int zothusn;
	int anypend = FALSE;
	int mode;
	byte *sentptr, *pendptr;
	byte mask;

	clear_entrymsg(usrn);
	sentptr = entrysent + (usrn * entrybytes);
	pendptr = entrypend + (usrn * entrybytes);

	for (zothusn = 0; zothusn < nterms; ++zothusn) {
		if (zothusn == usrn || !ingegame(zothusn))
			continue;

		mode = warusroff(zothusn)->options[MSG_FILTER] & MSGF_ENTRY_MASK;
		mask = (byte)(1 << (zothusn & 7));

		if (mode == 0x40)
			continue;

		if (mode == 0x00 || entryinrng(usrn,zothusn)) {
			send_entrymsg(usrn,zothusn);
			sentptr[zothusn >> 3] |= mask;
		}
		else {
			pendptr[zothusn >> 3] |= mask;
			anypend = TRUE;
		}
	}

	if (anypend) {
		entrytab[usrn].active = 1;
		entrytab[usrn].ticks = 0;
	}
}

/**************************************************************************
** Advance deferred entry notifications once per game tick              **
**************************************************************************/

void FUNC tick_entrymsg(void)
{
	int usrn, zothusn;
	byte *sentptr, *pendptr;
	byte mask;

	for (usrn = 0; usrn < nterms; ++usrn) {
		if (!entrytab[usrn].active)
			continue;

		if (!ingegame(usrn)) {
			clear_entrymsg(usrn);
			continue;
		}

		++entrytab[usrn].ticks;
		sentptr = entrysent + (usrn * entrybytes);
		pendptr = entrypend + (usrn * entrybytes);

		if (entrytab[usrn].ticks < ENTRYWAIT) {
			for (zothusn = 0; zothusn < nterms; ++zothusn) {
				mask = (byte)(1 << (zothusn & 7));
				if (!(pendptr[zothusn >> 3] & mask))
					continue;
				if (!ingegame(zothusn)) {
					pendptr[zothusn >> 3] &= ~mask;
					continue;
				}
				if (entryinrng(usrn,zothusn)) {
					send_entrymsg(usrn,zothusn);
					pendptr[zothusn >> 3] &= ~mask;
					sentptr[zothusn >> 3] |= mask;
				}
			}
		}
		else {
			for (zothusn = 0; zothusn < nterms; ++zothusn) {
				mask = (byte)(1 << (zothusn & 7));
				if (!(pendptr[zothusn >> 3] & mask))
					continue;
				if (ingegame(zothusn)) {
					send_entrymsg(usrn,zothusn);
					sentptr[zothusn >> 3] |= mask;
				}
				pendptr[zothusn >> 3] &= ~mask;
			}
			entrytab[usrn].active = 0;
			entrytab[usrn].ticks = 0;
		}
	}
}

/**************************************************************************
** Send exit notifications for a user leaving the game                  **
**************************************************************************/

void FUNC exit_entrymsg(int usrn)
{
	int zothusn;
	byte *sentptr;
	byte mask;

	sentptr = entrysent + (usrn * entrybytes);

	if (entrypend_empty(usrn)) {
		for (zothusn = 0; zothusn < nterms; ++zothusn) {
			if (zothusn != usrn && ingegame(zothusn))
				send_exitmsg(usrn,zothusn);
		}
	}
	else {
		for (zothusn = 0; zothusn < nterms; ++zothusn) {
			mask = (byte)(1 << (zothusn & 7));
			if (sentptr[zothusn >> 3] & mask) {
				if (ingegame(zothusn))
					send_exitmsg(usrn,zothusn);
			}
		}
	}

	clear_entrymsg(usrn);
}

/**************************************************************************
** Finish login/setup and place the selected ship into the game         **
**************************************************************************/

void FUNC tossingegame(void)
{
	int zothusn;
	byte *sentptr;
	byte mask;

	start_entrymsg(usrnum);

	prfmsg(ENTSHP,waruptr->userid);
	outprfge(FLT_NONE,usrnum);

	update_scantab(warshpoff(usrnum),usrnum);

	if (warsptr->cloak != 10)
		suddenappear(warsptr,usrnum);

	btupmt(usrnum,'>');
	prfmsg(WELCOM,waruptr->userid);
	outprfge(FLT_NONE,usrnum);
	usrptr->substt = FIGHTSUB;
	warsptr->status = GESTAT_USER;

	for (zothusn = 0; zothusn < nterms; ++zothusn) {
		if (zothusn == usrnum || !ingegame(zothusn) || entrypend_empty(zothusn))
			continue;

		sentptr = entrysent + (zothusn * entrybytes);
		mask = (byte)(1 << (usrnum & 7));
		sentptr[usrnum >> 3] |= mask;
	}

	assign_cybs(usrnum,0);
}

/**************************************************************************
** Initialize the temporary ship record for a new ship                   **
** NOTE: waruptr MUST be set to this channel first                       **
**************************************************************************/

int FUNC initshp(char *userid, int type)
{
	double ddistance;
	int i, flag;

	logthis(spr("GE:DBG:initship %d",type));
	logthis(spr("%s",userid));
	/* reject invalid class indexes before touching shipclass[] */
	if (!VALID_SHPCLASS(type)) {
		geshocst(0,spr("GE:ERR:initship bad class %d for %s",type,userid));
		return 1;
	}
	/* build a fresh ship record from zero, then layer explicit defaults on top */
	setmem(&tmpshp,sizeof(WARSHP),0);
	strncpy(tmpshp.userid,userid,UIDSIZ);
	tmpshp.userid[UIDSIZ - 1] = 0;
	tmpshp.shpclass	= type;

	if (shipclass[type].max_type == CLASSTYPE_USER) {
		tmpshp.coord.xcoord = NEUTRAL_X + rndm(.9999);
		tmpshp.coord.ycoord = NEUTRAL_Y + rndm(.9999);
		getsector(&tmpshp.coord);
		flag = 1;

		while (flag == 1) {
			tmpshp.coord.xcoord = NEUTRAL_X + rndm(.9999);
			tmpshp.coord.ycoord = NEUTRAL_Y + rndm(.9999);
			flag = 0;
			for (i = 0; i < sector.numplan; ++i) {
				if (sector.ptab[i].coord.xcoord != 0) {
					ddistance = cdistance(&tmpshp.coord,&sector.ptab[i].coord) * 10000;
					if (ddistance < 1000)
						flag = 1;
				}
			}
		}

		tmpshp.phasrtype	= 1;
		tmpshp.shieldtype	= 1;
		tmpshp.items[I_FLUXPOD]	= 3;
	}

	tmpshp.heading		= gernd()%360;
	tmpshp.head2b		= tmpshp.heading;
	tmpshp.phasr		= 100;
	tmpshp.lastfired	= -1;
	tmpshp.energy		= 50000L;
	tmpshp.shieldstat	= SHIELDDN;
	tmpshp.tponder		= TPONNORM;
	tmpshp.distress		= 255;
	tmpshp.lock		= -1;

	tmpshp.shipno = waruptr->topshipno + 1;

	++waruptr->topshipno;
	if (shipclass[type].max_type == CLASSTYPE_USER)
		++waruptr->noships;

	tmpshp.topspeed = shipclass[tmpshp.shpclass].max_warp;
	logthis(spr("Created ship - topspeed = %d",tmpshp.topspeed));
	return 0;
}

/**************************************************************************
** Initialize the temporary user record for a new GE account            **
**************************************************************************/

int FUNC initusr(char *userid)
{
	setmem(&tmpusr,sizeof(WARUSR),0);
	strncpy(tmpusr.userid,userid,UIDSIZ);
	tmpusr.userid[UIDSIZ - 1] = 0;
	tmpusr.cash		= startcash;
	tmpusr.options[0]	= FULLNAMES; /* set scan default */

	return 0;
}

/**************************************************************************
** Validate that a loaded ship record is a legal user ship              **
**************************************************************************/

int FUNC valid_user_ship(WARSHP *ptr)
{
	if (!VALID_SHPCLASS(ptr->shpclass)) {
		geshocst(0, spr("GE:ERR:BADUSRSHPCLS cls=%d shipno=%d uid=%s",
			ptr->shpclass, ptr->shipno, ptr->userid));
		return FALSE;
	}

	if (shipclass[ptr->shpclass].max_type != CLASSTYPE_USER) {
		geshocst(0, spr("GE:ERR:BADUSRSHPTYPE cls=%d type=%u shipno=%d uid=%s",
			ptr->shpclass, shipclass[ptr->shpclass].max_type, ptr->shipno, ptr->userid));
		return FALSE;
	}

	return TRUE;
}

/**************************************************************************
** Map an in-memory automaton slot to its configured ship class          **
**************************************************************************/

static int auto_slot_class(int usrn)
{
	int clscnt;
	int i;

	if (usrn < nterms || usrn >= nships)
		return -1;

	clscnt = usrn - nterms;
	for (i = 0; i < tot_classes; ++i) {
		if (shipclass[i].max_type == CLASSTYPE_CYBORG ||
			shipclass[i].max_type == CLASSTYPE_DROID) {
			if (clscnt < shipclass[i].tot_to_create)
				return i;
			clscnt -= shipclass[i].tot_to_create;
		}
	}

	return -1;
}

/**************************************************************************
** Return the expected cyborg class for a slot, or -1 if it is not Cyb   **
**************************************************************************/

int FUNC cyb_slot_class(int usrn)
{
	int class;

	class = auto_slot_class(usrn);
	if (class < 0)
		return -1;

	if (shipclass[class].max_type != CLASSTYPE_CYBORG)
		return -1;

	return class;
}

/**************************************************************************
** Parse a saved @Cybrg-N userid into its slot number                    **
**************************************************************************/

int FUNC cyb_user_slot(char *userid)
{
	int i;
	int usrn;

	if (strncmp(userid, "@Cybrg-", 7) != 0)
		return -1;

	if (userid[7] < '0' || userid[7] > '9')
		return -1;

	usrn = 0;
	for (i = 7; i < UIDSIZ && userid[i] != 0; ++i) {
		if (userid[i] < '0' || userid[i] > '9')
			return -1;
		usrn = (usrn * 10) + (userid[i] - '0');
		if (usrn >= nships)
			return -1;
	}

	return usrn;
}

/**************************************************************************
** Check whether a userid names a currently configured Cyb slot          **
**************************************************************************/

static int valid_cyb_userid(char *userid)
{
	int usrn;

	usrn = cyb_user_slot(userid);
	if (usrn < 0)
		return FALSE;

	return cyb_slot_class(usrn) >= 0;
}

/**************************************************************************
** Remove saved @ records that cannot belong to current Cyb slots        **
**************************************************************************/

void FUNC prune_stale_auto_records(void)
{
	int deleted;
	int shipdel;
	int userdel;

	shipdel = 0;
	do {
		deleted = FALSE;
		setbtv(gebb1);
		if (qlobtv(0)) {
			do {
				gcrbtv(&tmpshp, 0);
				if (tmpshp.userid[0] == '@' && !valid_cyb_userid(tmpshp.userid)) {
					geshocst(1, spr("GE:INF:AUTOSHPDEL uid=%s shipno=%d",
						tmpshp.userid, tmpshp.shipno));
					delbtv();
					++shipdel;
					deleted = TRUE;
					break;
				}
			} while (qnxbtv());
		}
	} while (deleted);

	userdel = 0;
	do {
		deleted = FALSE;
		setbtv(gebb5);
		if (qlobtv(0)) {
			do {
				gcrbtv(&tmpusr, 0);
				if (tmpusr.userid[0] == '@' && !valid_cyb_userid(tmpusr.userid)) {
					geshocst(1, spr("GE:INF:AUTOUSRDEL uid=%s", tmpusr.userid));
					delbtv();
					++userdel;
					deleted = TRUE;
					break;
				}
			} while (qnxbtv());
		}
	} while (deleted);

	if (shipdel != 0 || userdel != 0)
		geshocst(1, spr("GE:INF:Auto cleanup removed %d ships, %d users",
			shipdel, userdel));
}

/**************************************************************************
** find and list all the ships a single user has                         **
**************************************************************************/

int FUNC findships(int direction, int quiet)
{
	char *upg;
	int found = 0;
	int i, j, step, thispage, lastpage;
	int first_no = 0;
	int last_no = 0;
	int before = 0;
	int first_shipno = 0;
	SCANTAB *sptr;

	setbtv(gebb1);
	sptr = &scantab[usrnum];

	/* if we're paging, grab current page’s known first/last ship numbers from scantab */
	if (direction != 0) {
		for (i = 0; i < NOSCANTAB; ++i) {
			if (sptr->ship[i].shipno != 0) {
				first_no = sptr->ship[0].shipno;
				/* find last non-zero */
				for (j = NOSCANTAB - 1; j >= 0; --j) {
					if (sptr->ship[j].shipno != 0) {
						last_no = sptr->ship[j].shipno;
						break;
					}
				}
				break;
			}
		}
	}
	else {
		/* make sure we're at beginning */
		gepdb(GELOOKUPNAME, usaptr->userid, 0, warsptr);
	}

	/* page navigation */
	if (direction > 0 && last_no != 0) {
		/* last ship of current page, plus one */
		if (gepdb(GEGET, usaptr->userid, last_no, warsptr)) {
			if (!qnxbtv())	/* no next page */
				return 0;
		}
	}
	else if (direction < 0 && first_no != 0) {
		/* first ship of current page, then back NOSCANTAB, plus one */
		if (gepdb(GEGET, usaptr->userid, first_no, warsptr)) {
			step = 0;
			while (step < NOSCANTAB && qprbtv())
				++step;
		}
	}

	/* clear the page buffer to avoid stale shipnos */
	for (i = 0; i < NOSCANTAB; ++i)
		sptr->ship[i].shipno = 0;

	/* print header and one page */
	if (!quiet)
		prf("%s    Class                Name                 Sector         Status\r", CLR_CYAN2);
	do {
		gcrbtv(warsptr, 0);

		if (!sameas(usaptr->userid, warsptr->userid))
			break;
		if (!valid_user_ship(warsptr))
			continue;

		setsect(warsptr);
		if (!quiet) {
			upg = showupg(warsptr);
			prf("%s%2d  %s%s", CLR_WHITE2, found + 1,
				shipclass[warsptr->shpclass].typename, upg);
			for (j = strlen(shipclass[warsptr->shpclass].typename) + strlen(upg); j < 20; ++j)
				prf(" ");
			prf(" %-20s %6d %6d  ",
				(warsptr->shipname[0] == 0 ? " <NO NAME> " : warsptr->shipname), xsect, ysect);

			if (warsptr->energy < 5000 && warsptr->items[I_FLUXPOD] == 0)
				prf("%sflux depleted%s", CLR_RED1, CLR_WHITE2);
			else if (warsptr->damage > 75.5)
				prf("%ssevere%s damage", CLR_RED1, CLR_WHITE2);
			else if (warsptr->damage > 50.5)
				prf("%sheavy%s damage", CLR_RED1, CLR_WHITE2);
			else if (warsptr->cloak > 0)
				prf("cloak %sON%s", CLR_GREEN2, CLR_WHITE1);
			else if (warsptr->items[I_GOLD] >= 500) {
				sprintf(gechrbuf, "%lu", warsptr->items[I_GOLD]);
				prf("%s gold", gechrbuf);
			}
			else if (warsptr->where > 10)
				prf("orbiting planet %s%d%s", CLR_BLUE2, warsptr->where - 10, CLR_WHITE2);

			prf("\r");
			prf(CLR_WHITE2);
		}

		sptr->ship[found].shipno = warsptr->shipno;
		++found;
	} while (qnxbtv() && found < NOSCANTAB);

	/* display page X of Y if needed */
	if (!quiet && waruptr->noships > NOSCANTAB) {
		first_shipno = sptr->ship[0].shipno;
		before = 0;

		if (first_shipno != 0 && gepdb(GEGET, usaptr->userid, first_shipno, warsptr)) {
			while (qprbtv()) {
				gcrbtv(warsptr, 0);
				if (!sameas(usaptr->userid, warsptr->userid))
					break;
				if (!valid_user_ship(warsptr))
					continue;
				++before;
			}

			/* restore cursor to this page's first ship */
			gepdb(GEGET, usaptr->userid, first_shipno, warsptr);
		}

		thispage = (before / NOSCANTAB) + 1;
		lastpage = (waruptr->noships + NOSCANTAB - 1) / NOSCANTAB;
		prf("%sPage %d of %d, ", CLR_CYAN2, thispage, lastpage);
		if (thispage == 1)
			prf("\"n\" for next page.\r");
		else if (thispage == lastpage)
			prf("\"p\" for previous page.\r");
		else
			prf("\"p\" for previous page, \"n\" for next page.\r");
	}
	outprfge(FLT_NONE, usrnum);

	return found;
}

/**************************************************************************
** Select the ship to board from the list                                **
**************************************************************************/

void FUNC selectship(void)
{
	int selection;
	int shpno;
	int page_count;

	/* exit back */
	if (sameas(margv[0], "x") || sameas(margv[0], "X")) {
		disp_main_menu();
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = 1;
		return;
	}

	/* numeric selection on current page */
	selection = atoi(margv[0]) - 1;
	if (selection >= 0 && selection < NOSCANTAB && scantab[usrnum].ship[selection].shipno != 0) {
		shpno = scantab[usrnum].ship[selection].shipno;
		setbtv(gebb1);
		if (gepdb(GEGET, usaptr->userid, shpno, warsptr)) {
			if (!valid_user_ship(warsptr)) {
				prfmsg(FLEET4);
				findships(0, 0);
				prfmsg(FLEET3);
				usrptr->substt = CHOOSESH;
				outprfge(FLT_NONE, usrnum);
				return;
			}
			tossingegame(); /* into the game you go bud! */
			return;
		}
	}

	/* paging */
	if (sameas(margv[0], "N") || sameas(margv[0], "n")) {
		page_count = findships(1, 0);
		if (page_count == 0) {
			prfmsg(FLEET4);
			page_count = findships(0, 0);
		}
		prfmsg(FLEET3);
		usrptr->substt = CHOOSESH;
		outprfge(FLT_NONE, usrnum);
		return;
	}

	if (sameas(margv[0], "P") || sameas(margv[0], "p")) {
		page_count = findships(-1, 0);
		if (page_count == 0) {
			prfmsg(FLEET4);
			page_count = findships(0, 0);
		}
		prfmsg(FLEET3);
		usrptr->substt = CHOOSESH;
		outprfge(FLT_NONE, usrnum);
		return;
	}

	/* anything else, show error and first page again */
	prfmsg(FLEET4);
	page_count = findships(0, 0);
	prfmsg(FLEET3);
	usrptr->substt = CHOOSESH;
	outprfge(FLT_NONE, usrnum);
}

/**************************************************************************
** Repair the ship                                                       **
**************************************************************************/

void FUNC repairship(WARSHP *ptr, int usrn)
{
	if (ptr->repair > 0) {
		if (ptr->cantexit > 0) {
			prfmsg(MAINT10);
			ptr->repair = 0;
			outprfge(FLT_NONE, usrn);
			return;
		}
	}
}

/**************************************************************************
** Rotate the ship                                                       **
**************************************************************************/

void FUNC rotateship(WARSHP *ptr, int usrn)
{
	int angle;
	double rotamt;

	rotamt = ship_accel(ptr) / 10.0;

	if (ptr->heading != ptr->head2b) {
		if (fabs(normal(ptr->heading - ptr->head2b)) >= (360.0 - rotamt)
			|| fabs(normal(ptr->heading - ptr->head2b)) <= rotamt) {
			ptr->heading = ptr->head2b;
			angle = (int)ptr->heading;
			prfmsg(NOWTHER, angle);
			outprfge(FLT_NONE, usrn);
		}
		else {
			angle = (int)normal(ptr->heading - ptr->head2b);
			if (angle < 180)	/* rotate left */
				ptr->heading = normal(ptr->heading - rotamt);
			else			/* rotate right */
				ptr->heading = normal(ptr->heading + rotamt);
			angle = (int)ptr->heading;
			prfmsg(NOWTRNP, angle);
			outprfge(FLT_SHIP, usrn);
		}
	}
}

/**************************************************************************
** Accelerate the ship                                                   **
**************************************************************************/

void FUNC accel(WARSHP *ptr, int usrn)
{
	int usage;
	int newwarp;
	int need;
	double accelrate, decelrate;

	if (ptr->speed < ptr->speed2b) {
		accelrate = ship_accel(ptr);
		/* impulse-only accel is free; any warp-range acceleration consumes flux */
		if (ptr->speed < 1000 && ptr->speed2b < 1000)
			usage = 0;
		else
			usage = ACCENGAMT;
		if (fabs(ptr->speed - ptr->speed2b) <= accelrate) {
			need = usage;
			/* crossing into hyperspace also needs the extra warp-entry flux */
			if (ptr->speed / 1000 < 1 && ptr->speed2b >= 1000) {
				newwarp = (int)(ptr->speed2b / 1000.0);
				need += (newwarp + 10);
			}
			if (fluxstat(ptr, usrn, need) == 1) {
				if (ptr->speed2b >= 1000 && ptr->speed / 1000 < 1)
					hyperspace(ptr, usrn, 1);
				ptr->speed = ptr->speed2b;
				ptr->energy -= usage;

				prfmsg(SPEEDIS, showarp(ptr->speed));
				outprfge(FLT_NONE, usrn);
			}
			else {
				/* failed acceleration knocks the ship out of its climb and bleeds off speed */
				if (ptr->speed2b >= 1000 && ptr->energy < (ACCENGAMT + 10 + 1))
					prfmsg(MOVE5);
				else
					prfmsg(NOACCEL, (int)(ptr->speed / 1000.0));
				outprfge(FLT_NONE, usrn);
				decelrate = accelrate * 2.0;
				if (ptr->speed / 1000 >= 1 && (ptr->speed - decelrate) / 1000 < 1)
					hyperspace(ptr, usrn, 0);
				if (fabs(ptr->speed) <= decelrate)
					ptr->speed = 0;
				else
					ptr->speed -= decelrate;
				ptr->speed2b = 0;
			}
		}
		else {
			need = usage;
			if (ptr->speed / 1000 < 1 && (ptr->speed + accelrate) >= 1000) {
				newwarp = (int)((ptr->speed + accelrate) / 1000.0);
				need += (newwarp + 10);
			}
			if (fluxstat(ptr, usrn, need) == 1) {
				if (ptr->speed2b >= 1000 && ptr->speed / 1000 < 1 && (ptr->speed + accelrate) / 1000 >= 1)
					hyperspace(ptr, usrn, 1);
				/* report each visible acceleration step; most ships show whole warp steps, PTs show .20 steps */
				if ((int)(ptr->speed / accelrate) != (int)((ptr->speed + accelrate) / accelrate)) {
					sprintf(gechrbuf, "%.2f", (ptr->speed + accelrate) / 1000.0);
					prfmsg(WARP, gechrbuf);
					outprfge(FLT_SHIP, usrn);
				}
				ptr->speed += accelrate;
				ptr->energy -= usage;
			}
			else {
				/* failed acceleration knocks the ship out of its climb and bleeds off speed */
				if (ptr->speed2b >= 1000 && ptr->energy < (ACCENGAMT + 10 + 1))
					prfmsg(MOVE5);
				else
					prfmsg(NOACCEL, (int)(ptr->speed / 1000.0));
				outprfge(FLT_NONE, usrn);
				decelrate = accelrate * 2.0;
				if (ptr->speed / 1000 >= 1 && (ptr->speed - decelrate) / 1000 < 1)
					hyperspace(ptr, usrn, 0);
				if (fabs(ptr->speed) <= decelrate)
					ptr->speed = 0;
				else
					ptr->speed -= decelrate;
				ptr->speed2b = 0;
			}
		}
	}
	else if (ptr->speed > ptr->speed2b) {
		int was_over;

		decelrate = ship_accel(ptr) * 2.0;
		was_over = (ptr->speed / 1000 > ptr->topspeed);
		/* dropping below warp 1 returns the ship to normal space */
		if (ptr->speed2b < 1000 && ptr->speed / 1000 >= 1 && (ptr->speed - decelrate) / 1000 < 1)
			hyperspace(ptr, usrn, 0);

		if (fabs(ptr->speed - ptr->speed2b) <= decelrate) {
			ptr->speed = ptr->speed2b;
			if (was_over && ptr->speed / 1000 <= ptr->topspeed && ptr->topspeed < shipclass[ptr->shpclass].max_warp)
				prfmsg(WARPSPD, ptr->topspeed);
			if (ptr->speed > 0) {
				prfmsg(SPEEDIS, showarp(ptr->speed));
				outprfge(FLT_NONE, usrn);
			}
			else {
				prfmsg(DEADSTOP);
				outprfge(FLT_NONE, usrn);
			}
		}
		else {
			/* report each visible deceleration step using the same step-based display rule as acceleration */
			if ((int)(ptr->speed / decelrate) != (int)((ptr->speed - decelrate) / decelrate)) {
				if (ptr->speed > 0) {
					sprintf(gechrbuf, "%.2f", (ptr->speed - decelrate) / 1000.0);
					prfmsg(WARP, gechrbuf);
					outprfge(FLT_SHIP, usrn);
				}
				else {
					prfmsg(DEADSTOP);
					outprfge(FLT_NONE, usrn);
				}
			}
			ptr->speed -= decelrate;
			if (was_over && ptr->speed / 1000 <= ptr->topspeed && ptr->topspeed < shipclass[ptr->shpclass].max_warp) {
				prfmsg(WARPSPD, ptr->topspeed);
				outprfge(FLT_NONE, usrn);
			}
		}
	}
}

/**************************************************************************
** Make the jump to or from hyperspace                                   **
**************************************************************************/

void FUNC hyperspace(WARSHP *ptr, int usrn, int flag)
{
	int i, oi;
	byte owners[MAXTORPS], ocount, found;

	if (flag == 1 && ptr->where == 1)
		return;

	if (flag == 0 && ptr->where == 0)
		return;

	if (flag == 1) {
		if (ptr->shieldstat == SHIELDUP) {
			prfmsg(SHLDDN);
			ptr->shieldstat = SHIELDDN;
		}
		if (ptr->cloak > 0 && ptr->cloak != 3) {
			prfmsg(CLOKOFF);
			ptr->cloak = 3;
		}
		prfmsg(HYPERIN);
		outprfge(FLT_SHIP, usrn);

		ptr->where = 1;
		lock_simple(ptr, usrn, LOCKHY1, 1);

		if (ptr->status == GESTAT_AUTO)
			prfmsg(HYPERINN, ptr->shipname);
		else if (ptr->shipname[0] == 0)
			prfmsg(HYPERIN3, ptr->userid);
		else
			prfmsg(HYPERIN2, ptr->shipname);
		outsect(FLT_NONE, &warshpoff(usrn)->coord, usrn);

		/* entering hyperspace breaks all inbound torpedo tracks and notifies their live user owners once */
		ocount = 0;
		for (i = 0; i < MAXTORPS; ++i) {
			if (ptr->ltorps[i].distance > 0) {
				if (ptr->ltorps[i].channel < nterms && ingegame(ptr->ltorps[i].channel)) {
					found = FALSE;
					for (oi = 0; oi < (int)ocount; ++oi) {
						if (owners[oi] == ptr->ltorps[i].channel) {
							found = TRUE;
							break;
						}
					}
					if (found == FALSE && ocount < MAXTORPS)
						owners[ocount++] = ptr->ltorps[i].channel;
				}
			}
			ptr->ltorps[i].distance = 0;
		}
		for (oi = 0; oi < (int)ocount; ++oi) {
			prfmsg(TORMISS2, shpltr(owners[oi], usrn));
			outprfge(FLT_NONE, owners[oi]);
		}
		/* decoys do not persist through the jump; missiles are handled separately and can continue tracking */
		for (i = 0; i < MAXDECOY; ++i)
			ptr->decout[i] = 0;
	}
	else {
		prfmsg(HYPEROUT);
		outprfge(FLT_SHIP, usrn);

		ptr->where = 0;
		lock_simple(ptr, usrn, LOCKHY2, 1);

		if (ptr->status == GESTAT_AUTO) {
			/* tough cybs raise shields on exit now rather than waiting a tick for normal AI processing */
			if (shipclass[ptr->shpclass].tough_factor > 1 && ptr->shieldstat == SHIELDDN)
				shieldup(ptr, usrn);
			prfmsg(HYPEROU2, ptr->shipname);
		}
		else if (ptr->shipname[0] == 0)
			prfmsg(HYPERONO, ptr->userid);
		else
			prfmsg(HYPEROUN, ptr->shipname);
		outsect(FLT_NONE, &warshpoff(usrn)->coord, usrn);
	}
}

/**************************************************************************
** Move the ship                                                         **
**************************************************************************/

void FUNC moveship(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	COORD oldsect, newsect, neutsect;
	int overamt, intspeed, zothusn, movenergy;
	double ddist;
	float newtop;
	unsigned overadd;
	byte ptr_neb, oth_neb;

	neutsect.xcoord = 0.50001;
	neutsect.ycoord = 0.50001;

	if (ptr->speed > 0) {
		/* user ships pay movement energy while holding or climbing to their requested speed */
		if (ptr->status == GESTAT_USER && ptr->speed <= ptr->speed2b) {
			intspeed = ptr->speed / 1000.0;
			if (intspeed < 1)
				movenergy = 1;
			else
				movenergy = intspeed + 10;
			if (fluxstat(ptr, usrn, movenergy) == 0) {
				ptr->speed2b = 0;
				if (intspeed >= 1 && ptr->energy < (ACCENGAMT + 10 + 1))
					prfmsg(MOVE5);
				else
					prfmsg(MOVE4);
				outprfge(FLT_NONE, usrn);
				if ((ptr->speed / 1000 >= 1) && ((ptr->speed - (ship_accel(ptr) * 2.0)) / 1000 < 1))
					hyperspace(ptr, usrn, 0);
				if (fabs(ptr->speed) <= (ship_accel(ptr) * 2.0))
					ptr->speed = 0;
				else
					ptr->speed -= (ship_accel(ptr) * 2.0);
				return;
			}
			else
				ptr->energy -= movenergy;
		}

		movecoord(&oldsect, &ptr->coord);
		/* apply one movement tick along the current heading */
		ptr->coord.xcoord = ptr->coord.xcoord + ((ptr->speed * sin(degtorad(ptr->heading))) / 65000.0);
		ptr->coord.ycoord = ptr->coord.ycoord - ((ptr->speed * cos(degtorad(ptr->heading))) / 65000.0);

		if (ptr->where <= 1) {
			/* wrap or bounce ships at the world edge before sector-change reporting */
			if (ptr->coord.xcoord > univmax + 1) {
				if (univwrap) {
					ptr->coord.xcoord -= (double)((univmax * 2) + 1);
				}
				else {
					ptr->coord.xcoord = (double)(univmax - 2 - (int)(ptr->speed / 1000));
					if (ptr->coord.ycoord <= univmax + 1 && ptr->coord.ycoord >= (univmax * -1)) { /* avoid double bounce */
						ptr->head2b = normal(vector(&(ptr->coord), &neutsect));
						ptr->heading = ptr->head2b;
						telezip(ptr, usrn);
					}
				}
			}
			else if (ptr->coord.xcoord < (univmax * -1)) {
				if (univwrap) {
					ptr->coord.xcoord += (double)((univmax * 2) + 1);
				}
				else {
					ptr->coord.xcoord = (double)((univmax - 2 - (int)(ptr->speed / 1000)) * -1);
					if (ptr->coord.ycoord <= univmax + 1 && ptr->coord.ycoord >= (univmax * -1)) {
						ptr->head2b = normal(vector(&(ptr->coord), &neutsect));
						ptr->heading = ptr->head2b;
						telezip(ptr, usrn);
					}
				}
			}

			if (ptr->coord.ycoord > univmax + 1) {
				if (univwrap) {
					ptr->coord.ycoord -= (double)((univmax * 2) + 1);
				}
				else {
					ptr->coord.ycoord = (double)(univmax - 2 - (int)(ptr->speed / 1000));
					ptr->head2b = normal(vector(&(ptr->coord), &neutsect));
					ptr->heading = ptr->head2b;
					telezip(ptr, usrn);
				}
			}
			else if (ptr->coord.ycoord < (univmax * -1)) {
				if (univwrap) {
					ptr->coord.ycoord += (double)((univmax * 2) + 1);
				}
				else {
					ptr->coord.ycoord = (double)((univmax - 2 - (int)(ptr->speed / 1000)) * -1);
					ptr->head2b = normal(vector(&(ptr->coord), &neutsect));
					ptr->heading = ptr->head2b;
					telezip(ptr, usrn);
				}
			}
		}

		movecoord(&newsect, &ptr->coord);

		if (!samesect(&oldsect, &newsect)) {
			/* sector changes generate local navigation feedback plus visible departure/arrival messages */
			prfmsg(MOVE1,
				(innebula(coord1(oldsect.xcoord), coord1(oldsect.ycoord)) ? CLR_GREEN2 "nebula" : "sector"),
				coord1(oldsect.xcoord), coord1(oldsect.ycoord),
				(innebula(coord1(newsect.xcoord), coord1(newsect.ycoord)) ? CLR_GREEN2 "nebula" : "sector"),
				coord1(newsect.xcoord), coord1(newsect.ycoord));
			outprfge(FLT_SHIP, usrn);
			if (ptr->cloak != 10) {
				if (ptr->speed < 21000.0) {
					if (ptr->status == GESTAT_AUTO)
						prfmsg(MOVE2N, ptr->shipname);
					else if (ptr->shipname[0] == 0)
						prfmsg(MOVE2NO, ptr->userid);
					else
						prfmsg(MOVE2, ptr->shipname);
					outsect(FLT_NONE, &oldsect, usrn);
				}
				if (ptr->speed < 21000.0) {
					if (ptr->status == GESTAT_AUTO)
						prfmsg(MOVE3N, ptr->shipname);
					else if (ptr->shipname[0] == 0)
						prfmsg(MOVE3NO, ptr->userid);
					else
						prfmsg(MOVE3, ptr->shipname);
					outsect(FLT_NONE, &newsect, usrn);
				}
			}
			ptr->hostile = 0;
			if (ptr->destruct > 0 && neutral(&newsect)) {
				prfmsg(SELFD4);
				outprfge(FLT_NONE, usrn);
				ptr->destruct = 0;
			}
		}

		/* if I am cloaked tell the closer ones */
		if (ptr->cloak == 10) {
			unsigned int r = gernd();

			if (ptr->speed2b > (double)(((r >> 5) % 200) + 10) && (((r >> 8) % 25) == 0)) {
				ptr_neb = (byte)innebula(coord1(ptr->coord.xcoord), coord1(ptr->coord.ycoord));
				for (zothusn = 0; zothusn < nterms; zothusn++) {
					wptr = warshpoff(zothusn);
					if (ingegame(zothusn) && zothusn != usrn) {
						ddist = cdistance(&ptr->coord, &wptr->coord);
						ddist *= 10000;
						oth_neb = (byte)innebula(coord1(wptr->coord.xcoord), coord1(wptr->coord.ycoord));
						if ((ptr_neb || oth_neb) && !(ptr_neb && oth_neb && ddist < (double)NEBRNG))
							continue;

						if (ddist < (shipclass[wptr->shpclass].scanrange)
							&& ddist < 20000 && wptr->jam_sev <= (byte)2) {
							bearing = cbearing(&wptr->coord, &ptr->coord, wptr->heading);
							/* slop it up +- 10 degrees on either side */
							bearing += ((r >> 11) % 20) - 10;
							if (bearing > 180)
								bearing -= 360;
							else if (bearing <= -180)
								bearing += 360;
							prfmsg(CLOK3, bearing);
							outprfge(FLT_NONE, zothusn);
						}
					}
				}
			}
		}
		if (ptr->speed > 0.0 && ptr->status == GESTAT_USER) {
			unsigned int r = gernd();

			intspeed = ptr->speed / 1000.0;

			/* track long-term overwarp abuse separately from the per-tick movement energy drain */
			/* if this ship is exceeding top cruising speed and not in the process of going under it */
			if (intspeed > ptr->topspeed && (int)(ptr->speed2b / 1000.0) > ptr->topspeed) {
				newtop = shipclass[ptr->shpclass].max_warp * (1.0f - (float)ptr->overspeed / 10000.0f);
				if (newtop < 1.0f) {
					prfmsg(WARPBRK);
					outprfge(FLT_NONE, usrn);
					ptr->topspeed = 0;
					ptr->speed2b = 0;
					ptr->damage += r % 20;
				}
				else {
					if (ptr->topspeed != newtop && ptr->topspeed != 0)
						ptr->topspeed = (int)newtop;

					/* for every 10% over cruising speed, increase potential random damage */
					overamt = (intspeed * 100 / newtop) - 100;
					if (ptr->upgrade & NCORE)
						overadd = (unsigned)(r % ((overamt / 2) + 1));
					else
						overadd = (unsigned)(r % (overamt + 1));
					ptr->overspeed += overadd;

					/* if over twice new cruising speed, blow up the engines */
					if (overamt >= 110 && ptr->topspeed != 0) {
						prfmsg(WARPBRK);
						outprfge(FLT_NONE, usrn);
						ptr->topspeed = 0;
						ptr->speed2b = 0;
						ptr->damage += r % 20;
					}
					else if (overamt != ptr->warpmsg) {
						if (overamt >= 60 && overamt / 10 != ptr->warpmsg / 10) {
							prfmsg(WARPFAST + (int)((overamt / 10) - 6));
							outprfge(FLT_NONE, usrn);
						}
						ptr->warpmsg = overamt;
					}
				}
			}
		}
		if (ptr->hostile > 0)
			checkdist(ptr, usrn);
	}
	else {
		if (ptr->where == 1)	/* recover from stale hyper state on stopped ships */
			ptr->where = 0;
	}

	/* users check local planet proximity */
	if (ptr->where == 0 && ptr->status == GESTAT_USER)
		proximity(ptr, usrn);
}

/**************************************************************************
** Bounce a ship away from the world boundary toward neutral space       **
**************************************************************************/

void FUNC telezip(WARSHP *ptr, int usrn)
{
	ptr->speed2b = 0.0;
	ptr->speed = ptr->speed2b;
	ptr->where = 0;
	if (ptr->status == GESTAT_USER) {
		ptr->damage += TELEDAM;
		damstr(TELEDAM);
		prfmsg(TELEPORT, gechrbuf);
		prfmsg(NOWTHER, (int)ptr->heading);	/* show now pointing towards 0 0 */
		outprfge(FLT_NONE, usrn);
	}
	else {
		if (ptr->topspeed == 0)
			ptr->speed2b = 990;
		else
			ptr->speed2b = (double)ptr->topspeed * 1000.0;	/* head toward 0 0 for the moment */
		ptr->cybupdate = 20 + gernd() % 5;	/* save and pick new heading after a while */
	}
}


/**************************************************************************
** Check nearby planets and wormholes for user ships                    **
**************************************************************************/

void FUNC proximity(WARSHP *ptr, int usrn)
{
	int i;
	unsigned dist;

	refresh(ptr, usrn);

	/* check distances to plantets */
	for (i = 0; i < MAXPLANETS; ++i) {
		if (ptab[usrn].planets[i].type != 0) {
			dist = (unsigned)(cdistance(&ptr->coord,&ptab[usrn].planets[i].coord) * 10000);

			if (ptab[usrn].planets[i].type == PLTYPE_PLNT
				&& dist <= 1000
				&& beacontimer == 0) {
				if (getplanet(&ptr->coord,i + 1) && planet.type == PLTYPE_PLNT && planet.beacon[0] != 0) {
					if (planet.name[0] != 0)
						prfmsg(BEAC02,i + 1,planet.name,planet.beacon);
					else
						prfmsg(BEAC01,i + 1,planet.beacon);
					outprfge(FLT_BEACON,usrn);
					beacontimer = 60;
				}
			}
			if (ptr->speed <= 0)
				continue;
			if (dist < 250 && ptr->damage < 101.0) {	/* no addl msgs after crash */
				if (dist >= 50) {
					if (ptab[usrn].planets[i].type == PLTYPE_PLNT)
						prfmsg(GRAVITY1,i + 1);
					else
						prfmsg(GRAVWRM1,i + 1);

					outprfge(FLT_NONE,usrn);
				}
				else if (dist >= 25) {
					if (ptab[usrn].planets[i].type == PLTYPE_PLNT)
						prfmsg(GRAVITY2,i + 1);
					else
						prfmsg(GRAVWRM2,i + 1);

					outprfge(FLT_NONE,usrn);
				}
				else if (dist < 25) {
					if (ptab[usrn].planets[i].type == PLTYPE_PLNT)
						prfmsg(GRAVITY3,i + 1);
					else
						prfmsg(GRAVWRM3,i + 1);

					outprfge(FLT_NONE,usrn);
					if (ptab[usrn].planets[i].type == PLTYPE_PLNT) {
						ptr->damage = 101.0;
						ptr->cantexit = FIRETICKS; /* no exiting after crashing */
					}
					else {
						lock_sector(ptr,usrn,LOCKWORM);
						setsect(ptr); /* build PKEY */
						pkey.plnum = i + 1;
						/* load the wormhole destination record before moving the ship */
						if (!gesdb(GEGETNOW,&pkey,(GALSECT *)&worm)) {
							geshocst(0,spr("GE:ERR:Worm Load Fail %d,%d,%d",xsect,ysect,pkey.plnum));
							continue;
						}
						ptr->coord.xcoord = worm.destination.xcoord;
						ptr->coord.ycoord = worm.destination.ycoord;
						prfmsg(MOVE1,
							(innebula(xsect,ysect) ? CLR_GREEN2 "nebula" : "sector"),
							xsect,ysect,
							(innebula(coord1(worm.destination.xcoord),coord1(worm.destination.ycoord)) ? CLR_GREEN2 "nebula" : "sector"),
							coord1(worm.destination.xcoord),coord1(worm.destination.ycoord));
						ptr->damage += 5.5;
						outprfge(FLT_SHIP,usrn);
						if (ptr->cloak != 10)
							suddenappear(ptr,usrn);
						clearitm(usrn);	 /* clear the tors and missiles */
						ptr->jam_time = (byte)0;
						ptr->jam_sev = (byte)0;
						assign_cybs(usrn,0); /* clear current cyb pursuits and pick closest new ones */
					}
				}
			}
		}
	}
}

/**************************************************************************
** Clear a player's hostile-planet flag once they move far enough away   **
**************************************************************************/

void FUNC checkdist(WARSHP *ptr, int usrn)
{
	int i;
	unsigned dist;

	refresh(ptr, usrn);

	i = ptr->hostile - 11;
	if (ptab[usrn].planets[i].type != 0) {
		dist = (unsigned)(cdistance(&ptr->coord, &ptab[usrn].planets[i].coord) * 10000);
		if (dist > 1000)
			ptr->hostile = 0;
	}
}

/**************************************************************************
** Refresh the cached local planet table when a ship enters a new sector **
**************************************************************************/

void FUNC refresh(WARSHP *ptr, int usrn)
{
	int i;
	COORD ss, tmpcoord;

	movecoord(&ss, &ptr->coord);

	/* need to refresh planet coords? */

	tmpcoord.xcoord = 32767.000;
	tmpcoord.ycoord = 32767.000;

	for (i = 0; i < MAXPLANETS; ++i) {
		if (ptab[usrn].planets[i].type != 0) {
			movecoord(&tmpcoord, &ptab[usrn].planets[i].coord);
			break;
		}
	}

	if (!samesect(&tmpcoord, &ss)) {
		logthis(spr("GEFUNCS:refreshing sector for usrn %d", usrn));
		getsector(&ss);
		memcpy(&ptab[usrn], &sector.ptab, sizeof(PLANETAB));
	}
}

/**************************************************************************
** Check the damage and repair any - Also service weapons                **
**************************************************************************/

void FUNC checkdam(WARSHP *ptr, int usrn)
{
	double preload;
	double reprate;
	int repstep, i;

	logthis(spr("GE:Chn %d checkdam %s", usrn, ptr->userid));

	if (ptr->damage >= 100.0) {
		ptr->damage = 0.0;	/* reset damage so he can get back on */

		killem(ptr, usrn);

		prfmsg(YOURDEAD);
		outprfge(FLT_NONE, usrn);

		if (deathdeduct > 0) {
			sprintf(gechrbuf, "%lu", deathdeduct);
			prfmsg(YRDEAD2, gechrbuf);
			outprfge(FLT_NONE, usrn);
		}

		prfmsg(YRDEAD3);
		outprfge(FLT_NONE, usrn);

		/* only reset btupmt on "real" users */
		if (usrn < nterms)
			btupmt(usrn, 0);

		if (ptr->status == GESTAT_AUTO || ptr->userid[0] == '@')
			ptr->status = GESTAT_AVAIL;

		if (ptr->status == GESTAT_USER) {
			user[usrn].substt = 0;
			--numwar;
			ptr->where = -1;
		}
		return;
	}

	/* repair ship, always */
	reprate = repairrate;
	repstep = 1;
	if (ptr->repair > 0) {
		reprate = 3.0;
		repstep = 5;
	}
	else if (ptr->upgrade & DAMCTRL) {
		reprate *= 2.0;
		repstep = 2;
	}

	if (ptr->damage > 0.0)
		ptr->damage = ptr->damage - reprate;
	else
		ptr->damage = 0.0;
	if (ptr->damage < 0.0)
		ptr->damage = 0.0;

	/* charge phaser if not damaged */
	if (ptr->phasr < 100 && ptr->phasr >= 0) {
		if (fluxstat(ptr, usrn, PENGUSE) == 1) {
			ptr->energy -= PENGUSE;
			/* If phasers get to minimum fire power tell captain */
			preload = (double)(ptr->phasrtype * PRELOAD);
			/* If phaser goes from under to 100 in one step, just show one msg */
			if (ptr->phasr < PMINFIRE && ptr->phasr + preload >= PMINFIRE && ptr->phasr + preload < 100) {
				prfmsg(PHSRUP);
				outprfge(FLT_NONE, usrn);
			}

			ptr->phasr = ptr->phasr + preload;

			/* if phasers get to 100% tell captain, and set to 100% */
			if (ptr->phasr >= 100) {
				prfmsg(PHSRMAX);
				outprfge(FLT_NONE, usrn);
				ptr->phasr = 100;
			}
		}
	}

	/* repair shields separately */
	if (ptr->shieldstat == SHIELDDM && repstep > 1) {
		for (i = 1; i < repstep && ptr->shieldstat == SHIELDDM; ++i)
			shieldrep(ptr, usrn);
	}

	/* repair other systems in order of importance */
	repair_systems(ptr, usrn, repstep, (ptr->repair > 0));

	if (ptr->repair > 0 && !repair_needed(ptr)) {
		ptr->repair = 0;
		prfmsg(MAINT7);
		outprfge(FLT_NONE, usrn);
	}
	return;
}

void FUNC killem(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	WARUSR *wuptr;
	WARSHP *disptr;
	WARSHP *nearptr;
	unsigned i;
	unsigned long room100;
	int who, comma, full, lospos, winpos, nearby;
	long scr, amt, bonus1, bonus2, ded_amt;
	double ddist;
	unsigned int r = gernd();

	/* 12/19/91 fix to prevent a player from being awarded points for killing himself */

	clear_entrymsg(usrn);
	waruptr = warusroff(usrn);

	who = ptr->lastfired;

	comma = FALSE;
	full = FALSE;
	deathdeduct = 0;

	if (who >= 0 && who < nships && who != usrn) {
		wptr = warshpoff(who);
		wuptr = warusroff(who);
		logthis(spr("Killed by %s",wptr->userid));
		if (wptr->status == GESTAT_AUTO) {
			if (shipclass[wptr->shpclass].won_func != NULL)
				shipclass[wptr->shpclass].won_func(wptr,who,ptr);
		}

		if (ptr->status == GESTAT_AUTO)
			prfmsg(KILLDNPC,username(ptr),username(wptr));
		else
			prfmsg(KILLEDBY,username(ptr),username(wptr));
		outwar(FLT_NONE,usrn,0,0);

		++wuptr->kills;
		++wptr->kills;

		if (ptr->status == GESTAT_USER && wptr->status == GESTAT_USER) {
			++wuptr->ukills;
			++wptr->ukills;
		}

		if (ptr->status == GESTAT_AUTO)
			prfmsg(KILLGOTN,ptr->shipname);
		else if (ptr->shipname[0] == 0)
			prfmsg(KILLGTNO,ptr->userid);
		else
			prfmsg(KILLGOT1,ptr->shipname);

		if (shipclass[wptr->shpclass].max_tons <= calcweight(wptr)) {
			full = TRUE;
			comma = TRUE;
			prf(" nothing");
		}
		else {
			/* get gold drop first, complete amount */
			amt = ptr->items[I_GOLD];
			if (amt > 0) {
				if (!chkweight(wptr,I_GOLD,amt)) {
					room100 = ((unsigned long)shipclass[wptr->shpclass].max_tons * 100UL) - cargo_weight100(wptr);
					amt = (long)(room100 / (unsigned long)weight[I_GOLD]);
					full = TRUE;
				}
				if (amt > 0) {
					wptr->items[I_GOLD] += amt;
					sprintf(gechrbuf2,"%ld",amt);
					prf(" %s %s",gechrbuf2,item_name[I_GOLD]);
					comma = TRUE;
				}
			}
			/* get the rest except casualties, random amounts */
			for (i = 1; i < NUMITEMS; ++i) {
				if (full == TRUE)
					break;
				if (i != I_MEN && i != I_TROOPS && i != I_SPY && i != I_GOLD &&
					!(shipclass[ptr->shpclass].max_type == CLASSTYPE_CYBORG && i == I_FOOD)) {
					amt = ptr->items[i] / (r % 5 + 1);
					/* only collect as much as we can hold */
					if (amt > 0) {
						if (!chkweight(wptr,i,amt)) {
							room100 = ((unsigned long)shipclass[wptr->shpclass].max_tons * 100UL) - cargo_weight100(wptr);
							amt = (long)(room100 / (unsigned long)weight[i]);
							full = TRUE;
						}
						if (amt > 0) {
							wptr->items[i] += amt;
							sprintf(gechrbuf2,"%ld",amt);
							if (comma == TRUE)
								prf(", %s %s",gechrbuf2,item_name[i]);
							else {
								prf(" %s %s",gechrbuf2,item_name[i]);
								comma = TRUE;
							}
						}
					}
				}
			}
		}
		if (comma == FALSE)
			prf(" nothing.\r");
		else
			prf(".\r");

		if (full == TRUE)
			prfmsg(KILLFULL);

		outprfge(FLT_NONE,who);

		/* grant points for the kill */
		scr = (long)shipclass[ptr->shpclass].max_points;
		bonus1 = 0;
		bonus2 = 0;

		if (ptr->status == GESTAT_USER && wptr->status == GESTAT_USER) {
			lospos = 0;
			winpos = 0;

			rospos(waruptr, wuptr, &lospos, &winpos);

			/* bonus for lower ranked taking out higher ranked */
			if (lospos != 0 && winpos > lospos && waruptr->score > wuptr->score) {
				bonus1 += ((winpos - lospos) * (long)score_bonus);
				bonus1 += ((waruptr->score - wuptr->score) / (long)score_bonus);
			}

			/* adjust bonus for taking out more/less powerful ship */
			if (shipclass[ptr->shpclass].damfact > (shipclass[wptr->shpclass].damfact + 50))
				bonus2 = scr / 2;
			else if (shipclass[ptr->shpclass].damfact < (shipclass[wptr->shpclass].damfact - 50))
				bonus2 = -((scr * 1L) / 3L);

			amt = scr + bonus1 + bonus2;
			if (amt < 0)
				amt = 0;

			ded_amt = (amt * score_f2) / 100L;

			/* if loss exceeds total score, kill is worth nothing */
			if (ded_amt > waruptr->score) {
				ded_amt = 0;
				amt = 0;
				scr = 0;
			}
		}
		else {
			amt = scr;

			/* deduct less for losing to an NPC */
			ded_amt = (amt * score_f2) / 1000L;
		}

		/* cap deduction to combat-earned score */
		if (ded_amt > waruptr->klscore)
			ded_amt = waruptr->klscore;

		waruptr->klscore -= ded_amt;
		waruptr->score -= ded_amt;

		if (ded_amt > 0)
			deathdeduct = ded_amt;

		(wuptr->score) += amt;
		(wuptr->klscore) += amt;

		sprintf(gechrbuf,"%ld",scr);
		if (scr == 0)
			prfmsg(KILNOPTS);
		else {
			prfmsg(KILLPNTS,gechrbuf,shipclass[ptr->shpclass].typename,showupg(ptr));

			if (bonus2 > 0) {
				sprintf(gechrbuf,"%ld",bonus2);
				prfmsg(KILLBON1,gechrbuf);
			}
			else if (bonus2 < 0) {
				sprintf(gechrbuf,"%ld",(bonus2 * -1L));
				prfmsg(KILLBON2,gechrbuf);
			}

			if (bonus1 > 0) {
				sprintf(gechrbuf,"%ld",bonus1);
				prfmsg(KILLBON3,winpos,lospos,gechrbuf);
			}
		}

		outprfge(FLT_NONE,who);

		if (scr > 0 && chgloser > 0
			&& ptr->status == GESTAT_USER
			&& wptr->status == GESTAT_USER) {
			amt = (waruptr->cash / 100L) * (long)chgloser;
			if (amt > waruptr->cash)
				amt = waruptr->cash;

			if (amt > 0) {
				waruptr->cash -= amt;
				if (wuptr->cash > ULCAP - amt) {
					amt = ULCAP - wuptr->cash;
					sprintf(gechrbuf,"%lu",ULCAP);
					prfmsg(TOORICH,gechrbuf);
					outprfge(FLT_NONE,who);
				}
				wuptr->cash += amt;
				sprintf(gechrbuf,"%ld",amt);
				prfmsg(CHGLSR1,gechrbuf);
				outprfge(FLT_NONE,usrn);
				prfmsg(CHGLSR2,gechrbuf,ptr->userid);
				outprfge(FLT_NONE,who);
			}
		}

		/* if the clown just killed was the last to fire on me clean out
			my last fired flag so as not to award him with any points should
			I end up getting killed */
		if (wptr->lastfired == usrn)
			wptr->lastfired = -1;

		/* distress handling */
		for (i = nterms; i < nships; ++i) {
			if (ingegame(i)) {
				disptr = warshpoff(i);
				if (disptr->distress == usrn) {
					amt = (shipclass[disptr->shpclass].max_points) * 4;
					sprintf(gechrbuf,"%ld",amt);
					prfmsg(KILLDIS,gechrbuf,disptr->shipname);
					prfmsg(FACNAME0+shipclass[disptr->shpclass].faction);
					prf("\r");
					outprfge(FLT_NONE,who);
					wuptr->score += amt;
					wuptr->klscore += amt;
					wuptr->factions[shipclass[disptr->shpclass].faction] = 0;
					disptr->distress = 255;
				}
			}
		}

		if (ptr->status == GESTAT_USER && wptr->status == GESTAT_USER
			&& showdoc != 0 && r % (11 - showdoc) == 0) {
			setbtv(gebb2);

			if (qlobtv(0) && qeqbtv(ptr->userid,1)) {
				int bits = 1;
				if (waruptr->planets > 100)	/* if lots of planets, increase rnd threshold */
					bits = 4;
				else if (waruptr->planets > 50)
					bits = 2;
				prfmsg(CAPTDOC);
				prfmsg(PLAMSG1);
				i = 0;
				do {
					gcrbtv(&planet,1);
					if (sameas(planet.userid,ptr->userid)) {
						if ((bits == 1 && (r & 1) == 1) ||	/* different random list each time */
							(bits == 2 && (r & 3) == 3) ||	/* over 50 planets, 1 in 4 chance to be included */
							(bits == 4 && (r & 7) == 7)) {	/* read 3 bits (1 in 8 chance) even though we shift 4 */
							prf("%-24s %6d %6d  %6d\r",planet.name,planet.xsect,planet.ysect,planet.plnum);
							++i;
							if (i % 5 == 0)		/* cat five lines then print */
								outprfge(FLT_NONE,who);
						}
					}
					else
						break;
					r >>= bits;
					if (r == 0)
						r = gernd();
				} while (qnxbtv() && (i < 20));
				if (i == 0) {	/* oops, we didn't pick any planets, so print final planet */
					qprbtv();
					gcrbtv(&planet,1);
					prf("%-24s %6d %6d  %6d\r",planet.name,planet.xsect,planet.ysect,planet.plnum);
				}
				if (i % 5 != 0)	/* if we're not on a multiple of 5, we still have to print the remainder */
					outprfge(FLT_NONE,who);
			}
		}

		nearby = FALSE;
		for (i = 0; i < nships; ++i) {
			if (i != who && i != usrn && ingegame((int)i)) {
				nearptr = warshpoff((int)i);
				if (nearptr->damage < 100.0) {
					ddist = cdistance(&wptr->coord,&nearptr->coord);
					ddist *= 10000.0;
					if (ddist < 30000.0) {
						nearby = TRUE;
						break;
					}
				}
			}
		}
		if (!nearby && wptr->cantexit > (FIRETICKS/4))
			wptr->cantexit = FIRETICKS/4;

		if (shipclass[wptr->shpclass].max_type != CLASSTYPE_DROID) {
			if (!geudb(GEUPDATE,wuptr->userid,wuptr))
				geshocst(0,spr("GE:ERR:Kill Winner Update Fail %s",wuptr->userid));
		}

		if (shipclass[ptr->shpclass].kill_func != NULL)
			shipclass[ptr->shpclass].kill_func(ptr,usrn,wptr);
	}
	else {
		if (shipclass[ptr->shpclass].max_type != CLASSTYPE_USER)
			prfmsg(DIEDNPC,ptr->shipname);
		else if (ptr->shipname[0] == 0)
			prfmsg(DIEDNO,username(ptr));
		else
			prfmsg(DIED,ptr->shipname,username(ptr));
		outwar(FLT_NONE,usrn,0,0);
		if (shipclass[ptr->shpclass].kill_func != NULL)
			shipclass[ptr->shpclass].kill_func(ptr,usrn,NULL);
	}

	cleartm(usrn);	/* change destroyed user's torps and mis to 'no user' */

	if (shipclass[ptr->shpclass].max_type == CLASSTYPE_USER) {
		--(waruptr->noships);
		/* fix any wrap problem */
		if (waruptr->noships == 65535U)
			waruptr->noships = 0;
	}

	if (shipclass[ptr->shpclass].max_type != CLASSTYPE_DROID) {
		if (!gepdb(GEDELETE,ptr->userid,ptr->shipno,ptr))
			geshocst(0,spr("GE:ERR:Kill Delete Fail %s #%d",ptr->userid,ptr->shipno));
		if (!geudb(GEUPDATE,waruptr->userid,waruptr))
			geshocst(0,spr("GE:ERR:Kill Loser Update Fail %s",waruptr->userid));
	}

	logthis(spr("GE:INF:%s died!",waruptr->userid));
}

/**************************************************************************
** Recharge energy pool                                                  **
**************************************************************************/

void FUNC recharge(WARSHP *ptr)
{
	if (ptr->energy < ENGYMAX)
		ptr->energy = ptr->energy + ENGRECHG;
	else
		ptr->energy = ENGYMAX;
}


/**************************************************************************
** Check flux status                                                     **
**************************************************************************/

int FUNC fluxstat(WARSHP *ptr, int usrn, unsigned energy)
{
	if (ptr->energy < energy) {
		if (ptr->items[I_FLUXPOD] > 0) {
			ptr->energy = ENGYMAX;
			--ptr->items[I_FLUXPOD];
			prfmsg(FLUXLOAD);
			outprfge(FLT_SHIP, usrn);
			if (ptr->items[I_FLUXPOD] == 0) {
				prfmsg(LASTFLUX);
				outprfge(FLT_NONE, usrn);
			}
		return 1;
		}
		else {
			prfmsg(NOFLUX);
			outprfge(FLT_NONE, usrn);
		return 0;
		}
	}
	else
		return 1;
}


/**************************************************************************
** Check shield status                                                   **
**************************************************************************/

void FUNC shieldstat(WARSHP *ptr, int usrn)
{
	if (ptr->shieldstat == SHIELDUP) {
		if (fluxstat(ptr, usrn, SHENGUSE * ptr->shieldtype) == 0) {
			ptr->shieldstat = SHIELDDN;
			ptr->shield = 0;
			prfmsg(SHDNNOP);
			outprfge(FLT_NONE, usrn);
		}
		else {
			shieldchg(ptr, usrn);
		}
	}
	else if (ptr->shieldstat == SHIELDDM)
		shieldrep(ptr, usrn);
}


/**************************************************************************
** Check cloak status                                                    **
**************************************************************************/

void FUNC cloakstat(WARSHP *ptr, int usrn)
{
	int oldcloak;

	if (ptr->cloak > 0 && ptr->cloak != 3) {
		if (fluxstat(ptr, usrn, clenguse) == 0) {
			oldcloak = ptr->cloak;
			ptr->cloak = 3;
			prfmsg(CLOKNOP);
			outprfge(FLT_NONE, usrn);
			if (oldcloak == 10) {
				prfmsg(CLOK2);
				outrange(FLT_NONE, &ptr->coord);
				suddenappear(ptr, usrn);
			}
		}
		else
			ptr->energy -= clenguse;
	}
}

/**************************************************************************
** Check whether one ship can see another through cloak and nebula rules **
**************************************************************************/

int FUNC isvisible(WARSHP *ptr, WARSHP *wptr)
{
	double ddist;
	byte ptr_neb, oth_neb;

	if (wptr->cloak == 10)
		return FALSE;

	ptr_neb = (byte)innebula(coord1(ptr->coord.xcoord), coord1(ptr->coord.ycoord));
	oth_neb = (byte)innebula(coord1(wptr->coord.xcoord), coord1(wptr->coord.ycoord));

	if (!(ptr_neb || oth_neb))
		return TRUE;

	ddist = cdistance(&ptr->coord, &wptr->coord) * 10000.0;
	if (ptr_neb && oth_neb && ddist < (double)NEBRNG)
		return TRUE;

	return FALSE;
}

/**************************************************************************
** Check mine status                                                     **
**************************************************************************/

void FUNC checkmines(void)
{
	int i;
	int zothusn;	/* general purpose other-user channel number */
	int minechan;
	WARSHP *wptr;
	WARUSR *wuptr;
	double ddist, damfact;
	unsigned udist;
	byte mine_neb, ship_neb;
	MINE *mptr;

	setmbk(gemb);

	/* reset per-ship mine proximity status each tick */
	for (zothusn = 0; zothusn < nships; zothusn++) {
		wptr = warshpoff(zothusn);
		if (ingegame(zothusn))
			wptr->minesnear = FALSE;
	}

	for (i = 0, mptr = mines; i < nummines; ++mptr, ++i) {
		if (mptr->channel != 255) {	/* if a live mine */
			--mptr->timer;
			/* mines only do proximity work every fifth tick; timer zero is the actual detonation pass */
			if (mptr->timer % 5 == 0) {
				mine_neb = (byte)innebula(coord1(mptr->coord.xcoord),coord1(mptr->coord.ycoord));
				for (zothusn = 0; zothusn < nships; zothusn++) {
					wptr = warshpoff(zothusn);
					if (ingegame(zothusn)) {
						ddist = cdistance(&mptr->coord,&wptr->coord);
						ddist *= 10000;
						bearing = cbearing(&wptr->coord,&mptr->coord,wptr->heading);
						setsect(wptr);
						if (ddist < ((double)MINERANGE) && !neutral(&wptr->coord)) {
							udist = (unsigned)ddist;
							if (mptr->timer == 0) {
								minechan = (int)mptr->channel;
								ddist = 1.0 - (ddist / ((double)MINERANGE));
								if (ddist < 0)
									ddist = 0;
								ddist = ddist * ddist;
								if (wptr->shieldstat == SHIELDUP) {
									damfact = (double)(ddist * minedammax);
									damfact = ton_fact(wptr,damfact); /* adjust for weight */
									damfact = damfact / (gernd() % 5 + wptr->shieldtype);
									damstr((int)damfact);
									prfmsg(MINE4,bearing,udist,gechrbuf);
									outprfge(FLT_NONE,zothusn);
									shieldhitmsg(shieldhit(wptr,(int)damfact + 20),zothusn);
								}
								else {
									damfact = (double)(ddist * minedammax);
									damfact = ton_fact(wptr,damfact); /* adjust for weight */

									damstr((int)damfact);
									prfmsg(MINE4,bearing,udist,gechrbuf);
									outprfge(FLT_NONE,zothusn);
								}
								wptr->damage += damfact;
								randamage(wptr,zothusn,damfact);
								/* orphaned mines still detonate, but they no longer assign credit or faction dislike */
								if (minechan >= 0 && minechan < nships && ingegame(minechan)) {
									/* don't set lastfired if NPC blows up its own kind or user blows up self */
									if ((shipclass[wptr->shpclass].faction != shipclass[warshpoff(minechan)->shpclass].faction ||
										shipclass[wptr->shpclass].faction == 0 || shipclass[warshpoff(minechan)->shpclass].faction == 0) &&
										zothusn != minechan)
										wptr->lastfired = minechan;
									wuptr = warusroff(minechan);
									set_dislike(wuptr,shipclass[wptr->shpclass].faction,(int)damfact);
								}
								else
									wptr->lastfired = -1;
								wptr->minesnear = FALSE;
							}
							else {
								if (wptr->jam_sev <= (byte)2) {
									ship_neb = (byte)innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
									if (!(mine_neb || ship_neb) || (mine_neb && ship_neb && ddist < (double)NEBRNG)) {
										prfmsg(MINE6,bearing,udist);
										outprfge(FLT_NONE,zothusn);
									}
								}
								wptr->minesnear = TRUE;
							}
						}
						else if (mptr->timer == 0 && wptr->jam_sev <= (byte)2) {
							ship_neb = (byte)innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
							if (!(mine_neb || ship_neb) || (mine_neb && ship_neb && ddist < (double)NEBRNG)) {
								prfmsg(MINE5,bearing);
								outprfge(FLT_NONE,zothusn);
							}
						}
					}
				}
			}
		}
		if (mptr->timer == 0)
			mptr->channel = 255; /* stomp on it - HARD */
	}
}

/**************************************************************************
** Check for incoming torpedoes or missiles & track decoys               **
**************************************************************************/

void FUNC checktm(WARSHP *ptr, int usrn)
{
	int i, j, k, power, shotdown, shres;
	byte track_cnt, lost_cnt, acc_used, shmsg;
	byte acc_cnt[MAXTORPS], acc_chan[MAXTORPS];
	byte sh_cnt, un_cnt;
	double sh_dam, un_dam;
	WARUSR *wuptr;
	MISSILE *mptr;
	TORPEDO *tptr;
	unsigned *dptr;
	double damfact;
	WARSHP *sptr;
	byte ptr_neb, src_neb;
	double ndist;

	/* reload all single-tick systems */
	if (ptr->hypha > 0)
		--(ptr->hypha);

	if (ptr->jamload > 0)
		--(ptr->jamload);

	if (ptr->zipload > 0)
		--(ptr->zipload);

	if (ptr->mineload > 0)
		--(ptr->mineload);

	if (ptr->decload > 0)
		--(ptr->decload);

	ptr->torps_fired = 0;
	ptr->missl_fired = 0;

	/* knock down battle lock ticks */
	if (ptr->cantexit > 0)
		--(ptr->cantexit);

	ptr_neb = (byte)innebula(coord1(ptr->coord.xcoord),coord1(ptr->coord.ycoord));

	/* resolve inbound torpedoes, aggregating damage and attacker credit across all live slots */
	shotdown = 0;
	track_cnt = 0;
	acc_used = 0;
	shmsg = 0;
	sh_cnt = un_cnt = 0;
	sh_dam = un_dam = 0.0;
	for (i = 0, tptr = ptr->ltorps; i < MAXTORPS; ++i, ++tptr) {
		if (tptr->distance > 0) {
			ptr->cantexit = FIRETICKS;
			if (tptr->distance >= 5000 && (int)tptr->channel < nships && ingegame((int)tptr->channel)) {
				sptr = warshpoff((int)tptr->channel);
				src_neb = (byte)innebula(coord1(sptr->coord.xcoord),coord1(sptr->coord.ycoord));
				if (ptr_neb || src_neb) {
					ndist = cdistance(&ptr->coord,&sptr->coord) * 10000.0;
					if (!(ptr_neb && src_neb && ndist < (double)NEBRNG)) {
						tptr->distance = 0;
						if ((int)tptr->channel < nterms && ingegame((int)tptr->channel)) {
							prfmsg(TORMISS,shpltr(tptr->channel,usrn));
							outprfge(FLT_NONE,tptr->channel);
						}
						continue;
					}
				}
			}
			if (neutral(&ptr->coord) && tptr->distance < 5000) {
				tptr->distance = 0;
				++shotdown;
				if ((int)tptr->channel < nterms && ingegame((int)tptr->channel)) {
					prfmsg(TORMISS,shpltr(tptr->channel,usrn));
					outprfge(FLT_NONE,tptr->channel);
				}
			}
			else if (tptr->distance <= torpsped) {
				tptr->distance = 0;
				if (ptr->shieldstat == SHIELDUP) {
					damfact = tdammax * rndm(.5);
					damfact = ton_fact(ptr,damfact); /* damage factor */

					ptr->damage += damfact;
					if ((int)tptr->channel < nships && ingegame((int)tptr->channel)) {
						wuptr = warusroff((int)tptr->channel);
						set_dislike(wuptr,shipclass[ptr->shpclass].faction,(int)damfact);
					}
					if (++sh_cnt == 1)
						sh_dam = damfact;
					else
						sh_dam += damfact;
					for (k = 0; k < acc_used; ++k) {
						if (acc_chan[k] == tptr->channel)
							break;
					}
					if (k < acc_used)
						++acc_cnt[k];
					else {
						acc_chan[acc_used] = tptr->channel;
						acc_cnt[acc_used] = 1;
						++acc_used;
					}
					shres = shieldhit(ptr,(gernd() % 20) + 10);
					if (shres == 1 || (shres == 2 && shmsg == 0))
						shmsg = shres;
				}
				else {
					damfact = rndm(.5) + .5;
					damfact = tdammax * damfact;

					damfact = ton_fact(ptr,damfact); /* damage factor */

					ptr->damage += damfact;
					if ((int)tptr->channel < nships && ingegame((int)tptr->channel)) {
						wuptr = warusroff((int)tptr->channel);
						set_dislike(wuptr,shipclass[ptr->shpclass].faction,(int)damfact);
					}
					if (++un_cnt == 1)
						un_dam = damfact;
					else
						un_dam += damfact;
					for (k = 0; k < acc_used; ++k) {
						if (acc_chan[k] == tptr->channel)
							break;
					}
					if (k < acc_used)
						++acc_cnt[k];
					else {
						acc_chan[acc_used] = tptr->channel;
						acc_cnt[acc_used] = 1;
						++acc_used;
					}
				}
			}
			else {	/* still flying */
				for (j = 0, dptr = ptr->decout; j < MAXDECOY; ++j) {
					if (dptr[j] > 0) {
						if (tptr->distance < 5000 && (gernd() % decodds == 0)) {
							prfmsg(TORDEST);
							outprfge(FLT_NONE,usrn);
							if (tptr->channel < nterms && ingegame((int)tptr->channel)) {
								prfmsg(TORDEST2);
								outprfge(FLT_NONE,tptr->channel);
							}
							dptr[j] = 0;
							tptr->distance = 0;
							break;
						}
					}
				}
				if (tptr->distance > 0) {	/* torp still here? */
					tptr->distance -= torpsped;
					if (tptr->distance > 0)
						++track_cnt;
				}
			}
		}
	}

	if (sh_cnt > 0) {
		damstr((int)sh_dam);
		if (sh_cnt == 1)
			prfmsg(THIT1,gechrbuf);
		else
			prfmsg(THIT3,sh_cnt,gechrbuf);
		outprfge(FLT_NONE,usrn);
		shieldhitmsg(shmsg,usrn);
	}

	if (un_cnt > 0) {
		damstr((int)un_dam);
		if (un_cnt == 1)
			prfmsg(THIT2,gechrbuf);
		else
			prfmsg(THIT4,un_cnt,gechrbuf);
		outprfge(FLT_NONE,usrn);
	}

	if ((sh_dam + un_dam) > 0.0)
		randamage(ptr,usrn,sh_dam + un_dam); /* combined torpedo random damage check */

	if (shotdown > 0) {
		if (shotdown == 1)
			prfmsg(TORENF1);
		else
			prfmsg(TORENF,shotdown);
		outprfge(FLT_NONE,usrn);
	}

	if (track_cnt == 1) {
		prfmsg(TORP1);
		outprfge(FLT_NONE,usrn);
	}
	else if (track_cnt > 1) {
		prfmsg(TORP4,track_cnt);
		outprfge(FLT_NONE,usrn);
	}

	/* apply one attribution/update pass per distinct torpedo owner */
	for (k = 0; k < acc_used; ++k)
		acctm(ptr,usrn,0,acc_chan[k],acc_cnt[k]);

	/* resolve inbound missiles with the same aggregate-damage/credit pattern */
	shotdown = 0;
	track_cnt = 0;
	lost_cnt = 0;
	acc_used = 0;
	shmsg = 0;
	sh_cnt = un_cnt = 0;
	sh_dam = un_dam = 0.0;
	for (i = 0, mptr = ptr->lmissl; i < MAXMISSL; ++i, ++mptr) {
		if (mptr->distance > 0) {
		unsigned mstep;
		float menergy, mscale;

			ptr->cantexit = FIRETICKS;
			if (mptr->distance >= 5000 && (int)mptr->channel < nships && ingegame((int)mptr->channel)) {
				sptr = warshpoff((int)mptr->channel);
				src_neb = (byte)innebula(coord1(sptr->coord.xcoord),coord1(sptr->coord.ycoord));
				if (ptr_neb || src_neb) {
					ndist = cdistance(&ptr->coord,&sptr->coord) * 10000.0;
					if (!(ptr_neb && src_neb && ndist < (double)NEBRNG)) {
						mptr->distance = 0;
						++lost_cnt;
						if ((int)mptr->channel < nterms && ingegame((int)mptr->channel)) {
							prfmsg(MISMISS,shpltr(mptr->channel,usrn));
							outprfge(FLT_NONE,mptr->channel);
						}
						continue;
					}
				}
			}
			/* heavy missiles up to half speed, light missiles up to 2x speed */
			menergy = (float)mptr->energy + 300.0f;
			mscale = sqrt(5000.0f / menergy);
			if (mscale > 2.0f)
				mscale = 2.0f;
			if (mscale < 0.5f)
				mscale = 0.5f;
			mstep = (unsigned int)(mislsped * mscale);
			if (mstep == 0)
				mstep = 1;

			if (neutral(&ptr->coord) && mptr->distance < 5000) {
				mptr->distance = 0;
				++shotdown;
				if ((int)mptr->channel < nterms && ingegame((int)mptr->channel)) {
					prfmsg(MISMISS,shpltr(mptr->channel,usrn));
					outprfge(FLT_NONE,mptr->channel);
				}
			}
			else if (mptr->distance <= mstep) {
				mptr->distance = 0;
				/* reduce the energy by the damage factor of this ship */
				damfact = mptr->energy;
				damfact = ton_fact(ptr,damfact);
				mptr->energy = damfact;

				if (ptr->shieldstat == SHIELDUP) {
					damfact = damfact / 20000.0;
					damfact = mdammax * (damfact * rndm(.1));
					ptr->damage += damfact;
					if (++sh_cnt == 1)
						sh_dam = damfact;
					else
						sh_dam += damfact;
					if ((int)mptr->channel < nships && ingegame((int)mptr->channel)) {
						wuptr = warusroff((int)mptr->channel);
						set_dislike(wuptr,shipclass[ptr->shpclass].faction,(int)damfact);
					}
					for (k = 0; k < acc_used; ++k) {
						if (acc_chan[k] == mptr->channel)
							break;
					}
					if (k < acc_used)
						++acc_cnt[k];
					else {
						acc_chan[acc_used] = mptr->channel;
						acc_cnt[acc_used] = 1;
						++acc_used;
					}

					power = mptr->energy / 999;
					power = power * (rndm(.5) + .5);
					shres = shieldhit(ptr,power);
					if (shres == 1 || (shres == 2 && shmsg == 0))
						shmsg = shres;
				}
				else {
					damfact = damfact / 20000.0;
					damfact = mdammax * (damfact * (rndm(.5) + .5));
					ptr->damage += damfact;
					if (++un_cnt == 1)
						un_dam = damfact;
					else
						un_dam += damfact;
					if ((int)mptr->channel < nships && ingegame((int)mptr->channel)) {
						wuptr = warusroff((int)mptr->channel);
						set_dislike(wuptr,shipclass[ptr->shpclass].faction,(int)damfact);
					}
					for (k = 0; k < acc_used; ++k) {
						if (acc_chan[k] == mptr->channel)
							break;
					}
					if (k < acc_used)
						++acc_cnt[k];
					else {
						acc_chan[acc_used] = mptr->channel;
						acc_cnt[acc_used] = 1;
						++acc_used;
					}
				}
			}
			else {
				for (j = 0, dptr = ptr->decout; j < MAXDECOY; ++j) {
					if (dptr[j] > 0) {
						if (mptr->distance < 3000 && (gernd() % decodds == 0)) {
							prfmsg(MISDEST);
							outprfge(FLT_NONE,usrn);
							if (ingegame(mptr->channel) && mptr->channel < nterms) {
								prfmsg(MISDEST2);
								outprfge(FLT_NONE,mptr->channel);
							}
							dptr[j] = 0;
							mptr->distance = 0;
							break;
						}
					}
				}
				if (mptr->distance > 0) {	/* missl still here? */
					mptr->distance -= mstep;
					if (ptr->speed > 100000.0 || mptr->distance > 50000U - (int)(ptr->speed / 6.5) ||
						mptr->energy <= 500) {
						mptr->distance = 0;
						++lost_cnt;
						if (ingegame(mptr->channel) && mptr->channel < nterms) {
							prfmsg(MISMISS,shpltr(mptr->channel,usrn));
							outprfge(FLT_NONE,mptr->channel);
						}
					}
					else {
						mptr->distance += (int)(ptr->speed / 6.5);
						mptr->energy -= 500;	/* decrease energy over time */
					}
					if (mptr->distance > 0)
						++track_cnt;
				}
			}
		}
	}

	if (lost_cnt == 1) {
		prfmsg(MISSL2);
		outprfge(FLT_NONE,usrn);
	}
	else if (lost_cnt > 1) {
		prfmsg(MISSL4,lost_cnt);
		outprfge(FLT_NONE,usrn);
	}

	if (sh_cnt > 0) {
		damstr((int)sh_dam);
		if (sh_cnt == 1)
			prfmsg(MHIT1,gechrbuf);
		else
			prfmsg(MHIT3,sh_cnt,gechrbuf);
		outprfge(FLT_NONE,usrn);
		shieldhitmsg(shmsg,usrn);
	}

	if (un_cnt > 0) {
		damstr((int)un_dam);
		if (un_cnt == 1)
			prfmsg(MHIT2,gechrbuf);
		else
			prfmsg(MHIT4,un_cnt,gechrbuf);
		outprfge(FLT_NONE,usrn);
	}

	if ((sh_dam + un_dam) > 0.0)
		randamage(ptr,usrn,sh_dam + un_dam); /* combined missile random damage check */

	/* apply one attribution/update pass per distinct missile owner */
	for (k = 0; k < acc_used; ++k)
		acctm(ptr,usrn,1,acc_chan[k],acc_cnt[k]);

	if (shotdown > 0) {
		if (shotdown == 1)
			prfmsg(MISENF1);
		else
			prfmsg(MISENF,shotdown);
		outprfge(FLT_NONE,usrn);
	}

	if (track_cnt == 1) {
		prfmsg(MISSL1);
		outprfge(FLT_NONE,usrn);
	}
	else if (track_cnt > 1) {
		prfmsg(MISSL3,track_cnt);
		outprfge(FLT_NONE,usrn);
	}

	shotdown = 0;

	/* age out deployed decoys, then advance jammer and cloak transition timers */
	for (i = 0, dptr = ptr->decout; i < MAXDECOY; ++i) {
		if (dptr[i] > 0) {
			if (dptr[i] > 1)
				--dptr[i];
			else if (decpass == 0) {
				dptr[i] = 0;
				++shotdown;
			}
		}
	}
	if (shotdown == 1) {
		prfmsg(DECGONE);
		outprfge(FLT_NONE,usrn);
	}
	else if (shotdown > 1) {
		prfmsg(DECGONE2,shotdown);
		outprfge(FLT_NONE,usrn);
	}
	if (ptr->jam_time > (byte)0) {
		--ptr->jam_time;
		if (ptr->jam_time == (byte)0) {
			ptr->jam_sev = (byte)0;
			prfmsg(JAMMER5);
			outprfge(FLT_NONE,usrn);
		}
	}
	if (ptr->cloak == 1) {
		ptr->cloak = 2;
	}
	else if (ptr->cloak == 2) {
		ptr->cloak = 10;
		prfmsg(CLOKUP);
		outprfge(FLT_NONE,usrn);
		if (ptr->lock >= 0) {
			if (ptr->lock < nships && ingegame(ptr->lock)) {
				prfmsg(LOCK04,shpltr(ptr->lock,usrn));
				outprfge(FLT_NONE,ptr->lock);
			}
			ptr->lock = -1;
			ptr->track_grace = 0;
			prfmsg(LOCK01);
			outprfge(FLT_NONE,usrn);
		}
	}
	/* small weapon delay */
	if (ptr->cloak == 3) {
		ptr->cloak = 0;
		prfmsg(CLOKW);
		outprfge(FLT_NONE,usrn);
	}

	/* clear lastfired if npc no longer exists */
	if (ptr->lastfired >= 0 && ptr->lastfired < nships && warshpoff(ptr->lastfired)->status == GESTAT_AVAIL)
		ptr->lastfired = -1;

}

/**************************************************************************
** Verify that a maintained battle lock is still valid                  **
**************************************************************************/

void FUNC validate_lock(WARSHP *ptr, int usrn)
{
	WARSHP *lptr;
	double dist;
	int lockee;
	byte locker_neb;
	byte lockee_neb;

	if (ptr->lock < 0)
		return;

	if (ptr->lock >= nships) {
		ptr->lock = -1;
		ptr->track_grace = 0;
		prfmsg(LOCK01);
		outprfge(FLT_NONE, usrn);
		return;
	}

	lockee = ptr->lock;
	lptr = warshpoff(lockee);

	/* stale lock target: clear both the lock and its grace period */
	if (!ingegame(lockee) || lptr->status == GESTAT_AVAIL) {
		ptr->lock = -1;
		ptr->track_grace = 0;
		return;
	}

	dist = cdistance(&ptr->coord, &lptr->coord) * 10000.0;

	if (dist > (double)ship_scanrange(ptr)) {
		ptr->lock = -1;
		ptr->track_grace = 0;
		prfmsg(LOCK05);
		outprfge(FLT_NONE, usrn);
		prfmsg(LOCK04, shpltr(lockee, usrn));
		outprfge(FLT_NONE, lockee);
		return;
	}

	if (lptr->cloak >= 10) {
		if (ptr->track_grace > 0) {
			--ptr->track_grace;
			return;
		}
		ptr->lock = -1;
		ptr->track_grace = 0;
		prfmsg(LOCK05);
		outprfge(FLT_NONE, usrn);
		prfmsg(LOCK04, shpltr(lockee, usrn));
		outprfge(FLT_NONE, lockee);
		return;
	}

	locker_neb = (byte)innebula(coord1(ptr->coord.xcoord), coord1(ptr->coord.ycoord));
	lockee_neb = (byte)innebula(coord1(lptr->coord.xcoord), coord1(lptr->coord.ycoord));
	if (locker_neb || lockee_neb) {
		if (!(locker_neb && lockee_neb && dist < (double)NEBRNG)) {
			if (ptr->track_grace > 0) {
				--ptr->track_grace;
				return;
			}
			ptr->lock = -1;
			ptr->track_grace = 0;
			prfmsg(LOCK05);
			outprfge(FLT_NONE, usrn);
			prfmsg(LOCK04, shpltr(lockee, usrn));
			outprfge(FLT_NONE, lockee);
			return;
		}
	}

	ptr->track_grace = LOCKGRACE;
}

/**************************************************************************
** Credit projectile attackers and notify live user owners of impacts    **
**************************************************************************/

void FUNC acctm(WARSHP *ptr, int usrn, int mt, byte channel, int count)
{
	if (channel < nships && ingegame(channel))
		ptr->lastfired = channel;
	else
		ptr->lastfired = -1;

	/* any live ship keeps credit; only live real users get the attacker message */
	if (channel < nterms && ingegame(channel)) {
		if (count <= 1) {
			if (ptr->status == GESTAT_AUTO)
				prfmsg(MTACC1N + mt, shpltr(channel, usrn), ptr->shipname);
			else if (ptr->shipname[0] == 0)
				prfmsg(MTACC1NO + mt, shpltr(channel, usrn), ptr->userid);
			else
				prfmsg(MTACC1 + mt, shpltr(channel, usrn), ptr->shipname);
		}
		else {
			if (ptr->status == GESTAT_AUTO)
				prfmsg(MTACC3N + mt, count, shpltr(channel, usrn), ptr->shipname);
			else if (ptr->shipname[0] == 0)
				prfmsg(MTACC3NO + mt, count, shpltr(channel, usrn), ptr->userid);
			else
				prfmsg(MTACC3 + mt, count, shpltr(channel, usrn), ptr->shipname);
		}
		outprfge(FLT_NONE, channel);
	}
}

/**************************************************************************
** Check whether a ship has any inbound torpedoes or missiles           **
**************************************************************************/

int FUNC chkitm(int usrn)
{
	WARSHP *ptr;
	int i;

	ptr = warshpoff(usrn);

	for (i = 0; i < MAXTORPS; ++i) {
		if (ptr->ltorps[i].distance > 0)
			return FALSE;
	}
	for (i = 0; i < MAXMISSL; ++i) {
		if (ptr->lmissl[i].distance > 0)
			return FALSE;
	}
	return TRUE;
}

/**************************************************************************
** Clear all inbound torpedoes and missiles for one ship                **
**************************************************************************/

void FUNC clearitm(int usrn)
{
	WARSHP *ptr;
	byte owners[MAXTORPS], ocount, found;
	int i, oi;

	ptr = warshpoff(usrn);

	ocount = 0;

	for (i = 0; i < MAXTORPS; ++i) {
		if (ptr->ltorps[i].distance > 0) {
			ptr->ltorps[i].distance = 0;
			if (ptr->ltorps[i].channel < nterms && ingegame(ptr->ltorps[i].channel)) {
				found = FALSE;
				for (oi = 0; oi < (int)ocount; ++oi) {
					if (owners[oi] == ptr->ltorps[i].channel) {
						found = TRUE;
						break;
					}
				}
				if (found == FALSE && ocount < MAXTORPS)
					owners[ocount++] = ptr->ltorps[i].channel;
			}
		}
	}

	for (oi = 0; oi < (int)ocount; ++oi) {
		prfmsg(TORMISS2,shpltr(owners[oi],usrn));
		outprfge(FLT_NONE,owners[oi]);
	}

	ocount = 0;

	for (i = 0; i < MAXMISSL; ++i) {
		if (ptr->lmissl[i].distance > 0) {
			ptr->lmissl[i].distance = 0;
			if (ptr->lmissl[i].channel < nterms && ingegame(ptr->lmissl[i].channel)) {
				found = FALSE;
				for (oi = 0; oi < (int)ocount; ++oi) {
					if (owners[oi] == ptr->lmissl[i].channel) {
						found = TRUE;
						break;
					}
				}
				if (found == FALSE && ocount < MAXMISSL)
					owners[ocount++] = ptr->lmissl[i].channel;
			}
		}
	}

	for (oi = 0; oi < (int)ocount; ++oi) {
		prfmsg(MISMISS2,shpltr(owners[oi],usrn));
		outprfge(FLT_NONE,owners[oi]);
	}
}

/**************************************************************************
** Clear one attacker's ownership from every inbound projectile slot     **
**************************************************************************/

void FUNC cleartm(int channel)
{
	MINE *mptr;
	WARSHP *wptr;
	int i, j;
	int zothusn;

	for (i = 0, mptr = mines; i < nummines; ++i, ++mptr) {
		if (mptr->channel == (byte)channel)
			mptr->channel = 255;
	}

	for (zothusn = 0; zothusn < nships; zothusn++) {
		if (ingegame(zothusn)) {
			wptr = warshpoff(zothusn);
			for (j = 0; j < MAXTORPS; ++j) {
				if (wptr->ltorps[j].channel == (byte)channel)
					wptr->ltorps[j].channel = 255;
			}
			for (j = 0; j < MAXMISSL; ++j) {
				if (wptr->lmissl[j].channel == (byte)channel)
					wptr->lmissl[j].channel = 255;
			}
		}
	}
}


/**************************************************************************
** Check if the ION cannons need to be fired                             **
**************************************************************************/

void FUNC fireion(WARSHP *ptr, int usrn)
{
	double hitdam;
	int shmsg;

	if (ptr->hostile > 1) {
		plnum = ptr->hostile - 10;
		if (!getplanetdat(usrn))
			return;
		if (plptr->items[I_IONCANNON].qty > 0) {
			ptr->lastfired = -1;
			if (ptr->shieldstat == SHIELDUP) {
				/* shields soak most of the blast, but the ion burst still rattles the ship */
				hitdam = (idammax * rndm(.15));
				ptr->damage += hitdam;
				prfmsg(IHIT1);
				outprfge(FLT_NONE, usrn);
				shmsg = shieldhit(ptr, (gernd() % 50) + 40);
				shieldhitmsg(shmsg, usrn);
			}
			else {
				/* without shields up, the burst does a much larger direct hull hit */
				hitdam = (idammax * (rndm(.50) + .50));
				ptr->damage += hitdam;
				prfmsg(IHIT2);
				outprfge(FLT_NONE, usrn);
			}
			randamage(ptr, usrn, hitdam);
		}
	}
}

/**************************************************************************
** Self Destruct countdown                                               **
**************************************************************************/

void FUNC destruct(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn;
	double ddist;

	if (ptr->destruct > (byte)0) {
		if (--ptr->destruct > (byte)0) {
			if (ptr->destruct == 10) {
				if (ptr->shipname[0] == 0)
					prfmsg(SELFD2AO,ptr->userid);
				else
					prfmsg(SELFD2A,ptr->shipname);
				outrange(FLT_NONE,&ptr->coord);
			}

			if (ptr->destruct == 5) {
				if (ptr->shipname[0] == 0)
					prfmsg(SELFD2BO,ptr->userid);
				else
					prfmsg(SELFD2B,ptr->shipname);
				outrange(FLT_NONE,&ptr->coord);
			}

			if (ptr->destruct == 2) {
				if (ptr->shipname[0] == 0)
					prfmsg(SELFD2CO,ptr->userid);
				else
					prfmsg(SELFD2C,ptr->shipname);
				outrange(FLT_NONE,&ptr->coord);
			}

			prfmsg(SELFD2,ptr->destruct);
			outprfge(FLT_NONE,usrn);
		}
		else {
			prfmsg(SELFD3);
			ptr->damage = 101;
			outprfge(FLT_NONE,usrn);
			if (ptr->shipname[0] == 0)
				prfmsg(SELFD3AO,ptr->userid);
			else
				prfmsg(SELFD3A,ptr->shipname);
			for (zothusn = 0; zothusn < nships; zothusn++) {
				wptr = warshpoff(zothusn);
				if (ingegame(zothusn) && usrn != zothusn && shipclass[wptr->shpclass].max_type == CLASSTYPE_USER) {
					ddist = cdistance(&ptr->coord,&wptr->coord);
					ddist *= 10000;
					if (ddist < (double)shipclass[wptr->shpclass].scanrange)
						outprfge(FLT_NONE,zothusn);
				}
			}
			clrprf();
			if (ptr->shieldstat == SHIELDUP) {
				prfmsg(SELFD3S);
				outprfge(FLT_NONE,usrn);
			}
			else {
				for (zothusn = 0; zothusn < nships; zothusn++) {
					wptr = warshpoff(zothusn);
					if (ingegame(zothusn) && usrn != zothusn) {
						ddist = cdistance(&ptr->coord,&wptr->coord);
						ddist *= 10000;
						setsect(wptr);
						if (ddist < ((double)DESTRUCTRANGE) && !neutral(&wptr->coord)) {
							ddist = 1.0 - (ddist / ((double)DESTRUCTRANGE));
							if (ddist < 0)
								ddist = 0;
							ddist = ddist * ddist * ddist;
							if (wptr->shieldstat == SHIELDUP) {
								damage = (unsigned)(ddist * minedammax);
								damage = damage * (shipclass[ptr->shpclass].damfact / 100);
								damage = damage / (gernd() % 5 + wptr->shieldtype);
								damstr(damage);
								prfmsg(SELFD6,gechrbuf);
								outprfge(FLT_NONE,zothusn);
								shieldhitmsg(shieldhit(wptr,damage + 20),zothusn);
							}
							else {
								damage = (unsigned)(ddist * minedammax);
								damage = damage * (shipclass[ptr->shpclass].damfact / 100);
								damstr(damage);
								prfmsg(SELFD7,gechrbuf);
								outprfge(FLT_NONE,zothusn);
							}
							wptr->damage += (double)damage;
							wptr->lastfired = -1;
							if (wptr->shipname[0] == 0)
								prfmsg(SELFD3N,gechrbuf,username(wptr));
							else
								prfmsg(SELFD3O,gechrbuf,wptr->shipname);
							outprfge(FLT_NONE,usrn);
						}
					}
				}
			}
		}
	}
}

/**************************************************************************
** Verify percent for validaty                                           **
**************************************************************************/

int FUNC valpcnt(char *ptr, unsigned minnum, unsigned maxnum)
{
	int val;
	char *inpptr;

	stripb(ptr);
	if (inplen != 0) {
		for (inpptr = ptr; isdigit((unsigned char)*inpptr); inpptr++) {
		}
		if (*inpptr == 0 || *inpptr == ' ') {
			if ((val = atoi(ptr)) >= minnum && val <= maxnum) {
				warsptr->percent = val;
				return 1;
			}
		}
	}
	prfmsg(NUMOOR, minnum, maxnum);
	outprfge(FLT_NONE, usrnum);
	return 0;
}

/**************************************************************************
** Verify degree for validity                                            **
**************************************************************************/

int FUNC valdegree(char *ptr)
{
	int val;

	if (strlen(ptr) != 0) {
		val = atoi(ptr);
		if (val >= -180 && val <= 180) {
			warsptr->degrees = val;
		return 1;
		}
	}
	prfmsg(NUMOOR, -180, 180);
	outprfge(FLT_NONE, usrnum);
	return 0;
}

/**************************************************************************
** Assess any random damage                                              **
**************************************************************************/

static int rd_item(WARSHP *ptr, unsigned int r, int itemnum, int damcomb)
{
	unsigned int roll;
	unsigned long qty, maxloss, have;
	double frac;

	have = ptr->items[itemnum];
	if (!have)
		return 0;

	/* heavier existing damage pushes the loss ceiling up, but big inventories resist total wipes */
	frac = (double)damcomb;
	frac = frac / (frac + (double)have / 100.0);
	maxloss = (unsigned long)(have * frac);

	if (maxloss > have)
		maxloss = have;

	if (!maxloss)
		maxloss = 1;

	/* bias toward lighter losses most of the time, with occasional bigger bites */
	roll = r % 100;
	if (roll < 50)
		qty = 1 + (r % ((maxloss / 2) + 1));
	else if (roll < 80)
		qty = 1 + (r % (((3 * maxloss) / 4) + 1));
	else
		qty = 1 + (r % maxloss);

	/* protect tiny stacks from disappearing outright too easily */
	if (have < 10 && qty >= have)
		qty = 1 + r % (2 + (have >> 1));

	/* even for larger stacks, avoid losing the whole pile in one random damage event */
	if (qty >= have) {
		if (have == 1)
			qty = 1;
		else
			qty = (have >> 1) + 1;
	}

	if (qty > maxloss)
		qty = maxloss;

	if (!qty)
		qty = 1;

	ptr->items[itemnum] = have - qty;
	return qty > 32767UL ? 32767 : (int)qty;
}

static void rd_append(char *buf, byte *comma, int qty, const char *sing, const char *plur)
{
	if (qty <= 0)
		return;

	if (*comma)
		sprintf(buf + strlen(buf), ", ");

	sprintf(buf + strlen(buf), "%d %s", qty, qty == 1 ? sing : plur);
	(*comma)++;
}

static int rd_add(WARSHP *ptr, unsigned int r, int itemnum, int damcomb,
	char *buf, byte *comma, const char *sing, const char *plur)
{
	int qty = rd_item(ptr, r, itemnum, damcomb);

	if (qty <= 0)
		return 0;

	if (*comma)
		sprintf(buf + strlen(buf), ", ");

	sprintf(buf + strlen(buf), "%d %s", qty, qty == 1 ? sing : plur);

	(*comma)++;
	return qty;
}

void FUNC randamage(WARSHP *ptr, int usrn, double hitdam)
{
	int a, i, damcomb, qty, types, idx, item;
	byte comma = 0, doitems = 0, dosys = 0;
	unsigned int r, r2;

	gechrbuf[0] = 0;

	/* already dead */
	if (ptr->damage >= 100.0)
		return;

	damcomb = (int)(ptr->damage - hitdam);

	/* hit must be over 5 and damage before hit must be over 20 */
	if (hitdam < 5.0 || damcomb < 20)
		return;

	/* weight toward big single hits */
	damcomb = (int)((damcomb * 0.7) + (hitdam * (1.1 + hitdam * 0.025)));

	/* cap to ensure that all 11 options are possible */
	if (damcomb > 74)
		damcomb = 74;

	r = gernd();
	r2 = gernd();

	/* peel off low bits first so one roll can gate item and system damage separately */
	doitems = r & 1;
	dosys = (r >> 1) & 1;
	if (!doitems && !dosys) {
		doitems = 1;
		dosys = 1;
	}

	if (ptr->status == GESTAT_AUTO) {
		doitems = 0;
		if (!dosys)
			dosys = 1;
	}

	a = r % (85 - damcomb);

	if (a > 10)	/* no effect */
		return;

	switch (a) {
	case 0: /* missiles */
		if (shipclass[ptr->shpclass].max_missl > 0) {
			if (dosys == 1 && ptr->mislcntl == 0) {
				ptr->mislcntl = 2 + r % (damcomb/3);
				prfmsg(RNDMISL);
				dosys = 2;
			}
			if (doitems == 1 && ptr->items[I_MISSILE] > 0) {
				qty = rd_item(ptr, r, I_MISSILE, damcomb);
				prfmsg(RNDITEM, qty, qty == 1 ? "missile was" : "missiles were");
				doitems = 2;
			}
		}
		break;

	case 1: /* torpedoes */
		if (shipclass[ptr->shpclass].max_torps > 0) {
			if (dosys == 1 && ptr->torpcntl == 0) {
				ptr->torpcntl = 2 + r % (damcomb/3);
				prfmsg(RNDTORP);
				dosys = 2;
			}
			if (doitems == 1 && ptr->items[I_TORPEDO] > 0) {
				qty = rd_item(ptr, r, I_TORPEDO, damcomb);
				prfmsg(RNDITEM, qty, qty == 1 ? "torpedo was" : "torpedoes were");
				doitems = 2;
			}
		}
		break;

	case 2: /* decoys */
		if (shipclass[ptr->shpclass].has_decoy > 0) {
			if (dosys == 1 && ptr->decload >= 0) {
				ptr->decload = -2 - r % (damcomb/3);
				prfmsg(RNDDECY);
				dosys = 2;
			}
			if (doitems == 1 && ptr->items[I_DECOYS] > 0) {
				qty = rd_item(ptr, r, I_DECOYS, damcomb);
				prfmsg(RNDITEM, qty, qty == 1 ? "decoy was" : "decoys were");
				doitems = 2;
			}
		}
		break;

	case 3: /* zippers */
		if (shipclass[ptr->shpclass].has_zip > 0) {
			if (dosys == 1 && ptr->zipload >= 0) {
				ptr->zipload = -2 - r % (damcomb/3);
				prfmsg(RNDZIPR);
				dosys = 2;
			}
			if (doitems == 1 && ptr->items[I_ZIPPERS] > 0) {
				qty = rd_item(ptr, r, I_ZIPPERS, damcomb);
				prfmsg(RNDITEM, qty, qty == 1 ? "zipper was" : "zippers were");
				doitems = 2;
			}
		}
		break;

	case 4: /* jammers */
		if (shipclass[ptr->shpclass].has_jam > 0) {
			if (dosys == 1 && ptr->jamload >= 0) {
				ptr->jamload = -2 - r % (damcomb/3);
				prfmsg(RNDJAMR);
				dosys = 2;
			}
			if (doitems == 1 && ptr->items[I_JAMMERS] > 0) {
				qty = rd_item(ptr, r, I_JAMMERS, damcomb);
				prfmsg(RNDITEM, qty, qty == 1 ? "jammer was" : "jammers were");
				doitems = 2;
			}
		}
		break;

	case 5: /* mines */
		if (shipclass[ptr->shpclass].has_mine > 0) {
			if (dosys == 1 && ptr->mineload >= 0) {
				ptr->mineload = -2 - r % (damcomb/3);
				prfmsg(RNDMINE);
				dosys = 2;
			}
			if (doitems == 1 && ptr->items[I_MINE] > 0) {
				qty = rd_item(ptr, r, I_MINE, damcomb);
				prfmsg(RNDITEM, qty, qty == 1 ? "mine was" : "mines were");
				doitems = 2;
			}
		}
		break;

	case 6: /* shields */
		if (shipclass[ptr->shpclass].max_shlds > 0 && ptr->shieldstat != SHIELDDM && dosys == 1) {
			prfmsg(SHDAMAG);
			ptr->shield = (int)(-2 - r % (damcomb/3));
			ptr->shieldstat = SHIELDDM;
			dosys = 2;
		}
		break;

	case 7: /* phasers */
		if (shipclass[ptr->shpclass].max_phasr > 0 && ptr->phasr >= 0 && dosys == 1) {
			prfmsg(RNDPHSR);
			ptr->phasr = (int)(-2 - r % (damcomb/3));
			dosys = 2;
		}
		break;

	case 8: /* cloak */
		if (shipclass[ptr->shpclass].max_cloak > 0 && ptr->cloak >= 0 && dosys == 1) {
			prfmsg(RNDCLOK);
			ptr->cloak = -2 - r % (damcomb/3);
			dosys = 2;
		}
		break;

	case 9: /* scanners */
		if (ptr->tactical == 0 && dosys == 1) {
			prfmsg(RNDTACT);
			ptr->tactical = -2 - r % (damcomb/6);
			dosys = 2;
		}
		break;

	case 10: /* helm */
		if (ptr->helm == 0 && dosys == 1) {
			prfmsg(RNDNAVG);
			ptr->helm = -2 - r % (damcomb/9);
			dosys = 2;
		}
		break;

	default:
		break;
	}

	/* if we didn't blow up a system or that system's items, and still need to do items */
	if (doitems == 1 && dosys != 2) {
		a = r2 % 10;	/* 8 or 9 no effect */
		if (a == 4 || a == 5)	/* mess hall and head should happen less than the others */
			a = 0;
		if (a == 6 || a == 7)
			a = 1;

		switch (a) {
		case 0: /* cargo bay */
		{
			byte allowed[NUMITEMS-4];
			int count = 0;

			prfmsg(RNDCRGO);

			for (i = 0; i < NUMITEMS; i++) {
				if (i == I_MEN || i == I_TROOPS || i == I_SPY || i == I_GOLD)
					continue;
				if (ptr->items[i] < 2)
					continue;
				allowed[count++] = (byte)i;
			}

			if (!count)		/* nothing to damage */
				break;

			types = 1 + (damcomb / 20);	/* how many types of items to damage */
			if (types > 4)
				types = 4;

			if (types > count)
				types = count;

			/* use a different 4-bit slice each pass so one roll can choose several item types */
			for (i = 0; i < types; ++i) {
				idx = (r2 >> (i*4)) % count; /* each 4 bits gives new entropy slice */
				item = allowed[idx];

				/* swap-remove to prevent repeats without looping */
				allowed[idx] = allowed[--count];

				rd_add(ptr, r2 >> i, item, damcomb, gechrbuf, &comma, item_name[item], item_name[item]);
			}
			prfmsg(RNDITEM2, gechrbuf);
			break;
		}

		case 1: /* living quarters */
		{
			int count = 0;

			prfmsg(RNDLVNG);

			/* shift the same roll so each personnel class can lose a different amount */
			if (ptr->items[I_MEN] > 0)
				count += rd_add(ptr, r2, I_MEN, damcomb, gechrbuf, &comma, "man", "men");
			if (ptr->items[I_TROOPS] > 0)
				count += rd_add(ptr, r2 >> 1, I_TROOPS, damcomb, gechrbuf, &comma, "troop", "troops");
			if (ptr->items[I_SPY] > 0)
				count += rd_add(ptr, r2 >> 2, I_SPY, damcomb, gechrbuf, &comma, "spy", "spies");

			if (!count)
				break;
			if (count == 1)
				sprintf(gechrbuf + strlen(gechrbuf), " was");
			else
				sprintf(gechrbuf + strlen(gechrbuf), " were");

			prfmsg(RNDITEM2, gechrbuf);
			break;
		}

		case 2: /* head */
		{
			byte allowed[3];
			int count = 0;

			prfmsg(RNDHEAD);

			if (ptr->items[I_MEN] > 0)
				allowed[count++] = 0;
			if (ptr->items[I_TROOPS] > 0)
				allowed[count++] = 1;
			if (ptr->items[I_SPY] > 0)
				allowed[count++] = 2;

			if (count < 1)
				break;

			/* one low-order slice picks which surviving personnel type gets hit */
			item = allowed[r2 % count];

			if (item == 0) {
				ptr->items[I_MEN]--;
				prfmsg(RNDITEM2, "1 man was");
			}
			if (item == 1) {
				ptr->items[I_TROOPS]--;
				prfmsg(RNDITEM2, "1 troop was");
			}
			if (item == 2) {
				ptr->items[I_SPY]--;
				prfmsg(RNDITEM2, "1 spy was");
			}
			break;
		}

		case 3: /* mess hall */
		{
			byte allowed[3];
			int counter[3] = {0, 0, 0}, count = 0;
			prfmsg(RNDMESS);

			if (ptr->items[I_MEN] > 0)
				allowed[count++] = 0;
			if (ptr->items[I_TROOPS] > 0)
				allowed[count++] = 1;
			if (ptr->items[I_SPY] > 0)
				allowed[count++] = 2;

			if (count < 1)
				break;

			types = 1 + (r2 % 5);

			/* walk across the roll a few bits at a time so repeated picks are not identical */
			for (i = 0; i < types; ++i) {
				idx = (r2 >> (i * 3)) % count;  /* shift entropy slice a bit each time */
				item = allowed[idx];

				if (item == 0 && ptr->items[I_MEN] > 0) {
					ptr->items[I_MEN]--;
					counter[0]++;
				}
				else if (item == 1 && ptr->items[I_TROOPS] > 0) {
					ptr->items[I_TROOPS]--;
					counter[1]++;
				}
				else if (item == 2 && ptr->items[I_SPY] > 0) {
					ptr->items[I_SPY]--;
					counter[2]++;
				}
			}

			rd_append(gechrbuf, &comma, counter[0], "man", "men");
			rd_append(gechrbuf, &comma, counter[1], "troop", "troops");
			rd_append(gechrbuf, &comma, counter[2], "spy", "spies");

			qty = counter[0] + counter[1] + counter[2];

			if (!qty)
				break;
			if (qty == 1)
				sprintf(gechrbuf + strlen(gechrbuf), " was");
			else
				sprintf(gechrbuf + strlen(gechrbuf), " were");

			prfmsg(RNDITEM2, gechrbuf);
			break;
		}

		default:
			break;
		}
	}
	outprfge(FLT_NONE, usrn);
}

/**************************************************************************
** Determine the damage amount                                           **
**************************************************************************/

double FUNC pdamage(WARSHP *wptr, double dist, int foc)
{
	double dd, fd, dp, factor, disfact, dam;

#ifdef MBBSEMU
	int i;
	double fractional;
#endif

	if (wptr->where == 1) {
		/* hyper-phasers fall off only with distance from the beam center */
		factor = hpfirdst;
		/* dd is normalized reach left: 1.0 at point-blank, 0.0 at max hyper-phaser range */
		dd = 1.0 - (dist / 40000.0);
		if (dd < 0.0)
			dd = 0.0;

#ifdef MBBSEMU
		dp = 1.0;
		if (factor > 0.0) {
			for (i = 0; i < (int)factor; ++i)
				dp *= dd;
			fractional = factor - (int)factor;
			if (fractional > 0.0 && dd > 0.0)
				dp *= 1.0 + fractional * (dd - 1.0);
		}
		else
			dp = 0.0;
#else
		dp = pow(dd,factor);
#endif
		dam = hpdammax * dp;
	}
	else {
		/* standard phasers lose force with distance, beam width, and current phaser charge */
		factor = pfirdist;
		disfact = 20000.0 + ((double)wptr->phasrtype * 4000.0);
		/* dd is normalized distance inside this phaser type's effective reach */
		dd = 1.0 - (dist / disfact);
		if (dd < 0.0)
			dd = 0.0;
		/* fd is the beam-focus factor: centered shots stay near 1.0, edge hits collapse toward 0.0 */
		fd = 1.0 - ((double)foc / 11.0);

#ifdef MBBSEMU
		dp = 1.0;
		if (factor > 0.0) {
			for (i = 0; i < (int)factor; ++i)
				dp *= dd;
			fractional = factor - (int)factor;
			if (fractional > 0.0 && dd > 0.0)
				dp *= 1.0 + fractional * (dd - 1.0);
		}
		else {
			dp = 0.0;
		}
		dp *= (fd * fd) * (wptr->phasr / 100.0);
#else
		/* dp is the final damage proportion after distance falloff, beam focus, and current phaser charge */
		dp = (pow(dd,factor)) * (fd * fd) * (wptr->phasr / 100.0);
#endif
		dam = pdammax * dp;
	}

	logthis(spr("Pdamage %s %ld %d",wptr->userid,(long)dist,(int)dam));
	return dam;
}


/**************************************************************************
** set the xsect and ysect coordinates given a ship pntr                 **
**************************************************************************/

void FUNC setsect(WARSHP *ptr)
{
	xsect = coord1(ptr->coord.xcoord);
	ysect = coord1(ptr->coord.ycoord);
	xcord = coord2(ptr->coord.xcoord);
	ycord = coord2(ptr->coord.ycoord);
	pkey.xsect = xsect;
	pkey.ysect = ysect;
	pkey.plnum = 0;
}

/**************************************************************************
** move one coord to another (direction is <-- )                         **
**************************************************************************/

void FUNC movecoord(COORD *pointb, COORD *pointa)
{
	pointb->xcoord = pointa->xcoord;
	pointb->ycoord = pointa->ycoord;
}

/**************************************************************************
** Compare two coords to determine if they are equal                     **
**************************************************************************/

int FUNC samesect(COORD *pointb, COORD *pointa)
{
	int ax, ay, bx, by;

	ax = coord1(pointa->xcoord);
	ay = coord1(pointa->ycoord);
	bx = coord1(pointb->xcoord);
	by = coord1(pointb->ycoord);

	return ax == bx && ay == by;
}

/**************************************************************************
** MAIL functions                                                        **
**************************************************************************/

int FUNC mailscan(char *userid, int class)
{
	strncpy(mailkey.userid,userid,UIDSIZ);
	mailkey.class = class;

	setbtv(gebb4);

	/* if class = 0 scan if user has ANY mail */
	if (class == 0) {
		if (qeqbtv(userid,0)) {
			/* DEBUG
			prf("mail.userid=%s\rmail.class=%d\rmail.type=%d\r",mail.userid,mail.class,mail.type);*/
			return TRUE;
		}
	}
	else
	/* otherwize see if he has this class of mail */
	if (qeqbtv(&mailkey,1)) {
		return TRUE;
	}
	return FALSE;
}

int FUNC mailread(char *userid, int class)
{
	strncpy(mailkey.userid,userid,UIDSIZ);
	mailkey.class = class;
	mailkey.msgno = 0;

	setbtv(gebb4);

	setmem(gemsg,FIXEDMSGSIZ,0);

	if (qeqbtv(&mailkey,1)) {
		gcrbtv(gemsg,1);
		prf("%s------------------------------------------------------------------------------%s\r",CLR_BLUE2,CLR_CYAN2);
		prf(gemsg->text);
		prf("%s------------------------------------------------------------------------------%s",CLR_BLUE2,CLR_WHITE2);
		outprfge(FLT_NONE,usrnum);

		delbtv();

		return TRUE;
	}


	prfmsg(MAIL1);
	outprfge(FLT_NONE,usrnum);
	return FALSE;
}


void FUNC mailit(int flag)
{
	int i;

	setmbk(gemb);
	clrprf();

	if (flag == 1) {
		if (instat(mail.userid,gestt)) {
			if (othusp->substt >= FIGHTSUB) {
				return;
			}
		}
	}

	mail.stamp = cofdat(today());
	sprintf(mail.dtime,"%s - %.5s",ncedat(today()),nctime(now()));

	prfmsg(MAIL2,mail.dtime,mail.userid);

	switch (mail.type) {
		case MESG02:
		case MESG03:
		case MESG04:
		case MESG05:
			strcpy(mail.topic,"Distress Message");
			sprintf(gechrbuf,"%ld",mail.long1);
			prfmsg(mail.type,mail.name1,mail.int1,mail.int2,mail.string1,mail.name2,gechrbuf);
			sendit();
			break;

		case MESG06:
		case MESG07:
			strcpy(mail.topic,"Distress Message");
			sprintf(gechrbuf,"%ld",mail.long1);
			prfmsg(mail.type,mail.name1,mail.int1,mail.int2,gechrbuf);
			sendit();
			break;

		case MESG08:
		case MESG09:
		case MESG10:
		case MESG11:
		case MESG12:
		case MESG13:
		case MESG14:
		case MESG15:
		case MESG16:
		case MESG17:
		case MESG18:
		case MESG19:
		case MESG19A:
		case MESG19B:
		case MESG30:
			strcpy(mail.topic,"Status Message");
			sprintf(gechrbuf,"%ld",mail.long1);
			prfmsg(mail.type,mail.name1,mail.int1,mail.int2,gechrbuf);
			sendit();
			break;

		case MESG20:
			strcpy(mail.topic,"Production Report");
			memcpy(&tmpstat,&mail,sizeof(MAILSTAT));
			prfmsg(tmpstat.type,tmpstat.name1,tmpstat.int1,tmpstat.int2);
			sprintf(gechrbuf2,"%lu",tmpstat.cash);
			prf("Cash %s  ",gechrbuf2);
			sprintf(gechrbuf2,"%lu",tmpstat.tax);
			prf("Tax %s  \r",gechrbuf2);
			for (i = 0; i < NUMITEMS; ++i) {
				setmem(gechrbuf,20,'.');
				gechrbuf[20-strlen(item_name[i])] = 0;
				sprintf(gechrbuf2,"%lu",tmpstat.itemqty[i]);
				prf("%s%s%14s\r",item_name[i],gechrbuf,gechrbuf2);
			}
			sendit();
			break;

		default:
			prfmsg(MAIL3,mail.type);
			sendit();
			break;
	}

	rstmbk();
}

int FUNC sendit(void)
{
	/* don't send mail to non-live players */

	if (mail.userid[0] == '*')
		return FALSE;

	setmem(gemsg,FIXEDMSGSIZ,0);

	gemsg->msgno = 0;

	strcpy(gemsg->userto,mail.userid);
	strcpy(gemsg->from,"** Galactic Empire **");
	strcpy(gemsg->to,mail.userid);
	strcpy(gemsg->topic,mail.topic);
	gemsg->auxtpc[0] = 0;

	gemsg->flags = mail.class;

	gemsg->crdate=today();
	gemsg->crtime=now();

	gemsg->nreply = cofdat(today());

	prf2tx();

	return sendgemsg(gemsg);

}

void FUNC prf2tx(void)		/* xfer prfbuf contents to message text area */
{
	char *cp;

	stpans(prfbuf);
	if (strlen(prfbuf) >= GEMSGSIZ) {
		prfbuf[GEMSGSIZ-1]=0;
	}
	for (cp = prfbuf; *cp != 0; cp++) {
		if (*cp == '\n') {
			*cp='\r';
		}
	}
	strcpy(gemsg->text,prfbuf);
	clrprf();
}


int FUNC sendgemsg(struct message *msgptr)
{
	setbtv(gebb4);
	if (!dinsbtv(msgptr))
		logthis(spr("GE:ERR:Mail Insert Fail to=%s topic=%s", msgptr->to, msgptr->topic));
	rstbtv();
	return TRUE;
}

/**************************************************************************
** Shield functions                                                      **
**************************************************************************/

static void shieldhitmsg(int shmsg, int usrn)
{
	if (shmsg == 1) {
		prfmsg(SHDAMAG);
		outprfge(FLT_NONE,usrn);
	}
	else if (shmsg == 2) {
		prfmsg(SHKNKDN);
		outprfge(FLT_NONE,usrn);
	}
}

void FUNC shieldup(WARSHP *wptr, int usrn)
{
	prfmsg(SHLDCHP);
	outprfge(FLT_NONE,usrn);
	wptr->shieldstat = SHIELDUP;
}


void FUNC shielddn(WARSHP *wptr, int usrn)
{
	prfmsg(SHLDDN);
	outprfge(FLT_NONE,usrn);
	wptr->shieldstat = SHIELDDN;
}


int FUNC shieldhit(WARSHP *wptr, int dam)   /* 0% to 100% damage */
{
	int knock;
	double dmax, ddam;

	dmax = (double)( 80 - ((int)wptr->shieldtype * SHIELD_FACTOR));

	if (dmax < 0)
		dmax = 0;

	ddam = dam;
	ddam /=100;	/* make it a percentile */

	knock = (int)(dmax * ddam);

	wptr->shield -= knock;
	if (wptr->shield <=2 ) {
		wptr->shieldstat = SHIELDDM;
		wptr->shield -= (knock*3);
		return 1;
	}
	else if (wptr->shield < SHMINCHG )
		return 2;
	return 0;
}


void FUNC shieldrep(WARSHP *wptr, int usrn)
{
	wptr->shield += (int)(wptr->shieldtype);

	if (wptr->shield > 0) {
		wptr->shieldstat = SHIELDDN;
		wptr->shield = 0;
		prfmsg(SHREPR);
		outprfge(FLT_NONE,usrn);
	}
}

void FUNC shieldchg(WARSHP *wptr, int usrn)
{
	/*STATIC*/
	static int maxcharge;
	static int pcnt;
	int		type;

	type = wptr->shieldtype;

	wptr->energy -= (type * SHENGUSE);

	charge(wptr,&maxcharge,&pcnt); /* go figure maxcharge and percent */

	if (wptr->shield < maxcharge) {
		wptr->shield += (type * 3);
		if (wptr->shield >= maxcharge) {
			wptr->shield = maxcharge;
			prfmsg(SHLDUP);
			outprfge(FLT_NONE,usrn);
		}
		else {
			charge(wptr,&maxcharge,&pcnt); /* go figure maxcharge and percent */
			prfmsg(SHLDAT,pcnt);
			outprfge(FLT_SHIP,usrn);
		}
	}
}

/* calculate the relative charge the shields are at */

void FUNC charge(WARSHP *wptr, int *max, int *pct)
{
	*max = 40 + (wptr->shieldtype*10);
	*pct = (wptr->shield*100)/(*max);
}

/**************************************************************************
** Cargo size functions                                                  **
**************************************************************************/

unsigned long FUNC cargo_weight100(WARSHP *wptr)
{
	int i;
	unsigned long total = 0;

	for (i = 0; i < NUMITEMS; ++i)
		total += wptr->items[i] * (unsigned long)weight[i];

	return total;
}

/* check if the goods to be added will cause weight to be exceeded */

int FUNC chkweight(WARSHP *wptr, int itm, long amt)
{
	unsigned long total;
	unsigned long add;

	total = cargo_weight100(wptr);
	add = (unsigned long)amt * (unsigned long)weight[itm];

	return (total + add) <= ((unsigned long)shipclass[wptr->shpclass].max_tons * 100UL)
		&& (wptr->items[itm] <= ULCAP - amt);
}

/* tell the total weight on board */

unsigned long FUNC calcweight(WARSHP *wptr)
{
	unsigned long total100;

	total100 = cargo_weight100(wptr);
	return (total100 + 99UL) / 100UL;
}


/**************************************************************************
** Figure the ship letter for this user                                  **
**************************************************************************/

char FUNC shpltr(int usrn, int ship)
{
	int i;
	SCANTAB *sptr;

	sptr = &scantab[usrn];

	for (i=0; i<NOSCANTAB; ++i)
		{
		if (sptr->ship[i].flag && sptr->ship[i].shipno == ship)
			return sptr->ship[i].letter;
		}

	/* not found, update scantab */
	update_scantab(warshpoff(usrn), usrn);

	/* try again */
	for (i=0; i<NOSCANTAB; ++i)
		{
		if (sptr->ship[i].flag && sptr->ship[i].shipno == ship)
			return sptr->ship[i].letter;
		}

	/* still not found, too many ships */
	return '?';
}

/**************************************************************************
** Return the proper name for this user given a pointer to the ship      **
**************************************************************************/

char * FUNC username(WARSHP *ptr)
{
	if (shipclass[ptr->shpclass].max_type == CLASSTYPE_CYBORG)  /* CYBORG */
		return ptr->shipname;
	if (shipclass[ptr->shpclass].max_type == CLASSTYPE_DROID)  /* DROID */
		return ptr->shipname;
	return ptr->userid;
}

/**************************************************************************
** Data logger                                                           **
**************************************************************************/

void FUNC logthis(char *str)
{
	FILE *hdl;
	int idate, itime;
	char *c_date, *c_time;

	if (!logflag)
		return;

	hdl = fopen("mpogeout.log","at+");

	if (hdl != (FILE *)0) {
		idate = today();
		itime = now();
		c_date = ncdate(idate);
		c_time = nctime(itime);

		fprintf(hdl,"[%s %s] %s\r",c_date,c_time,str);
		fclose(hdl);
	}

	return;
}

/**************************************************************************
** Return a pointer to one WARUSR slot                                   **
**************************************************************************/

WARUSR * FUNC warusroff(int usrn)
{
	if (usrn >= 0 && usrn < nships)
		return((WARUSR *)((long)(warusr_ecl + (usrn << 3)) << 16));
	else {
		geshocst(0,spr("GE:WARUSROFF:bad usrn [%d]",usrn));
		return((WARUSR *)((long)0));
	}
}

/**************************************************************************
** Return a pointer to one WARSHP slot                                   **
**************************************************************************/

WARSHP * FUNC warshpoff(int usrn)
{
	if (usrn >= 0 && usrn < nships)
		return((WARSHP *)((long)(warshp_ecl + (usrn << 3)) << 16));
	else {
		geshocst(0,"GE:BAD WARSHPOFF CALL");
		logthis(spr("WARSHPOFF:bad usrn [%d]",usrn));
		return((WARSHP *)((long)0));
	}
}

/**************************************************************************
** Convert raw damage into ship-relative damage based on damage factor   **
**************************************************************************/

double FUNC ton_fact(WARSHP *ptr, double damfact)
{
	double temp;

	temp = damfact / ((double)(shipclass[ptr->shpclass].damfact) / 100.0);
	if (ptr->upgrade & ARMOR)
		temp *= 0.625;

	return temp;
}

/**************************************************************************
** Return a compact upgrade marker string                                **
**************************************************************************/

char * FUNC showupg(WARSHP *ptr)
{
	byte upg;
	int count;

	if (ptr->upgrade == 0)
		return "";

	upg = ptr->upgrade;
	count = 0;
	while (upg != 0) {
		if (upg & 1)
			++count;
		upg >>= 1;
	}

	if (count > 4)
		return "+++";
	else if (count > 2)
		return "++";
	else if (count > 0)
		return "+";

	return "";
}

/**************************************************************************
** Format one displayed warp-speed string                                **
**************************************************************************/

char * FUNC showarp(double speed)
{
	if (speed == 0.0)
		sprintf(warpbuf,"0.00");
	else
#ifdef MBBSEMU
	if (fabs(speed - (long)(speed / FARSPEED) * FARSPEED) < 1e-6)
#else
	if (fmod(speed, FARSPEED) == 0.0)
#endif
		sprintf(warpbuf,"??.??");
	else
		sprintf(warpbuf,"%.2f",speed/1000.0);
#ifdef MBBSEMU
	/* MBBSemu doesn't currently honor %.2f */
	if (warpbuf[strlen(warpbuf) - 2] == '.')
		strcat(warpbuf, "0");
	else
	if (warpbuf[strlen(warpbuf) - 3] != '.')
		strcat(warpbuf, ".00");
#endif
	return warpbuf;
}

/**************************************************************************
** Set faction dislike status                                            **
**************************************************************************/

void FUNC set_dislike(WARUSR *wuptr, int facnum, int dislike)
{
	if (facnum < 0 || facnum > 7) {
		geshocst(0,spr("GE:set_dislike:bad facnum [%d]",facnum));
		return;
	}

	if ((unsigned int)(wuptr->factions[facnum]) + dislike > 255)
		wuptr->factions[facnum] = 255;
	else
		wuptr->factions[facnum] += dislike;
}

/**************************************************************************
** Find loser and winner roster positions                                **
**************************************************************************/

void FUNC rospos(WARUSR *losptr, WARUSR *winptr, int *lospos, int *winpos)
{
	long ltarget = 0, wtarget = 0;
	int i = 0;
	int ranked;

	*lospos = 0;
	*winpos = 0;

	setbtv(gebb5);

	/* find user record for loser */
	if (qeqbtv(losptr->userid, 0))
		ltarget = absbtv();

	/* find user record for winner */
	if (qeqbtv(winptr->userid, 0))
		wtarget = absbtv();

	if (ltarget == 0 && wtarget == 0)
		return;	/* return both 0 */

	/* start at top of roster (key 1 = score order) */
	if (!qhibtv(1))
		return;

	do {
		gcrbtv(&tmpusr, 1);

		ranked = (tmpusr.score > 0 && tmpusr.userid[0] != '@');

		if (ranked)
			++i;

		if (ranked && ltarget != 0 && absbtv() == ltarget)
			*lospos = i;
		else if (ranked && wtarget != 0 && absbtv() == wtarget)
			*winpos = i;

		if (*lospos && *winpos)
			break;	/* we're done here */

	} while (qprbtv());

	if (*winpos == 0)	/* unranked winner */
		*winpos = i + 1;

}

/**************************************************************************
** Convert a raw damage number to descriptive text                       **
**************************************************************************/

void FUNC damstr(int damage)
{
	if (damage == 0)
		strcpy(gechrbuf,"no");
	else if (damage < 2)
		strcpy(gechrbuf,"negligible");
	else if (damage < 12)
		strcpy(gechrbuf,"very light");
	else if (damage < 25)
		strcpy(gechrbuf,"light");
	else if (damage < 50)
		strcpy(gechrbuf,"moderate");
	else if (damage < 75)
		strcpy(gechrbuf,"heavy");
	else
		strcpy(gechrbuf,"severe");
}

/**************************************************************************
** Refresh the local scantab view for one user                           **
**************************************************************************/

void FUNC update_scantab(WARSHP *ptr, int usrn)
{
	int i, j;
	char l;
	byte ptr_neb, oth_neb, flag;
	WARSHP *wptr;
	SCANTAB tmp;
	char lettab[300];

	setmem(&lettab[0],sizeof(char)*300,0);

	ptr_neb = (byte)innebula(coord1(ptr->coord.xcoord),coord1(ptr->coord.ycoord));

	/* save off the old letters */
	for (i = 0; i < NOSCANTAB; ++i) {
		j = scantab[usrn].ship[i].shipno;
		if (j >= 0 && j < 300) {
			/* only keep A through Z */
			l = scantab[usrn].ship[i].letter;
			lettab[j] = (l >= 'A' && l <= 'Z') ? l : 0;
		}
	}

	/* clear the table */
	for (i = 0; i < NOSCANTAB; ++i)
		tmp.ship[i].flag = 0;

	for (othusn = 0; othusn < nships; othusn++) {
		if (othusn != usrn && ingegame(othusn)) {

			wptr = warshpoff(othusn);
			ddistance = cdistance(&ptr->coord,&wptr->coord) * 10000;
			oth_neb = (byte)innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
			flag = 0;
			if (ddistance < ship_scanrange(ptr)) {
				if ((ptr_neb || oth_neb) && !(ptr_neb && oth_neb && ddistance < (double)NEBRNG)) {
					if (othusn == ptr->lock && ptr->track_grace > 0)
						flag = 2;
				}
				else if (wptr->cloak >= 10) {
					if (othusn == ptr->lock && ptr->track_grace > 0)
						flag = 2;
				}
				else
					flag = 1;
			}

			if (flag != 0) {
				for (i = 0; i < NOSCANTAB; ++i) {
					/* entry blank - fill it with this one */
					if (tmp.ship[i].flag == 0) {
						tmp.ship[i].flag = flag;
						tmp.ship[i].shipno = othusn;
						tmp.ship[i].dist = ddistance;
						tmp.ship[i].letter = lettab[othusn];

						break;
					}
					/* is this ship closer */
					if (ddistance < tmp.ship[i].dist) {
						/* Yes - Push down the rest */
						for (j = NOSCANTAB - 2; j >= i; j--) {
							if (tmp.ship[j].flag != 0)
								memcpy(&tmp.ship[j+1],&tmp.ship[j],sizeof(SHIPTAB));
						}
						/* fill the hole with the new ship */
						tmp.ship[i].flag = flag;
						tmp.ship[i].shipno = othusn;
						tmp.ship[i].dist = ddistance;
						tmp.ship[i].letter = lettab[othusn];
						break;
					}
				}
			}
		}
	}

	/* go fill in the new ships without letters */
	pick_letter(&tmp);

	/* Now go fill in the rest of the table */
	for (i = 0; i < NOSCANTAB; ++i) {
		/* clear out the empty entries */
		if (tmp.ship[i].flag == 0) {
			tmp.ship[i].letter = '?';
			tmp.ship[i].shipno = -1;
		}
		else {
			j = tmp.ship[i].shipno;
			wptr = warshpoff(j);
			tmp.ship[i].bearing = cbearing(&ptr->coord,&(wptr->coord),ptr->heading);
			tmp.ship[i].heading = cbearing(&(wptr->coord),&ptr->coord,wptr->heading);
			tmp.ship[i].speed = wptr->speed;
		}
	}

	/* update the current users master record */
	memcpy(&scantab[usrn],&tmp,sizeof(SCANTAB));

}

/**************************************************************************
** Pick the next available scan-table letters                            **
**************************************************************************/

static void pick_letter(SCANTAB *ptr)
{
#define LETSIZE 26
	char letters[LETSIZE] = {'A','B','C','D','E','F','G','H','I','J','K','L','M',
							 'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
	char lettmp[LETSIZE];
	int i, j;

	/* init the temp tab */
	memcpy(lettmp,letters,LETSIZE);

	/* look at each ship in the table and punch out the letter from the
	   list... when all done the letters remaining are available */

	for (i = 0; i < NOSCANTAB; ++i) {
		if (ptr->ship[i].flag != 0 && ptr->ship[i].letter != 0) {
			for (j = 0; j < LETSIZE; ++j) {
				if (ptr->ship[i].letter == lettmp[j]) {
					lettmp[j] = '@';
					break;
				}
			}
		}
	}

	/* now go through and fix up the ? */
	for (i = 0; i < NOSCANTAB; ++i) {
		if (ptr->ship[i].flag != 0 && ptr->ship[i].letter == 0) {
			for (j = 0; j < LETSIZE; ++j) {
				if (lettmp[j] != '@') {
					ptr->ship[i].letter = lettmp[j];
					lettmp[j] = '@';
					break;
				}
			}
		}
	}

	return;
}
