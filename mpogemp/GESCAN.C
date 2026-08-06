/*****************************************************************************
 * ge-next GESCAN.C                                                          *
 *                                                                           *
 * ge-next modifications by Anthony Schmidt / ManicPop.org                   *
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
 * Additional Terms for Contributors:                                        *
 * 1. By contributing to this project, you agree to assign all right, title, *
 *    and interest, including all copyrights, in and to your contributions   *
 *    to Rick Hadsall and Elwynor Technologies.                              *
 * 2. You grant Rick Hadsall and Elwynor Technologies a non-exclusive,       *
 *    royalty-free, worldwide license to use, reproduce, prepare derivative  *
 *    works of, publicly display, publicly perform, sublicense, and          *
 *    distribute your contributions                                          *
 * 3. You represent that you have the legal right to make your contributions *
 *    and that the contributions do not infringe any third-party rights.     *
 * 4. Rick Hadsall and Elwynor Technologies are not obligated to incorporate *
 *    any contributions into the project.                                    *
 * 5. This project is licensed under the AGPL v3, and any derivative works   *
 *    must also be licensed under the AGPL v3.                               *
 * 6. If you create an entirely new project (a fork) based on this work, it  *
 *    must also be licensed under the AGPL v3, you assign all right, title,  *
 *    and interest, including all copyrights, in and to your contributions   *
 *    to Rick Hadsall and Elwynor Technologies, and you must include these   *
 *    additional terms in your project's LICENSE file(s).                    *
 *                                                                           *
 * By contributing to this project, you agree to these terms.                *
 *                                                                           *
 *****************************************************************************/

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
#include "geglobal.h"

#define GESCAN 1

/* out of range mask for printmap */
static const int scan_side_blocks[15] =
	{6, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 6};

static int se_nebula = FALSE;
static byte owned_planet[MAXPLANETS];

static void scan_sh(void);
static void scan_pl(void);
static void scan_ra(void);
static void scan_se(void);
static void scan_lo(void);
#ifndef GE_ARENA
static void data_shipscan(void);
static void data_shipscan_systems(WARSHP *wptr);
#endif

/**************************************************************************
** Functions for printmap()                                              **
**************************************************************************/

static void set_map_color(int color)
{
	switch (color) {
	case 1: prf(CLR_RED2); break;
	case 2: prf(CLR_GREEN2); break;
	case 3: prf(CLR_YELLOW2); break;
	case 4: prf(CLR_BLUE2); break;
	case 5: prf(CLR_BLUE1); break;
	case 6: prf(CLR_RED1); break;
	default: prf(CLR_WHITE2); break;
	}
}

static void print_ship_letter(int othusn, char letter)
{
	int type = shipclass[warshpoff(othusn)->shpclass].max_type;

	if (warsptr->jam_sev > (byte)2 && (gernd()%(10 / warsptr->jam_sev)) == (byte)0)
		letter = '?';

	if (letter == '?')
		prf("%s%c", CLR_WHITE2, letter);
	else if (type == CLASSTYPE_CYBORG)
		prf("%s%c%s", CLR_RED2, letter, CLR_WHITE2);
	else if (type == CLASSTYPE_DROID)
		prf("%s%c%s", CLR_BLUE2, letter, CLR_WHITE2);
	else
		prf("%s%c%s", CLR_GREEN2, letter, CLR_WHITE2);
}

static void print_ship_data(long dist, int bearing, int heading, double speed)
{
	int i;

	/* build base string */
	sprintf(gechrbuf, "%s    %4d    %4d%9s", spr("%6ld", dist), bearing, heading, showarp(speed));

	/* heavy jamming: total scramble */
	if (warsptr->jam_sev > (byte)7) {
		for (i = 0; gechrbuf[i]; ++i)
			if (gechrbuf[i] != ' ' && gechrbuf[i] != '-' && gechrbuf[i] != 0)
				gechrbuf[i] = '?';
	}

	/* moderate jamming: speckle randomly */
	else if (warsptr->jam_sev > (byte)2) {
		for (i = 0; gechrbuf[i]; ++i)
			if (gechrbuf[i] != ' ' && gechrbuf[i] != '-' && gechrbuf[i] != 0 && gernd()%(10 - warsptr->jam_sev) == (byte)0)
				gechrbuf[i] = '?';
	}

	prf("%s", gechrbuf);
}

static void print_shiptab_data(SCANTAB *sptr, int shp)
{
	if (sptr->ship[shp].flag == 2)
		prf("%6s    %4s    %4s%9s","?????","????","????","??.??");
	else
		print_ship_data(sptr->ship[shp].dist, sptr->ship[shp].bearing, sptr->ship[shp].heading, sptr->ship[shp].speed);
}


static int build_ship_name(int othusn)
{
	WARSHP *wptr;
	int ulen, slen, show, maxlen;

	wptr = warshpoff(othusn);
	maxlen = 36;
	if ((warsptr->lock == othusn || wptr->distress != 255) && warsptr->jam_sev < (byte)3)
		maxlen -= 2;

	if (wptr->status == GESTAT_AUTO) {
		slen = (int)strlen(wptr->shipname);
		if (slen >= 254)
			slen = 254;
		memcpy(gechrbuf,wptr->shipname,slen);
		gechrbuf[slen] = 0;
		return slen;
	}

	if (wptr->shipname[0] == 0) {
		ulen = (int)strlen(wptr->userid);
		if (ulen >= 254)
			ulen = 254;
		memcpy(gechrbuf,wptr->userid,ulen);
		gechrbuf[ulen] = 0;
		return ulen;
	}

	ulen = (int)strlen(wptr->userid);
	slen = (int)strlen(wptr->shipname);
	if (ulen + slen + 3 <= maxlen) {
		sprintf(gechrbuf,"%s (%s)",wptr->userid,wptr->shipname);
		return ulen + slen + 3;
	}

	show = maxlen - ulen - 6;
	if (show < 1) {
		memcpy(gechrbuf,wptr->userid,ulen);
		gechrbuf[ulen] = 0;
		return ulen;
	}
	memcpy(gechrbuf2,wptr->shipname,show);
	gechrbuf2[show] = 0;
	sprintf(gechrbuf,"%s (%s...)",wptr->userid,gechrbuf2);
	return((int)strlen(gechrbuf));
}

static void print_ship_name(int othusn)
{
	int len, k;
	unsigned int rseed = gernd();

	/* user ships show userid, or userid plus ship name if one exists */
	len = build_ship_name(othusn);

	if (warsptr->jam_sev > (byte)7)
		for (k = 0; k < len; k++)
			gechrbuf[k] = '?';
	else if (warsptr->jam_sev > (byte)2)
		for (k = 0; k < len; k++) {
			rseed ^= rseed << 7;
			rseed ^= rseed >> 9;
			rseed ^= rseed << 8;
			if (rseed%(9 - warsptr->jam_sev) == (byte)0)
				gechrbuf[k] = '?';
		}

	if (warsptr->lock == othusn && warsptr->jam_sev < (byte)3)
		prf("%s*%s%s%s*", CLR_RED1, CLR_CYAN1, gechrbuf, CLR_RED1);
	else if (warshpoff(othusn)->distress != 255 && warsptr->jam_sev < (byte)3)
		prf("%s*%s%s%s*", CLR_GREEN2, CLR_CYAN1, gechrbuf, CLR_GREEN2);
	else
		prf(" %s%s", CLR_CYAN1, gechrbuf);
}

static void scramble_tail_digits(char *buf, unsigned int seed)
{
	int digits, i, len, limit;

	limit = (((int)warsptr->jam_sev < 6)
		? (int)warsptr->jam_sev / 2 : (int)warsptr->jam_sev - 2) + 1;
	digits = seed % limit;
	len = strlen(buf);
	if (digits > len)
		digits = len;
	for (i = len - digits; i < len; ++i)
		if (buf[i] != '-')
			buf[i] = '?';
}

static int print_planet_line(int shp)
{
	unsigned int rseed = gernd();
	int orbit = (warsptr->where - 11 == shp);
	int owned = owned_planet[shp];

	if (se_nebula && cdistance(&warsptr->coord,&ptab[usrnum].planets[shp].coord)*10000.0 > (double)NEBRNG && !orbit && !owned)
		return FALSE;

	if (rseed%(warsptr->jam_sev+1) > (byte)6 && !orbit)
		return FALSE;
	if (rseed%(warsptr->jam_sev+1) > (byte)3 && !orbit) {
		prf(CLR_WHITE2);
		sprintf(gechrbuf,"%c",'?');
	}
	else {
		if (ptab[usrnum].planets[shp].type == PLTYPE_WORM)
			prf(CLR_YELLOW2);
		else
			prf(CLR_BLUE1);
		sprintf(gechrbuf,"%d",shp + 1);
	}

	sprintf(gechrbuf2, "%d", (int)(cdistance(&warsptr->coord, &ptab[usrnum].planets[shp].coord) * 10000));
	sprintf(gechrbuf3, "%d", cbearing(&warsptr->coord, &ptab[usrnum].planets[shp].coord, warsptr->heading));

	/* if jammed, mess up the numbers */
	if (warsptr->jam_sev > (byte)2 && warsptr->where - 11 != shp) {
		scramble_tail_digits(gechrbuf2,rseed);
		scramble_tail_digits(gechrbuf3,rseed >> 4);
	}

	prf("     %s%s %8s %7s", gechrbuf, CLR_WHITE2, gechrbuf2, gechrbuf3);

	if (warsptr->where - 11 == shp)
		prf("   (orbiting)");
	return TRUE;
}

static void print_map_header(int maptype)
{
	switch (maptype) {
	case SECTORNOMAP:
		/* if no planets in ptab, don't print header */
		{
			if (ptab[usrnum].planets[0].type != 0)
				prfmsg(NOMAPSE);
			break;
		}
	case RANGENOMAP:
		/* if no ships in scantab, don't print header */
		{
			if (scantab[usrnum].ship[0].flag != 0)
				prfmsg(NOMAPRA);
			break;
		}
	case SECTORFULL:
		{
			int i, vispl = FALSE;

			if (!se_nebula)
				vispl = (ptab[usrnum].planets[0].type != 0);
			else {
				for (i=0; i<MAXPLANETS && ptab[usrnum].planets[i].type != 0; ++i) {
					if (cdistance(&warsptr->coord,&ptab[usrnum].planets[i].coord)*10000.0 <= (double)NEBRNG
						|| owned_planet[i]) {
						vispl = TRUE;
						break;
					}
				}
			}

			if (vispl)
				prfmsg(PLUSSECT);
			else
				prfmsg(PLUSDASH);
			break;
		}
	case RANGEFULL:
	case RANGENAMES:
	case RANGEEXTRA: prfmsg(PLUSFULL); break;
	default:
		prfmsg(PLUSDASH);
		break;
	}
	prf("\r");
}

static void print_map_row(int i, int maptype)
{
	int j = 0, start;
	int prev_color = 6, cur_color;
	unsigned int rseed = gernd();

	prf("%s   |", CLR_RED1);
	while (j < MAXX) {
		/* add red out of range highlighting */
		if (maptype >= RANGE && maptype <= RANGEEXTRA &&
			(j < scan_side_blocks[i] || j >= MAXX - scan_side_blocks[i]))
		{
			start = j;
			while (j < MAXX && (j < scan_side_blocks[i] || j >= MAXX - scan_side_blocks[i]))
				j++;

			if (prev_color != 6) {
				prf(CLR_RED1);
				prev_color = 6;
			}
			prf("%s", gedots(j - start));
		} else if (warsptr->jam_sev > (byte)0) {
			if (!(map[i][j] == '.' && mapc[i][j] == '4') && map[i][j] != '*') /* don't overwrite out of bounds or player indicator */
			{
				/* cheap xorshift random, avoid a gazillion gernd calls */
				rseed ^= rseed << 7;
				rseed ^= rseed >> 9;
				rseed ^= rseed << 8;
				if (map[i][j] != ' ') {
					if (rseed%(warsptr->jam_sev) > (byte)4) /* jam level increases chance of legit object blanked */
						map[i][j] = ' ';
				}
				if (map[i][j] == ' ') { /* scramble empty spaces and blanked objects */
					if (rseed%(80/warsptr->jam_sev) == (byte)0) {
						map[i][j] = (byte)((rseed%94)+33); /* printable ascii range */
						if (maptype != MAP_LONG && rseed%6 == 0)	/* add color speckles if not sca lo */
							mapc[i][j] = (byte)((rseed%7)+48); /* ascii 0-6 */
					}
				}
			}
		}
			if (map[i][j] != ' ') { /* if empty space, color code change not needed */
				switch (mapc[i][j]) {
				case '1': cur_color = 1; break;
				case '2': cur_color = 2; break;
				case '3': cur_color = 3; break;
				case '4': cur_color = 4; break;
				case '5': cur_color = 5; break;
				case '6': cur_color = 6; break;
				default: cur_color = 0; break;
				}
				if (prev_color != cur_color) {
					set_map_color(cur_color);
					prev_color = cur_color;
				}
			}

		prf("%c", map[i][j]);
		j++;
	}

	if (prev_color != 6)
		prf(CLR_RED1);
	prf("|");
}

/* RANGEEXTRA / RANGENOMAP pairs */
static void print_ship_pair(SCANTAB *sptr, int left, int right, long dist_filter)
{
	int i, pad, othusn;

	othusn = sptr->ship[left].shipno;
	prf("   ");
	print_ship_letter(othusn, sptr->ship[left].letter);
	prf("   ");
	print_shiptab_data(sptr,left);

	if (right < NOSCANTAB && sptr->ship[right].flag != 0 && (long)(sptr->ship[right].dist) < dist_filter) {
		othusn = sptr->ship[right].shipno;
		prf("  ");
		print_ship_letter(othusn, sptr->ship[right].letter);
		prf("   ");
		print_shiptab_data(sptr,right);
	}
	prf("\r");

	othusn = sptr->ship[left].shipno;
	pad = build_ship_name(othusn);
	prf("   ");
	print_ship_name(othusn);
	for (i = 0; i < 36 - pad; ++i)
		prf(" ");

	if (right < NOSCANTAB && sptr->ship[right].flag != 0 && (long)(sptr->ship[right].dist) < dist_filter) {
		othusn = sptr->ship[right].shipno;
		print_ship_name(othusn);
	}

	prf("\r");
}

/* RANGENAMES (or first eight of RANGEEXTRA) */
static int print_range_line(SCANTAB *sptr, int shp, int *ff, long dist_filter)
{
	int othusn;

	if (shp < NOSCANTAB && sptr->ship[shp].flag != 0 &&
		shp < (MAXY + 1) / 2 && (long)(sptr->ship[shp].dist) < dist_filter) {
		othusn = sptr->ship[shp].shipno;
		prf("     ");

		if (*ff == 0) {
			print_ship_letter(othusn, sptr->ship[shp].letter);
			prf("   ");
			print_shiptab_data(sptr,shp);
			*ff = 1;
		} else {
			print_ship_name(othusn);
			*ff = 0;
			return 1; /* move onto next ship */
		}
	}
	return 0; /* first line or no ship, don't increment */
}

/* RANGEFULL */
static int print_fullrange_line(SCANTAB *sptr, int shp, long dist_filter)
{
	int othusn;

	if (shp < NOSCANTAB && sptr->ship[shp].flag != 0 && (long)(sptr->ship[shp].dist) < dist_filter) {
		othusn = sptr->ship[shp].shipno;
		prf("     ");
		print_ship_letter(othusn, sptr->ship[shp].letter);
		prf("   ");
		print_shiptab_data(sptr,shp);
		return 1; /* move onto next ship */
	}
	return 0; /* no ship, don't increment */
}

/* RANGEEXTRA / RANGENOMAP */
static void print_range_summary(SCANTAB *sptr, int shp, long dist_filter, int maptype)
{
	byte visible[NOSCANTAB];
	int count = 0;
	int i;

	/* build a list of ships to show starting from current shp */
	for (; shp < NOSCANTAB; ++shp) {
		if (sptr->ship[shp].flag == 0)
			continue;
		if ((long)(sptr->ship[shp].dist) >= dist_filter)
			continue;
		/* heavy jamming: random chance to skip ship */
		if (warsptr->jam_sev > (byte)7 && (gernd() % 2 == 0))
			continue;

		visible[count++] = shp;
	}

	for (i = 0; i < count; i += 2) {
		int left = visible[i];
		int right = (i + 1 < count) ? visible[i + 1] : -1;
		print_ship_pair(sptr, left, right, dist_filter);
	}

	if (maptype == RANGENOMAP && count == 0 && warsptr->jam_sev <= (byte)7)
		prfmsg(SCANNOSH);
}

/* SECTORNOMAP */
static void print_planet_summary(int shp)
{
	for (; shp < MAXPLANETS && ptab[usrnum].planets[shp].type != 0; ++shp) {
		if (print_planet_line(shp) == TRUE)
			prf("\r");
	}

	if (shp == 0) /* no planets */
		prfmsg(SCANNOPL);
}

static void jam_scramble(char *buf, byte sev, unsigned int *rseed)
{
	int i;

	for (i = 0; buf[i]; ++i) {
		*rseed ^= *rseed << 7;
		*rseed ^= *rseed >> 9;
		*rseed ^= *rseed << 8;
		if (buf[i] != ' ' && buf[i] != '-' && (*rseed % (9 - sev) == 0))
			buf[i] = '?';
	}
}

static void map_planets(void)
{
	int i;
	unsigned x, y;

	getsector(&warsptr->coord);
	for (i = 0; i < sector.numplan; ++i) {
		if (sector.ptab[i].coord.xcoord != 0) {
			if (se_nebula && cdistance(&warsptr->coord,&sector.ptab[i].coord)*10000.0 > (double)NEBRNG
				&& !owned_planet[i])
				continue;
			x = coord2(sector.ptab[i].coord.xcoord)+25;
			y = coord2(sector.ptab[i].coord.ycoord)+25;
			if (map[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] == '*')
				mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '7';
			map[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '1' + i;
			if (mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] != '7') {
				if (sector.ptab[i].type == PLTYPE_WORM)
					mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '3';
				else
					mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '5';
			}
		}
	}
}

static void clearmap(void)
{
	int i, j;

	for (i = 0; i < MAXY; ++i) {
		for (j = 0; j < MAXX; ++j) {
			map[i][j] = ' ';
			mapc[i][j] = ' ';
		}
		map[i][MAXX] = 0;
		mapc[i][MAXX] = 0;
	}
}

/* SCAN SHIP FUNCTION */

void FUNC cmd_scan(void)
{
	if (warsptr->tactical != 0) {
		prfmsg(TABROKE);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->damage >= 100.0) {
		prfmsg(RNDTACT);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc > 1) {
		if (sameto("sh",margv[1]))
			scan_sh();
		else if (sameto("pl",margv[1]))
			scan_pl();
		else if (sameto("ra",margv[1]))
			scan_ra();
		else if (sameto("se",margv[1]))
			scan_se();
		else if (sameto("lo",margv[1]))
			scan_lo();
		else {
			prfmsg(FORMAT,"SCAN");
			outprfge(FLT_NONE,usrnum);
		}
	}
	else {
		prfmsg(FORMAT,"SCAN");
		outprfge(FLT_NONE,usrnum);
	}
}

/**************************************************************************
** Print the map                                                         **
**************************************************************************/

static void printmap(int maptype, long dist_filter)
{
	SCANTAB *sptr = &scantab[usrnum];
	int i, shp = 0, ff = 0;

	print_map_header(maptype);
	outprfge(FLT_NONE, usrnum);

	for (i = 0; i < MAXY + 1; ++i) {
		if (maptype == SECTORNOMAP || maptype == RANGENOMAP)
			continue;

		if (i == MAXY)
			prfmsg(PLUSDASH);
		else
			print_map_row(i, maptype);

		if (maptype == SECTORFULL) {
			while (shp < MAXPLANETS && ptab[usrnum].planets[shp].type != 0 && print_planet_line(shp) == FALSE)
				shp++;  /* skip jammed planet and immediately try next one */

			shp++;
		}

		if (maptype == RANGENAMES || maptype == RANGEEXTRA) {
			while (ff == 0 && warsptr->jam_sev > (byte)7 && gernd()%2 == 0)
				shp++;
			shp += print_range_line(sptr, shp, &ff, dist_filter);
		}

		if (maptype == RANGEFULL) {
			while (ff == 0 && warsptr->jam_sev > (byte)7 && gernd()%2 == 0)
				shp++;
			shp += print_fullrange_line(sptr, shp, dist_filter);
		}

		prf("\r");
	}

	if (maptype == RANGEEXTRA || maptype == RANGENOMAP)
		print_range_summary(sptr, shp, dist_filter, maptype);

	if (maptype == SECTORNOMAP)
		print_planet_summary(shp);

	prf("\r");
	prf(CLR_WHITE2);
}

static void scan_sh(void)
{
	int shpnum, gheading;
	WARSHP *wptr;
	WARUSR *wuptr;
	char ltr;
	unsigned int rseed = gernd();
	long scandist;	/* update_scantab uses ddistance, so we need a local here */
	int nebmask, target_neb;

	if (margc != 3) {
		prfmsg(FORMAT,"SCAN");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->jam_sev > (byte)7 || (warsptr->jam_sev > (byte)2 && rseed%(9 - (int)warsptr->jam_sev) == 0)) {
		prfmsg(JAMMER4);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	setsect(warsptr);
	nebmask = innebula(xsect,ysect);

	shpnum = findshp(margv[2],1);

	if (shpnum < 0 && margv[2][0] == '@') {
		if (warsptr->lock >= 0 && warsptr->lock < nships && ingegame(warsptr->lock))
			prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (shpnum >= 0) {
		wptr = warshpoff(shpnum);
		scandist = cdistance(&warsptr->coord,&wptr->coord)*10000;
		target_neb = innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
		if ((nebmask || target_neb) && !(nebmask && target_neb && scandist < (long)NEBRNG)) {
			if (nebmask)
				prfmsg(SCAN27);
			else
				prfmsg(NOSHIP);
			outprfge(FLT_NONE,usrnum);
			return;
		}
	} else if (nebmask) {
		prfmsg(SCAN27);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	/* you shouldn't be able to scan yourself */
	if (shpnum == usrnum) {
		/* stale lock points to self, clear it and pretend it never happened */
		if (margv[2][0] == '@') {
			warsptr->lock = -1;
			warsptr->track_grace = 0;
			prfmsg(NOLOCK);
		}
		/* this shouldn't happen, you should not be in your own scantab */
		else
			prfmsg(NOSHIP);
		outprfge(FLT_NONE,usrnum);
		return;
	} else if (shpnum >= 0) {
		char *tname;

		wptr = warshpoff(shpnum);
		wuptr = warusroff(shpnum);
		scandist = cdistance(&warsptr->coord,&wptr->coord)*10000;
		if (scandist < ship_scanrange(warsptr)) {
			bearing = cbearing(&warsptr->coord,&wptr->coord,warsptr->heading);
			heading = cbearing(&wptr->coord,&warsptr->coord,wptr->heading);
			gheading = (int)(wptr->heading + .5);
			speed = ((unsigned)(wptr->speed + .5));

			if (wptr->status == GESTAT_USER && wptr->shipname[0] == 0)
				sprintf(gechrbuf, "%s", wuptr->userid);
			else
				sprintf(gechrbuf, "%s", wptr->shipname);

			if (warsptr->jam_sev > 2)
				jam_scramble(gechrbuf, warsptr->jam_sev, &rseed);
			if (wptr->status == GESTAT_AUTO)
				prfmsg(SCAN01N, gechrbuf);
			else if (wptr->shipname[0] == 0)
				prfmsg(SCAN01O, gechrbuf);
			else
				prfmsg(SCAN01, gechrbuf);
			prfmsg(DASHES);
			if (warsptr->jam_sev < (byte)3)
				prfmsg(SCAN01A, shipclass[wptr->shpclass].typename, showupg(wptr));
			if (wptr->status == GESTAT_USER && warsptr->jam_sev < (byte)3) {
				if (wptr->shipname[0] != 0)
					prfmsg(SCAN02, username(wptr));
				if (warusroff(shpnum)->teamcode > 0) {
					tname = teamname(wuptr);
					if (tname != NULL)
						prfmsg(SCAN02A, tname);
				}
			}

			memset(gechrbuf, 0, 255);
			sprintf(gechrbuf, "%d", bearing);
			sprintf(gechrbuf2, "%d", heading);
			sprintf(gechrbuf3, "%ld", (long)scandist);
			if (warsptr->jam_sev > (byte)2) {
				jam_scramble(gechrbuf, warsptr->jam_sev, &rseed);
				jam_scramble(gechrbuf2, warsptr->jam_sev, &rseed);
				jam_scramble(gechrbuf3, warsptr->jam_sev, &rseed);
			}
			prfmsg(SCAN03, gechrbuf, gechrbuf2, gechrbuf3);
			setsect(wptr);
			sprintf(gechrbuf, "%d", gheading);
			sprintf(gechrbuf2, "%d %d", xsect, ysect);
			sprintf(gechrbuf3, "%s", showarp(wptr->speed));
			if (warsptr->jam_sev > 2) {
				jam_scramble(gechrbuf, warsptr->jam_sev, &rseed);
				jam_scramble(gechrbuf2, warsptr->jam_sev, &rseed);
				jam_scramble(gechrbuf3, warsptr->jam_sev, &rseed);
			}
			prfmsg(SCAN03A, gechrbuf, gechrbuf2);
			prfmsg(SCAN04, gechrbuf3);

			if (warsptr->jam_sev < (byte)3) {
				if ((warsptr->upgrade & ENHSCAN) || (warsptr->where != 1 && wptr->where != 1)) {
					damage = (unsigned)(wptr->damage + .5);
					damstr(damage);
					prfmsg(SCAN05, gechrbuf);
				}

				if (!(warsptr->upgrade & ENHSCAN) && warsptr->where != 1 && wptr->where != 1) {
					if (wptr->shieldstat == SHIELDUP)
						prfmsg(SCAN06);
					else
						prfmsg(SCAN07);
				}

				if ((warsptr->upgrade & ENHSCAN) && wptr->where != 1) {
					if (shipclass[wptr->shpclass].max_shlds != 0) {
						if (wptr->shieldstat == SHIELDUP)
							prfmsg(SCAN06M, wptr->shieldtype);
						else
							prfmsg(SCAN07M, wptr->shieldtype);
					}

					if (shipclass[wptr->shpclass].max_phasr != 0) {
						if (wptr->phasr >= PMINFIRE)
							prfmsg(SCAN19, wptr->phasrtype);
						else if (wptr->phasr >= 0)
							prfmsg(SCAN21, wptr->phasrtype);
						else
							prfmsg(SCAN20, wptr->phasrtype);
					}

					show_rep_sysdam(wptr,FALSE);
				}

				if (wptr->status == GESTAT_AUTO)
					prfmsg(SCAN08, wptr->kills);
				else {
					if (wptr->shipname[0] == 0)
						prfmsg(SCAN09, "this ship", wptr->kills, wptr->ukills);
					else
						prfmsg(SCAN09, wptr->shipname, wptr->kills, wptr->ukills);
					prfmsg(SCAN09, wuptr->userid, wuptr->kills, wuptr->ukills);
				}
			}

			prfmsg(DASHES);
			outprfge(FLT_NONE, usrnum);

			/* if beyond the "scanned" ships range display this msg */
			/* if scanner is already locked onto target, suppress scan notification */
			if (warsptr->jam_sev < (byte)3 && warsptr->lock != shpnum) {
				if ((long)scandist > ship_scanrange(wptr)) {
					bearing = cbearing(&wptr->coord, &warsptr->coord, wptr->heading);
					prfmsg(SCAN2, bearing);
				} else {
					/* all else get this */
					if (warsptr->cloak != 10) {
						ltr = shpltr(shpnum, usrnum);
						if (warsptr->shipname[0] == 0)
							prfmsg(SCAN1O, ltr, waruptr->userid);
						else
							prfmsg(SCAN1, ltr, warsptr->shipname);
					} else
						prfmsg(SCAN3);
				}
				outprfge(FLT_NONE, shpnum);
			}
		} else {
			prfmsg(NOSHIP);
			outprfge(FLT_NONE, usrnum);
		}
	} else {
		prfmsg(NOSHIP);
		outprfge(FLT_NONE, usrnum);
	}
}

#ifdef GE_ARENA
static void arena_scan_item(int item)
{
	if (plptr->items[item].qty == 0)
		return;
	sprintf(gechrbuf,"%s%s%12lu",item_name[item],
	    gedots(26 - (int)strlen(item_name[item])),plptr->items[item].qty);
	gechrbuf[0] = (char)toupper((unsigned char)gechrbuf[0]);
	prf("%s\r",gechrbuf);
}

static void arena_scan_powerup(char *name, int value)
{
	if (value > 0) {
		sprintf(gechrbuf,"%d",value);
		prf("%s%s%12s\r",name,gedots(26 - (int)strlen(name)),gechrbuf);
	}
	else
		prf("%s%s%12d\r",name,gedots(26 - (int)strlen(name)),1);
}

static void arena_scan_planet_contents(void)
{
	arena_scan_item(I_TORPEDO);
	arena_scan_item(I_MISSILE);
	arena_scan_item(I_MINE);
	arena_scan_item(I_JAMMERS);
	arena_scan_item(I_DECOYS);
	arena_scan_item(I_ZIPPERS);
	arena_scan_item(I_FLUXPOD);
	if (arena_mode == ARENA_MODE_HOARD)
		arena_scan_item(I_GOLD);
	if (plptr->arena_shield_boost)
		arena_scan_powerup("Shield booster",plptr->arena_shield_boost);
	if (plptr->arena_phaser_boost)
		arena_scan_powerup("Phaser booster",plptr->arena_phaser_boost);
	if (plptr->arena_flags & ARENA_PL_SCAN)
		arena_scan_powerup("Enhanced scanners",0);
	if (plptr->arena_flags & ARENA_PL_ARMOR)
		arena_scan_powerup("Armor plating",0);
	if (plptr->arena_flags & ARENA_PL_ACCEL)
		arena_scan_powerup("Acceleration booster",0);
	if (plptr->arena_flags & ARENA_PL_CORE)
		arena_scan_powerup("Reinforced neutron core",0);
	if (plptr->arena_flags & ARENA_PL_INSTANT)
		prf("%s%s%12d\r","Instant maintenance",
		    gedots(26 - (int)strlen("Instant maintenance")),1);
	if (plptr->arena_flags & ARENA_PL_MAINT)
		prf("Maintenance depot available\r");
}
#endif

static void scan_pl(void)
{
#ifndef GE_ARENA
	unsigned i;
#endif
	unsigned int rseed = gernd();
	int nebmask;

	/* SCAN PLANET FUNCTION */

	if (margc != 3) {
		prfmsg(FORMAT,"SCAN");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (warsptr->jam_sev > (byte)7 || (warsptr->jam_sev > (byte)2 && rseed%(9 - (int)warsptr->jam_sev) == 0)) {
		prfmsg(JAMMER4);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	plnum = atoi(margv[2]);

	setsect(warsptr);
	nebmask = innebula(xsect,ysect);

	if (plnum <= MAXPLANETS && plnum > 0) {
		if (!getplanetdat(usrnum)) {
			if (nebmask)
				prfmsg(SCAN26);
			else
				prfmsg(NOPLNT);
			outprfge(FLT_NONE,usrnum);
			return;
		}
		refresh(warsptr,usrnum);
		if (nebmask) {
			if (plnum > sector.numplan) {
				prfmsg(SCAN26);
				outprfge(FLT_NONE,usrnum);
				return;
			}
			if (cdistance(&warsptr->coord,&plptr->coord)*10000.0 > (double)NEBRNG) {
				if (sameas(plptr->userid,warsptr->userid))
					prfmsg(SCAN27);
				else
					prfmsg(SCAN26);
				outprfge(FLT_NONE,usrnum);
				return;
			}
		}
		if (plnum > sector.numplan) {
			prfmsg(NOPLNT);
			outprfge(FLT_NONE,usrnum);
		} else if (plptr->type == PLTYPE_PLNT) {
			bearing = cbearing(&warsptr->coord,&plptr->coord,warsptr->heading);
			ddistance = cdistance(&warsptr->coord,&plptr->coord)*10000;
			memset(gechrbuf, 0, 255);
			sprintf(gechrbuf,"%s",plptr->name);
			if (warsptr->jam_sev > (byte)2 && warsptr->where - 10 != plnum)
				jam_scramble(gechrbuf, warsptr->jam_sev, &rseed);
			prfmsg(SCAN10,plnum,gechrbuf);
			prfmsg(DASHES);

			if (plptr->userid[0] != 0 && (warsptr->jam_sev < (byte)3 || warsptr->where - 10 == plnum))
				prfmsg(SCAN11,plptr->userid);

			memset(gechrbuf2, 0, 20);
			memset(gechrbuf3, 0, 20);
			sprintf(gechrbuf2,"%d",bearing);
			sprintf(gechrbuf3,"%ld",(long)ddistance);
			if (warsptr->jam_sev > (byte)2 && warsptr->where - 10 != plnum) {
				jam_scramble(gechrbuf2, warsptr->jam_sev, &rseed);
				jam_scramble(gechrbuf3, warsptr->jam_sev, &rseed);
			}
			prfmsg(SCAN12,gechrbuf2,gechrbuf3);

#ifdef GE_ARENA
			if (warsptr->where != 1 &&
			    (warsptr->jam_sev < (byte)3 || warsptr->where - 10 == plnum))
				arena_scan_planet_contents();
#else
			if (warsptr->where != 1 && (warsptr->jam_sev < (byte)3 || warsptr->where - 10 == plnum)) {
				prfmsg(SCAN13);
				if (plptr->enviorn == 0)
					prfmsg(SCAN14);
				else if (plptr->enviorn == 1)
					prfmsg(SCAN15);
				else if (plptr->enviorn == 2)
					prfmsg(SCAN16);
				else if (plptr->enviorn == 3)
					prfmsg(SCAN17);

				prfmsg(SCAN18);
				if (plptr->resource == 0)
					prfmsg(SCAN14);
				else if (plptr->resource == 1)
					prfmsg(SCAN15);
				else if (plptr->resource == 2)
					prfmsg(SCAN16);
				else if (plptr->resource == 3)
					prfmsg(SCAN17);

				if (sameas(plptr->userid,warsptr->userid) || (plptr->userid[0] == 0 && warsptr->where - 10 == plnum)) {
					for (i=0; i<NUMITEMS; ++i) {
						if (plptr->items[i].qty > 0) {
							sprintf(gechrbuf,"%s%s%12lu",item_name[i],gedots(26-strlen(item_name[i])),plptr->items[i].qty);
							gechrbuf[0] = (char)toupper((unsigned char)gechrbuf[0]);
							prf("%s\r",gechrbuf);
						}
					}
				} else {
					if (plptr->items[I_MEN].qty + plptr->items[I_TROOPS].qty == 0)
						strcpy(gechrbuf,"None");
					else if (plptr->items[I_MEN].qty + plptr->items[I_TROOPS].qty < 100)
						strcpy(gechrbuf,"Less than 100");
					else if (plptr->items[I_MEN].qty + plptr->items[I_TROOPS].qty < 1000)
						strcpy(gechrbuf,"Hundreds");
					else if (plptr->items[I_MEN].qty + plptr->items[I_TROOPS].qty < 10000)
						strcpy(gechrbuf,"Thousands");
					else if (plptr->items[I_MEN].qty + plptr->items[I_TROOPS].qty < 100000L)
						strcpy(gechrbuf,"Tens of thousands");
					else if (plptr->items[I_MEN].qty + plptr->items[I_TROOPS].qty < 1000000L)
						strcpy(gechrbuf,"Hundreds of thousands");
					else
						strcpy(gechrbuf,"Millions");
					prfmsg(SCAN28,gechrbuf);

					if (plptr->items[I_MISSILE].qty == 0)
						strcpy(gechrbuf,"No");
					else if (plptr->items[I_MISSILE].qty < 250)
						strcpy(gechrbuf,"Small");
					else if (plptr->items[I_MISSILE].qty < 1000)
						strcpy(gechrbuf,"Moderate");
					else
						strcpy(gechrbuf,"Large");
					prfmsg(SCAN29,gechrbuf);

					if (plptr->items[I_TORPEDO].qty == 0)
						strcpy(gechrbuf,"No");
					else if (plptr->items[I_TORPEDO].qty < 250)
						strcpy(gechrbuf,"Small");
					else if (plptr->items[I_TORPEDO].qty < 1000)
						strcpy(gechrbuf,"Moderate");
					else
						strcpy(gechrbuf,"Large");
					prfmsg(SCAN30,gechrbuf);

					if (plptr->items[I_FLUXPOD].qty == 0)
						strcpy(gechrbuf,"No");
					else if (plptr->items[I_FLUXPOD].qty < 250)
						strcpy(gechrbuf,"Small");
					else if (plptr->items[I_FLUXPOD].qty < 1000)
						strcpy(gechrbuf,"Moderate");
					else
						strcpy(gechrbuf,"Large");
					prfmsg(SCAN33,gechrbuf);

					if (plptr->items[I_MINE].qty == 0)
						strcpy(gechrbuf,"No");
					else if (plptr->items[I_MINE].qty < 250)
						strcpy(gechrbuf,"Small");
					else if (plptr->items[I_MINE].qty < 1000)
						strcpy(gechrbuf,"Moderate");
					else
						strcpy(gechrbuf,"Large");
					prfmsg(SCAN34,gechrbuf);

					if (plptr->items[I_FIGHTER].qty == 0)
						prfmsg(SCAN31);
					else
						prfmsg(SCAN32);
				}
			}
#endif
				prfmsg(DASHES);
				outprfge(FLT_NONE,usrnum);
		} else if (plptr->type == PLTYPE_WORM) {
			memcpy(&worm,plptr,sizeof(GALWORM));
			bearing = cbearing(&warsptr->coord,&worm.coord,warsptr->heading);
			ddistance = cdistance(&warsptr->coord,&worm.coord)*10000;
			prfmsg(SCANWRM,plnum,worm.name);
			prfmsg(DASHES);

			memset(gechrbuf2, 0, 20);
			memset(gechrbuf3, 0, 20);
			sprintf(gechrbuf2,"%d",bearing);
			sprintf(gechrbuf3,"%ld",(long)ddistance);
			if (warsptr->jam_sev > (byte)2) {
				jam_scramble(gechrbuf2, warsptr->jam_sev, &rseed);
				jam_scramble(gechrbuf3, warsptr->jam_sev, &rseed);
			}
			prfmsg(SCAN12,gechrbuf2,gechrbuf3);
			if ((warsptr->upgrade & ENHSCAN) && warsptr->jam_sev < (byte)3) {
				if (coord1(worm.destination.xcoord) < 0)
					gechrbuf[0] = '-';
				else if (coord1(worm.destination.xcoord) > 0)
					gechrbuf[0] = '+';
				else
					gechrbuf[0] = '0';
				if (coord1(worm.destination.ycoord) < 0)
					gechrbuf[1] = '-';
				else if (coord1(worm.destination.ycoord) > 0)
					gechrbuf[1] = '+';
				else
					gechrbuf[1] = '0';
				gechrbuf[2] = 0;
				prfmsg(SCAN35,gechrbuf);
			}
			prfmsg(DASHES);
			outprfge(FLT_NONE,usrnum);
		} else {
			prfmsg(NOPLNT);
			outprfge(FLT_NONE,usrnum);
		}
	} else {
		prfmsg(NOPLNT);
		outprfge(FLT_NONE,usrnum);
	}
}

static void scan_ra(void)
{
	int i, x, y;
	double xf, yf, x1, y1, range;
	double minx, miny;
	double xfactor, yfactor;
	double origin_x, origin_y;
	double cell_left, cell_right, cell_top, cell_bottom;
	double gal_min, gal_max;
	double shiftx = 0.0, shifty = 0.0;

	WARSHP *wptr;
	MINE *mptr;

	if (margc < 2 || margc > 3) {
		prfmsg(FORMAT, "SCAN");
		outprfge(FLT_NONE, usrnum);
		return;
	}

	setsect(warsptr);

	if (innebula(xsect,ysect)) {
		prfmsg(SCAN27);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 3 && sameto("h",margv[2]))
		range = (double)(ship_scanrange(warsptr) / 2);
	else if (margc == 3 && sameto("q",margv[2]))
		range = (double)(ship_scanrange(warsptr) / 4);
	else {
		if (margc == 2)
			x = 9;
		else
			x = atoi(margv[2]);
		if (x < 1 || x > 9)
			x = 9;

		range = (double)(ship_scanrange(warsptr) / ((10 - x) * (10 - x)));
	}

	prfmsg(SCAN24, spr("%ld", (long)range), xsect, ysect);

	clearmap();
	update_scantab(warsptr, usrnum);

	x1 = warsptr->coord.xcoord;
	y1 = warsptr->coord.ycoord;

	xfactor = range / 5000.0 / (double)MAXX;
	yfactor = range / 5000.0 / (double)MAXY;

	minx = x1 - range / 10000.0;
	miny = y1 - range / 10000.0;

	gal_min = -((double)univmax);
	gal_max = (double)univmax + 1.0;

	/* determine shift for X if player would fall one left of center */
	if ((int)((x1 - minx) / xfactor) == (MAXX/2) - 1)
		shiftx = xfactor;

	/* determine shift for Y if player would fall one above center */
	if ((int)((y1 - miny) / yfactor) == (MAXY/2) - 1)
		shifty = yfactor;

	origin_x = minx - shiftx;
	origin_y = miny - shifty;

	/* mark out-of-bounds areas */
	for (y = 0; y < MAXY; y++) {
		for (x = 0; x < MAXX; x++) {
			cell_left = origin_x + x * xfactor;
			cell_right = cell_left + xfactor;
			cell_top = origin_y + y * yfactor;
			cell_bottom = cell_top + yfactor;

			if (cell_right <= gal_min || cell_left >= gal_max ||
				cell_bottom <= gal_min || cell_top >= gal_max) {
				map[y][x] = '.';
				mapc[y][x] = '4';
			}
		}
	}

	/* plot mines */
	for (i = 0, mptr = mines; i < nummines; ++mptr, ++i) {
		if (mptr->channel != MINE_UNUSED) {
			xf = ((mptr->coord.xcoord - x1) / xfactor) + ((double)MAXX)/2.0;
			yf = ((mptr->coord.ycoord - y1) / yfactor) + ((double)MAXY)/2.0;

			xf += shiftx / xfactor;
			yf += shifty / yfactor;

			if (xf >= 0.0 && xf < (double)MAXX && yf >= 0.0 && yf < (double)MAXY) {
				x = (int)xf;
				y = (int)yf;
				map[y][x] = '.';
				mapc[y][x] = '0';
			}
		}
	}

	/* plot ships from scantab */
	for (i = 0; i < NOSCANTAB; i++) {
		othusn = scantab[usrnum].ship[i].shipno;
		if (scantab[usrnum].ship[i].flag == 1) {
			wptr = warshpoff(othusn);

			xf = ((wptr->coord.xcoord - x1) / xfactor) + ((double)MAXX)/2.0 + shiftx / xfactor;
			yf = ((wptr->coord.ycoord - y1) / yfactor) + ((double)MAXY)/2.0 + shifty / yfactor;

			if (xf >= 0.0 && xf < (double)MAXX && yf >= 0.0 && yf < (double)MAXY) {
				x = (int)xf;
				y = (int)yf;
				/* don't replace locked ship on map */
				if (mapc[y][x] != '6') {
					map[y][x] = scantab[usrnum].ship[i].letter;

					if (warsptr->lock == othusn)
						mapc[y][x] = '6';
					else if (shipclass[wptr->shpclass].max_type == CLASSTYPE_CYBORG)
						mapc[y][x] = '1';
					else if (shipclass[wptr->shpclass].max_type == CLASSTYPE_DROID)
						mapc[y][x] = '4';
					else
						mapc[y][x] = '2';
				}
			}
		}
	}

	/* plot player at center */
	map[MAXY/2][MAXX/2] = '*';
	mapc[MAXY/2][MAXX/2] = '0';

	if (waruptr->options[SCANOPTS] == FULL)
		printmap(RANGEFULL,(long)range);
	else if (waruptr->options[SCANOPTS] == FULLNAMES)
		printmap(RANGENAMES,(long)range);
	else if (waruptr->options[SCANOPTS] == FULLEXTRA)
		printmap(RANGEEXTRA,(long)range);
	else if (waruptr->options[SCANOPTS] == NOMAP)
		printmap(RANGENOMAP,(long)range);
	else
		printmap(RANGE,0L);

	outprfge(FLT_NONE, usrnum);
}

static void scan_se(void)
{
	unsigned i, x, y;
	WARSHP *wptr;
	MINE *mptr;
	int nebmask;
	long px, py, dx, dy, r2;

	refresh(warsptr,usrnum);

	for (i = 0; i < MAXPLANETS; ++i)
		owned_planet[i] = 0;

	setsect(warsptr);
	nebmask = innebula(xsect,ysect);
	if (nebmask) {
		for (i = 0; i < sector.numplan; ++i) {
			plnum = i + 1;
			if (getplanetdat(usrnum) && sameas(plptr->userid,warsptr->userid))
				owned_planet[i] = 1;
		}
	}
	se_nebula = nebmask;
	prfmsg(SCAN25,(nebmask ? CLR_GREEN2 "nebula" : "sector"),xsect,ysect);
	clearmap();

	update_scantab(warsptr,usrnum);

	for (i=0,mptr = mines; i<nummines;++mptr,++i) {
		x = coord1(mptr->coord.xcoord);
		y = coord1(mptr->coord.ycoord);

		if (mptr->channel != MINE_UNUSED && (x==xsect && y==ysect)) {	/* if a live mine */
			if (nebmask && cdistance(&warsptr->coord,&mptr->coord)*10000.0 > (double)NEBRNG)
				continue;
			x = coord2(mptr->coord.xcoord) +50;
			y = coord2(mptr->coord.ycoord) +50;
			map[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '.';
			mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '0';
		}
	}

	for (i=0 ; i< NOSCANTAB; i++) {
		othusn = scantab[usrnum].ship[i].shipno;
		if (scantab[usrnum].ship[i].flag == 1) {
			wptr = warshpoff(othusn);
			if (samesect(&wptr->coord,&warsptr->coord)) {
				if (nebmask && cdistance(&warsptr->coord,&wptr->coord)*10000.0 > (double)NEBRNG)
					continue;
				x = coord2(wptr->coord.xcoord) +50;
				y = coord2(wptr->coord.ycoord) +50;
				map[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = scantab[usrnum].ship[i].letter;
				if (warsptr->lock == othusn)
					mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '6';
				else if (shipclass[wptr->shpclass].max_type == CLASSTYPE_CYBORG)
					mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '1';
				else if (shipclass[wptr->shpclass].max_type == CLASSTYPE_DROID)
					mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '4';
				else
					mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '2';
			}
		}
	}
	x = coord2(warsptr->coord.xcoord) +50;
	y = coord2(warsptr->coord.ycoord) +50;
	map[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '*';
	mapc[y/(SSMAX/(MAXY-1))][x/(SSMAX/(MAXX-1))] = '0';

	map_planets();

	if (nebmask) {
		px = (long)(coord2(warsptr->coord.xcoord) + 50);
		py = (long)(coord2(warsptr->coord.ycoord) + 50);
		r2 = (long)NEBRNG * (long)NEBRNG;
		for (y = 0; y < MAXY; ++y) {
			dy = (((long)y * (long)(SSMAX/(MAXY-1))) + (long)((SSMAX/(MAXY-1))/2)) - py;
			for (x = 0; x < MAXX; ++x) {
				dx = (((long)x * (long)(SSMAX/(MAXX-1))) + (long)((SSMAX/(MAXX-1))/2)) - px;
				if ((dx*dx + dy*dy) > r2) {
					if (map[y][x] == ' ') {
						map[y][x] = '.';
						mapc[y][x] = '2';
					}
				}
			}
		}
	}

	if (waruptr->options[SCANOPTS] == NOMAP)
		printmap(SECTORNOMAP,0L);
	else if (waruptr->options[SCANOPTS] == SIMPLE)
		printmap(SECTOR,0L);
	else
		printmap(SECTORFULL,0L);
	se_nebula = FALSE;
	outprfge(FLT_NONE,usrnum);
}

static void scan_lo(void)
{
	int x, y, fullgal;
	double xf, yf, x1, y1, range;
	double minx, miny;
	double xfactor, yfactor;
	double sx, sy;
	double origin_x, origin_y;
	double cell_left, cell_right, cell_top, cell_bottom;
	double gal_min, gal_max;
	double shiftx = 0.0;

	WARSHP *wptr;

	if (margc < 2 || margc > 3) {
		prfmsg(FORMAT,"SCAN");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	setsect(warsptr);

	if (innebula(xsect,ysect)) {
		prfmsg(SCAN27);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 3 && sameto("h",margv[2]))
		range = (double)ship_scanrange(warsptr) * 5.0;
	else if (margc == 3 && sameto("q",margv[2]))
		range = (double)ship_scanrange(warsptr) * 2.5;
	else
		range = (double)ship_scanrange(warsptr) * 10.0;

	x1 = warsptr->coord.xcoord * 10000.0;
	y1 = warsptr->coord.ycoord * 10000.0;

	gal_min = -(double)univmax * 10000.0;
	gal_max = ((double)univmax + 1.0) * 10000.0;

	/* will this scan touch all four corners of the galaxy? */
	fullgal = (range >= (x1 - gal_min) && range >= (gal_max - x1) &&
		range >= (y1 - gal_min) && range >= (gal_max - y1));

	if (fullgal)
		prfmsg(SCAN24G);
	else
		prfmsg(SCAN24, spr("%ld",(long)range), xsect, ysect);

	clearmap();

	if (fullgal) {
		minx = gal_min;
		miny = gal_min;
		xfactor = (gal_max - gal_min) / (double)MAXX;
		yfactor = (gal_max - gal_min) / (double)MAXY;
	} else {
		minx = x1 - range;
		miny = y1 - range;
		xfactor = (2.0 * range) / (double)MAXX;
		yfactor = (2.0 * range) / (double)MAXY;

		/* compute visual shift so player ends up at MAXX/2 */
		shiftx = ((double)MAXX / 2.0) - 0.5 - ((x1 - minx) / xfactor);
		shiftx *= xfactor;  /* convert from cells to units */
	}

	origin_x = minx - shiftx;
	origin_y = miny;

	/* mark outside of galaxy */
	for (y = 0; y < MAXY; y++) {
		for (x = 0; x < MAXX; x++) {
			cell_left = origin_x + x * xfactor;
			cell_right = cell_left + xfactor;
			cell_top = origin_y + y * yfactor;
			cell_bottom = cell_top + yfactor;

			if (cell_right <= gal_min || cell_left >= gal_max ||
				cell_bottom <= gal_min || cell_top >= gal_max) {
				map[y][x]  = '.';
				mapc[y][x] = '4';
			}
		}
	}

	/* place all ships except the player scanning */
	for (othusn = 0; othusn < nships; othusn++) {
		if (ingegame(othusn) && othusn != usrnum) {
			wptr = warshpoff(othusn);

			sx = (wptr->coord.xcoord * 10000.0 - minx + shiftx) / xfactor;
			sy = (wptr->coord.ycoord * 10000.0 - miny) / yfactor;

			if (sx >= 0.0 && sy >= 0.0) {
				x = (int)sx;
				y = (int)sy;

				if (x >= 0 && y >= 0 && x < MAXX && y < MAXY) {
					if (wptr->status == GESTAT_AUTO) {
						if (map[y][x] != '=') {  /* user ships take precedence */
							map[y][x] = '+';
							mapc[y][x] = '0';
						}
					} else {
						map[y][x] = '=';
						mapc[y][x] = '0';
					}
				}
			}
		}
	}

	/* place player */
	if (fullgal) {
		xf = (x1 - minx) / xfactor;
		yf = (y1 - miny) / yfactor;

		if (xf >= 0.0 && yf >= 0.0) {
			x = (int)xf;
			y = (int)yf;

			if (x >= MAXX) x = MAXX - 1;
			if (y >= MAXY) y = MAXY - 1;

			if (x >= 0 && y >= 0 && x < MAXX && y < MAXY) {
				map[y][x] = '*';
				mapc[y][x] = '0';
			}
		}
	} else {
		map[MAXY/2][MAXX/2] = '*';
		mapc[MAXY/2][MAXX/2] = '0';
	}

	printmap(MAP_LONG,0L);
	outprfge(FLT_NONE,usrnum);
}


/**************************************************************************
** DATA scan helpers                                                     **
**************************************************************************/

#ifndef GE_ARENA
static void data_text(const char *text)
{
	while (*text) {
		switch (*text) {
			case '%':
				prf("%%25");
				break;
			case ',':
				prf("%%2C");
				break;
			case '*':
				prf("%%2A");
				break;
			case '\r':
				prf("%%0D");
				break;
			case '\n':
				prf("%%0A");
				break;
			default:
				prf("%c",*text);
		}
		++text;
	}
}

static void data_scan_name(WARSHP *ptr, int marker)
{
	int len, maxlen, show;

	maxlen = marker == 0 ? 36 : 34;
	if (ptr->status == GESTAT_AUTO)
		stzcpy(gechrbuf,ptr->shipname,maxlen + 1);
	else if (ptr->shipname[0] == 0)
		stzcpy(gechrbuf,ptr->userid,maxlen + 1);
	else if ((int)(strlen(ptr->userid) + strlen(ptr->shipname) + 3) <= maxlen)
		sprintf(gechrbuf,"%s (%s)",ptr->userid,ptr->shipname);
	else {
		show = maxlen - (int)strlen(ptr->userid) - 6;
		if (show < 1)
			stzcpy(gechrbuf,ptr->userid,maxlen + 1);
		else {
			sprintf(gechrbuf,"%s (",ptr->userid);
			len = strlen(gechrbuf);
			memcpy(gechrbuf + len,ptr->shipname,show);
			strcpy(gechrbuf + len + show,"...)");
		}
	}
}

static void data_scan_scramble(char *buf, int keep_spaces)
{
	int i;

	if (warsptr->jam_sev > (byte)7) {
		for (i=0; buf[i]; ++i)
			if (!keep_spaces || (buf[i] != ' ' && buf[i] != '-'))
				buf[i] = '?';
	}
	else if (warsptr->jam_sev > (byte)2) {
		for (i=0; buf[i]; ++i)
			if ((!keep_spaces || (buf[i] != ' ' && buf[i] != '-'))
				&& gernd() % (10 - warsptr->jam_sev) == 0)
				buf[i] = '?';
	}
}

static int data_ship_type(WARSHP *ptr)
{
	if (shipclass[ptr->shpclass].max_type == CLASSTYPE_CYBORG)
		return 0;
	if (shipclass[ptr->shpclass].max_type == CLASSTYPE_DROID)
		return 1;
	return 2;
}

static void data_ship_contact_prefix(char *record, SCANTAB *sptr, int ndx)
{
	WARSHP *wptr;
	int marker;
	char letter;

	wptr = warshpoff(sptr->ship[ndx].shipno);
	marker = 0;
	if (warsptr->jam_sev < (byte)3) {
		if (warsptr->lock == sptr->ship[ndx].shipno)
			marker = 1;
		else if (wptr->distress != 255)
			marker = 2;
	}

	letter = sptr->ship[ndx].letter;
	if (warsptr->jam_sev > (byte)2
		&& gernd() % (10 / warsptr->jam_sev) == 0)
		letter = '?';

	prf("%s:%c,%d,",record,letter,marker);
	if (letter == '?')
		prf("?,");
	else
		prf("%d,",data_ship_type(wptr));

	data_scan_name(wptr,marker);
	data_scan_scramble(gechrbuf,FALSE);
	data_text(gechrbuf);
	prf(",");
}

static void data_scan(void)
{
	SCANTAB *sptr;
	MINE *mptr;
	double range, x1, y1, xfactor, yfactor, minx, miny;
	double shiftx, shifty, xf, yf, minedist;
	int i, count, minecount, minebearing;

	setsect(warsptr);
	if (innebula(xsect,ysect)) {
		prf("SCAN:NEBULA*\r");
		prf("STOP:SCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	range = (double)ship_scanrange(warsptr);
	prf("SCAN:%s,%d,%d*\r",spr("%ld",(long)range),xsect,ysect);
	update_scantab(warsptr,usrnum);
	sptr = &scantab[usrnum];
	count = 0;

	for (i=0; i<NOSCANTAB; ++i) {
		if (sptr->ship[i].flag == 0 || (long)sptr->ship[i].dist >= (long)range)
			continue;
		if (warsptr->jam_sev > (byte)7 && gernd() % 2 == 0)
			continue;

		data_ship_contact_prefix("SCANS",sptr,i);
		if (sptr->ship[i].flag == 2) {
			prf("?????,????,????,??.??*\r");
			outprfge(FLT_NONE,usrnum);
			++count;
			continue;
		}

		sprintf(gechrbuf,"%ld",(long)sptr->ship[i].dist);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s,",gechrbuf);
		sprintf(gechrbuf,"%d",sptr->ship[i].bearing);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s,",gechrbuf);
		sprintf(gechrbuf,"%d",sptr->ship[i].heading);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s,",gechrbuf);
		stzcpy(gechrbuf,showarp(sptr->ship[i].speed),20);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s*\r",gechrbuf);
		outprfge(FLT_NONE,usrnum);
		++count;
	}

	if (count == 0) {
		prf("SCANS:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	minecount = 0;
	x1 = warsptr->coord.xcoord;
	y1 = warsptr->coord.ycoord;
	xfactor = range / 5000.0 / (double)MAXX;
	yfactor = range / 5000.0 / (double)MAXY;
	minx = x1 - range / 10000.0;
	miny = y1 - range / 10000.0;
	shiftx = 0.0;
	shifty = 0.0;
	if ((int)((x1 - minx) / xfactor) == (MAXX/2) - 1)
		shiftx = xfactor;
	if ((int)((y1 - miny) / yfactor) == (MAXY/2) - 1)
		shifty = yfactor;

	for (i=0, mptr=mines; i<nummines; ++i, ++mptr) {
		if (mptr->channel == MINE_UNUSED)
			continue;
		xf = ((mptr->coord.xcoord - x1) / xfactor)
			+ ((double)MAXX)/2.0 + shiftx / xfactor;
		yf = ((mptr->coord.ycoord - y1) / yfactor)
			+ ((double)MAXY)/2.0 + shifty / yfactor;
		if (xf >= 0.0 && xf < (double)MAXX
			&& yf >= 0.0 && yf < (double)MAXY) {
			if (warsptr->jam_sev > (byte)7 && gernd() % 2 == 0)
				continue;
			minedist = cdistance(&warsptr->coord,&mptr->coord) * 10000.0;
			minebearing = cbearing(&warsptr->coord,&mptr->coord,warsptr->heading);
			sprintf(gechrbuf,"%ld",(long)minedist);
			data_scan_scramble(gechrbuf,TRUE);
			prf("SCANM:%s,",gechrbuf);
			sprintf(gechrbuf,"%d",minebearing);
			data_scan_scramble(gechrbuf,TRUE);
			prf("%s*\r",gechrbuf);
			outprfge(FLT_NONE,usrnum);
			++minecount;
		}
	}

	if (minecount == 0)
		prf("SCANM:*\r");
	prf("STOP:SCAN*\r");
	outprfge(FLT_NONE,usrnum);
}

static void data_sector(void)
{
	SCANTAB *sptr;
	WARSHP *wptr;
	MINE *mptr;
	double dist;
	unsigned int rseed;
	int i, nebula, orbit, owned;
	int objectcount, shipcount, minecount, objbearing;

	refresh(warsptr,usrnum);
	setsect(warsptr);
	nebula = innebula(xsect,ysect);
	prf("SECTOR:%d,%d,%d*\r",xsect,ysect,nebula);
	getsector(&warsptr->coord);

	objectcount = 0;
	for (i=0; i<sector.numplan; ++i) {
		if (sector.ptab[i].coord.xcoord == 0)
			continue;
		orbit = (warsptr->where - 11 == i);
		owned = FALSE;
		if (sector.ptab[i].type == PLTYPE_PLNT) {
			plnum = i + 1;
			if (getplanetdat(usrnum) && sameas(plptr->userid,warsptr->userid))
				owned = TRUE;
		}
		dist = cdistance(&warsptr->coord,&sector.ptab[i].coord) * 10000.0;
		if (nebula && dist > (double)NEBRNG && !orbit && !owned)
			continue;

		rseed = gernd();
		if (rseed % (warsptr->jam_sev + 1) > (byte)6 && !orbit)
			continue;
		if (rseed % (warsptr->jam_sev + 1) > (byte)3 && !orbit)
			strcpy(gechrbuf,"?");
		else
			sprintf(gechrbuf,"%d",i + 1);

		objbearing = cbearing(&warsptr->coord,&sector.ptab[i].coord,warsptr->heading);
		sprintf(gechrbuf2,"%ld",(long)dist);
		sprintf(gechrbuf3,"%d",objbearing);
		if (warsptr->jam_sev > (byte)2 && !orbit) {
			scramble_tail_digits(gechrbuf2,rseed);
			scramble_tail_digits(gechrbuf3,rseed >> 4);
		}
		prf("SECTORO:%s,",gechrbuf);
		if (gechrbuf[0] == '?')
			prf("?,");
		else
			prf("%d,",sector.ptab[i].type);
		prf("%s,%s,%d*\r",gechrbuf2,gechrbuf3,orbit);
		outprfge(FLT_NONE,usrnum);
		++objectcount;
	}
	if (objectcount == 0) {
		prf("SECTORO:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	update_scantab(warsptr,usrnum);
	sptr = &scantab[usrnum];
	shipcount = 0;
	for (i=0; i<NOSCANTAB; ++i) {
		if (sptr->ship[i].flag != 1)
			continue;
		wptr = warshpoff(sptr->ship[i].shipno);
		if (!samesect(&wptr->coord,&warsptr->coord))
			continue;
		if (nebula && sptr->ship[i].dist > (double)NEBRNG)
			continue;
		if (warsptr->jam_sev > (byte)7 && gernd() % 2 == 0)
			continue;

		data_ship_contact_prefix("SECTORS",sptr,i);
		sprintf(gechrbuf,"%ld",(long)sptr->ship[i].dist);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s,",gechrbuf);
		sprintf(gechrbuf,"%d",sptr->ship[i].bearing);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s*\r",gechrbuf);
		outprfge(FLT_NONE,usrnum);
		++shipcount;
	}
	if (shipcount == 0) {
		prf("SECTORS:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	minecount = 0;
	for (i=0, mptr=mines; i<nummines; ++i, ++mptr) {
		if (mptr->channel == MINE_UNUSED || !samesect(&mptr->coord,&warsptr->coord))
			continue;
		dist = cdistance(&warsptr->coord,&mptr->coord) * 10000.0;
		if (nebula && dist > (double)NEBRNG)
			continue;
		if (warsptr->jam_sev > (byte)7 && gernd() % 2 == 0)
			continue;
		objbearing = cbearing(&warsptr->coord,&mptr->coord,warsptr->heading);
		sprintf(gechrbuf,"%ld",(long)dist);
		data_scan_scramble(gechrbuf,TRUE);
		prf("SECTORM:%s,",gechrbuf);
		sprintf(gechrbuf,"%d",objbearing);
		data_scan_scramble(gechrbuf,TRUE);
		prf("%s*\r",gechrbuf);
		outprfge(FLT_NONE,usrnum);
		++minecount;
	}
	if (minecount == 0)
		prf("SECTORM:*\r");
	prf("STOP:SECTOR*\r");
	outprfge(FLT_NONE,usrnum);
}

static void data_inventory(void)
{
	int i;

	prf("INV:");
	for (i=0; i<NUMITEMS; ++i)
		prf("I%d:%s,",i,spr("%lu",warsptr->items[i]));
	prf("*\r");
	sprintf(gechrbuf,"%lu",calcweight(warsptr));
	sprintf(gechrbuf2,"%ld",shipclass[warsptr->shpclass].max_tons);
	prf("INV2:%s,%s*\r",gechrbuf,gechrbuf2);
	prf("STOP:INV*\r");
}

static void data_navigation(void)
{
	setsect(warsptr);
	prf("NAV:%s,%s,%d,%d,%u,%u,%d*\r",
		spr("%d",(int)(warsptr->heading + .5)),
		spr("%ld",(long)warsptr->speed),
		xsect,ysect,xcord,ycord,
		warsptr->where);
	prf("STOP:NAV*\r");
}

static void data_ordnance(void)
{
	WARSHP *ptr;
	double ddist;
	int i, zothusn, none, minebrg;

	none = TRUE;
	for (i=0; i<MAXTORPS; ++i) {
		if (warsptr->ltorps[i].distance != 0) {
			none = FALSE;
			if (warsptr->ltorps[i].channel < nships) {
				ptr = warshpoff(warsptr->ltorps[i].channel);
				prf("ORD1:");
				data_text(username(ptr));
				prf(",%d,",warsptr->lock == warsptr->ltorps[i].channel);
			} else if (warsptr->ltorps[i].channel == 255)
				prf("ORD1:destroyed,0,");
			else
				prf("ORD1:,0,");
			if (warsptr->jam_sev <= (byte)2)
				prf("%u",warsptr->ltorps[i].distance);
			else
				prf("?????");
			prf("*\r");
			outprfge(FLT_NONE,usrnum);
		}
	}
	if (none == TRUE) {
		prf("ORD1:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	none = TRUE;
	for (i=0; i<MAXMISSL; ++i) {
		if (warsptr->lmissl[i].distance != 0) {
			none = FALSE;
			if (warsptr->lmissl[i].channel < nships) {
				ptr = warshpoff(warsptr->lmissl[i].channel);
				prf("ORD2:");
				data_text(username(ptr));
				prf(",%d,",warsptr->lock == warsptr->lmissl[i].channel);
			} else if (warsptr->lmissl[i].channel == 255)
				prf("ORD2:destroyed,0,");
			else
				prf("ORD2:,0,");
			if (warsptr->jam_sev <= (byte)2)
				prf("%u",warsptr->lmissl[i].distance);
			else
				prf("?????");
			prf("*\r");
			outprfge(FLT_NONE,usrnum);
		}
	}
	if (none == TRUE) {
		prf("ORD2:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	none = TRUE;
	if (shipclass[warsptr->shpclass].max_torps != 0) {
		for (zothusn = 0; zothusn < nships; ++zothusn) {
			if (ingegame(zothusn)) {
				ptr = warshpoff(zothusn);
				for (i=0; i<MAXTORPS; ++i) {
					if (ptr->ltorps[i].distance != 0 && ptr->ltorps[i].channel == usrnum) {
						none = FALSE;
						prf("ORD3:");
						data_text(username(ptr));
						prf(",%d,",warsptr->lock == zothusn);
						if (warsptr->jam_sev <= (byte)2)
							prf("%u",ptr->ltorps[i].distance);
						else
							prf("?????");
						prf("*\r");
						outprfge(FLT_NONE,usrnum);
					}
				}
			}
		}
	}
	if (none == TRUE) {
		prf("ORD3:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	none = TRUE;
	if (shipclass[warsptr->shpclass].max_missl != 0) {
		for (zothusn = 0; zothusn < nships; ++zothusn) {
			if (ingegame(zothusn)) {
				ptr = warshpoff(zothusn);
				for (i=0; i<MAXMISSL; ++i) {
					if (ptr->lmissl[i].distance != 0 && ptr->lmissl[i].channel == usrnum) {
						none = FALSE;
						prf("ORD4:");
						data_text(username(ptr));
						prf(",%d,",warsptr->lock == zothusn);
						if (warsptr->jam_sev <= (byte)2)
							prf("%u",ptr->lmissl[i].distance);
						else
							prf("?????");
						prf("*\r");
						outprfge(FLT_NONE,usrnum);
					}
				}
			}
		}
	}
	if (none == TRUE) {
		prf("ORD4:*\r");
		outprfge(FLT_NONE,usrnum);
	}

	none = 0;
	for (i=0; i<MAXDECOY; ++i)
		if (warsptr->decout[i] > 0)
			++none;
	prf("ORD5:%d*\r",none);

	none = TRUE;
	if (shipclass[warsptr->shpclass].has_mine != 0) {
		for (i=0; i<nummines; ++i) {
			if (mines[i].channel != (byte)usrnum)
				continue;
			none = FALSE;
			ddist = cdistance(&warsptr->coord,&mines[i].coord) * 10000.0;
			minebrg = cbearing(&warsptr->coord,&mines[i].coord,warsptr->heading);
			prf("ORD6:%d,%d,%u,",
				(int)mines[i].coord.xcoord,
				(int)mines[i].coord.ycoord,
				mines[i].timer);
			if (warsptr->jam_sev <= (byte)2)
				prf("%d,%s*\r",minebrg,spr("%ld",(long)ddist));
			else
				prf("????,?????*\r");
			outprfge(FLT_NONE,usrnum);
		}
	}
	if (none == TRUE)
		prf("ORD6:*\r");
	prf("STOP:ORD*\r");
}

static int data_damage_eta(WARSHP *ptr, int steps, int active, int fuzz)
{
	int secs;

	/* preserve why REPORT SYS omitted a numeric ETA without exposing steps */
	if (steps <= 0)
		return 0;
	if (ptr->repair > 0)
		return -3;
	if (!(ptr->upgrade & DAMCTRL))
		return -1;
	if (!active)
		return -2;

	secs = ((steps + 1) / 2) * TICKTIME + fuzz;
	if (secs < TICKTIME)
		secs = TICKTIME;
	return secs;
}

static void data_systems(void)
{
	int active, cloakstate, fuzz, maxcharge, need;
	int phaserstate, shieldpct, shieldstat, shieldtype;
	int shfuzz, warpstate, warptop;

	damstr((int)(warsptr->damage + .5));
	prf("SYS1:%s,%s*\r",
		gechrbuf,
		spr("%u",(unsigned)(warsptr->energy + .5)));

	shieldtype = 0;
	shieldstat = 0;
	shieldpct = -1;
	if (shipclass[warsptr->shpclass].max_shlds != 0) {
		if (warsptr->shieldstat == SHIELDUP
			|| warsptr->shieldstat == SHIELDDN
			|| warsptr->shieldstat == SHIELDDM) {
			shieldtype = warsptr->shieldtype;
			shieldstat = warsptr->shieldstat;
		}
		if (warsptr->shieldstat == SHIELDUP)
			charge(warsptr,&maxcharge,&shieldpct);
	}

	phaserstate = 0;
	if (shipclass[warsptr->shpclass].max_phasr != 0) {
		if (warsptr->phasr >= PMINFIRE)
			phaserstate = 1;
		else if (warsptr->phasr >= 0)
			phaserstate = 2;
		else
			phaserstate = 3;
	}

	cloakstate = 0;
	if (shipclass[warsptr->shpclass].max_cloak == 1)
		cloakstate = (warsptr->cloak > 0 && warsptr->cloak != 3) ? 2 : 1;

	prf("SYS2:%d,%d,%d,%d,%d,%d*\r",
		shieldtype,shieldstat,shieldpct,
		phaserstate == 0 ? 0 : warsptr->phasrtype,
		phaserstate,cloakstate);

	active = TRUE;
	fuzz = gernd();
	shfuzz = (fuzz % 5) - 2;
	fuzz = ((fuzz / 5) % 5) - 2;
	prf("SYS3:");
	/* match the subsystem order used by show_rep_sysdam() */
	need = 0;
	if (warsptr->shieldstat == SHIELDDM) {
		need = 1 - warsptr->shield;
		if (need <= 0)
			need = 1;
		need = (need + warsptr->shieldtype - 1) / warsptr->shieldtype;
	}
	prf("%d,",data_damage_eta(warsptr,need,TRUE,shfuzz));
	need = warsptr->helm < 0 ? -warsptr->helm : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->tactical < 0 ? -warsptr->tactical : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->phasr < 0 ? (int)ceil(-warsptr->phasr) : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->torpcntl > 0 ? warsptr->torpcntl : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->mislcntl > 0 ? warsptr->mislcntl : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->cloak < 0 ? -warsptr->cloak : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->jamload < 0 ? -warsptr->jamload : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->decload < 0 ? -warsptr->decload : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->zipload < 0 ? -warsptr->zipload : 0;
	prf("%d,",data_damage_eta(warsptr,need,active,fuzz));
	if (need > 0)
		active = FALSE;
	need = warsptr->mineload < 0 ? -warsptr->mineload : 0;
	prf("%d*\r",data_damage_eta(warsptr,need,active,fuzz));

	warpstate = 0;
	warptop = -1;
	if (shipclass[warsptr->shpclass].max_warp != 0) {
		if (warsptr->topspeed == 0)
			warpstate = 1;
		else if (warsptr->speed / 1000 > warsptr->topspeed)
			warpstate = 2;
		else if (warsptr->overspeed > 0) {
			warpstate = 3;
			warptop = warsptr->topspeed;
		}
	}
	prf("SYS4:%d,%d,%u*\r",
		warpstate,warptop,
		warsptr->repair > 0 ? repair_eta(warsptr) : 0);
	prf("STOP:SYS*\r");
}

static void data_shipscan_text(char *buf, unsigned int *rseed)
{
	if (warsptr->jam_sev > (byte)2)
		jam_scramble(buf,warsptr->jam_sev,rseed);
	data_text(buf);
}

static void data_shipscan_damage(WARSHP *wptr)
{
	int scandmg, phaserstate, shieldstat;
	int show_shields, show_enhanced;

	if (warsptr->jam_sev >= (byte)3
		|| !((warsptr->upgrade & ENHSCAN) || (warsptr->where != 1 && wptr->where != 1))) {
		prf("SHIPSCAN2:*\rSHIPSCAN3:*\r");
		return;
	}

	show_shields = (!(warsptr->upgrade & ENHSCAN) && warsptr->where != 1 && wptr->where != 1)
		|| ((warsptr->upgrade & ENHSCAN) && wptr->where != 1);
	show_enhanced = (warsptr->upgrade & ENHSCAN) && wptr->where != 1;

	scandmg = (int)(wptr->damage + .5);
	damstr(scandmg);
	prf("SHIPSCAN2:");
	data_text(gechrbuf);

	shieldstat = 0;
	if (show_shields && shipclass[wptr->shpclass].max_shlds != 0) {
		if (wptr->shieldstat == SHIELDUP)
			shieldstat = 1;
		else
			shieldstat = 2;
	}
	prf(",");
	if (show_shields)
		prf("%d",shieldstat);

	if (!show_enhanced) {
		prf(",,,*\rSHIPSCAN3:*\r");
		return;
	}

	phaserstate = 0;
	if (shipclass[wptr->shpclass].max_phasr != 0) {
		if (wptr->phasr >= PMINFIRE)
			phaserstate = 1;
		else if (wptr->phasr >= 0)
			phaserstate = 2;
		else
			phaserstate = 3;
	}

	prf(",%d,%d,%d*\r",
		shipclass[wptr->shpclass].max_shlds == 0 ? 0 : wptr->shieldtype,
		phaserstate == 0 ? 0 : wptr->phasrtype,
		phaserstate);

	prf("SHIPSCAN3:");
	data_shipscan_systems(wptr);
}

static void data_shipscan_systems(WARSHP *wptr)
{
	/* keep this order and damage predicates in sync with show_rep_sysdam() and SYS3 */
	prf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*\r",
		wptr->shieldstat == SHIELDDM,
		wptr->helm < 0,
		wptr->tactical < 0,
		wptr->phasr < 0,
		wptr->torpcntl > 0,
		wptr->mislcntl > 0,
		wptr->cloak < 0,
		wptr->jamload < 0,
		wptr->decload < 0,
		wptr->zipload < 0,
		wptr->mineload < 0);
}

static void data_shipscan_notify(int shpnum, long scandist)
{
	char ltr;
	int target_bearing;
	WARSHP *wptr;

	if (warsptr->jam_sev >= (byte)3 || warsptr->lock == shpnum)
		return;

	wptr = warshpoff(shpnum);
	if ((long)scandist > ship_scanrange(wptr)) {
		target_bearing = cbearing(&wptr->coord,&warsptr->coord,wptr->heading);
		prfmsg(SCAN2,target_bearing);
	}
	else {
		if (warsptr->cloak != 10) {
			ltr = shpltr(shpnum,usrnum);
			if (warsptr->shipname[0] == 0)
				prfmsg(SCAN1O,ltr,waruptr->userid);
			else
				prfmsg(SCAN1,ltr,warsptr->shipname);
		}
		else
			prfmsg(SCAN3);
	}
	outprfge(FLT_NONE,shpnum);
}

static void data_shipscan(void)
{
	WARSHP *wptr;
	WARUSR *wuptr;
	char *tname;
	unsigned int rseed;
	long scandist;
	int scanbrg, gheading, scanhdg, nebmask, shpnum, target_neb;

	if (warsptr->tactical != 0 || warsptr->damage >= 100.0) {
		prf("SHIPSCAN:UNAVAILABLE*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	rseed = gernd();
	if (warsptr->jam_sev > (byte)7
		|| (warsptr->jam_sev > (byte)2 && rseed % (9 - (int)warsptr->jam_sev) == 0)) {
		prf("SHIPSCAN:JAMMED*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	setsect(warsptr);
	nebmask = innebula(xsect,ysect);
	if (margv[2][0] == '@') {
		if (warsptr->lock < 0 || warsptr->lock >= nships) {
			warsptr->lock = -1;
			warsptr->track_grace = 0;
			shpnum = -1;
		}
		else {
			shpnum = warsptr->lock;
			if (!ingegame(shpnum)) {
				warsptr->lock = -1;
				warsptr->track_grace = 0;
				shpnum = -1;
			}
			else {
				wptr = warshpoff(shpnum);
				if ((cdistance(&warsptr->coord,&wptr->coord) * 10000.0) > (double)ship_scanrange(warsptr)) {
					warsptr->lock = -1;
					warsptr->track_grace = 0;
					shpnum = -1;
				}
			}
		}
	}
	else
		shpnum = findshp(margv[2],1);

	if (shpnum < 0 && margv[2][0] == '@') {
		prf("SHIPSCAN:NOTFOUND*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (shpnum >= 0) {
		wptr = warshpoff(shpnum);
		scandist = (long)(cdistance(&warsptr->coord,&wptr->coord) * 10000);
		target_neb = innebula(coord1(wptr->coord.xcoord),coord1(wptr->coord.ycoord));
		if ((nebmask || target_neb) && !(nebmask && target_neb && scandist < (long)NEBRNG)) {
			if (nebmask)
				prf("SHIPSCAN:NEBULA*\r");
			else
				prf("SHIPSCAN:NOTFOUND*\r");
			prf("STOP:SHIPSCAN*\r");
			outprfge(FLT_NONE,usrnum);
			return;
		}
	}
	else if (nebmask) {
		prf("SHIPSCAN:NEBULA*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (shpnum == usrnum) {
		if (margv[2][0] == '@') {
			warsptr->lock = -1;
			warsptr->track_grace = 0;
		}
		prf("SHIPSCAN:NOTFOUND*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (shpnum < 0) {
		prf("SHIPSCAN:NOTFOUND*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	wptr = warshpoff(shpnum);
	wuptr = warusroff(shpnum);
	scandist = (long)(cdistance(&warsptr->coord,&wptr->coord) * 10000);
	if (scandist >= ship_scanrange(warsptr)) {
		prf("SHIPSCAN:NOTFOUND*\rSTOP:SHIPSCAN*\r");
		outprfge(FLT_NONE,usrnum);
		return;
	}

	scanbrg = cbearing(&warsptr->coord,&wptr->coord,warsptr->heading);
	scanhdg = cbearing(&wptr->coord,&warsptr->coord,wptr->heading);
	gheading = (int)(wptr->heading + .5);
	setsect(wptr);

	prf("SHIPSCAN1:");
	if (wptr->status == GESTAT_USER && wptr->shipname[0] == 0)
		sprintf(gechrbuf,"%s",wuptr->userid);
	else
		sprintf(gechrbuf,"%s",wptr->shipname);
	data_shipscan_text(gechrbuf,&rseed);
	prf(",");

	if (warsptr->jam_sev < (byte)3) {
		data_text(shipclass[wptr->shpclass].typename);
		prf(",");
		data_text(showupg(wptr));
		prf(",");
		if (wptr->status == GESTAT_USER && wptr->shipname[0] != 0)
			data_text(username(wptr));
		prf(",");
		tname = NULL;
		if (wptr->status == GESTAT_USER && wuptr->teamcode > 0)
			tname = teamname(wuptr);
		data_text(tname == NULL ? "" : tname);
	}
	else
		prf(",,,");
	prf(",");

	sprintf(gechrbuf,"%d",scanbrg);
	data_shipscan_text(gechrbuf,&rseed);
	prf(",");
	sprintf(gechrbuf,"%d",scanhdg);
	data_shipscan_text(gechrbuf,&rseed);
	prf(",");
	sprintf(gechrbuf,"%ld",scandist);
	data_shipscan_text(gechrbuf,&rseed);
	prf(",");
	sprintf(gechrbuf,"%d",gheading);
	data_shipscan_text(gechrbuf,&rseed);
	prf(",");
	sprintf(gechrbuf,"%d %d",xsect,ysect);
	data_shipscan_text(gechrbuf,&rseed);
	prf(",");
	stzcpy(gechrbuf,showarp(wptr->speed),20);
	data_shipscan_text(gechrbuf,&rseed);

	if (warsptr->jam_sev < (byte)3) {
		if (wptr->status == GESTAT_AUTO)
			prf(",%u,0,0,0*\r",wptr->kills);
		else
			prf(",%u,%u,%u,%u*\r",wptr->kills,wptr->ukills,wuptr->kills,wuptr->ukills);
	}
	else
		prf(",,,,*\r");

	data_shipscan_damage(wptr);
	prf("STOP:SHIPSCAN*\r");
	outprfge(FLT_NONE,usrnum);
	data_shipscan_notify(shpnum,scandist);
}

static void data_user(void)
{
	char *tname;

	tname = waruptr->teamcode > 0 ? teamname(waruptr) : NULL;
	prf("USER1:");
	data_text(waruptr->userid);
	prf(",");
	data_text(tname == NULL ? "" : tname);
	prf(",%u,%u,%u,%u*\r",
		waruptr->noships,
		waruptr->kills,
		waruptr->ukills,
		waruptr->planets);
	sprintf(gechrbuf,"%lu",waruptr->score);
	sprintf(gechrbuf2,"%lu",waruptr->cash);
	sprintf(gechrbuf3,"%lu",waruptr->population);
	prf("USER2:%s,%s,%s*\r",gechrbuf,gechrbuf2,gechrbuf3);
	prf("STOP:USER*\r");
}

static void data_messages(void)
{
	byte msgfilter;
	int entry;

	msgfilter = waruptr->options[MSG_FILTER];
	entry = (msgfilter & MSGF_ENTRY_MASK) >> 5;
	if (entry > 2)
		entry = 2;
	prf("MSG:%u,%u,%u,%u,%u,%u*\r",
		(unsigned)(msgfilter & MSGF_CYBS_MASK),
		(unsigned)((msgfilter & MSGF_DISTRESS) != 0),
		(unsigned)((msgfilter & MSGF_BEACON) != 0),
		(unsigned)((msgfilter & MSGF_HAIL) != 0),
		(unsigned)entry,
		(unsigned)((msgfilter & MSGF_SHIP) != 0));
	prf("STOP:MSG*\r");
}

static void data_upgrades(void)
{
	prf("UPG:%u,%u*\r",
		(unsigned)warsptr->upgrade,
		(unsigned)warsptr->tponder);
	prf("STOP:UPG*\r");
}

static void data_ship(void)
{
	prf("SHIP:");
	data_text(warsptr->shipname);
	prf(",");
	data_text(shipclass[warsptr->shpclass].typename);
	prf(",%d,%u,%u,%u*\r",
		warsptr->shipno,
		warsptr->kills,
		warsptr->ukills,
		warsptr->freq);
	prf("STOP:SHIP*\r");
}

static void data_capabilities(void)
{
	sprintf(gechrbuf,"%ld",(long)ship_accel(warsptr));
	sprintf(gechrbuf2,"%ld",ship_scanrange(warsptr));
	prf("CAP:%u,%u,%u,%u,%u,%u,%u,%s,%u,%s*\r",
		(unsigned)shipclass[warsptr->shpclass].max_torps,
		(unsigned)shipclass[warsptr->shpclass].max_missl,
		(unsigned)shipclass[warsptr->shpclass].has_decoy,
		(unsigned)shipclass[warsptr->shpclass].has_jam,
		(unsigned)shipclass[warsptr->shpclass].has_zip,
		(unsigned)shipclass[warsptr->shpclass].has_mine,
		(unsigned)shipclass[warsptr->shpclass].max_cloak,
		gechrbuf,
		(unsigned)warsptr->topspeed,
		gechrbuf2);
	prf("STOP:CAP*\r");
}

void FUNC cmd_data(void)
{
	if (sameas(frontend,"OFF")) {
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 3 && sameas(margv[1],"ON")) {
		if (sameas(frontend,"ALL") || strcmp(margv[2],frontend) == 0) {
			data_enabled[usrnum] |= GEDATA_ACCESS;
			prf("ON:");
			data_text(margv[2]);
			prf(",");
			data_text(PROJECT_NAME);
			prf(",");
			data_text(PROJECT_VERSION);
			prf(",");
			data_text(PROJECT_ARCH);
			prf("*\rSTOP:ON*\r");
			outprfge(FLT_NONE,usrnum);
			return;
		}
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (!sameas(frontend,"ALL")
		&& !(data_enabled[usrnum] & GEDATA_ACCESS)) {
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 3 && sameas(margv[1],"METADATA")) {
		if (sameas(margv[2],"ON")) {
			prf("METADATA:ON*\rSTOP:METADATA*\r");
			outprfge(FLT_NONE,usrnum);
			data_enabled[usrnum] |= GEDATA_METADATA;
			return;
		}
		if (sameas(margv[2],"OFF")) {
			data_enabled[usrnum] &= (byte)~GEDATA_METADATA;
			prf("METADATA:OFF*\rSTOP:METADATA*\r");
			outprfge(FLT_NONE,usrnum);
			return;
		}
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (margc == 3 && sameas(margv[1],"SHIPSCAN")) {
		data_shipscan();
		return;
	}

	if (margc != 2) {
		prfmsg(INVCMD);
		outprfge(FLT_NONE,usrnum);
		return;
	}

	if (sameas(margv[1],"cap")) {
		data_capabilities();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"ship")) {
		data_ship();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"user")) {
		data_user();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"upg")) {
		data_upgrades();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"msg")) {
		data_messages();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"inv")) {
		data_inventory();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"nav")) {
		data_navigation();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"ord")) {
		data_ordnance();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"sys")) {
		data_systems();
		outprfge(FLT_NONE,usrnum);
		return;
	}
	if (sameas(margv[1],"scan")) {
		data_scan();
		return;
	}
	if (sameas(margv[1],"sector")) {
		data_sector();
		return;
	}

	prfmsg(INVCMD);
	outprfge(FLT_NONE,usrnum);
}
#else
void FUNC cmd_data(void)
{
	prfmsg(INVCMD);
	outprfge(FLT_NONE,usrnum);
}
#endif
