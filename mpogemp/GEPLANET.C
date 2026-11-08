/*****************************************************************************
 * ge-next GEPLANET.C                                                        *
 *                                                                           *
 * ge-next modifications ONLY copyright (C) 2024-2026 Anthony Schmidt        *
 * Based on Galactic Empire (c) 2025 Elwynor Technologies                    *
 *                                                                           *
 * https://manicpop.org/ge-next/  https://github.com/manicpop/ge-next        *
 *                                                                           *
 * All development through v3.2e         M. Murdock     03/17/1992           *
 * Worldgroup 3.2 Conversion v3.3        R. Hadsall     04/03/2021           *
 * Major BBS v10  Conversion v3.4        R. Hadsall     12/05/2025           *
 *                                                                           *
 * Copyright (C) 2006-2025 Rick Hadsall.  All Rights Reserved.               *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as published  *
 * by the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

#include "gcomm.h"
#include "string.h"

#include "majorbbs.h"

#include "gemain.h"


#define GEPLANET 1

#include "geglobal.h"

/* LOCAL GLOBAL DEFS *****************************************************/

#ifdef GE_ARENA
#define WORM_DESTINATIONS MAXPLANETS
#else
/* One random sector plus neutral, four homes and four Express sectors. */
#define WORM_DESTINATIONS (MAXPLANETS * (NUM_FACTION_HOMES + 6))
#endif
WORMTAB	wormtab[WORM_DESTINATIONS];
int	wormnum;

#ifdef GE_ARENA
/* ceil(pct * 32767 / 100), folded at compile time for the literal percentages. */
#define ARENA_PCT_LIMIT(pct) ((unsigned)(((unsigned long)(pct) * 32767UL + 99UL) / 100UL))

static byte arena_planet_qty(void)
{
	unsigned r;

	r = gernd();
	if (r < ARENA_PCT_LIMIT(55))
		return 0;
	if (r < ARENA_PCT_LIMIT(75))
		return 1;
	if (r < ARENA_PCT_LIMIT(87))
		return 2;
	if (r < ARENA_PCT_LIMIT(94))
		return 3;
	if (r < ARENA_PCT_LIMIT(98))
		return (byte)(4 + (gernd() % 3));
	if (r < ARENA_PCT_LIMIT(99))
		return (byte)(7 + (gernd() % 3));
	return 10;
}

static byte arena_planet_boost(void)
{
	unsigned r;

	r = gernd();
	if (r < ARENA_PCT_LIMIT(88))
		return 0;
	if (r < ARENA_PCT_LIMIT(96))
		return 1;
	if (r < ARENA_PCT_LIMIT(99))
		return 2;
	return 3;
}

static unsigned long arena_planet_gold(void)
{
	unsigned r;

	r = gernd();
	if (r < ARENA_PCT_LIMIT(55))
		return 0UL;
	if (r < ARENA_PCT_LIMIT(80))
		return 1UL + (unsigned long)(gernd() % 40);
	if (r < ARENA_PCT_LIMIT(92))
		return 41UL + (unsigned long)(gernd() % 60);
	if (r < ARENA_PCT_LIMIT(98))
		return 101UL + (unsigned long)(gernd() % 60);
	return 161UL + (unsigned long)(gernd() % 40);
}

static void arena_init_planet_loot(void)
{
	unsigned maint_roll;

	planet.items[I_TORPEDO].qty = arena_planet_qty();
	planet.items[I_MISSILE].qty = arena_planet_qty();
	planet.items[I_MINE].qty = arena_planet_qty();
	planet.items[I_JAMMERS].qty = arena_planet_qty();
	planet.items[I_DECOYS].qty = arena_planet_qty();
	planet.items[I_ZIPPERS].qty = arena_planet_qty();
	planet.items[I_FLUXPOD].qty = arena_planet_qty();
	if (arena_mode == ARENA_MODE_HOARD)
		planet.items[I_GOLD].qty = arena_planet_gold();

	if (arena_mode == ARENA_MODE_BASE) {
		planet.arena_shield_boost = 0;
		planet.arena_phaser_boost = 0;
	}
	else {
		planet.arena_shield_boost = arena_planet_boost();
		planet.arena_phaser_boost = arena_planet_boost();
	}
	planet.arena_flags = 0;
	if (gernd() < ARENA_PCT_LIMIT(6))
		planet.arena_flags |= ARENA_PL_SCAN;
	if (gernd() < ARENA_PCT_LIMIT(6))
		planet.arena_flags |= ARENA_PL_ARMOR;
	if (gernd() < ARENA_PCT_LIMIT(6))
		planet.arena_flags |= ARENA_PL_ACCEL;
	if (gernd() < ARENA_PCT_LIMIT(6))
		planet.arena_flags |= ARENA_PL_CORE;
	maint_roll = gernd();
	if (maint_roll < ARENA_PCT_LIMIT(6))
		planet.arena_flags |= ARENA_PL_INSTANT;
	else if (maint_roll < ARENA_PCT_LIMIT(31))
		planet.arena_flags |= ARENA_PL_MAINT;
}

static void arena_init_empty_planet(void)
{
	int i;

	for (i = 0; i < NUMITEMS; ++i) {
		planet.items[i].qty = 0;
		planet.items[i].sell = 'N';
	}
	planet.arena_shield_boost = 0;
	planet.arena_phaser_boost = 0;
	planet.arena_flags = 0;
}
#endif

/**************************************************************************
** Check Spy Function                                                    **
**************************************************************************/

#ifndef GE_ARENA
void FUNC check_spy(void)
{
	int spycnt, odds, i, j;
	long itemcnt;
	double d_itemcnt, d_rptcnt, d_odds;

	/* is the spy owned by the planet owner? - if so kill him */
	if (sameas(plptr->spyowner, plptr->userid))
		plptr->spyowner[0] = 0;

	if (plptr->spyowner[0] != 0) {
			setmbk(gemb);
			xsect = coord1(plptr->coord.xcoord);
			ysect = coord1(plptr->coord.ycoord);
			logthis(spr("Spy on %s in %d %d present", plptr->name, xsect, ysect));
			/* there is a spy on this planet */
			/* calculate his odds of getting caught */
			spycnt = (int)plptr->items[I_SPY].qty;

			if (spycnt > 0) {
				odds = (50 / spycnt) + 1;

				if (gernd() % odds == 0) {
					/* caught him */
					logthis(spr("Spy on %s in %d %d caught", plptr->name, xsect, ysect));
					/* mail message to spy owner informing him of the death of his spy */
					clrprf();
					prfmsg(SPYC1, plptr->name, xsect, ysect);
					strcpy(mail.userid, plptr->spyowner);
					strcpy(mail.topic, "** Official Protest **");
					mail.mailclass = MAIL_CLASS_DISTRESS;
					sendit();

					/* mail message to the planet owner informing him that a spy was caught */
					clrprf();
					prfmsg(SPYC2, plptr->name, xsect, ysect, plptr->spyowner, plptr->spyowner);
					strcpy(mail.userid, plptr->userid);
					strcpy(mail.topic, "** Official Protest **");
					mail.mailclass = MAIL_CLASS_DISTRESS;
					sendit();
					clrprf();

					/* kill the spy */
					plptr->spyowner[0] = 0;

					return;
				}
			}

			/* calculate the odds of the spy finding some usefull information */
			if (gernd() % 10 == 0) {
				logthis(spr("Spy on %s in %d %d found stuff", plptr->name, xsect, ysect));

				/* pick a topic to report */
				itemcnt = -1;
				for (j = 0; j < 10; ++j) {
					i = gernd() % NUMITEMS;
					itemcnt = plptr->items[i].qty;
					if (itemcnt > 0)
						break;
				}

				if (itemcnt > 0) {

					/* calculate the accuracy of the information */
					d_odds = 50.0 + rndm(48.0);
					odds = d_odds;

					d_itemcnt = itemcnt;

					/* convert it to a factor */
					d_odds = (100.0 - d_odds) / 100.0;

					/* calc the deviation */
					d_rptcnt = d_itemcnt - (d_itemcnt * rndm(d_odds)) +
						(d_itemcnt * rndm(d_odds));

					itemcnt = d_rptcnt;

					clrprf();
					prfmsg(SPYM2, plptr->name, xsect, ysect, odds, item_name[i],
						spr("%ld", itemcnt));
					strcpy(mail.userid, plptr->spyowner);
					strcpy(mail.topic, "Intelligence Report");
					mail.mailclass = MAIL_CLASS_DISTRESS;

					sendit();
					clrprf();
				}
			}
	}
}


void FUNC multiply(int final_mult)
{
	int i, j;
	double qty, men, rate, hrs, fact, tfact, taxfact, maxf;
	long max, cnt;
	unsigned long temp;

	/* kill off 1/8th the troops if no food */
	if ((plptr->items[I_TROOPS].qty / 100) > plptr->items[I_FOOD].qty) {
		cnt = plptr->items[I_TROOPS].qty / 8;
		plptr->items[I_TROOPS].qty -= cnt;

		if (final_mult == TRUE) {
			mail.mailclass = MAIL_CLASS_DISTRESS;
			mail.type = MESG06;
			strncpy(mail.userid, plptr->userid, UIDSIZ);
			mail.userid[UIDSIZ - 1] = 0;
			strcpy(mail.name1, plptr->name);
			mail.int1 = plptr->xsect;
			mail.int2 = plptr->ysect;
			mail.long1 = cnt;
			mailit(0);
		}
	}

	/* troops eat first */
	temp = plptr->items[I_FOOD].qty;
	if (temp > plptr->items[I_TROOPS].qty / 100) {
		temp -= plptr->items[I_TROOPS].qty / 100;

		plptr->items[I_FOOD].qty = temp;
	} else
		plptr->items[I_FOOD].qty = 0;

	/* kill off 1/8th the men if no food */
	if ((plptr->items[I_MEN].qty / 100) > plptr->items[I_FOOD].qty) {
		cnt = plptr->items[I_MEN].qty / 8;
		temp = plptr->items[I_MEN].qty;

		if (temp > cnt)
			temp -= cnt;
		else
			temp = 0;

		plptr->items[I_MEN].qty = temp;

		if (final_mult == TRUE) {
			mail.mailclass = MAIL_CLASS_DISTRESS;
			mail.type = MESG07;
			strncpy(mail.userid, plptr->userid, UIDSIZ);
			mail.userid[UIDSIZ - 1] = 0;
			strcpy(mail.name1, plptr->name);
			mail.int1 = plptr->xsect;
			mail.int2 = plptr->ysect;
			mail.long1 = cnt;
			mailit(0);
		}
	}

	/* men eat */
	if (meneat) {
		temp = plptr->items[I_FOOD].qty;
		if (temp > plptr->items[I_MEN].qty / 100) {
			temp -= plptr->items[I_MEN].qty / 100;
			plptr->items[I_FOOD].qty = temp;
		} else
			plptr->items[I_FOOD].qty = 0;
	}

	taxfact = 1 - ((float)plptr->taxrate / 120.0);

	/* take any donated cash */

	temp = (long)baseprice[I_GOLD];
	temp = plptr->items[I_GOLD].qty * temp;
	if (plptr->cash > ULCAP - temp)
		temp = ULCAP - plptr->cash;
	plptr->cash += temp;

	plptr->items[I_GOLD].qty = 0;

	for (i = 0; i < NUMITEMS; ++i) {
		men = (float)plptr->items[I_MEN].qty;
		rate = (float)plptr->items[i].rate;
		hrs = (float)manhours[i];
		hrs = hrs / 10000.0;

		fact = 1.0;
		qty = (men * (rate / 100.0) * (hrs / 6.0)) / 7.0;

		fact *= ((float)(plptr->enviorn + plptr->resource + 2) * .25);
		fact *= taxfact;

		j = (6 - (plptr->resource + plptr->enviorn)) * 10;
		tfact = .95 - ((float)j / 100);
		plptr->cash = (float)plptr->cash * tfact;

		if (plptr->cash > 0) /* if plenty of cash increase production */
			fact *= 1.5;
		temp = plptr->items[i].qty;
		temp += (long)(qty * fact);

		/* cap out planet maximums */
		maxf = maxpl[i];
		max = (long)(maxf * fact);

		sprintf(gechrbuf, "Pl prod %d %d %s %s max:%ld f:%g hrs:%f qty:%f maxf:%f cnt:%ld",
			plptr->xsect,
			plptr->ysect,
			plptr->userid,
			item_name[i],
			max,
			fact,
			hrs,
			qty,
			maxf,
			temp);

		logthis(gechrbuf);

		if (final_mult == TRUE && temp > max) {
			logthis("Mailing production report");
			temp = max;
			mail.mailclass = MAIL_CLASS_PRODRPT;
			mail.type = MESG08 + i;
			strncpy(mail.userid, plptr->userid, UIDSIZ);
			mail.userid[UIDSIZ - 1] = 0;
			strcpy(mail.name1, plptr->name);
			mail.int1 = plptr->xsect;
			mail.int2 = plptr->ysect;
			mail.long1 = max;
			mailit(0);
			logthis("Mail sent");
		}

		plptr->items[i].qty = temp;
	}

	fact = (float)plptr->taxrate / 1200;
	qty = fact * (float)plptr->items[I_MEN].qty;
	if (plptr->tax > ULCAP - qty)
		qty = ULCAP - plptr->tax;
	plptr->tax += (unsigned long)qty;

	/* see if there is a revolt */
	if (!sameas(plptr->userid, "**Free**")) {
		/* the higher the taxes the greater the chance of revolt */
		tfact = ((float)plptr->taxrate / 120.0) * .35;

		qty = (float)plptr->items[I_TROOPS].qty;
		men = (float)plptr->items[I_MEN].qty;
		men *= tfact;

		/* if there are not enough troops the odds are really high */
		if (men > qty) {
			if (gernd() % 10 == 0) {
				logthis("Revolt!!");
				cnt = plptr->items[I_TROOPS].qty / (ge_rand_bits(3) + 2);
				plptr->items[I_TROOPS].qty = cnt;

				/* decrement planet counter for this user */
				setbtv(gebb5);
				if (qeqbtv(plptr->userid, 0)) {
					gcrbtv(&tmpusr, 0);
					if (tmpusr.planets > 0)
						--tmpusr.planets;
					geudb(GEUPDATE, tmpusr.userid, &tmpusr);
				}
				setbtv(gebb2);

				mail.mailclass = MAIL_CLASS_DISTRESS;
				mail.type = MESG30;
				strncpy(mail.userid, plptr->userid, UIDSIZ);
				mail.userid[UIDSIZ - 1] = 0;
				strcpy(mail.name1, plptr->name);
				mail.int1 = plptr->xsect;
				mail.int2 = plptr->ysect;
				mail.long1 = cnt;
				mailit(0);

				logthis("back from mailing it");
				strcpy(plptr->userid, "**Free**");
				plptr->owner_type = PL_OWNER_FREE;
				plptr->owner_faction = 0;
			}
		}
	}
}
#endif

#ifndef GE_ARENA
static SECTORPOS home_sectors[NUM_FACTION_HOMES];
static int home_sectors_ready;

/* Two 15-bit draws cover every sector in the largest supported galaxy. */
static int sector_random(int low, int high)
{
	unsigned long span, limit, value;

	span = (unsigned long)((long)high - (long)low + 1L);
	limit = 0x40000000UL - (0x40000000UL % span);
	do {
		value = ((unsigned long)gernd() << 15) | (unsigned long)gernd();
	} while (value >= limit);
	return (int)((long)low + (long)(value % span));
}

static int express_sector(int x, int y)
{
	return univmax >= 50 && (x == -50 || x == 50) && (y == -50 || y == 50);
}

static int valid_home_sector(int x, int y, int layout)
{
	int i, outer, inner;

	if (x < -univmax || x > univmax || y < -univmax || y > univmax)
		return FALSE;
	if ((x == 0 && y == 0) || express_sector(x, y))
		return FALSE;
	for (i = 0; i < layout; ++i)
		if (home_sectors[i].xsect == x && home_sectors[i].ysect == y)
			return FALSE;

	if (layout == LAYOUT_VAKORY) {
		outer = (int)((univmax * 9L + 9L) / 10L);
		inner = (int)((univmax + 1L) / 2L);
		if (abs(x) < inner || abs(y) < inner || (abs(x) < outer && abs(y) < outer))
			return FALSE;
	}
	return TRUE;
}

static void home_planet_coord(int layout, COORD *coord)
{
	coord->xcoord = (double)home_sectors[layout].xsect + outpost_layouts[layout].objects[0].xcoord;
	coord->ycoord = (double)home_sectors[layout].ysect + outpost_layouts[layout].objects[0].ycoord;
}

static void choose_home_sector(int layout, COORD *anchor, int radius)
{
	int attempt, outer, inner;
	long x, y, swap;
	COORD candidate;

	outer = (int)((univmax * 9L + 9L) / 10L);
	inner = (int)((univmax + 1L) / 2L);

	for (attempt = 0; attempt < 10000; ++attempt) {
		if (layout == LAYOUT_VAKORY) {
			x = sector_random(outer, univmax);
			y = sector_random(inner, univmax);
			if (ge_rand_bits(1)) {
				swap = x;
				x = y;
				y = swap;
			}
			if (ge_rand_bits(1))
				x = -x;
			if (ge_rand_bits(1))
				y = -y;
		}
		else if (radius > 0) {
			/* The extra sector allows for different local planet positions. */
			x = (long)coord1(anchor->xcoord) + sector_random(-radius - 1, radius + 1);
			y = (long)coord1(anchor->ycoord) + sector_random(-radius - 1, radius + 1);
		}
		else {
			x = sector_random(-univmax, univmax);
			y = sector_random(-univmax, univmax);
		}

		if (x < -univmax || x > univmax || y < -univmax || y > univmax)
			continue;
		if (!valid_home_sector((int)x, (int)y, layout))
			continue;

		candidate.xcoord = (double)x + outpost_layouts[layout].objects[0].xcoord;
		candidate.ycoord = (double)y + outpost_layouts[layout].objects[0].ycoord;
		if (radius > 0 && cdistance(&candidate, anchor) > (double)radius)
			continue;

		/* Do not replace a sector already generated by a wormhole or a player. */
		pkey.xsect = (int)x;
		pkey.ysect = (int)y;
		pkey.plnum = 0;
		if (gesdb(GEGETNOW, &pkey, &sector))
			continue;

		home_sectors[layout].xsect = (int)x;
		home_sectors[layout].ysect = (int)y;
		return;
	}
	catastro(spr("GE:ERR:Cannot place faction layout %d in this galaxy", layout));
}

static void ensure_outpost(int layout, int x, int y)
{
	COORD coord;

	coord.xcoord = (double)x + outpost_layouts[layout].objects[0].xcoord;
	coord.ycoord = (double)y + outpost_layouts[layout].objects[0].ycoord;
	getsector(&coord);

	pkey.xsect = x;
	pkey.ysect = y;
	pkey.plnum = 1;
	if (!gesdb(GEGETNOW, &pkey, (GALSECT *)&planet))
		catastro(spr("GE:ERR:Missing outpost planet in sector %d %d", x, y));
	if (planet.type != PLTYPE_PLNT || planet.owner_type != PL_OWNER_FACTION
		|| planet.owner_faction != outpost_layouts[layout].objects[0].faction)
		catastro(spr("GE:ERR:Outpost ownership mismatch in sector %d %d", x, y));
	geshocst(1, spr("GE:INF:%s outpost at sector %d %d", planet.name, x, y));
}

void FUNC init_galaxy(void)
{
	COORD origin, anchor;
	int i, intkey = PLTYPE_PLNT;

	home_sectors_ready = FALSE;
	if (s00[0].type != PLTYPE_PLNT || s00[0].outpost_type != OUTPOST_ZYGOR)
		catastro("GE:ERR:Sector 0 0 planet 1 must be the Zygor outpost");
	for (i = 0; i < NUM_OUTPOST_LAYOUTS; ++i)
		if (outpost_layouts[i].objects[0].type != PLTYPE_PLNT
			|| outpost_layouts[i].objects[0].outpost_type != OUTPOST_LYDORIA + i)
			catastro("GE:ERR:Faction layouts must begin with their primary outpost planet");

	/* Create the timekeeper before any other predefined or random sectors. */
	origin.xcoord = 0.5;
	origin.ycoord = 0.5;
	getsector(&origin);

	setbtv(gebb2);
	if (!agebtv(&planet, &intkey, 2))
		catastro("GE:ERR:Missing Zygor timekeeper record");
	if (planet.xsect != 0 || planet.ysect != 0 || planet.plnum != 1)
		catastro("GE:ERR:First planet record must be sector 0 0 planet 1");
	if (planet.owner_type != PL_OWNER_FACTION || planet.outpost_type != OUTPOST_ZYGOR)
		catastro("GE:ERR:Planet ownership data requires a fresh 0.4 galaxy");

	origin = planet.coord;
	if (planet.home_locations_set == 1) {
		memcpy(home_sectors, planet.home_sectors, sizeof(home_sectors));
		for (i = 0; i < NUM_FACTION_HOMES; ++i)
			if (!valid_home_sector(home_sectors[i].xsect, home_sectors[i].ysect, i))
				catastro("GE:ERR:Saved faction sectors do not fit this galaxy");
	}
	else if (planet.home_locations_set == 0) {
		choose_home_sector(LAYOUT_LYDORIA, NULL, 0);
		home_planet_coord(LAYOUT_LYDORIA, &anchor);
		choose_home_sector(LAYOUT_SARTEN, &anchor, 15);
		choose_home_sector(LAYOUT_MURDONIA, &origin, 25);
		choose_home_sector(LAYOUT_VAKORY, NULL, 0);

		/* Save the plan before building, so interrupted startup cannot reroll it. */
		memcpy(planet.home_sectors, home_sectors, sizeof(home_sectors));
		planet.home_locations_set = 1;
		if (!gesdb(GEUPDATE, (PKEY *)&planet, (GALSECT *)&planet))
			catastro("GE:ERR:Cannot save faction sector locations");
	}
	else
		catastro("GE:ERR:Invalid faction location state in Zygor");

	home_sectors_ready = TRUE;
	for (i = 0; i < NUM_FACTION_HOMES; ++i)
		ensure_outpost(i, home_sectors[i].xsect, home_sectors[i].ysect);
	if (univmax >= 50)
		for (i = 0; i < 4; ++i)
			ensure_outpost(LAYOUT_ZYGOR_EXPRESS, (i & 1) ? 50 : -50, (i & 2) ? 50 : -50);

	setbtv(gebb2);
	if (!agebtv(&planet, &intkey, 2)
		|| planet.xsect != 0 || planet.ysect != 0 || planet.plnum != 1)
		catastro("GE:ERR:Zygor must remain the first planet record");
}
#endif

/* Display identity only; never use this text for ownership/access checks. */
char *FUNC planet_owner_name(GALPLNT *pl)
{
#ifndef GE_ARENA
	if (pl->owner_type == PL_OWNER_FACTION) {
		if (pl->owner_faction < 8)
			return faction_names[pl->owner_faction];
		return "";
	}
#endif
	return pl->userid;
}

static const PLANETDEF *predefined_sector(int x, int y, int *count)
{
#ifndef GE_ARENA
	int i;
#endif

	if (x == 0 && y == 0) {
		*count = s00plnum;
		return s00;
	}
#ifndef GE_ARENA
	if (home_sectors_ready)
		for (i = 0; i < NUM_FACTION_HOMES; ++i)
			if (home_sectors[i].xsect == x && home_sectors[i].ysect == y) {
				*count = outpost_layouts[i].count;
				return outpost_layouts[i].objects;
			}
	if (express_sector(x, y)) {
		*count = outpost_layouts[LAYOUT_ZYGOR_EXPRESS].count;
		return outpost_layouts[LAYOUT_ZYGOR_EXPRESS].objects;
	}
#endif
	*count = 0;
	return NULL;
}

/**************************************************************************
** Build a planet from a predefined sector entry                         **
**************************************************************************/

static void build_defined_planet(const PLANETDEF *def, int number)
{
	setmem(&planet, sizeof(GALPLNT), 0);

	strncpy(planet.userid, def->owner, UIDSIZ);
	planet.userid[UIDSIZ - 1] = 0;
	strncpy(planet.name, def->name, sizeof(planet.name) - 1);
	planet.name[sizeof(planet.name) - 1] = 0;
	strcpy(planet.password, "none");
	planet.enviorn = def->env;
	planet.resource = def->res;

	pkey.xsect = sector.xsect;
	pkey.ysect = sector.ysect;
	pkey.plnum = number;
	planet.xsect = pkey.xsect;
	planet.ysect = pkey.ysect;
	planet.plnum = pkey.plnum;
	planet.coord.xcoord = (double)sector.xsect + def->xcoord;
	planet.coord.ycoord = (double)sector.ysect + def->ycoord;
	planet.type = PLTYPE_PLNT;

	plptr = &planet;
#ifdef GE_ARENA
	if (def->type == 1 || def->type == 2)
		arena_init_empty_planet();
#else
	planet.owner_type = PL_OWNER_FACTION;
	planet.owner_faction = def->faction;
	planet.outpost_type = def->outpost_type;
	if (planet.xsect == 0 && planet.ysect == 0 && planet.plnum == 1) {
		planet.nebseed = nebseed;
		if (planet.nebseed == 0L)
			planet.nebseed = 1L;
	}
	update_outpost();
#endif

	logthis("GE:DBG:Built predefined planet");
}

static void build_defined_worm(const PLANETDEF *def, int number)
{
	setmem(&worm, sizeof(GALWORM), 0);

	strncpy(worm.name, def->name, 20);
	worm.name[19] = 0;

	pkey.plnum = number;
	pkey.xsect = sector.xsect;
	pkey.ysect = sector.ysect;

	worm.xsect = pkey.xsect;
	worm.ysect = pkey.ysect;
	worm.plnum = pkey.plnum;

	worm.coord.xcoord = (double)sector.xsect + def->xcoord;
	worm.coord.ycoord = (double)sector.ysect + def->ycoord;

	worm.type = PLTYPE_WORM;

	worm.visible = 1;
	worm.destination.xcoord = rndm((double)univmax * 2) - (double)univmax;
	worm.destination.ycoord = rndm((double)univmax * 2) - (double)univmax;

	logthis("GE:DBG:Getsector-add worm record");

	/* add to worm table so we can go build the other sector */
	if (wormnum >= WORM_DESTINATIONS)
		catastro("GE:ERR:Too many wormhole destinations during sector creation");
	wormtab[wormnum].coord.xcoord = worm.destination.xcoord;
	wormtab[wormnum].coord.ycoord = worm.destination.ycoord;
	wormtab[wormnum].dest.xcoord = worm.coord.xcoord;
	wormtab[wormnum].dest.ycoord = worm.coord.ycoord;
	++wormnum;

	logthis("Worm build first time");
}

/**************************************************************************
** look up planet.                                                       **
**************************************************************************/

static int xgetsector(COORD *sect, int wormy)
{
	int p, i, k, j, count;
	const PLANETDEF *defs;
	double ddist;
	byte tooclose;

	/* Build the integer key for lookup */
	pkey.xsect = coord1(sect->xcoord);
	pkey.ysect = coord1(sect->ycoord);
	pkey.plnum = 0;
	defs = predefined_sector(pkey.xsect, pkey.ysect, &count);

	if (!gesdb(GEGET, &pkey, &sector)) {
		/* Required predefined sectors, like wormhole destinations, must still be built. */
#ifdef GE_ARENA
		if (cntrbtv() >= max_plrec && !wormy) {
#else
		if (cntrbtv() >= max_plrec && !wormy && defs == NULL) {
#endif
			geshocst(1, spr("GE:INF:Planet DB full, sector %d %d shown as empty",
				pkey.xsect, pkey.ysect));
			setmem(&sector, sizeof(GALSECT), 0);
			sector.xsect = pkey.xsect;
			sector.ysect = pkey.ysect;
			sector.numplan = 0;
			sector.type = SECTYPE_NORMAL;
			return FALSE;
		}

		/* Didn't find a record - make one */
		logthis("GE:DBG:Getsector-new sect");

		setmem(&sector, sizeof(GALSECT), 0);

		sector.xsect = pkey.xsect;
		sector.ysect = pkey.ysect;
		sector.plnum = 0;
		sector.type = SECTYPE_NORMAL;

		/* figure out how many planets this sector has */
		if (gernd() % plodds == 0)
			p = gernd() % (maxplanets + 1);
		else
			p = 0;

		logthis(spr("GE: Ins sect X:%d Y:%d Pl:%d",
			pkey.xsect, pkey.ysect, p));

		/* Figure out where they are */
		if (neutral(sect) || defs != NULL) {
			for (i = 0; i < count; ++i) {
				logthis(spr("GE:DBG:Bld Pl %d %d %s", i, defs[i].type, defs[i].name));

				sector.ptab[i].coord.xcoord = (double)sector.xsect + defs[i].xcoord;
				sector.ptab[i].coord.ycoord = (double)sector.ysect + defs[i].ycoord;


#ifdef GE_ARENA
				sector.ptab[i].type = defs[i].type == 3 ? PLTYPE_WORM : PLTYPE_PLNT;
#else
				sector.ptab[i].type = defs[i].type;
#endif
				if (sector.ptab[i].type == PLTYPE_WORM) {
					build_defined_worm(&defs[i], i + 1);
					gesdb(GEADD, &pkey, (GALSECT *)&worm);
				}
				else {
					build_defined_planet(&defs[i], i + 1);
					gesdb(GEADD, &pkey, (GALSECT *)&planet);
				}

			}
			/* write the sector database record */

			sector.plnum = 0;
			sector.type = SECTYPE_NORMAL;
			sector.numplan = count;

			pkey.plnum = 0;
			pkey.xsect = sector.xsect;
			pkey.ysect = sector.ysect;

			gesdb(GEADD, &pkey, &sector);
		}
		else {
			for (i = 0; i < p; ++i) {
				/* is it a worm hole */
				if (wormy == FALSE && gernd() % wormodds == 0)
					sector.ptab[i].type = PLTYPE_WORM;
				else
					sector.ptab[i].type = PLTYPE_PLNT;
				/* place it and check distance to other planets */
				do {
					sector.ptab[i].coord.xcoord = (((double)pkey.xsect) + rndm(0.800)) + .1;
					sector.ptab[i].coord.ycoord = (((double)pkey.ysect) + rndm(0.800)) + .1;
					tooclose = FALSE;
					for (j = 0; j < i; ++j) {
						ddist = cdistance(&sector.ptab[i].coord, &sector.ptab[j].coord);
						if (ddist < 0.0700)
							tooclose = TRUE;
					}
					} while (tooclose == TRUE);

				sector.numplan = p;
				sector.type = SECTYPE_NORMAL;
			}

			logthis("GE:DBG:Getsector-add sect record");

		/* write the sector database record */
		gesdb(GEADD, (PKEY *)&sector, &sector);

			logthis("GE:DBG:Getsector-add planet records");

		/* add the planet records */
			for (i = 0; i < p; ++i) {
				if (sector.ptab[i].type == PLTYPE_PLNT) {
					setmem(&planet, sizeof(GALPLNT), 0);
					planet.xsect = pkey.xsect;
					planet.ysect = pkey.ysect;
					planet.plnum = i + 1;
					planet.coord.xcoord = sector.ptab[i].coord.xcoord;
					planet.coord.ycoord = sector.ptab[i].coord.ycoord;
					planet.type = sector.ptab[i].type;
					planet.enviorn = (char)ge_rand_bits(2);
					planet.resource = (char)ge_rand_bits(2);
					strcpy(planet.password, "none");

					for (k = 0; k < NUMITEMS; ++k)
						planet.items[k].sell = 'N';
#ifdef GE_ARENA
					arena_init_planet_loot();
#else
					/* Same population cutoff as rndm(3.99) > 3. */
					if (gernd() > 24636U) {
						/* Preserve the uncommon 5 from truncated rndm(5.1). */
						for (k = 0; k < NUMITEMS; ++k)
							planet.items[k].rate = gernd() / 6425U;

						planet.items[I_MEN].qty = (unsigned long)rndm(50000.0);
						planet.items[I_MEN].rate = 5 + (unsigned int)rndm(25.0);
						planet.items[I_FOOD].qty = (unsigned long)rndm(3200.0);
						planet.items[I_FOOD].rate = 15 + (unsigned int)rndm(15.0);
					}
					if (planet.items[I_MEN].qty > 0)
						planet.owner_type = PL_OWNER_FREE;
#endif
					logthis("GE:DBG:Getsector-write planet record");
					/* write the database record */
					gesdb(GEADD, (PKEY *)&planet, (GALSECT *)&planet);
				} else if (sector.ptab[i].type == PLTYPE_WORM) {
					setmem(&worm, sizeof(GALWORM), 0);
					worm.xsect = pkey.xsect;
					worm.ysect = pkey.ysect;
					worm.plnum = i + 1;
					worm.coord.xcoord = sector.ptab[i].coord.xcoord;
					worm.coord.ycoord = sector.ptab[i].coord.ycoord;
					worm.visible = 1;
					worm.destination.xcoord = rndm((double)univmax * 2) - (double)univmax;
					worm.destination.ycoord = rndm((double)univmax * 2) - (double)univmax;
					worm.type = sector.ptab[i].type;
					logthis("GE:DBG:Getsector-add worm record");
					/* write the database record */
					gesdb(GEADD, (PKEY *)&worm, (GALSECT *)&worm);
					/* add to worm table so we can go build the other sector */
					wormtab[wormnum].coord.xcoord = worm.destination.xcoord;
					wormtab[wormnum].coord.ycoord = worm.destination.ycoord;
					wormtab[wormnum].dest.xcoord = worm.coord.xcoord;
					wormtab[wormnum].dest.ycoord = worm.coord.ycoord;
					++wormnum;
				}
			}
		}
	}
	if (wormnum > 0)
		return TRUE;
	return FALSE;
}

/**************************************************************************
** look up sector. If not fnd make one... xsect and ysect are used       **
**************************************************************************/

void FUNC getsector(COORD *sect)
{
	int i;

	/* please keep this MBM comment here forever, thank you --AGS 2026 */

	/* I know when you look at this later Mike you will never remember why
	   you did this. So before you go tear it apart or go crazy trying to
	   figure it out I'll explain.

	   We call the getsector routine the first time to either get a previously
	   created sector or to create one. If one is created and there is a
	   wormhole (or more) in it the function Xgetsect returns TRUE. If so then
	   we must call it again to get the sectors that are the destination points
	   of the wormhole(s). Since we don't want it to create more wormholes in
	   the new sector (risking a runaway cascade effect) a flag is tripped on
	   telling Xgetsector to NOT create any wormholes.

	   If on the second pass Xgetsector gets a sector that already exists then
	   we have to insert a wormhole into the next planet slot and create
	   the planet record. If there are already 9 planets then too bad, this
	   wormhole is a one way bugger.
	*/

	wormnum = 0;

	if (xgetsector(sect, FALSE))
	{
		logthis("GE:DBG:Getsector-had worms");
		for (i = 0; i < wormnum; ++i)
		{
			logthis("GE:DBG:Getsector-get worm dest sect");

			xgetsector(&wormtab[i].coord, TRUE);
			if (sector.numplan < MAXPLANETS)
			{
				/* insert new wormhole */
				setmem(&worm, sizeof(GALWORM), 0);
				worm.xsect = pkey.xsect;
				worm.ysect = pkey.ysect;
				worm.plnum = sector.numplan + 1;
				worm.coord.xcoord = wormtab[i].coord.xcoord;
				worm.coord.ycoord = wormtab[i].coord.ycoord;
				worm.visible = 1;
				worm.destination.xcoord = wormtab[i].dest.xcoord;
				worm.destination.ycoord = wormtab[i].dest.ycoord;
				worm.type = PLTYPE_WORM;

				logthis("GE:DBG:Getsector-add dest worm record");
				/* write the database record */
				gesdb(GEADD, (PKEY *)&worm, (GALSECT *)&worm);

				logthis("GE:DBG:Getsector-update sector record");

				/* now update the sector record */
				sector.ptab[sector.numplan].type = worm.type;
				sector.ptab[sector.numplan].coord.xcoord = worm.coord.xcoord;
				sector.ptab[sector.numplan].coord.ycoord = worm.coord.ycoord;
				sector.numplan++;
				gesdb(GEUPDATE, (PKEY *)&sector, &sector);
			}
		}
		/* if we had to add another sector get the original again */
		logthis("GE:DBG:Getsector-get orig record");
		xgetsector(sect, FALSE);
	}
}

/**************************************************************************
** look up planet.                                                       **
**************************************************************************/

int FUNC getplanet(COORD *sect, int plnt)
{
	setmem(&planet, sizeof(GALPLNT), 0);

	pkey.xsect = coord1(sect->xcoord);
	pkey.ysect = coord1(sect->ycoord);
	pkey.plnum = plnt;

	if (!gesdb(GEGET, &pkey, (GALSECT *)&planet))
	{
		geshocst(0, spr("GE:ERR:Missing planet record pl=%d sect=%d,%d",
			pkey.plnum, pkey.xsect, pkey.ysect));
		return FALSE;
	}
	return TRUE;
}


/**************************************************************************
** is this players ship in the neutral zone ?                            **
**************************************************************************/

int FUNC neutral(COORD *coord)
{
	xsect = coord1(coord->xcoord);
	ysect = coord1(coord->ycoord);
	return xsect == 0 && ysect == 0;
}

/**************************************************************************
** is this coordinate in a nebula ?                                      **
**************************************************************************/

int FUNC innebula(int x, int y)
{
	unsigned dmod;
	unsigned long work;

	if (x == 0 && y == 0)
		return FALSE;

	if (nebodds <= 0)
		return FALSE;

	work = nebseed;
	work ^= (((unsigned long)(unsigned)x) << 16) | (unsigned)y;
	work += ((unsigned long)(unsigned)x * 214013L) + ((unsigned long)(unsigned)y * 2531011L);
	work ^= (work >> 16);

	dmod = (unsigned)(180 / nebodds);
	if (dmod == 0)
		dmod = 1;

	return (unsigned)(work % (unsigned long)dmod) == 0;
}

#ifndef GE_ARENA
void FUNC update_outpost(void)
{
	switch (plptr->outpost_type) {
	case OUTPOST_ZYGOR:
		update_plan_1();
		break;
	case OUTPOST_TAHANIAN:
		update_plan_2();
		break;
	case OUTPOST_ENFORCER:
		update_plan_3();
		break;
	}
}

void FUNC update_plan_1(void)
{
	int i;
	for (i = 0; i < NUMITEMS; ++i)
	{
		plptr->items[i].qty = ULCAP;
		plptr->items[i].sell = 'Y';
		plptr->items[i].markup2a = (baseprice[i]*2) + (gernd() % baseprice[i]);
	}
}

void FUNC update_plan_2(void)
{
	int i;
	for (i = 0; i < NUMITEMS; ++i)
	{
		plptr->items[i].qty = 0;
		plptr->items[i].sell = 'N';
		plptr->items[i].markup2a = (baseprice[i]*2) + (gernd() % baseprice[i]);
	}
	plptr->items[I_MEN].qty = ULCAP;
	plptr->items[I_FOOD].qty = ULCAP;
	plptr->items[I_TROOPS].qty = ULCAP;
	plptr->items[I_MEN].sell = 'Y';
	plptr->items[I_FOOD].sell = 'Y';
	plptr->items[I_TROOPS].sell = 'Y';
}

void FUNC update_plan_3(void)
{
	int i;
	for (i = 0; i < NUMITEMS; ++i)
	{
		plptr->items[i].qty = 0;
		plptr->items[i].sell = 'N';
	}
}
#endif
