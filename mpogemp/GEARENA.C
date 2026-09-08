/*****************************************************************************
 * ge-next GEARENA.C                                                         *
 *                                                                           *
 * ge-next modifications ONLY copyright (C) 2024-2026 Anthony Schmidt       *
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

#ifdef PHARLAP

#include "gcomm.h"
#include "string.h"

#else

#include "stdio.h"
#include "ctype.h"
#include "mem.h"
#include "dos.h"
#include "usracc.h"
#include "btvstf.h"
#include "portable.h"
#include "dosface.h"

#endif

#include "math.h"
#include "stdlib.h"
#include "majorbbs.h"

#include "gemain.h"

#define GEARENA 1

#include "geglobal.h"

/* LOCAL GLOBAL DEFS *****************************************************/

static void arena_start_match(void);
static void arena_end_match(void);
static void arena_observe_match(int usrn);

static int arena_king_x;
static int arena_king_y;

/*
 * Arena ships and galaxy records are disposable match state; user records
 * persist only account settings and wins. ARENAPLAYER.state describes what
 * a terminal is doing now, while ready records its intent for the next match.
 */

/**************************************************************************
** Mode and persistent score helpers                                     **
**************************************************************************/

/* return the player-facing name for an arena mode */
static char *arena_mode_name(int mode)
{
	if (mode == ARENA_MODE_BATTLE)
		return "Battle";
	if (mode == ARENA_MODE_HOARD)
		return "Hoard";
	if (mode == ARENA_MODE_KING)
		return "King";
	if (mode == ARENA_MODE_BASE)
		return "Base";
	if (mode == ARENA_MODE_SCORED)
		return "Scored";
	return "unknown";
}

/* map a game mode to its persistent wins slot in WARUSR */
static int arena_mode_win_slot(int mode)
{
	if (mode == ARENA_MODE_BATTLE)
		return ARENA_WIN_BATTLE;
	if (mode == ARENA_MODE_HOARD)
		return ARENA_WIN_HOARD;
	if (mode == ARENA_MODE_KING)
		return ARENA_WIN_KING;
	if (mode == ARENA_MODE_BASE)
		return ARENA_WIN_BASE;
	if (mode == ARENA_MODE_SCORED)
		return ARENA_WIN_SCORED;
	return -1;
}

/* identify the stationary Cyber-Base configured for Base mode */
static int arena_is_base_ship(WARSHP *ptr)
{
	if (ptr == NULL || !VALID_SHPCLASS(ptr->shpclass))
		return FALSE;
	return shipclass[ptr->shpclass].max_type == CLASSTYPE_CYBORG &&
	    shipclass[ptr->shpclass].arena_mode == ARENA_MODE_BASE &&
	    shipclass[ptr->shpclass].max_accel == 0;
}

/* sum all persistent mode wins for roster sorting and display */
unsigned long FUNC arena_total_wins(WARUSR *ptr)
{
	unsigned long total;
	int i;

	total = 0UL;
	for (i = 0; i < ARENA_WIN_SLOTS; ++i)
		total += (unsigned long)ptr->arena_wins[i];
	return total;
}

/* refresh the legacy score fields used by the Btrieve roster index */
void FUNC arena_refresh_win_scores(void)
{
	unsigned long total;

	setbtv(gebb5);
	if (!qlobtv(0))
		return;
	do {
		gcrbtv(&tmpusr,0);
		total = arena_total_wins(&tmpusr);
		if (tmpusr.score != total || tmpusr.planets != 0 ||
		    tmpusr.plscore != 0 || tmpusr.population != 0) {
			tmpusr.score = total;
			tmpusr.planets = 0;
			tmpusr.plscore = 0;
			tmpusr.population = 0;
			updbtv(&tmpusr);
		}
		gcrbtv(&tmpusr,0);
	} while (qnxbtv());
}

/* load or create a persistent arena user in WARUSR */
static int arena_load_user(int usrn)
{
	struct usracc *uap;
	WARUSR *wuptr;

	uap = uacoff(usrn);
	if (uap == NULL || uap->userid[0] == 0)
		return FALSE;
	wuptr = warusroff(usrn);
	if (geudb(GELOOKUP,uap->userid,wuptr))
		return geudb(GEGET,uap->userid,wuptr);
	initusr(uap->userid);
	if (!geudb(GEADD,tmpusr.userid,&tmpusr))
		return FALSE;
	memcpy(wuptr,&tmpusr,sizeof(WARUSR));
	return TRUE;
}

/* record one completed-match win and update its roster sort value */
static void arena_record_win(int usrn)
{
	WARUSR *wuptr;
	int slot;

	slot = arena_mode_win_slot(arena_mode);
	if (usrn < 0 || usrn >= nterms || slot < 0 || slot >= ARENA_WIN_SLOTS)
		return;
	wuptr = warusroff(usrn);
	if (wuptr->arena_wins[slot] != (unsigned)~0U)
		++wuptr->arena_wins[slot];
	wuptr->score = arena_total_wins(wuptr);
	geudb(GEUPDATE,wuptr->userid,wuptr);
}

/**************************************************************************
** Ship selection and loadouts                                           **
**************************************************************************/

/* return the player's valid selection, falling back to this mode's first class */
int FUNC arena_selected_shipclass(int usrn)
{
	int cls;

	cls = arena_player[usrn].shipclass;
	if (!VALID_SHPCLASS(cls) || shipclass[cls].max_type != CLASSTYPE_USER ||
	    shipclass[cls].arena_mode != arena_mode)
		return arena_shipclass_for_choice(0);
	return cls;
}

/* translate a zero-based menu choice into this mode's ship-class index */
int FUNC arena_shipclass_for_choice(int choice)
{
	int i;

	if (choice < 0)
		return -1;
	for (i = 0; i < tot_classes; ++i) {
		if (shipclass[i].max_type != CLASSTYPE_USER ||
		    shipclass[i].arena_mode != arena_mode)
			continue;
		if (choice-- == 0)
			return i;
	}
	return -1;
}

/* return the sole user class for this mode, or -1 when selection is needed */
static int arena_single_shipclass(void)
{
	int cls;

	cls = arena_shipclass_for_choice(0);
	if (cls >= 0 && arena_shipclass_for_choice(1) < 0)
		return cls;
	return -1;
}

/* validate a numeric ship choice and return its zero-based menu index */
int FUNC arena_parse_ship_choice(char *text)
{
	char *ptr;
	long choice;

	if (text == NULL || *text == 0)
		return -1;
	for (ptr = text; *ptr != 0; ++ptr)
		if (!isdigit((unsigned char)*ptr))
			return -1;
	choice = atol(text);
	if (choice < 1L || choice > 32767L ||
	    arena_shipclass_for_choice((int)choice - 1) < 0)
		return -1;
	return (int)choice - 1;
}

/* assign systems and items based on this class' loadout */
void FUNC arena_apply_loadout(WARSHP *ptr, int cls)
{
	SHIP *classptr;
	int item;

	classptr = &shipclass[cls];
	setmem(ptr->items, sizeof(ptr->items), 0);
	ptr->phasrtype = classptr->arena_start_phaser;
	ptr->shieldtype = classptr->arena_start_shield;
	for (item = 0; item < NUMITEMS; ++item)
		ptr->items[item] = classptr->arena_start_items[item];
}

/* commit a ship choice to an existing arena ship and expose it to scans */
void FUNC arena_select_ship(int usrn, int choice)
{
	WARSHP *ptr;
	int cls;

	cls = arena_shipclass_for_choice(choice);
	if (cls < 0)
		cls = arena_shipclass_for_choice(0);
	if (cls < 0)
		return;
	arena_player[usrn].shipclass = (byte)cls;
	ptr = warshpoff(usrn);
	ptr->shpclass = arena_selected_shipclass(usrn);
	ptr->topspeed = shipclass[ptr->shpclass].max_warp;
	ptr->speed = 0.0;
	ptr->speed2b = 0.0;
	ptr->where = 0;
	ptr->damage = 0.0;
	ptr->energy = 50000L;
	ptr->phasr = 100;
	ptr->shieldstat = SHIELDDN;
	ptr->lastfired = -1;
	ptr->lock = -1;
	arena_apply_loadout(ptr, ptr->shpclass);
	ptr->status = GESTAT_USER;
	arena_player[usrn].flags &= ~ARENA_F_NEEDSHIP;
	update_scantab(ptr, usrn);
}

/* append one configured loadout item, wrapping at commas before column 80 */
static void arena_show_choice_item(int *column, int *first, unsigned qty,
	char *singular, char *plural)
{
	char item[40];
	int length;

	if (qty == 0)
		return;
	sprintf(item,"%u %s",qty,qty == 1 ? singular : plural);
	length = (int)strlen(item);
	if (*first) {
		prf("     ");
		*column = 5;
		*first = FALSE;
	}
	else if (*column + 2 + length > 78) {
		prf(",\r     ");
		*column = 5;
	}
	else {
		prf(", ");
		*column += 2;
	}
	prf("%s",item);
	*column += length;
}

/* print one wrapped ship loadout generated from the class table */
static void arena_show_ship_loadout(SHIP *classptr, int choice)
{
	char points[24];
	int column, first, length;

	if (choice > 0)
		prf("  %s%d %s%s%s:",CLR_CYAN2,choice,CLR_CYAN1,
		    classptr->typename,CLR_WHITE2);
	else
		prf("  %s%s%s:",CLR_CYAN1,classptr->typename,CLR_WHITE2);
	if (classptr->arena_start_shield > 0)
		prf(" Mark-%d shield",classptr->arena_start_shield);
	if (classptr->arena_start_phaser > 0) {
		if (classptr->arena_start_shield > 0)
			prf(",");
		prf(" Mark-%d phaser",classptr->arena_start_phaser);
	}
	prf("\r");
	column = 0;
	first = TRUE;
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_TORPEDO],"torpedo","torpedoes");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_MISSILE],"missile","missiles");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_MINE],"mine","mines");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_DECOYS],"decoy","decoys");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_ZIPPERS],"zipper","zippers");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_JAMMERS],"jammer","jammers");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_FLUXPOD],"flux pod","flux pods");
	arena_show_choice_item(&column,&first,
	    classptr->arena_start_items[I_GOLD],"gold","gold");
	if (arena_mode == ARENA_MODE_SCORED) {
		sprintf(points,"(%u point%s)",classptr->max_points,
		    classptr->max_points == 1 ? "" : "s");
		length = (int)strlen(points);
		if (first) {
			prf("     %s",points);
			first = FALSE;
		}
		else if (column + 1 + length > 78)
			prf("\r     %s",points);
		else
			prf(" %s",points);
	}
	if (!first)
		prf("\r");
}

/* print the wrapped ship-selection list generated from the class table */
void FUNC arena_show_ship_choices(void)
{
	SHIP *classptr;
	int choice, i;

	prf("\rShips:\r");
	choice = 0;
	for (i = 0; i < tot_classes; ++i) {
		classptr = &shipclass[i];
		if (classptr->max_type != CLASSTYPE_USER ||
		    classptr->arena_mode != arena_mode)
			continue;
		++choice;
		arena_show_ship_loadout(classptr,choice);
	}
	if (choice > 0)
		prfmsg(SHPPROM, choice);
}

/* place a player at the initial arena ship prompt */
static void arena_show_initial_ship_choice(int usrn)
{
	user[usrn].substt = FIGHTSUB;
	if (usrn == usrnum)
		usrptr->substt = FIGHTSUB;
	btupmt(usrn,'>');
	prfmsg(ENTSHP);
	outprfge(FLT_NONE,usrn);
	clrprf();
}

/* mini HELP CLASS with functions unused in arena removed */
void FUNC arena_show_ship_classes(void)
{
	char warp[12], shields[12], phasers[12], torps[12], missiles[12];
	char accel[12], points[6];
	SHIP *classptr;
	int choice, i;

	setmbk(gehlpmb);
	prfmsg(HLPCLS1, arena_mode_name(arena_mode));
	choice = 0;
	for (i = 0; i < tot_classes; ++i) {
		classptr = &shipclass[i];
		if (classptr->max_type != CLASSTYPE_USER ||
		    classptr->arena_mode != arena_mode)
			continue;
		++choice;

		if (arena_mode == ARENA_MODE_HOARD)
			strcpy(gechrbuf, "-");
		else if (classptr->max_tons > 999999L)
			sprintf(gechrbuf, "%ldm", classptr->max_tons / 1000000L);
		else if (classptr->max_tons > 999L)
			sprintf(gechrbuf, "%ldk", classptr->max_tons / 1000L);
		else
			sprintf(gechrbuf, "%ld", classptr->max_tons);

		if (classptr->scanrange > 999999L)
			sprintf(gechrbuf3, "%ldm", classptr->scanrange / 1000000L);
		else if (classptr->scanrange > 999L)
			sprintf(gechrbuf3, "%ldk", classptr->scanrange / 1000L);
		else
			sprintf(gechrbuf3, "%ld", classptr->scanrange);

		if (classptr->max_warp == 0)
			strcpy(warp, CLR_RED1 " N");
		else
			sprintf(warp, "%s%2d", CLR_WHITE2, classptr->max_warp);
		if (classptr->max_shlds == 0)
			strcpy(shields, CLR_RED1 " N");
		else
			sprintf(shields, "%s%2d", CLR_WHITE2, classptr->max_shlds);
		if (classptr->max_phasr == 0)
			strcpy(phasers, CLR_RED1 " N");
		else
			sprintf(phasers, "%s%2d", CLR_WHITE2, classptr->max_phasr);
		if (classptr->max_torps == 0)
			strcpy(torps, CLR_RED1 "N");
		else
			sprintf(torps, "%s%1d", CLR_GREEN2, classptr->max_torps);
		if (classptr->max_missl == 0)
			strcpy(missiles, CLR_RED1 "N");
		else
			sprintf(missiles, "%s%1d", CLR_GREEN2, classptr->max_missl);
		if (classptr->max_accel > 999)
			sprintf(accel, "%s%2dk", CLR_WHITE2, classptr->max_accel / 1000);
		else
			sprintf(accel, "%s%3d", CLR_WHITE2, classptr->max_accel);

		if (arena_mode == ARENA_MODE_SCORED)
			sprintf(points, "%u", classptr->max_points);
		else
			strcpy(points, "-");
		prf("%s%2d %s%-24s       %s %s %s %s %s %s %s %s %s %s   %4s %4s %4s %5s\r",
			CLR_CYAN2, choice,
			CLR_CYAN1, classptr->typename,
			warp,
			shields,
			phasers,
			torps,
			missiles,
			classptr->has_mine ? CLR_GREEN2 "Y" : CLR_RED1 "N",
			" ",
			classptr->has_decoy ? CLR_GREEN2 "Y" : CLR_RED1 "N",
			classptr->has_jam ? CLR_GREEN2 "Y" : CLR_RED1 "N",
			classptr->has_zip ? CLR_GREEN2 "Y" : CLR_RED1 "N",
			accel,
			gechrbuf3,
			gechrbuf,
			points);
	}
	prfmsg(HLPCLS2);
}

/**************************************************************************
** Lobby counts, hosting, and broadcasts                                 **
**************************************************************************/

/* report whether a player is participating, including between ships */
static int arena_player_active(int usrn)
{
	if (usrn < 0 || usrn >= nterms)
		return FALSE;
	return arena_player[usrn].state == ARENA_P_PLAYING ||
	    arena_player[usrn].state == ARENA_P_RESPAWN;
}

/* count every terminal currently represented in the arena module */
static int arena_count_present(void)
{
	int i, count;

	count = 0;
	for (i = 0; i < nterms; ++i)
		if (arena_player[i].state != ARENA_P_EMPTY)
			++count;
	return count;
}

/* count players committed to the next match, including the host */
static int arena_count_ready(void)
{
	int i, count;

	count = 0;
	for (i = 0; i < nterms; ++i)
		if (arena_player[i].state != ARENA_P_EMPTY && arena_player[i].ready)
			++count;
	return count;
}

/* count terminals whose current arena state is observing */
static int arena_count_observe(void)
{
	int i, count;

	count = 0;
	for (i = 0; i < nterms; ++i)
		if (arena_player[i].state == ARENA_P_OBSERVE)
			++count;
	return count;
}

/* count active ships and players waiting to respawn */
static int arena_count_playing(void)
{
	int i, count;

	count = 0;
	for (i = 0; i < nterms; ++i)
		if (arena_player_active(i))
			++count;
	return count;
}

/* scale scanner range for the player count encoded in the galaxy radius */
int FUNC arena_scan_percent(void)
{
	int players;

	if (arena_state != ARENA_STAGING && arena_state != ARENA_RUNNING)
		return 100;
	players = univmax / 5;
	if (players <= 2)
		return 50;
	if (players == 3)
		return 75;
	return 100;
}

/* print the queue or active-match summary, optionally aligned to its table */
static void arena_show_mini_status(int align_table)
{
	int active, fieldgap, lastgap, line_width, present, playing, used_width;
	char *mode;

	present = arena_count_present();
	mode = arena_mode_name(arena_mode);
	active = arena_state == ARENA_STAGING || arena_state == ARENA_RUNNING;
	fieldgap = 2;
	lastgap = 2;
	if (align_table) {
		line_width = (arena_mode == ARENA_MODE_BATTLE ||
		    arena_mode == ARENA_MODE_BASE) ? 67 : 74;
		if (arena_mode == ARENA_MODE_BATTLE)
			fieldgap = 4;
		else if (arena_mode == ARENA_MODE_BASE)
			fieldgap = 5;
		else
			fieldgap = 6;
		/* Text and values excluding the three gaps use 49 active, 47 in lobby. */
		used_width = active ? 49 : 47;
		lastgap = line_width - used_width - (int)strlen(mode) -
		    (fieldgap * 2);
	}
	sprintf(gechrbuf,"%*s",fieldgap,"");
	sprintf(gechrbuf2,"%*s",lastgap,"");
	if (active) {
		playing = arena_count_playing();
		prfmsg(MATSTAT, present, gechrbuf, playing, gechrbuf,
		    present - playing, gechrbuf2, mode);
	}
	else {
		prfmsg(LOBSTAT, present, gechrbuf, arena_count_ready(), gechrbuf,
		    arena_count_observe(), gechrbuf2, mode);
	}
}

/* notify the selected terminal that it has inherited host duties */
static void arena_notify_new_host(void)
{
	if (arena_host < 0 || arena_host >= nterms)
		return;
	clrprf();
	prfmsg(LOBNEWH);
	arena_show_mini_status(FALSE);
	outprfge(FLT_NONE, arena_host);
	clrprf();
}

/* select the lowest eligible terminal as host, optionally skipping one user */
static void arena_choose_host_ex(int skip, int notify)
{
	int i;
	int oldhost;

	if (arena_host >= 0 && arena_host < nterms &&
	    arena_player[arena_host].state != ARENA_P_EMPTY &&
	    arena_player[arena_host].state != ARENA_P_OBSERVE)
		return;
	oldhost = arena_host;
	arena_host = -1;
	/* prefer a participant who has not explicitly chosen to observe */
	for (i = 0; i < nterms; ++i) {
		if (i != skip && arena_player[i].state != ARENA_P_EMPTY &&
		    arena_player[i].state != ARENA_P_OBSERVE) {
			arena_host = i;
			if (arena_state != ARENA_STAGING && arena_state != ARENA_RUNNING) {
				arena_player[i].state = ARENA_P_READY;
				arena_player[i].ready = TRUE;
			}
			if (notify && oldhost != arena_host)
				arena_notify_new_host();
			return;
		}
	}
	/* if everyone is observing, promote the first remaining terminal */
	for (i = 0; i < nterms; ++i) {
		if (i != skip && arena_player[i].state != ARENA_P_EMPTY) {
			arena_host = i;
			if (arena_state != ARENA_STAGING && arena_state != ARENA_RUNNING) {
				arena_player[i].state = ARENA_P_READY;
				arena_player[i].ready = TRUE;
			}
			if (notify && oldhost != arena_host)
				arena_notify_new_host();
			return;
		}
	}
}

/* select a host without excluding or notifying any terminal */
static void arena_choose_host(void)
{
	arena_choose_host_ex(-1, FALSE);
}

/* restore idle defaults after the last arena user leaves */
static void arena_reset_if_empty(void)
{
	if (arena_count_present() != 0)
		return;
	arena_state = ARENA_IDLE;
	arena_host = -1;
	arena_ticks = 0;
	arena_match_ticks = 0;
	arena_mode = ARENA_MODE_BATTLE;
}

/* send the current print buffer to all arena users except one terminal */
static void arena_broadcast_prf_except(int skip)
{
	int i;

	for (i = 0; i < nterms; ++i) {
		if (i != skip && arena_player[i].state != ARENA_P_EMPTY) {
			outprfge(FLT_NONE, i);
		}
	}
	clrprf();
}

/* send the current print buffer to every terminal in the arena module */
static void arena_broadcast_prf(void)
{
	arena_broadcast_prf_except(-1);
}

/* broadcast a one-line lobby chat message entered with the > command */
static void arena_lobby_chat(void)
{
	char *msg;

	if (margc < 2) {
		prfmsg(FORMAT,"SEND");
		outprfge(FLT_NONE,usrnum);
		return;
	}
	msg = margv[1];
	rstrin();
	prfmsg(LOBCHAT,waruptr->userid,msg);
	arena_broadcast_prf();
}

/* start or cancel autostart when the queue crosses two ready players */
static void arena_update_queue_timer(void)
{
	if (arena_state != ARENA_QUEUE)
		return;
	if (arena_count_ready() >= 2) {
		if (arena_ticks <= 0) {
			arena_ticks = ARENA_AUTOSTART_TIME;
			prfmsg(AUTOBRDC, arena_mode_name(arena_mode), arena_ticks);
			arena_broadcast_prf_except(usrnum);
		}
	}
	else if (arena_ticks > 0) {
		arena_ticks = 0;
		prfmsg(AUTOCAN);
		arena_broadcast_prf();
	}
}

/* announce a lobby departure and refresh the host's population summary */
static void arena_announce_lobby_exit(char *userid, int newhost)
{
	int i;

	if (arena_state != ARENA_QUEUE || userid == NULL || userid[0] == 0)
		return;
	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state == ARENA_P_EMPTY)
			continue;
		clrprf();
			prfmsg(PEACEOUT, userid);
			if (i == arena_host) {
				if (newhost)
					prfmsg(LOBNEWH);
				arena_show_mini_status(FALSE);
			}
		outprfge(FLT_NONE, i);
		clrprf();
	}
}

/* remove a terminal from arena state and end an undersized active match */
void FUNC arena_leave(int usrn)
{
	WARUSR *wuptr;
	int announce;
	int oldhost;
	int was_playing;

	if (usrn < 0 || usrn >= nterms || arena_player == NULL)
		return;
	was_playing = arena_player_active(usrn);
	/* use the normal match-exit path first to score, announce, and clear the ship */
	if (was_playing)
		arena_observe_match(usrn);
	announce = (arena_state == ARENA_QUEUE &&
	    arena_player[usrn].state != ARENA_P_EMPTY &&
	    arena_count_present() > 1);
	wuptr = warusroff(usrn);
	oldhost = arena_host;
	/* remove transient membership, then choose a replacement host before announcing */
	setmem(&arena_player[usrn], sizeof(ARENAPLAYER), 0);
	if (arena_host == usrn)
		arena_host = -1;
	arena_choose_host_ex(-1, FALSE);
	if (announce)
		arena_announce_lobby_exit(wuptr->userid, oldhost != arena_host);
	/* end an undersized match, or adjust the queue and reset an empty arena */
	if (was_playing && arena_state != ARENA_QUEUE && arena_count_playing() < 2)
		arena_end_match();
	arena_update_queue_timer();
	arena_reset_if_empty();
}

/**************************************************************************
** Lobby display and command actions                                     **
**************************************************************************/

/* list selectable modes and give the host the mode-selection hint */
static void arena_show_modes(void)
{
	prfmsg(MODLIST, arena_mode_name(arena_mode));
	if (arena_host == usrnum)
		prfmsg(MODHINT);
}

/* print the remaining match time with compact minute/second formatting */
static void arena_show_match_time(void)
{
	if (arena_match_ticks < 60)
		prfmsg(MATTIME, arena_match_ticks);
	else
		prfmsg(MATTIME2, arena_match_ticks / 60, arena_match_ticks % 60);
}

/* print how much scheduled time remained when a match ended early */
static void arena_show_match_ended_time(void)
{
	if (arena_match_ticks < 60)
		prfmsg(ENDTIME, arena_match_ticks);
	else
		prfmsg(ENDTIME2, arena_match_ticks / 60, arena_match_ticks % 60);
}

/* print only the commands appropriate to the current user's lobby role */
static void arena_show_lobby_prompt(void)
{
	if (arena_host == usrnum)
		prfmsg(LOBPHST);
	else if (arena_player[usrnum].ready)
		prfmsg(LOBPRDY);
	else
		prfmsg(LOBPOBS);
}

/* print lobby status, the user's queue state, timers, and command prompt */
static void arena_show_lobby(void)
{
	arena_show_mini_status(FALSE);
	if (arena_host == usrnum)
		prfmsg(LOBHOST);
	else if (arena_player[usrnum].ready)
		prfmsg(LOBREADY);
	else if (arena_player[usrnum].state == ARENA_P_OBSERVE)
		prfmsg(LOBOBS);
	if (arena_state == ARENA_STAGING)
		prfmsg(STGSTAT, arena_ticks);
	else if (arena_state == ARENA_QUEUE && arena_ticks > 0)
		prfmsg(AUTOTIME, arena_mode_name(arena_mode), arena_ticks);
	else if (arena_state == ARENA_RUNNING)
		arena_show_match_time();
	arena_show_lobby_prompt();
}

/* announce a new lobby arrival to everyone already present */
static void arena_announce_lobby_entry(int usrn)
{
	int i;
	WARUSR *wuptr;

	if (arena_state != ARENA_QUEUE)
		return;
	wuptr = warusroff(usrn);
	for (i = 0; i < nterms; ++i) {
		if (i == usrn || arena_player[i].state == ARENA_P_EMPTY)
			continue;
			clrprf();
			prfmsg(ANNOUN, wuptr->userid);
			if (i == arena_host)
				arena_show_mini_status(FALSE);
			outprfge(FLT_NONE, i);
		clrprf();
	}
}

/* admit the current terminal, load its user, and initialize its queue state */
int FUNC arena_enter_lobby(void)
{
	int first;
	int entering;
	int needs_host;

	if (arena_player[usrnum].state == ARENA_P_EMPTY &&
	    arena_count_present() >= ARENA_MAX_PLAYERS) {
		prfmsg(LOBFULL);
		btupmt(usrnum, 0);
		return FALSE;
	}
	if (!arena_load_user(usrnum)) {
		btupmt(usrnum, 0);
		return FALSE;
	}
	first = (arena_count_present() == 0);
	needs_host = (arena_host < 0);
	entering = (arena_player[usrnum].state == ARENA_P_EMPTY);
	if (arena_player[usrnum].state == ARENA_P_EMPTY) {
		arena_player[usrnum].state = (first || needs_host) ? ARENA_P_READY : ARENA_P_OBSERVE;
		arena_player[usrnum].ready = first || needs_host;
	}
	if (arena_state == ARENA_IDLE)
		arena_state = ARENA_QUEUE;
	arena_choose_host();
	if (entering && !first)
		arena_announce_lobby_entry(usrnum);
	arena_update_queue_timer();
	prfmsg(WELCOM, waruptr->userid);
	arena_show_lobby();
	usrptr->substt = ARENASUB;
	btupmt(usrnum, '>');
	return TRUE;
}

/* mark the current user ready now or queued for the match after this one */
static int arena_set_ready(void)
{
	if (arena_state == ARENA_STAGING || arena_state == ARENA_RUNNING) {
		if (arena_player_active(usrnum)) {
			prfmsg(LOBREADY);
			return TRUE;
		}
		arena_player[usrnum].ready = TRUE;
		arena_player[usrnum].state = ARENA_P_OBSERVE;
		prfmsg(LOBREADY);
		return TRUE;
	}
	if (arena_host == usrnum)
		return TRUE;
	arena_player[usrnum].state = ARENA_P_READY;
	arena_player[usrnum].ready = TRUE;
	arena_choose_host();
	arena_update_queue_timer();
	return TRUE;
}

/* mark the current user observing and hand off hosting when necessary */
static void arena_set_observe(void)
{
	arena_player[usrnum].state = ARENA_P_OBSERVE;
	arena_player[usrnum].ready = FALSE;
	if (arena_host == usrnum)
		arena_host = -1;
	arena_choose_host_ex(usrnum, TRUE);
	arena_update_queue_timer();
}

/* validate and apply a host's numeric game-mode selection */
static int arena_set_mode(char *mode)
{
	int newmode;

	if (arena_host != usrnum) {
		prfmsg(MODHOST);
		return FALSE;
	}
	if (arena_state == ARENA_STAGING || arena_state == ARENA_RUNNING) {
		prfmsg(MODLOCK);
		return FALSE;
	}
	if (sameas("1", mode))
		newmode = ARENA_MODE_BATTLE;
	else if (sameas("2", mode))
		newmode = ARENA_MODE_HOARD;
	else if (sameas("3", mode))
		newmode = ARENA_MODE_KING;
	else if (sameas("4", mode))
		newmode = ARENA_MODE_BASE;
	else if (sameas("5", mode))
		newmode = ARENA_MODE_SCORED;
	else {
		prfmsg(MODUNK);
		arena_show_modes();
		return FALSE;
	}
	if (newmode != arena_mode) {
		arena_mode = newmode;
		prfmsg(MODCHG, arena_mode_name(arena_mode));
		arena_broadcast_prf_except(usrnum);
	}
	prfmsg(MODSET, arena_mode_name(arena_mode));
	return TRUE;
}

/* let the host bypass autostart and begin staging when enough users are ready */
static int arena_start_countdown(void)
{
	if (arena_host != usrnum) {
		prfmsg(GOHOST);
		return FALSE;
	}
	if (arena_state == ARENA_STAGING) {
		prfmsg(GOSTAGE);
		return FALSE;
	}
	if (arena_state == ARENA_RUNNING) {
		prfmsg(GORUN);
		return FALSE;
	}
	if (arena_count_ready() < 2) {
		prfmsg(GOMIN);
		return FALSE;
	}
	arena_ticks = 0;
	arena_start_match();
	return TRUE;
}

/**************************************************************************
** Match lifecycle                                                       **
**************************************************************************/

/* build a temporary arena ship, preserving the caller's terminal globals */
static int arena_spawn_player(int usrn)
{
	int oldusr;
	int cls;
	int respawning;
	struct user *oldusrptr;
	WARSHP *oldsptr;
	WARUSR *olduptr;
	int span;

	oldusr = usrnum;
	respawning = arena_player[usrn].state == ARENA_P_RESPAWN;
	oldusrptr = usrptr;
	oldsptr = warsptr;
	olduptr = waruptr;
	/* legacy ship initialization operates on the current-terminal globals */
	usrnum = usrn;
	usrptr = &user[usrn];
	warsptr = warshpoff(usrn);
	waruptr = warusroff(usrn);
	waruptr->topshipno = 0;
	waruptr->noships = 0;
	cls = arena_selected_shipclass(usrn);
	if (cls < 0 || initshp(waruptr->userid,cls)) {
		usrnum = oldusr;
		usrptr = oldusrptr;
		warsptr = oldsptr;
		waruptr = olduptr;
		return FALSE;
	}
	memcpy(warsptr,&tmpshp,sizeof(WARSHP));
	arena_apply_loadout(warsptr,cls);
	if (arena_state == ARENA_RUNNING) {
		span = (univmax * 2) + 1;
		do {
			warsptr->coord.xcoord =
			    (double)((int)(gernd() % span) - univmax)
			    + rndm(.9998) + .0001;
			warsptr->coord.ycoord =
			    (double)((int)(gernd() % span) - univmax)
			    + rndm(.9998) + .0001;
		} while (arena_mode == ARENA_MODE_KING &&
		    coord1(warsptr->coord.xcoord) == arena_king_x &&
		    coord1(warsptr->coord.ycoord) == arena_king_y);
	}
	warsptr->where = 0;
	if (arena_state == ARENA_RUNNING || respawning) {
		warsptr->status = GESTAT_USER;
		arena_player[usrn].flags &= ~ARENA_F_NEEDSHIP;
		update_scantab(warsptr,usrn);
	}
	else {
		/* staged ships remain hidden until their players select a class */
		warsptr->status = GESTAT_AVAIL;
		arena_player[usrn].flags |= ARENA_F_NEEDSHIP;
	}
	usrptr->substt = FIGHTSUB;
	user[usrn].substt = FIGHTSUB;
	btupmt(usrn,'>');
	if (respawning) {
		prfmsg(RSPAWN, shipclass[arena_selected_shipclass(usrn)].typename,
		    coord1(warsptr->coord.xcoord), coord1(warsptr->coord.ycoord));
		outprfge(FLT_NONE,usrn);
		clrprf();
	}
	usrnum = oldusr;
	usrptr = oldusrptr;
	warsptr = oldsptr;
	waruptr = olduptr;
	return TRUE;
}

/* move a player with a failed ship initialization to a safe observing state */
static void arena_spawn_failed(int usrn)
{
	arena_player[usrn].state = ARENA_P_OBSERVE;
	arena_player[usrn].ready = FALSE;
	arena_player[usrn].flags = 0;
	arena_player[usrn].respawn = 0;
	if (arena_host == usrn) {
		arena_host = -1;
		arena_choose_host_ex(usrn,FALSE);
	}
}

/* advance pending respawns during both staging and active combat */
static void arena_respawn_tick(void)
{
	int i;

	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state == ARENA_P_RESPAWN) {
			if (arena_player[i].respawn > 0)
				--arena_player[i].respawn;
			if (arena_player[i].respawn == 0) {
				if (arena_spawn_player(i))
					arena_player[i].state = ARENA_P_PLAYING;
				else
					arena_spawn_failed(i);
			}
		}
	}
}

/* convert ready users into staged players and create their hidden ships */
static void arena_start_match(void)
{
	unsigned long newseed;
	int i;
	int players;
	int single_class;

	/* staging has its own countdown; the match clock starts with combat */
	arena_state = ARENA_STAGING;
	arena_ticks = ARENA_STAGING_TIME;
	arena_match_ticks = 0;
	/* set the nebseed here for each match instead of using Zygor */
	newseed = (((unsigned long)gernd()) << 16) | (unsigned long)gernd();
	if (newseed == 0L)
		newseed = 1L;
	if (newseed == nebseed) {
		++newseed;
		if (newseed == 0L)
			newseed = 1L;
	}
	nebseed = newseed;
	/* scale the disposable galaxy to the number entering this match */
	players = arena_count_ready();
	if (players > 0)
		univmax = players * 5;
	single_class = arena_single_shipclass();
	for (i = 0; i < nterms; ++i) {
		arena_player[i].kills = 0;
		arena_player[i].deaths = 0;
		arena_player[i].kingtime = 0;
		arena_player[i].basekills = 0;
		arena_player[i].score = 0L;
		if (arena_player[i].state == ARENA_P_READY) {
			if (single_class >= 0)
				arena_player[i].shipclass = (byte)single_class;
			arena_show_initial_ship_choice(i);
		}
	}
	/* announce staging before sending each participant the ship list */
	prfmsg(STGSTART, arena_mode_name(arena_mode), arena_ticks);
	arena_broadcast_prf();
	for (i = 0; i < nterms; ++i) {
		if (single_class < 0 && arena_player[i].state == ARENA_P_READY) {
			arena_show_ship_choices();
			outprfge(FLT_NONE,i);
			clrprf();
		}
	}
	/* create staged ships; multi-class ships stay hidden until selected */
	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state == ARENA_P_READY) {
			arena_player[i].ready = FALSE;
			if (arena_spawn_player(i)) {
				arena_player[i].state = ARENA_P_PLAYING;
				if (single_class >= 0) {
					arena_select_ship(i,0);
					prfmsg(SHPLOAD);
					arena_show_ship_loadout(&shipclass[single_class],0);
					outprfge(FLT_NONE,i);
					clrprf();
				}
			}
			else
				arena_spawn_failed(i);
		}
	}
	/* initialization failures must not leave a one-player match in staging */
	if (arena_count_playing() < 2) {
		arena_end_match();
		return;
	}
}

/* auto-select missing ships, expose all players, and start the match clock */
static void arena_start_combat(void)
{
	int i;

	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state == ARENA_P_PLAYING &&
		    (arena_player[i].flags & ARENA_F_NEEDSHIP)) {
			arena_select_ship(i,0);
			prfmsg(SHPSEL, shipclass[arena_selected_shipclass(i)].typename);
			outprfge(FLT_NONE,i);
			clrprf();
		}
	}
	arena_state = ARENA_RUNNING;
	arena_ticks = 0;
	arena_match_ticks = ARENA_MATCH_TIME;
	prfmsg(MATSTART, arena_mode_name(arena_mode));
	arena_broadcast_prf();
	if (arena_mode == ARENA_MODE_KING) {
		arena_king_x = 0;
		arena_king_y = 0;
		prfmsg(KNGSECT,arena_king_x,arena_king_y);
		arena_broadcast_prf();
	}
}

/* apply normal neutral rules except while arena combat is active */
int FUNC arena_neutral_fire_blocked(WARSHP *ptr, int usrn)
{
	if (arena_player == NULL)
		return neutral(&ptr->coord);
	if (arena_state == ARENA_STAGING)
		return TRUE;
	if (arena_state == ARENA_RUNNING) {
		if (usrn >= 0 && usrn < nterms &&
		    arena_player[usrn].state == ARENA_P_PLAYING)
			return FALSE;
		if (usrn >= nterms && usrn < nships &&
		    ptr->status == GESTAT_AUTO)
			return FALSE;
	}
	return neutral(&ptr->coord);
}

/* report whether neutral-zone projectile protection should be enforced */
int FUNC arena_neutral_protection_active(void)
{
	return arena_state != ARENA_RUNNING;
}

/* keep staged players inside sector 0 0 using the normal barrier bounce */
void FUNC arena_enforce_staging_bounds(WARSHP *ptr, int usrn)
{
	COORD center;
	int bounced;

	if (arena_player == NULL || arena_state != ARENA_STAGING ||
	    usrn < 0 || usrn >= nterms ||
	    arena_player[usrn].state != ARENA_P_PLAYING)
		return;

	bounced = FALSE;
	if (ptr->coord.xcoord < 0.0) {
		ptr->coord.xcoord = 0.1;
		bounced = TRUE;
	}
	else if (ptr->coord.xcoord >= 1.0) {
		ptr->coord.xcoord = 0.9;
		bounced = TRUE;
	}
	if (ptr->coord.ycoord < 0.0) {
		ptr->coord.ycoord = 0.1;
		bounced = TRUE;
	}
	else if (ptr->coord.ycoord >= 1.0) {
		ptr->coord.ycoord = 0.9;
		bounced = TRUE;
	}
	if (bounced) {
		center.xcoord = 0.50001;
		center.ycoord = 0.50001;
		ptr->head2b = normal(vector(&ptr->coord,&center));
		ptr->heading = ptr->head2b;
		telezip(ptr,usrn);
	}
}

/* return the current Hoard score; ships awaiting respawn have no gold */
static unsigned long arena_player_gold(int usrn)
{
	WARSHP *ptr;

	if (usrn < 0 || usrn >= nterms ||
	    arena_player[usrn].state != ARENA_P_PLAYING)
		return 0UL;
	ptr = warshpoff(usrn);
	if (ptr->status != GESTAT_USER)
		return 0UL;
	return ptr->items[I_GOLD];
}

/* reveal exact Hoard standings for the final three minutes and results */
static int arena_hoard_scores_public(void)
{
	return arena_mode == ARENA_MODE_HOARD &&
	    arena_state == ARENA_RUNNING && arena_match_ticks <= 180;
}

/* move the King objective to a different random sector */
static void arena_king_move_sector(void)
{
	int newx, newy, span;

	span = (univmax * 2) + 1;
	do {
		newx = (int)(gernd() % span) - univmax;
		newy = (int)(gernd() % span) - univmax;
	} while ((newx == arena_king_x && newy == arena_king_y) ||
	    innebula(newx,newy));
	arena_king_x = newx;
	arena_king_y = newy;
	prfmsg(KNGSECT,arena_king_x,arena_king_y);
	arena_broadcast_prf();
}

/* award one second to each live player occupying the King sector */
static void arena_king_score_tick(void)
{
	WARSHP *ptr;
	int i;

	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state != ARENA_P_PLAYING)
			continue;
		ptr = warshpoff(i);
		if (ptr->status == GESTAT_USER &&
		    coord1(ptr->coord.xcoord) == arena_king_x &&
		    coord1(ptr->coord.ycoord) == arena_king_y &&
		    arena_player[i].kingtime != (unsigned)~0U)
			++arena_player[i].kingtime;
	}
}

/* determine and announce the result, persist valid wins, and show final status */
static void arena_show_results(void)
{
	int i;
	int winner;
	int highscore;
	int tied;
	int forfeit;
	unsigned long gold;
	unsigned long highgold;
	unsigned hightime;
	unsigned highbases;
	long highpoints;
	WARUSR *wuptr;

	forfeit = arena_match_ticks > 0 && arena_count_playing() == 1;
	winner = -1;
	highscore = 0;
	highgold = 0UL;
	hightime = 0;
	highbases = 0;
	highpoints = 0L;
	tied = 0;
	for (i = 0; i < nterms; ++i) {
		if (!arena_player_active(i))
			continue;
		if (arena_mode == ARENA_MODE_HOARD) {
			gold = arena_player_gold(i);
			if (winner < 0 || gold > highgold) {
				winner = i;
				highgold = gold;
				tied = 1;
			}
			else if (gold == highgold)
				++tied;
		}
		else if (arena_mode == ARENA_MODE_KING) {
			if (winner < 0 || arena_player[i].kingtime > hightime) {
				winner = i;
				hightime = arena_player[i].kingtime;
				tied = 1;
			}
			else if (arena_player[i].kingtime == hightime)
				++tied;
		}
		else if (arena_mode == ARENA_MODE_BASE) {
			if (winner < 0 || arena_player[i].basekills > highbases) {
				winner = i;
				highbases = arena_player[i].basekills;
				tied = 1;
			}
			else if (arena_player[i].basekills == highbases)
				++tied;
		}
		else if (arena_mode == ARENA_MODE_SCORED) {
			if (winner < 0 || arena_player[i].score > highpoints) {
				winner = i;
				highpoints = arena_player[i].score;
				tied = 1;
			}
			else if (arena_player[i].score == highpoints)
				++tied;
		}
		else {
			if (winner < 0 || arena_player[i].kills > highscore) {
				winner = i;
				highscore = arena_player[i].kills;
				tied = 1;
			}
			else if (arena_player[i].kills == highscore)
				++tied;
		}
	}
	if (tied == 1) {
		wuptr = warusroff(winner);
		/* the winner hosts the next match; ties leave the current host in place */
		arena_host = winner;
		/* completed matches and combat forfeits both earn roster credit */
		if (arena_match_ticks == 0 || forfeit)
			arena_record_win(winner);
		if (forfeit)
			prfmsg(MATFORF, wuptr->userid, arena_mode_name(arena_mode));
		else if (arena_mode == ARENA_MODE_HOARD) {
			sprintf(gechrbuf,"%lu",highgold);
			prfmsg(HODWIN,wuptr->userid,gechrbuf);
		}
		else if (arena_mode == ARENA_MODE_KING)
			prfmsg(KNGWIN,wuptr->userid,(int)(hightime / 60),
			    (int)(hightime % 60));
		else if (arena_mode == ARENA_MODE_BASE)
			prfmsg(BASWIN,wuptr->userid,(int)highbases);
		else if (arena_mode == ARENA_MODE_SCORED) {
			sprintf(gechrbuf,"%ld",highpoints);
			prfmsg(SCOWIN,wuptr->userid,gechrbuf);
		}
		else
			prfmsg(MATWIN, wuptr->userid, arena_mode_name(arena_mode),
			    highscore);
		arena_broadcast_prf();
	}
	else if (tied > 1) {
		if (arena_mode == ARENA_MODE_HOARD) {
			sprintf(gechrbuf,"%lu",highgold);
			prfmsg(HODTIE,gechrbuf);
		}
		else if (arena_mode == ARENA_MODE_KING)
			prfmsg(KNGTIE,(int)(hightime / 60),(int)(hightime % 60));
		else if (arena_mode == ARENA_MODE_BASE)
			prfmsg(BASTIE,(int)highbases);
		else if (arena_mode == ARENA_MODE_SCORED) {
			sprintf(gechrbuf,"%ld",highpoints);
			prfmsg(SCOTIE,gechrbuf);
		}
		else
			prfmsg(MATTIE, arena_mode_name(arena_mode), highscore);
		arena_broadcast_prf();
	}
	if (arena_match_ticks > 0) {
		arena_show_match_ended_time();
		arena_match_ticks = 0;
	}
	arena_show_status();
	arena_broadcast_prf();
}

/* select the status sort appropriate to the current mode and phase */
static int arena_status_sort_mode(void)
{
	if (arena_state != ARENA_STAGING && arena_state != ARENA_RUNNING)
		return ARENA_SORT_NONE;
	if (arena_mode == ARENA_MODE_BATTLE)
		return ARENA_SORT_KILLS;
	if (arena_mode == ARENA_MODE_HOARD && arena_hoard_scores_public())
		return ARENA_SORT_GOLD;
	if (arena_mode == ARENA_MODE_KING)
		return ARENA_SORT_TIME;
	if (arena_mode == ARENA_MODE_BASE)
		return ARENA_SORT_BASES;
	if (arena_mode == ARENA_MODE_SCORED)
		return ARENA_SORT_SCORE;
	return ARENA_SORT_NONE;
}

/* compare two status rows, keeping active players first and scores descending */
static int arena_status_compare(int left, int right, int sortmode)
{
	int leftactive;
	int rightactive;

	leftactive = arena_player_active(left);
	rightactive = arena_player_active(right);
	if (leftactive && !rightactive)
		return -1;
	if (!leftactive && rightactive)
		return 1;
	if (sortmode == ARENA_SORT_KILLS) {
		if (arena_player[left].kills > arena_player[right].kills)
			return -1;
		if (arena_player[left].kills < arena_player[right].kills)
			return 1;
	}
	else if (sortmode == ARENA_SORT_GOLD) {
		if (arena_player_gold(left) > arena_player_gold(right))
			return -1;
		if (arena_player_gold(left) < arena_player_gold(right))
			return 1;
	}
	else if (sortmode == ARENA_SORT_TIME) {
		if (arena_player[left].kingtime > arena_player[right].kingtime)
			return -1;
		if (arena_player[left].kingtime < arena_player[right].kingtime)
			return 1;
	}
	else if (sortmode == ARENA_SORT_BASES) {
		if (arena_player[left].basekills > arena_player[right].basekills)
			return -1;
		if (arena_player[left].basekills < arena_player[right].basekills)
			return 1;
	}
	else if (sortmode == ARENA_SORT_SCORE) {
		if (arena_player[left].score > arena_player[right].score)
			return -1;
		if (arena_player[left].score < arena_player[right].score)
			return 1;
	}
	return left - right;
}

/* announce whether one player or several players hold the current gold lead */
static void arena_hoard_leader_hint(void)
{
	int i;
	int leader;
	int tied;
	unsigned long gold;
	unsigned long highgold;

	leader = -1;
	tied = 0;
	highgold = 0UL;
	for (i = 0; i < nterms; ++i) {
		if (!arena_player_active(i))
			continue;
		gold = arena_player_gold(i);
		if (leader < 0 || gold > highgold) {
			leader = i;
			highgold = gold;
			tied = 1;
		}
		else if (gold == highgold)
			++tied;
	}
	if (leader < 0)
		return;
	if (tied == 1)
		prfmsg(HODLEAD,warusroff(leader)->userid);
	else
		prfmsg(HODCONT);
	arena_broadcast_prf();
}

/* print one mode-aware player row for the arena status table */
static void arena_print_status_row(int usrn)
{
	WARUSR *wuptr;
	char *state;
	unsigned seconds;

	if (arena_player[usrn].state == ARENA_P_EMPTY)
		return;
	wuptr = warusroff(usrn);
	if (usrn == arena_host && arena_player[usrn].state == ARENA_P_PLAYING)
		state = "playing (host)";
	else if (usrn == arena_host && arena_player[usrn].state == ARENA_P_RESPAWN)
		state = "respawn (host)";
	else if (usrn == arena_host)
		state = "ready (host)";
	else if (arena_player[usrn].state == ARENA_P_PLAYING)
		state = "playing";
	else if (arena_player[usrn].state == ARENA_P_RESPAWN)
		state = "respawn";
	else if (arena_player[usrn].state == ARENA_P_OBSERVE)
		state = (arena_player[usrn].ready && arena_state != ARENA_STAGING &&
		    arena_state != ARENA_RUNNING) ? "ready" : "observing";
	else if (arena_player[usrn].ready)
		state = "ready";
	else
		state = "unknown";
	if ((arena_state == ARENA_STAGING || arena_state == ARENA_RUNNING) &&
	    arena_player_active(usrn)) {
		if (arena_mode == ARENA_MODE_HOARD) {
			if (arena_hoard_scores_public() || usrn == usrnum) {
				sprintf(gechrbuf,"%lu",arena_player_gold(usrn));
				prf("%-22s %-24s %11s  %5d  %6u\r",wuptr->userid,state,
				    gechrbuf,arena_player[usrn].kills,arena_player[usrn].deaths);
			}
			else
				prf("%-22s %-24s %11s  %5d  %6u\r",wuptr->userid,state,
				    "?",arena_player[usrn].kills,arena_player[usrn].deaths);
		}
		else if (arena_mode == ARENA_MODE_KING) {
			seconds = arena_player[usrn].kingtime % 60;
			if (seconds < 10)
				sprintf(gechrbuf,"%u:0%u",arena_player[usrn].kingtime / 60,
				    seconds);
			else
				sprintf(gechrbuf,"%u:%u",arena_player[usrn].kingtime / 60,
				    seconds);
			prf("%-22s %-24s %11s  %5d  %6u\r",wuptr->userid,state,
			    gechrbuf,arena_player[usrn].kills,arena_player[usrn].deaths);
		}
		else if (arena_mode == ARENA_MODE_BASE)
			prf("%-22s %-24s %11u  %6u\r",wuptr->userid,state,
			    arena_player[usrn].basekills,arena_player[usrn].deaths);
		else if (arena_mode == ARENA_MODE_SCORED) {
			sprintf(gechrbuf,"%ld",arena_player[usrn].score);
			prf("%-22s %-24s %11s  %5d  %6u\r",wuptr->userid,state,
			    gechrbuf,arena_player[usrn].kills,arena_player[usrn].deaths);
		}
		else
			prf("%-22s %-24s %11d  %6u\r", wuptr->userid, state,
			    arena_player[usrn].kills, arena_player[usrn].deaths);
	}
	else {
		if (arena_mode == ARENA_MODE_HOARD)
			prf("%-22s %-24s %11s  %5s  %6s\r",wuptr->userid,state,"-","-","-");
		else if (arena_mode == ARENA_MODE_KING)
			prf("%-22s %-24s %11s  %5s  %6s\r",wuptr->userid,state,"-","-","-");
		else if (arena_mode == ARENA_MODE_BASE)
			prf("%-22s %-24s %11s  %6s\r",wuptr->userid,state,"-","-");
		else if (arena_mode == ARENA_MODE_SCORED)
			prf("%-22s %-24s %11s  %5s  %6s\r",wuptr->userid,state,"-","-","-");
		else
			prf("%-22s %-24s %11s  %6s\r",wuptr->userid,state,"-","-");
	}
}

/* build and print the sorted status table for the current arena phase */
void FUNC arena_show_status(void)
{
	int i, j;
	int count;
	int best;
	int tmp;
	int sortmode;
	int order[ARENA_MAX_PLAYERS];

	prf("\r");
	if (arena_state == ARENA_RUNNING && arena_match_ticks > 0) {
		if (arena_mode == ARENA_MODE_KING) {
			if (arena_match_ticks < 60)
				prfmsg(KNGTIME,arena_match_ticks,arena_king_x,arena_king_y);
			else
				prfmsg(KNGTIM2,arena_match_ticks / 60,
				    arena_match_ticks % 60,arena_king_x,arena_king_y);
		}
		else
			arena_show_match_time();
	}
	arena_show_mini_status(TRUE);
	if (arena_state == ARENA_QUEUE && arena_ticks > 0)
		prfmsg(AUTOTIME, arena_mode_name(arena_mode), arena_ticks);
	if (arena_mode == ARENA_MODE_HOARD)
		prfmsg(HODSTAT);
	else if (arena_mode == ARENA_MODE_KING)
		prfmsg(KNGSTAT);
	else if (arena_mode == ARENA_MODE_BASE)
		prfmsg(BASSTAT);
	else if (arena_mode == ARENA_MODE_SCORED)
		prfmsg(SCOSTAT);
	else
		prfmsg(BATSTAT);

	count = 0;
	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state != ARENA_P_EMPTY && count < ARENA_MAX_PLAYERS)
			order[count++] = i;
	}

	sortmode = arena_status_sort_mode();
	/* a fixed-size selection sort avoids recursion and additional stack use */
	for (i = 0; i < count - 1; ++i) {
		best = i;
		for (j = i + 1; j < count; ++j)
			if (arena_status_compare(order[j], order[best], sortmode) < 0)
				best = j;
		if (best != i) {
			tmp = order[i];
			order[i] = order[best];
			order[best] = tmp;
		}
	}

	for (i = 0; i < count; ++i)
		arena_print_status_row(order[i]);
}

/* award a kill to a valid active last attacker without persistent ship writes */
static int arena_credit_last_attacker(WARSHP *ptr, int usrn)
{
	int who;
	WARSHP *wptr;
	WARUSR *wuptr;

	who = ptr->lastfired;
	if (who == usrn || !arena_player_active(who))
		return FALSE;
	wptr = warshpoff(who);
	if (arena_player[who].state == ARENA_P_PLAYING &&
	    wptr->status != GESTAT_USER)
		return FALSE;
	wuptr = warusroff(who);
	if (wptr->status == GESTAT_USER) {
		++wptr->kills;
		if (ptr->status == GESTAT_USER)
			++wptr->ukills;
	}
	++wuptr->kills;
	if (ptr->status == GESTAT_USER)
		++wuptr->ukills;
	++arena_player[who].kills;
	return TRUE;
}

/* apply Scored mode's class value, power adjustment, and player loss */
static void arena_score_user_kill(WARSHP *ptr, int usrn, int who)
{
	WARSHP *wptr;
	long points;
	long bonus;
	long deduct;

	if (arena_mode != ARENA_MODE_SCORED || who < 0 || who >= nterms ||
	    !VALID_SHPCLASS(ptr->shpclass))
		return;
	wptr = warshpoff(who);
	points = (long)shipclass[ptr->shpclass].max_points;
	bonus = 0L;
	if (ptr->status == GESTAT_USER && VALID_SHPCLASS(wptr->shpclass)) {
		if (shipclass[ptr->shpclass].damfact >
		    shipclass[wptr->shpclass].damfact + 50)
			bonus = points / 2L;
		else if (shipclass[ptr->shpclass].damfact + 50 <
		    shipclass[wptr->shpclass].damfact)
			bonus = -(points / 3L);
	}
	arena_player[who].score += points + bonus;

	sprintf(gechrbuf,"%ld",points);
	prfmsg(KILLPNTS,gechrbuf,shipclass[ptr->shpclass].typename,showupg(ptr));
	if (bonus > 0L) {
		sprintf(gechrbuf,"%ld",bonus);
		prfmsg(KILLBON1,gechrbuf);
	}
	else if (bonus < 0L) {
		sprintf(gechrbuf,"%ld",-bonus);
		prfmsg(KILLBON2,gechrbuf);
	}
	outprfge(FLT_NONE,who);
	clrprf();

	if (ptr->status != GESTAT_USER || usrn < 0 || usrn >= nterms)
		return;
	deduct = ((points + bonus) * (long)score_f2) / 100L;
	arena_player[usrn].score -= deduct;
	if (deduct > 0L) {
		sprintf(gechrbuf,"%ld",deduct);
		prfmsg(YRDEAD2,gechrbuf);
		outprfge(FLT_NONE,usrn);
		clrprf();
	}
}

/* apply the smaller standard loss when a Cyb destroys a Scored player */
static void arena_score_cyb_kill(WARSHP *ptr, int usrn)
{
	long deduct;

	if (arena_mode != ARENA_MODE_SCORED || usrn < 0 || usrn >= nterms ||
	    !VALID_SHPCLASS(ptr->shpclass))
		return;
	deduct = ((long)shipclass[ptr->shpclass].max_points *
	    (long)score_f2) / 1000L;
	arena_player[usrn].score -= deduct;
	if (deduct > 0L) {
		sprintf(gechrbuf,"%ld",deduct);
		prfmsg(YRDEAD2,gechrbuf);
		outprfge(FLT_NONE,usrn);
		clrprf();
	}
}

/* penalize an uncredited Scored death by the destroyed ship's value */
static void arena_score_uncredited_death(WARSHP *ptr, int usrn)
{
	long deduct;

	if (arena_mode != ARENA_MODE_SCORED || usrn < 0 || usrn >= nterms ||
	    !VALID_SHPCLASS(ptr->shpclass))
		return;
	deduct = (long)shipclass[ptr->shpclass].max_points;
	arena_player[usrn].score -= deduct;
	sprintf(gechrbuf,"%ld",deduct);
	prfmsg(YRDEAD2,gechrbuf);
	outprfge(FLT_NONE,usrn);
	clrprf();
}

/* return the configured Cyb that last damaged this ship, or -1 */
static int arena_last_cyb_attacker(WARSHP *ptr, int usrn)
{
	int who;
	WARSHP *wptr;

	who = ptr->lastfired;
	if (who < nterms || who >= nships || who == usrn)
		return -1;
	wptr = warshpoff(who);
	if (cyb_slot_class(who) < 0 || !VALID_SHPCLASS(wptr->shpclass) ||
	    shipclass[wptr->shpclass].max_type != CLASSTYPE_CYBORG)
		return -1;
	return who;
}

/* give an active user the normal ge-next spoils and retrieval messages */
static void arena_collect_user_spoils(WARSHP *ptr, int who)
{
	WARSHP *wptr;

	if (who < 0 || who >= nterms ||
	    arena_player[who].state != ARENA_P_PLAYING)
		return;
	wptr = warshpoff(who);
	if (wptr->status != GESTAT_USER)
		return;
	if (ptr->status == GESTAT_AUTO)
		prfmsg(KILLGOTN,ptr->shipname);
	else if (ptr->shipname[0] == 0)
		prfmsg(KILLGTNO,ptr->userid);
	else
		prfmsg(KILLGOT1,ptr->shipname);
	collect_spoils(ptr,wptr,who,gernd());
	clrprf();
}

/* let a Cyb retain normal spoils without sending user-only output */
static void arena_collect_cyb_spoils(WARSHP *ptr, int who)
{
	WARSHP *wptr;

	if (who < nterms || who >= nships)
		return;
	wptr = warshpoff(who);
	if (wptr->status != GESTAT_AUTO || !VALID_SHPCLASS(wptr->shpclass) ||
	    shipclass[wptr->shpclass].max_type != CLASSTYPE_CYBORG)
		return;
	++wptr->kills;
	++warusroff(who)->kills;
	if (shipclass[wptr->shpclass].won_func != NULL)
		shipclass[wptr->shpclass].won_func(wptr,who,ptr);
	collect_spoils_silent(ptr,wptr,gernd());
}

/* discard a transient arena ship and mark its channel unused */
static void arena_clear_ship(WARSHP *ptr)
{
	setmem(ptr,sizeof(WARSHP),0);
	ptr->status = GESTAT_AVAIL;
	ptr->where = -1;
}

/* silently discard projectiles tracking a ship that has left arena play */
static void arena_clear_inbound(WARSHP *ptr)
{
	setmem(ptr->ltorps,sizeof(ptr->ltorps),0);
	setmem(ptr->lmissl,sizeof(ptr->lmissl),0);
}

/* remove one player from the current match while retaining lobby membership */
static void arena_observe_match(int usrn)
{
	WARSHP *wptr;
	int active_ship, was_playing, who;

	if (usrn < 0 || usrn >= nterms)
		return;
	was_playing = arena_player_active(usrn);
	active_ship = arena_player[usrn].state == ARENA_P_PLAYING;
	wptr = warshpoff(usrn);
	if (active_ship && arena_state == ARENA_RUNNING) {
		who = wptr->lastfired;
		if (arena_credit_last_attacker(wptr,usrn)) {
			arena_score_user_kill(wptr,usrn,who);
			arena_collect_user_spoils(wptr,who);
		}
		else {
			who = arena_last_cyb_attacker(wptr,usrn);
			if (who >= 0) {
				arena_score_cyb_kill(wptr,usrn);
				arena_collect_cyb_spoils(wptr,who);
			}
		}
	}
	if (was_playing && arena_state == ARENA_RUNNING) {
		prfmsg(MATLEFT, username(wptr));
		arena_broadcast_prf_except(usrn);
	}
	arena_player[usrn].state = ARENA_P_OBSERVE;
	arena_player[usrn].ready = FALSE;
	arena_player[usrn].kills = 0;
	arena_player[usrn].deaths = 0;
	arena_player[usrn].score = 0L;
	arena_player[usrn].respawn = 0;
	arena_player[usrn].flags = 0;
	user[usrn].substt = ARENASUB;
	if (usrn == usrnum)
		usrptr->substt = ARENASUB;
	cleartm(usrn);
	arena_clear_inbound(wptr);
	arena_clear_ship(wptr);
	if (arena_host == usrn) {
		arena_host = -1;
		arena_choose_host_ex(usrn,FALSE);
	}
	btupmt(usrn,'>');
}

/* return the current player to the lobby and finish a one-player match */
void FUNC arena_exit_match(void)
{
	arena_observe_match(usrnum);
	prfmsg(MATOBS);
	outprfge(FLT_NONE,usrnum);
	clrprf();
	if (arena_count_playing() < 2)
		arena_end_match();
	else {
		arena_show_lobby();
		outprfge(FLT_NONE,usrnum);
	}
}

/* delete generated world state and clear transient ships, mines, and caches */
static void arena_wipe_galaxy(void)
{
	WARSHP *wptr;
	unsigned deleted;
	int i;

	deleted = 0;
	setbtv(gebb2);
	/* every generated sector record belongs exclusively to the prior match */
	do {
		if (!qlobtv(0))
			break;
		gcrbtv(&sector,0);
		delbtv();
		++deleted;
	} while (TRUE);

	for (i = 0; i < nummines; ++i) {
		setmem(&mines[i],sizeof(MINE),0);
		mines[i].channel = MINE_UNUSED;
	}

	for (i = 0; i < nships; ++i) {
		wptr = warshpoff(i);
		if (i >= nterms) {
			arena_clear_ship(wptr);
			continue;
		}
		setmem(wptr->ltorps,sizeof(wptr->ltorps),0);
		setmem(wptr->lmissl,sizeof(wptr->lmissl),0);
		wptr->minesnear = FALSE;
	}

	setmem(ptab,nships * sizeof(PLANETAB),0);

	setmem(&sector,sizeof(GALSECT),0);
	sector.xsect = 32767;
	sector.ysect = 32767;
	sector.plnum = 32767;
	setmem(&planet,sizeof(GALPLNT),0);
	planet.xsect = 32767;
	planet.ysect = 32767;
	planet.plnum = 32767;
	setmem(&worm,sizeof(GALWORM),0);
	worm.xsect = 32767;
	worm.ysect = 32767;
	worm.plnum = 32767;

	geshocst(1,spr("GE:INF:Arena galaxy wipe records=%u",deleted));
}

/* clear any arena world records left by a previous or interrupted run */
void FUNC arena_initialize_world(void)
{
	arena_wipe_galaxy();
}

/* return participants to the queue, announce completion, and wipe the galaxy */
static void arena_end_match(void)
{
	int i;
	WARSHP *wptr;

	if (arena_state == ARENA_RUNNING)
		arena_show_results();
	arena_state = ARENA_QUEUE;
	arena_ticks = 0;
	arena_match_ticks = 0;
	for (i = 0; i < nterms; ++i) {
		if (arena_player_active(i)) {
			arena_player[i].ready = TRUE;
			arena_player[i].respawn = 0;
			user[i].substt = ARENASUB;
			if (i == usrnum)
				usrptr->substt = ARENASUB;
			cleartm(i);
			wptr = warshpoff(i);
			arena_clear_inbound(wptr);
			arena_clear_ship(wptr);
			btupmt(i,'>');
		}
	}
	arena_choose_host();
	for (i = 0; i < nterms; ++i) {
		if (arena_player[i].state == ARENA_P_EMPTY)
			continue;
		if (arena_player[i].ready) {
			arena_player[i].state = ARENA_P_READY;
		}
		else {
			arena_player[i].state = ARENA_P_OBSERVE;
			arena_player[i].ready = FALSE;
		}
	}
	prfmsg(MATEND);
	arena_broadcast_prf();
	arena_wipe_galaxy();
}

/* score a destruction, distribute spoils, and put the victim into respawn */
void FUNC arena_ship_destroyed(WARSHP *ptr, int usrn)
{
	int who;
	int single_class;
	WARSHP *wptr;

	who = ptr->lastfired;
	if (arena_credit_last_attacker(ptr,usrn)) {
		wptr = warshpoff(who);
		prfmsg(KILLEDBY, username(ptr), warusroff(who)->userid);
		arena_broadcast_prf();
		arena_score_user_kill(ptr,usrn,who);
		arena_collect_user_spoils(ptr,who);
	}
	else if ((who = arena_last_cyb_attacker(ptr,usrn)) >= 0) {
		wptr = warshpoff(who);
		prfmsg(KILLEDBY,username(ptr),username(wptr));
		arena_broadcast_prf();
		arena_score_cyb_kill(ptr,usrn);
		arena_collect_cyb_spoils(ptr,who);
	}
	else {
		if (who == -1 && arena_mode == ARENA_MODE_BATTLE)
			--arena_player[usrn].kills;
		if (ptr->shipname[0] == 0)
			prfmsg(DIEDNO, username(ptr));
		else
			prfmsg(DIED, ptr->shipname, username(ptr));
		arena_broadcast_prf();
		arena_score_uncredited_death(ptr,usrn);
	}

	prfmsg(RSPWAIT, ARENA_RESPAWN_TIME);
	single_class = arena_single_shipclass();
	if (single_class < 0)
		arena_show_ship_choices();
	else
		arena_player[usrn].shipclass = (byte)single_class;
	outprfge(FLT_NONE,usrn);
	clrprf();
	++arena_player[usrn].deaths;
	arena_player[usrn].state = ARENA_P_RESPAWN;
	arena_player[usrn].ready = FALSE;
	arena_player[usrn].respawn = ARENA_RESPAWN_TIME;
	user[usrn].substt = ARENASUB;
	if (usrn == usrnum)
		usrptr->substt = ARENASUB;
	arena_clear_inbound(ptr);
	arena_clear_ship(ptr);
	btupmt(usrn,'>');
}

/* score an arena Cyb death without invoking classic persistence or points */
void FUNC arena_cyb_destroyed(WARSHP *ptr, int usrn)
{
	int who;
	WARSHP *wptr;

	who = ptr->lastfired;
	if (arena_credit_last_attacker(ptr,usrn)) {
		if (arena_mode == ARENA_MODE_BASE && arena_is_base_ship(ptr) &&
		    arena_player[who].basekills != (unsigned)~0U)
			++arena_player[who].basekills;
		wptr = warshpoff(who);
		prfmsg(KILLDNPC,username(ptr),warusroff(who)->userid);
		arena_broadcast_prf();
		arena_score_user_kill(ptr,usrn,who);
		arena_collect_user_spoils(ptr,who);
	}
	else {
		prfmsg(DIEDNPC,ptr->shipname);
		arena_broadcast_prf();
		wptr = NULL;
	}

	arena_clear_inbound(ptr);
	if (VALID_SHPCLASS(ptr->shpclass) &&
	    shipclass[ptr->shpclass].kill_func != NULL)
		shipclass[ptr->shpclass].kill_func(ptr,usrn,wptr);
	else {
		ptr->status = GESTAT_AVAIL;
		ptr->where = -1;
	}
}

/* announce each Base-mode damage milestone once during a base's lifetime */
void FUNC arena_cyb_damage_notice(WARSHP *ptr)
{
	if (arena_state != ARENA_RUNNING || arena_mode != ARENA_MODE_BASE ||
	    !arena_is_base_ship(ptr) || ptr->damage >= 100.0)
		return;
	if (ptr->damage > 75.5 && ptr->damage_notice < 2) {
		ptr->damage_notice = 2;
		prfmsg(BASSEVER,ptr->shipname);
		arena_broadcast_prf();
	}
	else if (ptr->damage > 50.5 && ptr->damage_notice < 1) {
		ptr->damage_notice = 1;
		prfmsg(BASHEAVY,ptr->shipname);
		arena_broadcast_prf();
	}
}

/**************************************************************************
** Planet drops and periodic state                                       **
**************************************************************************/

/* add an item to the current planet without exceeding its arena cap */
static void arena_add_drop_item(int item, unsigned long qty, unsigned long cap)
{
	if (planet.items[item].qty >= cap)
		return;
	if (qty > cap - planet.items[item].qty)
		qty = cap - planet.items[item].qty;
	planet.items[item].qty += qty;
}

/* consume two random bits to vary one base item quantity from base to base+3 */
static unsigned long arena_drop_qty(unsigned int *r, unsigned long base)
{
	unsigned long qty;

	qty = base + (*r & 3);
	*r >>= 2;
	return qty;
}

/* create the next staged Base-mode Cyber-Base in its configured slot */
static void arena_spawn_base(void)
{
	WARSHP *ptr;
	int cls;
	int i;

	for (i = nterms; i < nships; ++i) {
		ptr = warshpoff(i);
		cls = cyb_slot_class(i);
		if (ptr->status != GESTAT_AVAIL || cls < 0 ||
		    shipclass[cls].arena_mode != ARENA_MODE_BASE ||
		    shipclass[cls].max_accel != 0)
			continue;
		if (shipclass[cls].init_func != NULL)
			(*(shipclass[cls].init_func))(ptr,i,cls);
		return;
	}
}

/* add the scheduled item and upgrade drop to Zygor, then announce it */
static void arena_drop_zygor(void)
{
	COORD coord;
	byte special;
	unsigned int r;

	coord.xcoord = 0.50001;
	coord.ycoord = 0.50001;
	getsector(&coord);
	if (!getplanet(&coord,1) || planet.type != PLTYPE_PLNT)
		return;
	r = gernd();
	arena_add_drop_item(I_TORPEDO,arena_drop_qty(&r,7UL),20UL);
	arena_add_drop_item(I_MISSILE,arena_drop_qty(&r,7UL),20UL);
	arena_add_drop_item(I_MINE,arena_drop_qty(&r,7UL),20UL);
	arena_add_drop_item(I_FLUXPOD,arena_drop_qty(&r,4UL),12UL);
	arena_add_drop_item(I_DECOYS,arena_drop_qty(&r,2UL),10UL);
	arena_add_drop_item(I_ZIPPERS,arena_drop_qty(&r,2UL),10UL);
	/* fewer jammers */
	arena_add_drop_item(I_JAMMERS,arena_drop_qty(&r,2UL) / 2UL,5UL);
	if (arena_mode == ARENA_MODE_HOARD)
		arena_add_drop_item(I_GOLD,
		    300UL + (unsigned long)(gernd() % 201),ULCAP);
	if (arena_mode != ARENA_MODE_BASE) {
		special = (byte)(2 + (gernd() % 2));
		planet.arena_shield_boost += special;
		special = (byte)(2 + (gernd() % 2));
		planet.arena_phaser_boost += special;
	}
	switch (gernd() % 5) {
	case 0:
		planet.arena_flags |= ARENA_PL_SCAN;
		break;
	case 1:
		planet.arena_flags |= ARENA_PL_ARMOR;
		break;
	case 2:
		planet.arena_flags |= ARENA_PL_ACCEL;
		break;
	case 3:
		planet.arena_flags |= ARENA_PL_CORE;
		break;
	default:
		planet.arena_flags |= ARENA_PL_INSTANT;
		break;
	}
	pkey.xsect = 0;
	pkey.ysect = 0;
	pkey.plnum = 1;
	gesdb(GEUPDATE,&pkey,(GALSECT *)&planet);
	prfmsg(PLDROP);
	arena_broadcast_prf();
}

/* advance queue, staging, match, drop, and respawn timers once per second */
void FUNC arena_tick(void)
{
	if (arena_state == ARENA_QUEUE) {
		if (arena_count_ready() < 2) {
			if (arena_ticks > 0) {
				arena_ticks = 0;
				prfmsg(AUTOCAN);
				arena_broadcast_prf();
			}
			return;
		}
		if (arena_ticks <= 0) {
			arena_ticks = ARENA_AUTOSTART_TIME;
			prfmsg(AUTOBRDC, arena_mode_name(arena_mode), arena_ticks);
			arena_broadcast_prf();
			return;
		}
		--arena_ticks;
		if (arena_ticks == 30 || arena_ticks == 10) {
			prfmsg(AUTOBRDC, arena_mode_name(arena_mode), arena_ticks);
			arena_broadcast_prf();
		}
		else if (arena_ticks == 5) {
			prfmsg(AUTO5, arena_mode_name(arena_mode));
			arena_broadcast_prf();
		}
		else if (arena_ticks > 0 && arena_ticks < 5) {
			prfmsg(AUTOFIN, arena_ticks);
			arena_broadcast_prf();
		}
		if (arena_ticks <= 0)
			arena_start_match();
		return;
	}
	if (arena_state == ARENA_STAGING) {
		arena_respawn_tick();
		if (arena_ticks > 0)
			--arena_ticks;
		if (arena_mode == ARENA_MODE_BASE &&
		    (arena_ticks == 50 || arena_ticks == 40 || arena_ticks == 30 ||
		    arena_ticks == 20 || arena_ticks == 10))
			arena_spawn_base();
		if (arena_ticks == 45) {
			if (arena_scan_percent() < 100)
				prfmsg(STGSIZE2, univmax, arena_scan_percent());
			else
				prfmsg(STGSIZE, univmax);
			arena_broadcast_prf();
		}
		else if (arena_ticks == 30 || arena_ticks == 10) {
			prfmsg(STGTIME, arena_mode_name(arena_mode), arena_ticks);
			arena_broadcast_prf();
		}
		else if (arena_ticks == 5) {
			prfmsg(STG5, arena_mode_name(arena_mode));
			arena_broadcast_prf();
		}
		else if (arena_ticks > 0 && arena_ticks < 5) {
			prfmsg(STGFIN, arena_ticks);
			arena_broadcast_prf();
		}
		if (arena_ticks <= 0)
			arena_start_combat();
		return;
	}
	if (arena_state == ARENA_RUNNING) {
		arena_respawn_tick();
		if (arena_mode == ARENA_MODE_KING)
			arena_king_score_tick();
		if (arena_match_ticks > 0)
			--arena_match_ticks;
		if (arena_mode == ARENA_MODE_KING &&
		    (arena_match_ticks == 720 || arena_match_ticks == 540 ||
		    arena_match_ticks == 360 || arena_match_ticks == 180))
			arena_king_move_sector();
		if (arena_mode == ARENA_MODE_HOARD &&
		    (arena_match_ticks == 720 || arena_match_ticks == 540 ||
		    arena_match_ticks == 360))
			arena_hoard_leader_hint();
		if (arena_mode == ARENA_MODE_HOARD && arena_match_ticks == 180) {
			prfmsg(HODOPEN);
			arena_broadcast_prf();
		}
		if (arena_match_ticks == 600 || arena_match_ticks == 300)
			arena_drop_zygor();
		if (arena_match_ticks == 60) {
			prfmsg(MATWARN, arena_mode_name(arena_mode));
			arena_broadcast_prf();
		}
		if (arena_count_playing() < 2 || arena_match_ticks <= 0)
			arena_end_match();
	}
}

/**************************************************************************
** Command dispatch                                                      **
**************************************************************************/

/* dispatch lobby and respawn commands, handing active players to combat */
int FUNC mnu_arena_lobby(void)
{
	int choice;

	if (arena_player[usrnum].state == ARENA_P_PLAYING) {
		usrptr->substt = FIGHTSUB;
		user[usrnum].substt = FIGHTSUB;
		return mnu_fightsub();
	}

	if (arena_player[usrnum].state == ARENA_P_RESPAWN &&
	    (arena_state == ARENA_STAGING || arena_state == ARENA_RUNNING)) {
		if (margc == 1 && sameas(margv[0], "X")) {
			arena_exit_match();
			return 1;
		}
		if (margc == 1 && sameas(margv[0], "?")) {
			arena_show_ship_classes();
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		if (margc == 1 && sameto("sta", margv[0])) {
			arena_show_status();
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		if (arena_single_shipclass() >= 0) {
			prfmsg(RSPHOLD);
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		choice = margc == 1 ? arena_parse_ship_choice(margv[0]) : -1;
		if (choice >= 0) {
			arena_player[usrnum].shipclass =
			    (byte)arena_shipclass_for_choice(choice);
			prfmsg(RSPSEL, shipclass[arena_selected_shipclass(usrnum)].typename);
		}
		else {
			prfmsg(RSPPROM);
		}
		outprfge(FLT_NONE, usrnum);
		return 1;
	}

	if (margc == 0) {
		arena_show_lobby();
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc >= 1 &&
	    (sameto("?", margv[0]) || sameto("hel", margv[0]))) {
		gwar();
		return 1;
	}
	else if (margc >= 1 && sameas(margv[0], ">")) {
		arena_lobby_chat();
		return 1;
	}
	else if (margc == 1 && sameto("cls", margv[0])) {
		cmd_clear();
		return 1;
	}
	else if ((margc == 1 || (margc == 2 && sameas(margv[1], "all"))) &&
	    sameto("ros", margv[0])) {
		cmd_geroster();
		return 1;
	}
	else if (margc >= 1 && sameto("sen", margv[0])) {
		prfmsg(LOBCHATH);
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc >= 1 && sameto("set", margv[0])) {
		cmd_set();
		return 1;
	}
	else if (margc >= 1 && sameto("sys", margv[0])) {
		/* cmd_sysop enforces sysop restriction */
		cmd_sysop();
		return 1;
	}
	else if (margc == 1 && sameto("who", margv[0])) {
		cmd_who();
		return 1;
	}
	else if (margc == 1 && sameto("sta", margv[0])) {
		arena_show_status();
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc == 1 && sameto("rea", margv[0])) {
		if (arena_host == usrnum)
			prfmsg(LOBHOST);
		else {
			arena_set_ready();
			arena_show_lobby();
		}
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc == 1 && sameto("obs", margv[0])) {
		arena_set_observe();
		arena_show_lobby();
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc == 1 && sameto("mod", margv[0])) {
		arena_show_modes();
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc == 2 && sameto("mod", margv[0])) {
		arena_set_mode(margv[1]);
		arena_show_lobby();
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc == 1 && sameto("go", margv[0])) {
		if (!arena_start_countdown()) {
			outprfge(FLT_NONE, usrnum);
		}
		return 1;
	}
	else if (margc == 1 && sameas(margv[0], "X")) {
		prfmsg(EXIWAR);
		outprfge(FLT_NONE, usrnum);
		arena_leave(usrnum);
		btupmt(usrnum, 0);
		return 0;
	}
	arena_show_lobby_prompt();
	outprfge(FLT_NONE, usrnum);
	return 1;
}
