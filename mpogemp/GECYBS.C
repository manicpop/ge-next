
/***************************************************************************
 *                                                                         *
 *   GECYBS.C                                                              *
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


#define GECYBS 1

#include "geglobal.h"

/* LOCAL GLOBAL DEFS ****************************************************/

static char cybname[UIDSIZ];
int cybhaltflg = 0;
double d_topspeed;

/**************************************************************************
** Validate a saved cyborg ship record for the current cyb slot          **
**************************************************************************/

static int valid_cyb_ship(WARSHP *ptr, int usrn, int class)
{
	if (strncmp(ptr->userid, cybname, UIDSIZ) != 0)
		return FALSE;

	if (ptr->shipno <= 0)
		return FALSE;

	if (!VALID_SHPCLASS(ptr->shpclass))
		return FALSE;

	if (shipclass[ptr->shpclass].max_type != CLASSTYPE_CYBORG)
		return FALSE;

	return ptr->shpclass == class && cyb_user_slot(ptr->userid) == usrn;
}

/**************************************************************************
** Load one valid cyb ship and delete invalid or duplicate records       **
**************************************************************************/

static int load_cyb_ship(WARSHP *ptr, int usrn, int class)
{
	int deleted;
	int have_ship;
	int keep_shipno;

	have_ship = FALSE;
	keep_shipno = -1;

	do {
		deleted = FALSE;
		setbtv(gebb1);
		if (qeqbtv(cybname, 0)) {
			do {
				gcrbtv(&tmpshp, 0);
				if (strncmp(tmpshp.userid, cybname, UIDSIZ) != 0)
					break;

				if (valid_cyb_ship(&tmpshp, usrn, class)) {
					if (!have_ship) {
						memcpy(ptr, &tmpshp, sizeof(WARSHP));
						keep_shipno = tmpshp.shipno;
						have_ship = TRUE;
						logthis(spr("GE:INF:Load %s ship %d",
							ptr->userid, ptr->shipno));
					} else if (tmpshp.shipno != keep_shipno) {
						geshocst(1, spr("GE:INF:CYBDUPSHP uid=%s shipno=%d keep=%d",
							tmpshp.userid, tmpshp.shipno, keep_shipno));
						if (!gepdb(GEDELETE, tmpshp.userid, tmpshp.shipno, &tmpshp)) {
							geshocst(0, spr("GE:ERR:CYBDELSHP uid=%s shipno=%d",
								tmpshp.userid, tmpshp.shipno));
						} else {
							deleted = TRUE;
							break;
						}
					}
				} else {
					if (VALID_SHPCLASS(tmpshp.shpclass) &&
						shipclass[tmpshp.shpclass].max_type == CLASSTYPE_CYBORG &&
						cyb_user_slot(tmpshp.userid) == usrn) {
						geshocst(1, spr("GE:INF:CYBSHPRECLS usn=%d old=%d new=%d shipno=%d uid=%s",
							usrn, tmpshp.shpclass, class, tmpshp.shipno, tmpshp.userid));
					} else {
						geshocst(0, spr("GE:ERR:BADCYBSHP usn=%d cls=%d shipno=%d uid=%s",
							usrn, tmpshp.shpclass, tmpshp.shipno, tmpshp.userid));
					}
					if (!gepdb(GEDELETE, tmpshp.userid, tmpshp.shipno, &tmpshp)) {
						geshocst(0, spr("GE:ERR:CYBDELSHP uid=%s shipno=%d",
							tmpshp.userid, tmpshp.shipno));
					} else {
						deleted = TRUE;
						break;
					}
				}
			} while (qnxbtv());
		}
	} while (deleted);

	return have_ship;
}

/**************************************************************************
** Initialize or load a cyborg ship                                      **
**************************************************************************/

void FUNC cyb_init(WARSHP *ptr, int usrn, int class)
{
	WARSHP *wptr;
	int i, goldwin, goldspin, goldtry, zothusn;
	double ddist;
	int have_ship = FALSE;
	int expected_class;

	logthis(spr("@Cyb_init usrn=%d,class=%d", usrn, class));

	if (usrn < 0 || usrn >= nships) {
		logthis(spr("CYB_INIT:bad usrn [%d]",usrn));
		return;
	}

	expected_class = cyb_slot_class(usrn);
	if (expected_class != class) {
		geshocst(0, spr("GE:ERR:CYBSLOTCLS usn=%d cls=%d exp=%d",
			usrn, class, expected_class));
		return;
	}

	strncpy(cybname, "@Cybrg-", UIDSIZ);
	sprintf(&cybname[7], "%d", usrn);

	if (!geudb(GELOOKUP, cybname, &tmpusr)) {
		initusr(cybname);
		geudb(GEADD, tmpusr.userid, &tmpusr);
		logthis(spr("GE:INF:Adding %s user", tmpusr.userid));
	}

	waruptr = warusroff(usrn);

	if (geudb(GELOOKUP, cybname, waruptr)) {
		if (!geudb(GEGET, cybname, waruptr)) {
			geshocst(0, spr("GE:ERR:CYBGETUSR usrn=%d uid=%s",
				usrn, cybname));
			return;
		}

		logthis(spr("GE:INF:Load %s user", waruptr->userid));

		if (load_cyb_ship(ptr, usrn, class)) {
			ptr->status = GESTAT_AUTO;
			ptr->shield = 40 + (ptr->shieldtype * 10);
			ptr->phasr = 100;
			ptr->track_grace = 0;
			ptr->npcstate = 255;
			ptr->npcmsg = (byte)255;
			ptr->holdcourse = 0;
			ptr->cantexit = 0;
			npc_cruise(ptr, usrn, 0);
			ptr->cybupdate = 100 + gernd() % 20;
			ptr->tick = CYBTICKTIME + gernd() % (CYBTICKTIME * 5);
			have_ship = TRUE;
		}

		if (!have_ship) {
			/* make me a Cybertron */
			logthis(spr("GE:INF:Adding %s ship - %d", cybname, class));

			initshp(cybname, class);
			if (!gepdb(GEADD, tmpshp.userid, tmpshp.shipno, &tmpshp))
				geshocst(0, spr("GE:ERR:CYBADDSHP uid=%s shipno=%d",
					tmpshp.userid, tmpshp.shipno));
			memcpy(ptr, &tmpshp, sizeof(WARSHP));	/* make is the current ship */

			logthis(spr("GE:INF:Add shp,cls=%d/%d", class, ptr->shpclass));
			strncpy(ptr->shipname, shipclass[class].npcprefx, sizeof(ptr->shipname) - 1);
			ptr->shipname[sizeof(ptr->shipname) - 1] = 0;
			sprintf(gechrbuf, "%u", usrn * usrn + gernd() % (2 * usrn + 1) + 1000);
			strncat(ptr->shipname, gechrbuf,
				sizeof(ptr->shipname) - strlen(ptr->shipname) - 1);
			logthis(spr("  Named: %s", ptr->shipname));

			waruptr->kills = 0;	/* new cyb so clear this */

			if (shipclass[ptr->shpclass].max_accel == 0 && univmax > 100) {
				/* make sure bases aren't too close to 0 0 */
				ptr->coord.xcoord = rndm((double)univmax - 60) + 50.0;
				if (gernd() % 2 == 0)
					ptr->coord.xcoord *= -1.0;
				ptr->coord.ycoord = rndm((double)univmax - 60) + 50.0;
				if (gernd() % 2 == 0)
					ptr->coord.ycoord *= -1.0;
			} else {
				ptr->coord.xcoord = rndm((double)univmax * 2.0) - (double)univmax;
				ptr->coord.ycoord = rndm((double)univmax * 2.0) - (double)univmax;
			}

			/* phaser and shields between 50 and 100% of max */
			if (shipclass[class].max_phasr > 0)
				ptr->phasrtype = (shipclass[class].max_phasr / 2) +
					(gernd() % (shipclass[class].max_phasr / 2 + 1));
			else
				ptr->phasrtype = 0;

			if (shipclass[class].max_shlds > 0)
				ptr->shieldtype = (shipclass[class].max_shlds / 2) +
					(gernd() % (shipclass[class].max_shlds / 2 + 1));
			else
				ptr->shieldtype = 0;

			ptr->cybmine = (byte)255;
			ptr->track_grace = 0;
			ptr->distress = (byte)255;
			ptr->npcstate = 255;
			ptr->npcmsg = (byte)255;
			ptr->holdcourse = 0;
			ptr->cantexit = 0;
			ptr->shield = 40 + (ptr->shieldtype * 10);
			ptr->phasr = 100;

			ptr->items[I_FLUXPOD] = (gernd() % 20) + 10;
			if (shipclass[ptr->shpclass].has_decoy)
				ptr->items[I_DECOYS] = (gernd() % 20) + 10;
			if (shipclass[ptr->shpclass].max_missl)
				ptr->items[I_MISSILE] = (gernd() % 10) + 20;
			if (shipclass[ptr->shpclass].max_torps)
				ptr->items[I_TORPEDO] = (gernd() % 20) + 20;
			if (shipclass[ptr->shpclass].has_mine)
				ptr->items[I_MINE] = (gernd() % 40) + 10;
			if (shipclass[ptr->shpclass].has_jam)
				ptr->items[I_JAMMERS] = (gernd() % 20) + 10;
			if (shipclass[ptr->shpclass].has_zip)
				ptr->items[I_ZIPPERS] = (gernd() % 5) + 5;

			/* favor higher gold amounts for tougher cybertrons */
			/* level 2 gets one spin of the wheel, other levels get mulitple */
			/* higher levels get the best outcome, lowest the worst */

			if (cyb_gold > 0) {
				goldwin = gernd() % cyb_gold;
				goldtry = abs(shipclass[ptr->shpclass].tough_factor - 2);

				for (i = 0; i < goldtry; i++) {
					goldspin = gernd() % cyb_gold;
					if (shipclass[ptr->shpclass].tough_factor < 2 && goldspin < goldwin)
						goldwin = goldspin;
					if (shipclass[ptr->shpclass].tough_factor > 2 && goldspin > goldwin)
						goldwin = goldspin;
				}
				ptr->items[I_GOLD] = goldwin;
			}

			ptr->status = GESTAT_AUTO;
			ptr->tick = CYBTICKTIME + gernd() % (CYBTICKTIME * 5);

			ptr->cybupdate = 1;

			if (!gepdb(GEUPDATE, ptr->userid, ptr->shipno, ptr))
				geshocst(0, spr("GE:ERR:CYBUPDSHP uid=%s shipno=%d",
					ptr->userid, ptr->shipno));

			/* show users sector of new Cyb if in scan range */
			/* show bearing if far away */
			/* thanks Dave Walton for the idea */
			for (zothusn = 0; zothusn < nterms; zothusn++) {
				wptr = warshpoff(zothusn);
				if (ingegame(zothusn) && wptr->jam_sev <= (byte)2) {
					ddist = cdistance(&ptr->coord, &wptr->coord);
					ddist *= 10000;
					if (ddist > shipclass[wptr->shpclass].scanrange ||
						innebula(coord1(wptr->coord.xcoord), coord1(wptr->coord.ycoord))) {
						bearing = cbearing(&wptr->coord, &ptr->coord, wptr->heading);
						prfmsg(CYBNEW, bearing);
						outprfge(FLT_SHIP, zothusn);
					} else {
						setsect(ptr);
						prfmsg(CYBNEW2, xsect, ysect);
						outprfge(FLT_SHIP, zothusn);
					}
				}
			}
		}
	} else {
		/* DEBUG */
		geshocst(0, spr("GE:ERR:NO FIND %s user", cybname));
	}
}

/**************************************************************************
** Assign closest cybs to ship entering game or going through wormhole   **
**************************************************************************/

void FUNC assign_cybs(int usrnum, int call)
{
	WARSHP *wptr;
	WARSHP *ptr;
	int zothusn;
	double ddist;
	double low_dist = 999999999.0;
	int low_ship;
	int lta; /* lowest to attack */
	int i, cybpick, claims, noclaim, tpmag;

	claims = 0;

	/* call 0 = clear all current cyb pursuits */
	/* call 1 = count all current cyb pursuits */
	for (zothusn = nterms; zothusn < nships; ++zothusn) {
		ptr = warshpoff(zothusn);
		if (ptr->status == GESTAT_AUTO &&
			shipclass[ptr->shpclass].max_type == CLASSTYPE_CYBORG &&
			ptr->cybmine == usrnum) {
			if (call == 0)
				ptr->cybmine = 255;
			else
				++claims;
		}
	}

	wptr = warshpoff(usrnum);
	/* if we're not clearing, only claim enough to fill claims */
	noclaim = shipclass[wptr->shpclass].noclaim;
	if (wptr->upgrade & TPONDER) {
		if (noclaim <= 2)
			tpmag = 1;
		else
			tpmag = 2;
		if (wptr->tponder == TPONHIGH)
			noclaim += tpmag;
		else if (wptr->tponder == TPONLOW)
			noclaim -= tpmag;
		if (noclaim < 0)
			noclaim = 0;
		if (noclaim > 5)
			noclaim = 5;
	}
	cybpick = noclaim - claims;

	for (i = 0; i < cybpick; ++i) {
		low_dist = 999999999.0;
		low_ship = -1;

		for (zothusn = nterms; zothusn < nships; ++zothusn) {
			ptr = warshpoff(zothusn);
			lta = shipclass[ptr->shpclass].lowest_to_attk - 1;
			if (ingegame(zothusn) &&
				ptr->status == GESTAT_AUTO &&
				shipclass[ptr->shpclass].max_accel > 0 &&
				shipclass[ptr->shpclass].max_type == CLASSTYPE_CYBORG &&
				lta <= wptr->shpclass &&
				ptr->cybmine == 255) {
				ddist = cdistance(&ptr->coord, &wptr->coord);
				if (ddist < low_dist) {
					low_dist = ddist;
					low_ship = zothusn;
				}
			}
		}
		if (low_ship == -1)
			return;

		ptr = warshpoff(low_ship);
		ptr->cybmine = usrnum;
		ptr->track_grace = CYBGRACE;
	}
}

/**************************************************************************
** Check whether a cyb is in fast pursuit                                **
**************************************************************************/

int FUNC cyb_fast(WARSHP *ptr)
{
	return ptr->speed != 0.0 &&
#ifdef MBBSEMU
		(fabs(ptr->speed - (long)(ptr->speed / FARSPEED) * FARSPEED) < 1e-6);
#else
		(fmod(ptr->speed, FARSPEED) == 0.0);
#endif
}

/**************************************************************************
** Check whether a ship still has room for more cyb claims               **
**************************************************************************/

static int notclaimed(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn, nc, noclaim, tpmag;

	nc = 0;
	for (zothusn = nterms; zothusn < nships; zothusn++) {
		wptr = warshpoff(zothusn);
		if (wptr->status == GESTAT_AUTO &&
			shipclass[wptr->shpclass].max_type == CLASSTYPE_CYBORG &&
			wptr->cybmine == (byte)usrn)
			++nc;
	}

	noclaim = shipclass[ptr->shpclass].noclaim;
	if (ptr->upgrade & TPONDER) {
		if (noclaim <= 2)
			tpmag = 1;
		else
			tpmag = 2;
		if (ptr->tponder == TPONHIGH)
			noclaim += tpmag;
		else
		if (ptr->tponder == TPONLOW)
			noclaim -= tpmag;
		if (noclaim < 0)
			noclaim = 0;
		if (noclaim > 5)
			noclaim = 5;
	}

	logthis(spr("notclaimed: nc = %d, class = %d, class.noclaim = %d",
		nc, ptr->shpclass, noclaim));
	return nc < noclaim;
}

/**************************************************************************
** Emit one cyb message to a user                                         **
**************************************************************************/

static void cyb_msg(WARSHP *ptr, int usrn, int msgtype)
{
	int base, sel;

	base = CYBBASEM + (msgtype * 4);

	sel = base + (gernd() % 4) + 1;

	if (sel < CYBLASTM) {
		prfmsg(sel, ptr->shipname);
		if (msgtype == APPROACH)
			outprfge(FLT_CYB_APP, usrn);
		else if (msgtype == LOATTACK || msgtype == HIATTACK ||
			msgtype == CYBTORP || msgtype == CYBBASEB || msgtype == FLEE)
			outprfge(FLT_CYB_BAT, usrn);
		else
			outprfge(FLT_CYB_ALL, usrn);
	}
}

/**************************************************************************
** Decide whether a cyb message should be shown                          **
**************************************************************************/

static void cyb_annoy(WARSHP *ptr, int usrn, int msgtype)
{
	/* skip NPCs entirely */
	if (usrn >= nterms)
		return;

	/* if already blowed up, don't msg */
	if (ptr->damage >= 100.0)
		return;

	/* if out of user's range, don't msg */
	if (cdistance(&ptr->coord, &warshpoff(usrn)->coord) * 10000 >
		(double)shipclass[warshpoff(usrn)->shpclass].scanrange)
		return;

	/* if we are fleeing from this user, don't send other msgs to this user until done */
	/* allow an explicit flee message to print when transitioning from silent missile avoidance */
	if (usrn == ptr->cybmine && ptr->npcmsg == FLEE &&
		(msgtype != FLEE || ptr->holdcourse > 0)) {
		return;
	}

	/* if base is targeting user, don't show approach msg */
	if (usrn == ptr->cybmine && msgtype == CYBBASEA)
		return;

	/* these messages can be called on users that aren't being targeted */
	/* bypass logic, use simple random, and don't change npcmsg */
	if (usrn != ptr->cybmine) {
		if (((msgtype == LOATTACK || msgtype == HIATTACK || msgtype == TAUNT) &&
			gernd() % 12 == 0) ||
			((msgtype == CYBBASEA || msgtype == CYBTORP) &&
			gernd() % 7 == 0))
			cyb_msg(ptr, usrn, msgtype);
		return;
	}

	/* add in and increase likelihood of base battle messages */
	if ((msgtype == LOATTACK || msgtype == HIATTACK || msgtype == CYBTORP) &&
		shipclass[ptr->shpclass].max_accel == 0 && gernd() % 2 == 0)
		msgtype = CYBBASEB;

	/* let some of these through on occasion */
	if ((msgtype == NEUTRAL || msgtype == IGNORE) &&
		gernd() % (16 + (shipclass[ptr->shpclass].tough_factor * 2)) == 0)
		ptr->npcmsg = 255;	/* adjust for tougher ships going faster */

	/* allow a real flee message after silent missile avoidance */
	if (msgtype == FLEE && ptr->npcmsg == FLEE && ptr->holdcourse == 0)
		ptr->npcmsg = 255;

	/* otherwise don't do the same message twice in a row */
	if (ptr->npcmsg == msgtype)
		return;

	/* throttle impulse battle msgs */
	if ((ptr->npcmsg == TAUNT || ptr->npcmsg == CYBTORP ||
		ptr->npcmsg == LOATTACK) &&
		(msgtype == TAUNT || msgtype == CYBTORP || msgtype == LOATTACK))
		if (gernd() % (6 + (shipclass[ptr->shpclass].tough_factor)) != 0) {
			ptr->npcmsg = msgtype;
			return;
		}

	/* if you're fleeing, be quiet after flee message */
	if (ptr->holdcourse > 0)
		return;

	/* remember which message type was called last (even if it doesn't necessarily get displayed) */
	ptr->npcmsg = msgtype;

	/* show some messages always, the rest sometimes */
	if (msgtype == FLEE || msgtype == APPROACH || gernd() % 3 == 0)
		cyb_msg(ptr, usrn, msgtype);
}
/**************************************************************************
** Count down and perform cyb database updates                           **
**************************************************************************/

static void db_update(WARSHP *ptr, int usrn)
{
	WARUSR *wuptr;

	if (ptr->cybupdate > 1) {
		--ptr->cybupdate;
		return;
	}
	/* if cruising around and about to update, change speed/direction */
	if (ptr->cybupdate == 1 &&
		(ptr->cybmine >= nships ||
		(warshpoff(ptr->cybmine)->status == GESTAT_AUTO && ptr->cantexit == 0))) {
		npc_cruise(ptr, usrn, 0);	/* keep cyb from endlessly chasing npcs it can't catch */
		--ptr->cybupdate;
		return;
	}
	if (ptr->cybupdate == 0) {
		wuptr = warusroff(usrn);
		logthis(spr("GE:DBG:Cyb UUpd %s", wuptr->userid));
		if (!geudb(GEUPDATE, wuptr->userid, wuptr))
			geshocst(0, spr("GE:ERR:CYBUPDUSR uid=%s", wuptr->userid));
		logthis(spr("GE:DBG:Cyb PUpd %s", ptr->userid));
		if (!gepdb(GEUPDATE, ptr->userid, ptr->shipno, ptr))
			geshocst(0, spr("GE:ERR:CYBUPDSHP uid=%s shipno=%d",
				ptr->userid, ptr->shipno));
		ptr->cybupdate = 100 + gernd() % 100;
		return;
	}
	ptr->cybupdate = 20; /* if engaged with another ship, update later */
}

/**************************************************************************
** Attack the other player                                               **
**************************************************************************/

static void cyb_attack(WARSHP *ptr, int usrn, WARSHP *wptr, int zothusn)
{
	int i, j, acted;
	int zipden, mden, tden;

	acted = 0;

	if (neutral(&ptr->coord))
		return;
	if (neutral(&wptr->coord))
		return;

	if (shipclass[ptr->shpclass].max_phasr > 0 && ptr->phasr >= PMINFIRE &&
		gernd() % (4 - (shipclass[ptr->shpclass].tough_factor / 2)) == 0) {
		ptr->degrees = cbearing(&ptr->coord, &wptr->coord, ptr->heading);
		ptr->percent = 2;
		firep(ptr, usrn);
		acted = 1;
	}

	/* fire torpedoes or missiles at the fool */
	mden = 12 - (shipclass[ptr->shpclass].tough_factor * 2);
	if (mden < 3)
		mden = 3;
	for (i = 0; i < shipclass[ptr->shpclass].max_missl; ++i) {
		if (ptr->items[I_MISSILE] > 0 && gernd() % mden == 0 &&
			misl(ptr, usrn, zothusn,
			(shipclass[ptr->shpclass].tough_factor + 1) * 4000, 0) == 1)
			acted = 1;
	}
	tden = 5 - shipclass[ptr->shpclass].tough_factor;
	if (tden < 1)
		tden = 1;
	for (j = 0; j < shipclass[ptr->shpclass].max_torps; ++j) {
		if (ptr->items[I_TORPEDO] > 0 && gernd() % tden == 0 &&
			torp(ptr, usrn, zothusn) == 1)
			acted = 2;
	}

	/* launch Zippers if needed */
	zipden = 12 - (shipclass[ptr->shpclass].tough_factor * 2);
	if (zipden < 3)
		zipden = 3;
	if (gernd() % zipden == 0 && shipclass[ptr->shpclass].has_zip &&
		ptr->items[I_ZIPPERS] > 0 && ptr->zipload == 0 &&
		shipclass[ptr->shpclass].max_accel > 0 && wptr->minesnear == TRUE) {
		zip(ptr);
		acted = 1;
		/* get the hell out of here ...then come back */
		npc_cruise(ptr, usrn, 3);
	}

	/* if damage in flee range, don't talk trash */
	if (ptr->damage <= CYB_MINDAM) {
		if (acted == 1)
			cyb_annoy(ptr, zothusn, LOATTACK);
		else if (acted == 2)
			cyb_annoy(ptr, zothusn, CYBTORP);
		else
			cyb_annoy(ptr, zothusn, TAUNT);
	}
}

/**************************************************************************
** if hunting, and badly damaged dump mines, jam, and boogie             **
**************************************************************************/

static void cyb_check_damage(WARSHP *ptr, int usrn)
{
	if (ptr->damage > CYB_MINDAM &&
		((gernd() % 10 == 0) || ptr->holdcourse > 0)) {
		if (shipclass[ptr->shpclass].has_mine
			&& ptr->items[I_MINE] > 0
			&& ptr->mineload == 0
			&& !neutral(&ptr->coord)
			&& gernd() % 8 == 0)
			laymine(ptr, usrn, 10);

		if (shipclass[ptr->shpclass].has_jam
			&& ptr->items[I_JAMMERS] > 0
			&& ptr->jamload == 0
			&& gernd() % 40 == 0)
			jam(ptr, usrn);
		if (ptr->holdcourse == 1) {
			ptr->cybmine = 255;
			ptr->npcstate = 255;
			ptr->npcmsg = 255;
		}
		if (ptr->holdcourse == 0) {
			cyb_annoy(ptr, ptr->cybmine, FLEE);
			ptr->head2b = normal(vector(&ptr->coord, &warshpoff(ptr->cybmine)->coord) +
				180.0 + (rand() % 51 - 25));
			npc_cruise(ptr, usrn, 3);
		}
	}
}

/**************************************************************************
** Don't pick new fights with NPCs if no users are playing               **
** Allow msg-configurable frequency of cyb-on-droid attacks              **
**************************************************************************/

static int cyb_pick_fight(int usrn, int call)
{
	int zothusn, usersin, nc;
	WARSHP *wptr;

	usersin = FALSE;
	nc = 0;

	/* users always */
	if (usrn < nterms)
		return TRUE;

	/* cyb vs droid can be turned off */
	if (cattkd <= 0)
		return FALSE;

	/* is anyone actually playing */
	for (zothusn = 0; zothusn < nterms; zothusn++)
		if (ingegame(zothusn) && warshpoff(zothusn)->status == GESTAT_USER) {
			usersin = TRUE;
			break;
		}

	/* don't attack npc if no users around to see it */
	if (usersin == FALSE)
		return FALSE;

	/* you want mayhem? you asked for it */
	if (cattkd >= 10)
		return TRUE;

	/* limit total amount of cybs pursuing droids to cattkd */
	for (zothusn = nterms; zothusn < nships; zothusn++) {
		wptr = warshpoff(zothusn);
		if (wptr->status == GESTAT_AUTO &&
			shipclass[wptr->shpclass].max_type == CLASSTYPE_CYBORG &&
			wptr->cybmine >= nterms && wptr->cybmine < nships)
			++nc;
	}
	if (nc >= cattkd)
		return FALSE;

	/* random encounter */
	if (call == 0)
		return TRUE;

	/* picks on a 600x scale of 600 to 5400 */
	if (call == 1 && gernd() % ((10 - cattkd) * 600) == 0)
		return TRUE;

	return FALSE;
}

/**************************************************************************
** React to incoming projectiles                                         **
**************************************************************************/
static void cyb_check_proj(WARSHP *ptr, int usrn)
{
	MISSILE *mptr;
	TORPEDO *tptr;
	WARSHP *wptr;
	int i;

	if (ptr->npcmsg == FLEE && ptr->holdcourse > 0)
		return;

	if (ptr->where == 0 && ptr->topspeed > 0 && ptr->cybmine < nships &&
		ingegame(ptr->cybmine)) {
		wptr = warshpoff(ptr->cybmine);
		if (wptr->where == 1) {
			for (i = 0, tptr = ptr->ltorps; i < MAXTORPS; ++i, ++tptr) {
				if (tptr->distance > 0 && tptr->channel == ptr->cybmine) {
					ptr->head2b = vector(&ptr->coord, &wptr->coord);
					npc_cruise(ptr, usrn, 2);
					return;
				}
			}
		}
	}

	for (i = 0, mptr = ptr->lmissl; i < MAXMISSL; ++i, ++mptr) {
		if (ptr->where == 1 && mptr->distance > 20000 && ptr->holdcourse == 0 &&
			mptr->channel < nships && ingegame(mptr->channel)) {
			ptr->npcmsg = FLEE;	/* don't send APPROACH again after returning from this */
			if (cdistance(&ptr->coord, &warshpoff(mptr->channel)->coord) > 2.0)
				ptr->head2b = vector(&ptr->coord, &warshpoff(mptr->channel)->coord);
			else
				ptr->head2b = vector(&ptr->coord, &warshpoff(mptr->channel)->coord) +
					45.0 + (rand() % 11 - 5);
			ptr->speed2b = (double)ptr->topspeed * 1000;
			ptr->holdcourse = 3;
			break;
		} else if (mptr->distance < 5000 && mptr->distance > 0 &&
			ptr->shieldstat != SHIELDDM) {
			ptr->speed2b = 990;
			ptr->holdcourse = 1;
			break;
		}
	}
}

/**************************************************************************
** Check lockon status                                                   **
**************************************************************************/

static void cyb_check_lockon(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn, inbound, keepwarp, attackwarp, closemove, phatwarp;
	double ddist;
	double low_dist = 999999999.0;
	int low_ship;
	int lta; /* lowest to attack */

	low_ship = -1;

	/* if cyborg not seeking - countdown */
	zothusn = ptr->cybmine;

	if (ptr->holdcourse > 0) {
		--(ptr->holdcourse);
		return;
	}

	if (zothusn >= nships) {
		ptr->cybmine = (byte)255;
		ptr->track_grace = 0;
		ptr->npcmsg = 255;
	} else {
		if (!ingegame(zothusn)) {
			ptr->track_grace = 0;
			npc_cruise(ptr, usrn, 0);
			return;
		}

		wptr = warshpoff(zothusn);

		if (!isvisible(ptr, wptr)) {
			if (ptr->track_grace > 0) {
				--ptr->track_grace;
			} else {
				ptr->cybmine = 255;
				ptr->track_grace = 0;
				ptr->npcstate = 255;
				ptr->npcmsg = 255;
			}
			return;
		}
		ptr->track_grace = CYBGRACE;

		low_ship = zothusn;
		low_dist = cdistance(&ptr->coord, &(wptr->coord));
	}

	if (ptr->cybmine == (byte)255 && ptr->damage <= CYB_MINDAM) {
		/* don't pick new pursuit if heavily damaged */
		lta = shipclass[ptr->shpclass].lowest_to_attk - 1;

			for (zothusn = 0; zothusn < nships; zothusn++) {
				wptr = warshpoff(zothusn);
				/* if playing, visible, and not same faction */
				if (ingegame(zothusn) && isvisible(ptr, wptr) &&
					shipclass[wptr->shpclass].faction != shipclass[ptr->shpclass].faction &&
				/* and high enough class to attack, and not already claimed, and passes npc throttle */
				lta <= wptr->shpclass && notclaimed(wptr, zothusn) &&
				cyb_pick_fight(zothusn, 1) &&
				/* and if a user or a droid that we target */
				(wptr->status == GESTAT_USER || shipclass[wptr->shpclass].cybs_can_att)) {
				/* figure out who is closest */
				ddist = cdistance(&ptr->coord, &wptr->coord);
				if (ddist < low_dist) {
					low_dist = ddist;
						low_ship = zothusn;
					}
				}
			}
		}

		if (low_ship == -1 || low_ship >= nships) {
		ptr->cybmine = 255;
		ptr->track_grace = 0;
		ptr->npcstate = 255;
		ptr->npcmsg = 255;
	} else {
		if (ptr->cybmine != (byte)low_ship) {
			ptr->npcmsg = 255;
			ptr->npcstate = 255;
		}
		ptr->cybmine = (byte)low_ship;
		ptr->track_grace = CYBGRACE;
		wptr = warshpoff(low_ship);
		if (low_dist >= hyperdist1) {
			ptr->speed2b = ((int)(low_dist / hyperdist1)) * FARSPEED;
			if (ptr->speed < 1000)
				hyperspace(ptr, usrn, 1);
			ptr->speed = ptr->speed2b;
			ptr->head2b = vector(&ptr->coord, &(wptr->coord));
			/* DEBUG
			prf("***\r%s, LONG, Sector %d %d, Speed: %s \rhyperdist1: %s, hyperdist2: %s, low_dist: %s\r",cybname,(int)ptr->coord.xcoord,(int)ptr->coord.ycoord,spr("%ld",(long)ptr->speed2b),
				spr("%ld",(long)hyperdist1),spr("%ld",(long)hyperdist2),spr("%ld",(long)low_dist));
			outwar(ALWAYS,usrn,0); */
		} else if (low_dist >= hyperdist2) {
			if (cyb_fast(ptr)) {
				ptr->speed2b = FARSPEED;
				ptr->speed = ptr->speed2b;
			} else {
				if (ptr->speed < 1000)
					hyperspace(ptr, usrn, 1);
				ptr->speed2b = d_topspeed;
			}
			ptr->head2b = vector(&ptr->coord, &(wptr->coord));
			/* DEBUG
			prf("***\r%s, MID, Sector %d %d, Speed: %s \rhyperdist1: %s, hyperdist2: %s, low_dist: %s\r",cybname,(int)ptr->coord.xcoord,(int)ptr->coord.ycoord,spr("%ld",(long)ptr->speed2b),
				spr("%ld",(long)hyperdist1),spr("%ld",(long)hyperdist2),spr("%ld",(long)low_dist));
			outwar(ALWAYS,usrn,0); */
			if (low_dist * 10000 < shipclass[wptr->shpclass].scanrange &&
				ptr->npcstate != (unsigned)low_ship) {
				cyb_annoy(ptr, low_ship, APPROACH);
				ptr->npcstate = low_ship;
			}
		} else if (low_dist >= 2.85 + (.175 * (d_topspeed / shipclass[ptr->shpclass].max_accel))) {
			/* fast ships with low accel brake earlier */
			if (cyb_fast(ptr)) {
				ptr->speed2b = d_topspeed;
				ptr->speed = ptr->speed2b;
			} else {
				if (ptr->speed < 1000)
					hyperspace(ptr, usrn, 1);
				ptr->speed2b = d_topspeed;
			}
			ptr->head2b = vector(&ptr->coord, &(wptr->coord));
			/* DEBUG
			prf("***\r%s, SHORT, Sector %d %d, Speed: %s \rhyperdist1: %s, hyperdist2: %s, low_dist: %s\r",cybname,(int)ptr->coord.xcoord,(int)ptr->coord.ycoord,spr("%ld",(long)ptr->speed2b),
				spr("%ld",(long)hyperdist1),spr("%ld",(long)hyperdist2),spr("%ld",(long)low_dist));
			outwar(ALWAYS,usrn,0); */
			if (low_dist * 10000 < shipclass[wptr->shpclass].scanrange &&
				ptr->npcstate != (unsigned)low_ship) {
				cyb_annoy(ptr, low_ship, APPROACH);
				ptr->npcstate = low_ship;
			}
		} else if (wptr->where == 1 && d_topspeed >= 1000) {
			inbound = FALSE;
			keepwarp = FALSE;
			attackwarp = FALSE;
			phatwarp = FALSE;
			ptr->head2b = vector(&ptr->coord, &(wptr->coord));
			if (low_dist < 2.0 && fabs(normal(wptr->heading - ptr->head2b)) > 120.0)
				inbound = TRUE;
			if (shipclass[ptr->shpclass].max_phasr > 0 && ptr->hypha == 0 && ptr->phasr >= 0)
				keepwarp = TRUE;
			if (shipclass[ptr->shpclass].max_phasr >= phatowrp && ptr->phasr >= PMINFIRE) {
				phatwarp = TRUE;
				attackwarp = TRUE;
			}
			if (shipclass[ptr->shpclass].max_missl > 0 && ptr->items[I_MISSILE] > 0) {
				if (!phatwarp)
					keepwarp = TRUE;
				attackwarp = TRUE;
			}
			if (inbound && attackwarp &&
				(phatwarp || !keepwarp ||
				gernd() % (5 - ((shipclass[ptr->shpclass].tough_factor + 1) / 2)) == 0)) {
				ptr->speed2b = 990.0;
				if (cyb_fast(ptr)) {
					hyperspace(ptr, usrn, 0);
					ptr->speed = ptr->speed2b;
				}
			} else {
				if (cyb_fast(ptr)) {
					ptr->speed2b = d_topspeed;
					ptr->speed = ptr->speed2b;
				}
				/* if following and not shooting first, slow down */
				if (shipclass[wptr->shpclass].cybs_can_att == 0 &&
					ptr->cantexit == 0 && wptr->cantexit == 0 && low_dist < 1) {
					if (wptr->speed * .5 > d_topspeed)
						ptr->speed2b = d_topspeed;
					else {
						ptr->speed2b = ((long)(wptr->speed * 0.5) / 1000L) * 1000.0;
						if (ptr->speed2b == 0)
							ptr->speed2b = 990;
					}
				} else if (wptr->speed * 1.25 >= d_topspeed)
					ptr->speed2b = d_topspeed;
				else
					ptr->speed2b = ((long)(wptr->speed * 1.25) / 1000L) * 1000.0;
			}
				/* DEBUG
				prf("***\r%s, CLOSE, Sector %d %d, Speed: %s \rhyperdist1: %s, hyperdist2: %s, low_dist: %s\r",cybname,(int)ptr->coord.xcoord,(int)ptr->coord.ycoord,spr("%ld",(long)ptr->speed2b),
					spr("%ld",(long)hyperdist1),spr("%ld",(long)hyperdist2),spr("%ld",(long)low_dist));
			outwar(ALWAYS,usrn,0); */
		} else if ((shipclass[wptr->shpclass].cybs_can_att ||
			wptr->cantexit > 0 || ptr->cantexit > 0) && !neutral(&wptr->coord)) {
			if (low_dist > .5) {
				if (ptr->where == 0 && wptr->where == 0 &&
					wptr->speed >= 990.0 && d_topspeed >= 1000.0 && low_dist > 1.5) {
					ptr->head2b = vector(&ptr->coord, &(wptr->coord));
					ptr->speed2b = d_topspeed;
					ptr->holdcourse = 1;
				} else {
					ptr->speed2b = 990.0;
					ptr->head2b = vector(&ptr->coord, &(wptr->coord));
					if (cyb_fast(ptr)) {
						hyperspace(ptr, usrn, 0);
						ptr->speed = ptr->speed2b;
					}
				}
			} else {
				ptr->speed2b = ((int)(rndm(350.0) + 150.0) / 10) * 10;
				if (cyb_fast(ptr)) {
					hyperspace(ptr, usrn, 0);
					ptr->speed = ptr->speed2b;
				}
				/* vary the close maneuver */
				closemove = gernd() % 6;
				if (closemove < 2)
					/* press the attack with a slight offset */
					ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) + rndm(60.0) - 30.0);
				else if (closemove < 4)
					/* make a short breakaway turn before coming back around */
					ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) + 180.0 + rndm(90.0) - 45.0);
				else
					/* sometimes keep the old full-random move for unpredictability */
					ptr->head2b = rndm(359.9);
			}
			/* DEBUG
			prf("***\r%s, IMPULSE, Sector %d %d, Speed: %s \rhyperdist1: %s, hyperdist2: %s, low_dist: %s\r",cybname,(int)ptr->coord.xcoord,(int)ptr->coord.ycoord,spr("%ld",(long)ptr->speed2b),
				spr("%ld",(long)hyperdist1),spr("%ld",(long)hyperdist2),spr("%ld",(long)low_dist));
			outwar(ALWAYS,usrn,0); */
		} else {
			/* back off if not going to shoot first */
			ptr->speed2b = ((int)(rndm(750.0) + 150.0) / 10) * 10;
			if (cyb_fast(ptr)) {
				hyperspace(ptr, usrn, 0);
				ptr->speed = ptr->speed2b;
			}
			if (low_dist < 1.5)
				/* if already close, make a brief angled breakaway */
				ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) + 150.0 + rndm(60.0));
			else
				/* otherwise drift across the target's front instead of creeping straight in */
				ptr->head2b = normal(vector(&ptr->coord, &wptr->coord) + 30.0 + rndm(60.0));
			if (shipclass[wptr->shpclass].cybs_can_att == 0)
				cyb_annoy(ptr, low_ship, IGNORE);
			else if (neutral(&wptr->coord))
				cyb_annoy(ptr, low_ship, NEUTRAL);
			ptr->holdcourse = (gernd() % 3) + 2;
		}
		/* make sure speed jumps won't leave us in a weird state */
		/* avoid fractional warp values */
		if (ptr->speed < 1000 && ptr->speed2b >= 1000) {
			hyperspace(ptr, usrn, 1);
			if (ptr->speed2b <= shipclass[ptr->shpclass].max_accel)
				ptr->speed = ptr->speed2b;
			else
				ptr->speed = shipclass[ptr->shpclass].max_accel;
		}
	}
}

/**************************************************************************
** Main per-tick cyborg behavior                                         **
**************************************************************************/

void FUNC cyb_lives(WARSHP *ptr, int usrn)
{
	WARSHP *wptr;
	int zothusn;
	double ddist;

	if (!sameas(ptr->userid, warusroff(usrn)->userid))
		geshocst(0, "GE:ERR:Cyb Names !=");

	sprintf(&cybname[7], "%d", usrn);

	logthis(spr("@cyb_lives %s", cybname));

	/* if already dead, don't do anything */
	if (ptr->damage >= 100.0)
		return;

	/* reset the ticker to 255 to cause it to recalc */
	ptr->tick = 255;

	/* save off the topspeed in 1000's */
	/* if no warp, top speed is impulse 99 */
	if (ptr->topspeed == 0 && shipclass[ptr->shpclass].max_accel > 0)
		d_topspeed = 990;
	else
		d_topspeed = (double)ptr->topspeed * 1000.0;

	/* countdown to database update */
	db_update(ptr, usrn);

	/* if cyb loses scanning ability, kick back and chill until fixed */
	if (ptr->tactical < 0) {
		ptr->cybmine = 255;
		ptr->npcstate = 255;
		ptr->npcmsg = 255;
		ptr->holdcourse = 0;
		if (shipclass[ptr->shpclass].max_accel > 0)
			npc_cruise(ptr, usrn, 0);
		if (ptr->where == 0 && ptr->shieldstat == SHIELDDN)
			shieldup(ptr, usrn);
		ptr->energy = 50000L;
		ptr->tick = (CYBTICKTIME + gernd() % CYBTICKTIME) * 5;
		return;
	}

	/* still moving at pursuit speed, but no longer pursuing */
	if (cyb_fast(ptr) && ptr->cybmine == 255)
		npc_cruise(ptr, usrn, 0);

	/* am I being jammed ? */
	if (ptr->jam_sev <= (byte)3) {
		/* look at all the other ships */
		for (zothusn = 0; zothusn < nships; zothusn++) {
			wptr = warshpoff(zothusn);
			/* if in game, visible, and not same faction, go getem */
			if (ingegame(zothusn) && isvisible(ptr, wptr) &&
				(shipclass[wptr->shpclass].faction != shipclass[ptr->shpclass].faction)) {
				ddist = cdistance(&ptr->coord, &wptr->coord);
				ddist *= 10000;
				/* if a user is around, pay more attention */
				if (ddist < (double)shipclass[ptr->shpclass].scanrange &&
					wptr->status == GESTAT_USER)
					ptr->tick = CYBTICKTIME +
						gernd() % (5 - shipclass[ptr->shpclass].tough_factor);
				if (!neutral(&ptr->coord) &&
					ddist < (double)shipclass[ptr->shpclass].scanrange) {
					/* bases don't approach... so send msg when wptr approaches */
					if (shipclass[ptr->shpclass].max_accel == 0 &&
						ddist < (double)shipclass[wptr->shpclass].scanrange)
						cyb_annoy(ptr, zothusn, CYBBASEA);
					/* in range, and target not in neutral zone, AND... */
					if (ddist < 30000.0 +
						((double)shipclass[ptr->shpclass].tough_factor * 2000.0) &&
						!neutral(&wptr->coord) &&
						/* if target is NPC, and not traveling to neutral zone or is already targeting me */
						((wptr->status == GESTAT_AUTO &&
						((wptr->npcstate < 2 || wptr->npcstate > 7) || wptr->cybmine == usrn) &&
						/* ...and is attackable class and i've already targeted it or decide to do so */
						(shipclass[wptr->shpclass].cybs_can_att &&
						(ptr->cybmine == zothusn || cyb_pick_fight(zothusn, 0)))) ||
						/* if target is user, and attackable class... */
						(wptr->status == GESTAT_USER &&
						(shipclass[wptr->shpclass].cybs_can_att ||
						/* ...or i've fired recently or my target has fired recently or gets too close to me */
						ptr->cantexit > 0 || wptr->cantexit > 0 ||
						ddist < (tooclose + rndm(tooclose)))))) {
						if (wptr->where == 1) {
							if (gernd() % (4 - (shipclass[ptr->shpclass].tough_factor / 2)) == 0) {
								cyb_annoy(ptr, zothusn, HIATTACK);
								/* fire phasers (or maybe even missiles) at the fool */
								if (shipclass[ptr->shpclass].max_missl &&
									(ptr->items[I_MISSILE] > 0) &&
									(gernd() % 10 == 0))
									misl(ptr, usrn, zothusn,
										(shipclass[ptr->shpclass].tough_factor + 1) * 4000, 0);
								else {
									ptr->degrees = cbearing(&ptr->coord, &wptr->coord, ptr->heading);
									if (ptr->where == 1 &&
										shipclass[ptr->shpclass].max_phasr > 0 &&
										ptr->hypha == 0 && ptr->phasr >= 0)
										firehp(ptr, usrn);
									if (ptr->where == 0 &&
										shipclass[ptr->shpclass].max_phasr >= phatowrp &&
										ptr->phasr >= PMINFIRE) {
										ptr->percent = 2;
										firep(ptr, usrn);
									}
								}
							}
						} else if (ptr->where == 0) {
							cyb_attack(ptr, usrn, wptr, zothusn);
							if (shipclass[ptr->shpclass].has_decoy &&
								ptr->items[I_DECOYS] > 0)
								npc_lay_decoys(ptr);
						}
					}
				}
			}
		}
	} else {
		/* don't mine or move if immobile */
		if (shipclass[ptr->shpclass].max_accel > 0) {
			/* as long as they can't see ... the other player must be trying to get
			away.... might as well mine the area */
			if (shipclass[ptr->shpclass].has_mine && ptr->items[I_MINE] > 0 &&
				ptr->mineload == 0 && !neutral(&ptr->coord) && gernd() % 5 == 0) {
				laymine(ptr, usrn, 10);
				npc_cruise(ptr, usrn, 2);
			}
		}
	}

	if (shipclass[ptr->shpclass].max_accel > 0 && ptr->helm >= 0) {
		if (ptr->cybmine < nships && ingegame(ptr->cybmine))
			cyb_check_damage(ptr, usrn);
		if (shipclass[ptr->shpclass].tough_factor > 1)	/* tougher/faster ships use missile avoidance logic */
			cyb_check_proj(ptr, usrn);
		cyb_check_lockon(ptr, usrn);
	}

	if (ptr->where == 0 && ptr->shieldstat == SHIELDDN)
		shieldup(ptr, usrn);

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
** Handle cyb victory cleanup                                            **
**************************************************************************/

void FUNC cyb_won(WARSHP *ptr, int usrn, WARSHP *wptr)
{
	wptr = wptr;
	npc_cruise(ptr, usrn, 0);
	ptr->cybupdate = 0;
}

/**************************************************************************
** Handle cyb death cleanup                                              **
**************************************************************************/

void FUNC cyb_died(WARSHP *ptr, int usrn, WARSHP *wptr)
{
	usrn = usrn;
	wptr = wptr;
	ptr->status = GESTAT_AVAIL;
}
