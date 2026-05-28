
/***************************************************************************
 *                                                                         *
 *   GEMAIN.C                                                              *
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

#define GEMAIN 1

#include "geglobal.h"

/* LOCAL GLOBAL DEFS *****************************************************/

int gestt;			/* module number */

struct module mpoge = {		/* module interface block		*/
	"",			/* description for main menu		*/
	gelogon,		/* user logon supplemental routine	*/
	galemp,			/* input routine if selected		*/
	stshdlr,		/* status-input routine if selected	*/
	NULL,			/* "injoth" routine for this module	*/
	pwarlof,		/* user logoff supplemental routine	*/
	pwarhup,		/* hangup (lost carrier) routine	*/
	pgemidnight,		/* midnight cleanup routine		*/
	pgedelete,		/* delete-account routine		*/
	pclswar			/* finish-up (sys shutdown) routine	*/
};

BTVFILE *gebb1,		/* MPOGESHP.DAT */
	*gebb2,		/* MPOGEPLT.DAT */
	*gebb4,		/* MPOGEMAL.DAT */
	*gebb5;		/* MPOGEUSR.DAT */

FILE *gemb,		/* MPOGEMSG.MSG: main GE messages */
	*gehlpmb,	/* MPOGEHLP.MSG: GE help messages */
	*geshmb;	/* MPOGESHP.MSG: ship class messages */

static char *geuser,		/* configured user database name */
	*geship,		/* configured ship database name */
	*geplnt,		/* configured planet database name */
	*gemail,		/* configured mail database name */
	*geshipcl;		/* configured ship class message name */

static char *endmark;		/* startup message/config integrity marker */

int numwar = 0;		/* number of users in game */
int decpass = 0;	/* decoy expiry batching pass counter */

WARSHP tmpshp;		/* used to temporarily set up a new ship */

WARSHP *warshp,		/* base pointer to ship table */
	*warsptr;	/* current ship pointer */

WARUSR *warusr,		/* base pointer to user table */
	*waruptr;	/* current user pointer */

int warusr_ecl,		/* expanded-memory handle for user table */
	warshp_ecl;	/* expanded-memory handle for ship table */

WARUSR tmpusr;		/* temporary user-record workspace */

GALSECT sector;		/* current sector data workspace */

GALPLNT planet;		/* temporary planet-record workspace */

GALWORM worm;		/* temporary wormhole-record workspace */

static int mnu_admenu1a(void);
static int mnu_menug(void);

PLANETAB *ptab;		/* base pointer to per-user planet scan tables */

char map[MAXY][MAXX + 1];	/* scan map character buffer */
char mapc[MAXY][MAXX + 1];	/* scan map color buffer */

MINE *mines;			/* base pointer to active mine table */

MAIL mail;			/* temporary mail record workspace */
MAILSTAT tmpstat;		/* temporary mail-status workspace */

byte beacontimer;		/* beacon timing state */
ENTRYTAB *entrytab;		/* delayed entry-message state table */
byte *entrysent;		/* per-entrant sent-recipient bitsets */
byte *entrypend;		/* per-entrant pending-recipient bitsets */
int entrybytes;			/* bytes per entrysent/entrypend bitset */

struct message *gemsg;		/* temporary GE mail/message buffer */


/***********************************************************************/
/* Global variables                                                    */
/***********************************************************************/

int				nships;		/* total number of ship slots in game */
int				heading;	/* shared heading workspace */
unsigned			speed;		/* shared speed workspace */
int				game_day;	/* current in-game day value */

/* do not touch the next two definitions !!! */
int				xsect,ysect;	/* current sector coordinate workspace */
unsigned			xcord,ycord;	/* current intra-sector coordinate workspace */

PKEY				pkey;		/* planet database key workspace */
unsigned			distance;	/* shared integer distance workspace */
double				ddistance;	/* shared floating-point distance workspace */
int				bearing;	/* shared bearing workspace */
unsigned			energy;		/* shared energy workspace */
unsigned			damage;		/* shared damage workspace */
char				*gechrbuf,*gechrbuf2,*gechrbuf3;	/* shared general-purpose text buffers */
char				*warpbuf;	/* shared warp-speed formatting buffer */
int				plnum;		/* current planet number workspace */
GALPLNT				*plptr;		/* current planet pointer workspace */
ITEM				*titems;	/* per-user temporary item-edit table */
TEAM				*teamtab;	/* base pointer to team table */

int				tmp_usrnum;	/* temporary user-number workspace */

long				max_plrec,	/* max MPOGEPLT.DAT records (2x MAXPLREC, 512-byte records) */
				teambonus,	/* team bonus configuration value */
				pltvcash,	/* planet value cash scaling factor */
				pltvdiv,	/* planet value item scaling divisor */
				startcash;	/* starting user cash amount */

unsigned			plantime;	/* time between planet updates */

unsigned long			nebseed;	/* nebula random seed */

int				gemaxplrs,	/* max simultaneous GE players */
				gefreebies,	/* free-account/newbie access toggle */
				gemaxlist,	/* max roster/list output count */
				gerostxt,	/* nightly roster text-file export toggle */
				maxships,	/* max ships per user */
				se100dam,	/* enforcer planet damage per violation */
				profon,		/* profanity checking */
				showopt,	/* log level to console */
				syscmds,	/* sysop-command availability toggle */
				sysonly,	/* restrict sys commands to sysops only */
				max_plnts,	/* max owned planets per user */
				trans_opt,	/* allow goods transfers to planets not owned */
				numships,	/* total non-terminal ship slots */
				univmax,	/* radius of galaxy outside of 0 0 */
				plodds,		/* planet creation odds */
				wormodds,	/* wormhole creation odds */
				nebodds,	/* nebula existence odds */
				hpfirdst,	/* hyperphaser distance factor */
				hpdammax,	/* hyperphaser max damage factor */
				pfirdist,	/* phaser distance factor */
				pdammax,	/* phaser max damage factor */
				jamtime,	/* jammer duration */
				maildays,	/* mail retention days */
				torpsped,	/* torpedo speed */
				mislsped,	/* base missile speed */
				decodds,	/* decoy effectiveness odds */
				nummines,	/* total mine slots */
				usermines,	/* max mines per user */
				cyb_class,	/* base cyborg class index */
				dr_class,	/* base droid class index */
				clenguse,	/* cloak energy use */
				logflag,	/* GE logging toggle */
				optmenu,	/* option-menu mode toggle */
				cyb_gold,	/* cyborg gold reward setting */
				tot_classes,	/* total ship class slots defined */
				team_max,	/* max team size */
				fse_state,	/* FSE/editor module state id */
				phatowrp,	/* minimum phasers required to hit ships at warp */
				score_bonus,	/* score bonus factor */
				score_f2,	/* score scaling factor */
				chgloser,	/* loser cash-charge percentage */
				univwrap,	/* universe wrap toggle */
				maxplanets,	/* max planets per sector */
				meneat,		/* do men eat? (they don't in classic) */
				showdoc,	/* how often to give winner planet list from loser */
				cattkd,		/* how often do cybs attack droids */
				gcnum,		/* which faction does the neutral zone belong to */
				planupd,	/* planet updates per day */
				passnum;	/* current planet-update pass number */

char				*opttxt,	/* option-text menu prompt */
				optchr;		/* option-text trigger character */

long				*opttbl;	/* per-user option-text file offsets */

double				tor_fact,	/* torpedo hit-distance factor */
				tdammax,	/* torpedo max damage factor */
				mdammax,	/* missile max damage factor */
				mis_fact,	/* missile hit-distance factor */
				idammax,	/* ion cannon max damage factor */
				minedammax,	/* mine max damage factor */
				repairrate,	/* per-tick repair rate */
				tooclose,	/* cyborg close-range threshold */
				hyperdist1,	/* cyborg long-range pursuit threshold */
				hyperdist2,	/* cyborg mid-range pursuit threshold */
				plattrf1,	/* planet troops destroying attacking fighters */
				plattrf2,	/* defending fighters destroying attacking fighters */
				plattrf3,	/* attacking fighters destroying defending fighters */
				plattrt1,	/* defending troops destroying attacking troops */
				plattrt2;	/* attacking troops destroying defending troops */

SHPKEY				shpkey;		/* ship database key workspace */
MAILKEY				mailkey;	/* mail database key workspace */

SHIP				*shipclass;	/* base pointer to ship class table */

S00				*s00;		/* sector 0,0 predefined object table */
int				s00plnum;	/* number of predefined sector 0,0 objects */

SCANTAB				*scantab;	/* base pointer to per-user scan tables */

long shieldprice[TOPSHIELD];		/* shield upgrade prices */
long phaserprice[TOPPHASOR];		/* phaser upgrade prices */
long upgrprice[8];			/* general ship upgrade prices */
unsigned baseprice[NUMITEMS];		/* base item prices */

double maxpl[NUMITEMS];			/* max items a planet can hold (adjusted later) */

long weight[NUMITEMS];			/* net weight of each item */
long value[NUMITEMS];			/* net score value of each item on a planet */
long manhours[NUMITEMS];		/* items produced per 1000 men per day */

typedef struct _menu {
	int	substt;
	int	(*func)(void);
} MENU;

#define MENUNUM (sizeof(menu) / sizeof(MENU))

MENU menu[] = {
	{0,        mnu_main},       /* selected GE from the MajorBBS main menu */
	{1,        mnu_main_ans},   /* choosing an option from the GE main menu */
	{FIGHTSUB, mnu_fightsub},   /* entering commands while in flight */
	{ADMENU1,  mnu_admenu1},    /* do you wish to claim this planet */
	{ADMENU1A, mnu_admenu1a},   /* enter the new planet name */
	{ADMENU2,  mnu_admenu2},    /* choose an option from the planet admin menu */
	{ADMENU2B, mnu_admenu2b},   /* enter the amount of tax cash to transfer */
	{ADMENU2E, mnu_admenu2e},   /* choose which planet item to modify */
	{ADMEN2F1, mnu_admenu2f1},  /* enter production effort for the selected item */
	{ADMEN2F2, mnu_admenu2f2},  /* enter the sale price for the selected item */
	{ADMEN2F3, mnu_admenu2f3},  /* choose whether to sell the selected item */
	{ADMEN2F4, mnu_admenu2f4},  /* enter the reserve stockpile for the selected item */
	{ADMENU2H, mnu_admenu2h},   /* enter the planet tax rate */
	{ADMENU2I, mnu_admenu2i},   /* enter the planet trade password */
	{ADMENU2J, mnu_admenu2j},   /* enter the planet beacon message */
	{CHOOSESH, mnu_choosesh},   /* choose which ship to enter */
	{MENUG,    mnu_menug},      /* choose an option from the mail menu */
	{MENUG1,   mnu_menug1},     /* advance or exit distress messages */
	{MENUG2,   mnu_menug2}      /* advance or exit production reports */
};


/**************************************************************************
** System start up function                                              **
**************************************************************************/

#ifdef PHARLAP
void EXPORT init__galemp(void)
{
#ifdef GETRAINER
	stzcpy(mpoge.descrp, gmdnam("MPOG2.MDF"), MNMSIZ);
#else
	stzcpy(mpoge.descrp, gmdnam("MPOGEMP.MDF"), MNMSIZ);
#endif

	iniwara();
	gestt = register_module(&mpoge);
}
#else
int FUNC iniwar(void)
{
	iniwara();
	return 0;
}
#endif

void FUNC dummy(void)
{
}

void FUNC iniwara(void)
{
	int i, n, type, classbase;
	int j;

	int class_tab[50];

	gemb = opnmsg(GEMSG);
	endmark = stgopt(ENDMARK);
	if (!sameas(endmark, "ENDMARK")) {
		catastro("GE:ERR:MPOGEMSG.MCV Corrupted");
	}

	geuser = stgopt(GEUSER);		/* configured user database name */
	geship = stgopt(GESHIP);		/* configured ship database name */
	geplnt = stgopt(GEPLNT);		/* configured planet database name */
	gemail = stgopt(GEMAIL);		/* configured mail database name */
	geshipcl = stgopt(GESHIPCL);		/* configured ship class message name */

	gemaxplrs = numopt(MAXPLRS, 1, 256);	/* max simultaneous GE players */
	gefreebies = numopt(FREEBIES, 0, 1);	/* allow non-paying/freebie access */
	gemaxlist = numopt(MAXLIST, 3, 50);	/* max entries shown in GE lists */
	gerostxt = ynopt(GEROSTXT);		/* nightly roster text-file export enabled */
	maxships = numopt(MAXSHIPS, 1, 60);	/* max ships allowed per user */
	se100dam = numopt(SE100DAM, 1, 101);	/* enforcer planet damage per violation */
	showopt = numopt(SHOWOPT, 0, 5);	/* log level to console */
	trans_opt = ynopt(TRANSOPT);		/* allow goods transfers to planets not owned */
	syscmds = ynopt(SYSCMDS);		/* sysop commands enabled */
	sysonly = ynopt(SYSONLY);		/* restrict special commands to sysops */
	max_plnts = numopt(MAXPLNTS, 1, 256);	/* max planets a user may own */
	planupd = numopt(PLANUPD, 1, 15);	/* planet updates per day */
	plodds = numopt(PLODDS, 1, 20);		/* planet creation odds */
	wormodds = numopt(WORMODDS, 1, 100);	/* wormhole creation odds */
	nebodds = numopt(NEBODDS, 0, 10);	/* nebula existence odds */
	univmax = numopt(UNIVMAX, 10, 32767);	/* radius of galaxy outside of 0 0 */
	univwrap = ynopt(UNIVWRAP);		/* universe wraparound enabled */
	s00plnum = numopt(S00PLNUM, 3, 9);	/* number of sector 0,0 predefined objects */
	maxplanets = numopt(MAXPLSE, 1, 9);	/* max planets allowed per sector */
	teambonus = numopt(TEAMBONU, 0, 32000) * 100L;	/* team score/cash bonus factor */
	team_max = numopt(TEAMMAX, 0, 32000);	/* max members allowed on a team */
	meneat = ynopt(MENEAT);			/* do men eat */
	showdoc = numopt(SHOWDOC, 0, 10);	/* how often to give winner planet list from loser */
	cattkd = numopt(CATTKD, 0, 10);		/* how often do cybs attack droids */
	gcnum = numopt(GCNUM, 0, 8);		/* which faction does the neutral zone belong to*/

	profon = ynopt(PROFON);			/* profiling output enabled */
	logflag = ynopt(LOGFLG);		/* extended GE trace logging enabled */

	if (logflag)
		geshocst(0, "GE:Ext Trace Logging ON!");
#ifdef FASTPLANET
	geshocst(0, "GE:FASTPLANET ON!");
#endif

	hpfirdst = numopt(HPFIRDST, 1, 20);		/* hyperphaser distance factor */
	hpdammax = numopt(HPDAMMAX, 1, 200);		/* hyperphaser max damage factor */
	pfirdist = numopt(PFIRDST, 1, 20);		/* phaser distance factor */
	pdammax = numopt(PDAMMAX, 1, 200);		/* phaser max damage factor */

	jamtime = numopt(JAMTIME, 1, 10);		/* jammer duration */
	maildays = numopt(MAILDAYS, 1, 7);		/* mail retention days */
	torpsped = numopt(TORPSPED, 1, 10000);		/* torpedo speed */
	mislsped = numopt(MISLSPED, 1, 10000);		/* base missile speed */

	nummines = numopt(NUMMINES, 1, 200);		/* total mine slots */
	usermines = numopt(USRMINES, 1, 200);		/* max mines per user */
	decodds = numopt(DECODDS, 1, 20);		/* decoy effectiveness odds */

	tor_fact = (double)numopt(TORFACT, 1, 50);	/* torpedo hit-distance factor */
	tor_fact /= 10.0;
	tdammax = (double)numopt(TDAMMAX, 1, 100);	/* torpedo max damage factor */
	mis_fact = (double)numopt(MISFACT, 1, 50);	/* missile hit-distance factor */
	mis_fact /= 10.0;
	mdammax = (double)numopt(MDAMMAX, 1, 100);	/* missile max damage factor */
	idammax = (double)numopt(IDAMMAX, 1, 100);	/* ion cannon max damage factor */
	minedammax = (double)numopt(MNDAMMAX, 1, 200);	/* mine max damage factor */
	repairrate = (double)numopt(REPAIRRT, 1, 50);	/* per-tick repair rate */
	repairrate /= 100.0;

	tooclose = (double)numopt(TOOCLOSE, 1, 32000);	/* cyborg close-range threshold */
	clenguse = numopt(CLENGUSE, 1, 32000);		/* cloak energy use */

	startcash = (long)numopt(STRTCASH, 1, 32000);	/* starting user cash in thousands */
	startcash *= 1000L;

	max_plrec = (long)numopt(MAXPLREC, 10, 32767);	/* max MPOGEPLT.DAT size in KiB */
	max_plrec *= 2;					/* max planet records (512 bytes each) */

	cyb_gold = numopt(CYBGOLD, 0, 32000);		/* cyborg gold reward setting */

	hyperdist1 = (double)numopt(HYPDST1, 1, 32000);	/* cyborg long-range pursuit threshold */
	hyperdist2 = (double)numopt(HYPDST2, 1, 32000);	/* cyborg mid-range pursuit threshold */

	plattrf1 = (double)numopt(PLATTRF1, 5, 1000);	/* planet troops destroying attacking fighters */
	plattrf1 /= 100.0;
	plattrf2 = (double)numopt(PLATTRF2, 5, 1000);	/* defending fighters destroying attacking fighters */
	plattrf2 /= 100.0;
	plattrf3 = (double)numopt(PLATTRF3, 5, 1000);	/* attacking fighters destroying defending fighters */
	plattrf3 /= 100.0;
	plattrt1 = (double)numopt(PLATTRT1, 5, 1000);	/* defending troops destroying attacking troops */
	plattrt1 /= 100.0;
	plattrt2 = (double)numopt(PLATTRT2, 5, 1000);	/* attacking troops destroying defending troops */
	plattrt2 /= 100.0;

	/* load the planet maximum table */
	for (i = 0; i < NUMITEMS; ++i) {
		logthis(spr("Item %s", item_name[i]));

		maxpl[i] = (double)lngopt(ITMPL01 + i, 0L, 201228378L);
		logthis(spr("Itm #%d maxpl=%ld", i, (long)maxpl[i]));

		weight[i] = lngopt(ITMWT01 + i, 1L, 201228378L);
		logthis(spr("Itm #%d weight=%ld", i, weight[i]));

		value[i] = lngopt(ITMVAL01 + i, 0L, 201228378L);
		logthis(spr("Itm #%d value=%ld", i, value[i]));

		manhours[i] = lngopt(ITMMH01 + i, 0L, 201228378L);
		logthis(spr("Itm #%d manhours=%ld", i, manhours[i]));

		baseprice[i] = numopt(ITMPR01 + i, 1, 32000);
		logthis(spr("Itm #%d baseprice=%d", i, baseprice[i]));
	}

	/* load the shieldprice table */
	for (i = 0; i < TOPSHIELD; ++i) {
		logthis(spr("shieldtype %d", i));

		shieldprice[i] = lngopt(SHLDPR01 + i, 0L, 201228378L);
		logthis(spr("Shld #%d Price=%ld", i, shieldprice[i]));
	}

	/* load the phaserprice table */
	for (i = 0; i < TOPPHASOR; ++i) {
		logthis(spr("phasertype %d", i));

		phaserprice[i] = lngopt(PHSRPR01 + i, 0L, 201228378L);
		logthis(spr("Phaser #%d Price=%ld", i, phaserprice[i]));
	}

	/* load the upgrade price table */
	for (i = 0; i < 8; ++i) {
		logthis(spr("upgrade %d", i + 1));

		upgrprice[i] = lngopt(UPGRPR1 + i, 0L, 201228378L);
	logthis(spr("Upgr #%d Price=%ld",i+1,upgrprice[i]));
	}

	pltvcash = lngopt(PLTVCASH, 0L, 201228378L);	/* planet value cash scaling factor */
	logthis(spr("pltvcash=%ld", pltvcash));
	pltvdiv = lngopt(PLTVDIV, 0L, 201228378L);	/* planet value item scaling divisor */

	phatowrp = numopt(PHATOWRP, 0, 100);		/* minimum phasers required to hit ships at warp */
	score_bonus = numopt(SCRBONUS, 0, 32700);	/* score bonus setting */
	score_f2 = numopt(SCRFACT, 0, 32700);		/* score scaling factor */
	chgloser = numopt(CHGLOSER, 0, 100);		/* loser cash-charge percentage */

	optmenu = ynopt(OPTMENU);			/* optional text/help menu enabled */
	optchr = chropt(OPTCHR);			/* optional text trigger character */
	optchr = (char)toupper((unsigned char)optchr);
	opttxt = stgopt(OPTTXT);			/* optional text/help prompt */

	opttbl = (long *)alcmem(n = nterms * sizeof(long));	/* per-user option-text offsets */
	setmem(opttbl, n, 0);

	gebb1 = opnbtv(geship, sizeof(WARSHP));			/* open ship database */
	gebb4 = opnbtv(gemail, sizeof(struct message) + GEMSGSIZ);	/* open mail database */
	gebb2 = opnbtv(geplnt, sizeof(GALSECT));		/* open planet database */

	nebseed = 1L;		/* default nebula seed until a saved seed is restored */
	pkey.xsect = 0;
	pkey.ysect = 0;
	pkey.plnum = 1;		/* let's see if we already have a nebula seed in Zygor */
	if (gesdb(GEGET, &pkey, (GALSECT *)&planet)) {
		if (planet.type == PLTYPE_PLNT && planet.nebseed != 0L)
			nebseed = planet.nebseed;	/* restore persisted nebula seed */
	} else {
		nebseed = (((unsigned long)gernd()) << 16) | (unsigned long)gernd();
		if (nebseed == 0L)
			nebseed = 1L;
	}

	/* cofdat is number of days since 1980-01-01 */
	/* this value is used to determine if/how planets should be updated */
	game_day = cofdat(today());

	cyb_class = 0;
	dr_class = 0;

	/* load the ship class table */
	geshmb = opnmsg(geshipcl);
	setmbk(geshmb);

#define NCL 28		/* ship class table entries per class slot */

	/* first audit the table */

	/* does the table have all the elements? */
	n = (SXXEND - S01TYPE);
	i = n / NCL;
	if ((i*NCL) != n)
		catastro("GE:ERR:Ship Class Tbl Corrupted");

	/* this is how many inactive and active classes we have	*/
	/* since we only load active classes we must go figure	*/
	/* out how many that really is.				*/

	geshocst(1, spr("GE:INF:Fnd %d class slots", i));
	tot_classes = i;

	n = 0;

	for (i = 0; i < tot_classes; ++i) {
		classbase = S01TYPE + (i * NCL);
		type = tokopt(classbase, "USER", "CYBORG", "DROID", "<NONE>", NULL);

		class_tab[i] = classbase;

		if (type != CLASSTYPE_NONE) {
			++n;
		}
	}

	geshocst(1, spr("GE:INF:Fnd %d defined classes", n));

	/* allocate memory for ship class table */

	shipclass = (SHIP *)alcmem(n = tot_classes * sizeof(SHIP));
	setmem(shipclass, n, 0);
	geshocst(1, spr("GE:INF:Ship Class Mem: %d", n));

	/* read in the ship classes */
	i = 0;

	/* each class is loaded from its fixed-width option block in MPOGESHP.MSG */
	for (n = 0; n < tot_classes; ++n) {
		classbase = class_tab[i];
		shipclass[i].max_type = tokopt(classbase, "USER", "CYBORG", "DROID", "<NONE>", NULL);
		shipclass[i].typename = stgopt(++classbase);
		logthis(spr("Loaded class %d - %s", i, shipclass[i].typename));

		shipclass[i].npcprefx = stgopt(++classbase);
		logthis(spr("  NPC prefix %s", shipclass[i].npcprefx));

		shipclass[i].max_shlds = numopt(++classbase, 0, 19);
		shipclass[i].max_phasr = numopt(++classbase, 0, 19);
		shipclass[i].max_torps = numopt(++classbase, 0, 3);
		shipclass[i].max_missl = numopt(++classbase, 0, 3);
		shipclass[i].has_decoy = ynopt(++classbase);
		shipclass[i].has_jam = ynopt(++classbase);
		shipclass[i].has_zip = ynopt(++classbase);
		shipclass[i].has_mine = ynopt(++classbase);
		shipclass[i].max_attk = ynopt(++classbase);
		shipclass[i].max_cloak = ynopt(++classbase);
		shipclass[i].max_accel = numopt(++classbase, 0, 32767);
		shipclass[i].max_warp = numopt(++classbase, 0, 255);
		shipclass[i].max_tons = lngopt(++classbase, 1, 2000000000L);
		shipclass[i].max_price = lngopt(++classbase, 1, 2000000000L);
		shipclass[i].max_points = numopt(++classbase, 1, 32767);
		shipclass[i].scanrange = lngopt(++classbase, 1, 9999999L);
		shipclass[i].cybs_can_att = ynopt(++classbase);
		shipclass[i].noclaim = numopt(++classbase, 0, 5);
		shipclass[i].lowest_to_attk = numopt(++classbase, 0, 255);
		shipclass[i].tot_to_create = numopt(++classbase, 0, 255);
		shipclass[i].tough_factor = numopt(++classbase, 0, 4);
		shipclass[i].damfact = numopt(++classbase, 0, 32767);
		shipclass[i].faction = numopt(++classbase, 0, 32767);
		shipclass[i].loadout = numopt(++classbase, 0, 32767);

		shipclass[i].hlpmsg = ++classbase;

		shipclass[i].init_func = NULL;
		shipclass[i].tick_func = NULL;
		shipclass[i].kill_func = NULL;
		shipclass[i].won_func = NULL;

		/* how many NPCs of this class to make */
		if (shipclass[i].max_type == CLASSTYPE_CYBORG ||
			shipclass[i].max_type == CLASSTYPE_DROID) {
			numships += shipclass[i].tot_to_create;
		}

		/* attach class-specific behavior hooks; user classes leave these NULL */
		if (shipclass[i].max_type == CLASSTYPE_CYBORG) {
			if (cyb_class == 0)	/* remember the first cyborg class as the base class */
				cyb_class = i;
			shipclass[i].init_func = cyb_init;
			shipclass[i].tick_func = cyb_lives;
			shipclass[i].kill_func = cyb_died;
			shipclass[i].won_func = cyb_won;
		} else if (shipclass[i].max_type == CLASSTYPE_DROID) {
			if (dr_class == 0)	/* remember the first droid class as the base class */
				dr_class = i;
			shipclass[i].init_func = droid_init;
			shipclass[i].tick_func = droid_lives;
			shipclass[i].kill_func = droid_died;
			shipclass[i].won_func = droid_won;
		}
		geshocst(1, spr("GE:INF:Init Class %s", shipclass[i].typename));

		++i; /* index the next table entry */
	}

	gebb5 = opnbtv(geuser, sizeof(WARUSR));

	gehlpmb = opnmsg(GEHELP);

	/* ships in game is at least number of terminal channels */
	nships = nterms + numships;
	if (nships > 125) {
		geshocst(0, spr("GE:ERR:Too many ships defined"));
		nships = 125;
	}

	/* allocate memory for user data table */
	warusr_ecl = pltile(nships * (long)sizeof(WARUSR), 0, sizeof(WARUSR), sizeof(WARUSR));
	warusr = MK_FP(warusr_ecl, 0);

	for (j = 0; j < nships; j++) {
		setmem((void *)warusroff(j), sizeof(WARUSR), 0);
	}
	geshocst(1, spr("GE:INF:User Mem: %ld", nships * sizeof(WARUSR)));

	/* allocate memory for ship data table */
	warshp_ecl = pltile(nships * (long)sizeof(WARSHP), 0, sizeof(WARSHP), sizeof(WARSHP));
	warshp = MK_FP(warshp_ecl, 0);

	for (j = 0; j < nships; j++) {
		setmem((void *)warshpoff(j), sizeof(WARSHP), 0);
		warshpoff(j)->status = GESTAT_AVAIL;
	}
	geshocst(1, spr("GE:INF:Ship Mem: %ld", nships * sizeof(WARSHP)));

	/* allocate memory for planet table */
	ptab = (PLANETAB *)alcmem(n = nships * sizeof(PLANETAB));
	setmem(ptab, n, 0);
	geshocst(1, spr("GE:INF:Planet Table Mem: %d", n));

	/* allocate memory for a temporary item table */
	titems = (ITEM *)alcmem(n = nships * sizeof(ITEM));
	setmem(titems, n, 0);
	geshocst(1, spr("GE:INF:Temp Items Mem: %d", n));

	/* allocate memory for a team table */
	teamtab = (TEAM *)alcmem(n = MAXTEAMS * sizeof(TEAM));
	geshocst(1, spr("GE:INF:Team Tab Mem: %d", n));

	/* allocate memory for scan table */
	scantab = (SCANTAB *)alcmem(n = nships * sizeof(SCANTAB));
	setmem(scantab, n, 0);
	geshocst(1, spr("GE:INF:Scantab Mem: %d", n));

	/* allocate memory for delayed entry message state */
	entrytab = (ENTRYTAB *)alcmem(n = nterms * sizeof(ENTRYTAB));	/* per-user delayed entry-message timers/state */
	setmem(entrytab, n, 0);
	entrybytes = (nterms + 7) / 8;					/* bytes needed for one nterms-bit recipient map */
	entrysent = (byte *)alcmem(n = nterms * entrybytes);	/* per-entrant bitmaps of recipients already sent entry */
	setmem(entrysent, n, 0);
	entrypend = (byte *)alcmem(n = nterms * entrybytes);	/* per-entrant bitmaps of recipients still pending entry */
	setmem(entrypend, n, 0);

	/* allocate memory for S00 table */
	s00 = (S00 *)alcmem(n = s00plnum * sizeof(S00));
	setmem(s00, n, 0);
	geshocst(1, spr("GE:INF:S00 Mem: %d", n));

	beacontimer = 0;

	/* allocate memory for the mail message table */
	gemsg = (struct message *)alcmem(sizeof(struct message) + GEMSGSIZ);

	/* allocate memory for mine table */
	mines = (MINE *)alcmem(n = nummines * sizeof(MINE));
	setmem(mines, n, 0);
	geshocst(1, spr("GE:INF:Mines Mem: %d", n));

	/* allocate memory for garbage bucket */
	gechrbuf = (char *)alcmem(255);
	gechrbuf2 = (char *)alcmem(20);
	gechrbuf3 = (char *)alcmem(20);
	warpbuf = (char *)alcmem(40);

	/* init empty mine field */
	for (n = 0; n < nummines; ++n)
		mines[n].channel = 255;

	/* init sector, planet, and worm table to bad values */
	sector.xsect = 32767;
	sector.ysect = 32767;
	sector.plnum = 32767;

	planet.xsect = 32767;
	planet.ysect = 32767;
	planet.plnum = 32767;

	worm.xsect = 32767;
	worm.ysect = 32767;
	worm.plnum = 32767;

	setmbk(gemb);

#define NPL 8	/* number of parameters per planet entry */

	/* read in the neutral planets */
	for (i = 0; i < s00plnum; ++i) {
		if ((S00P1RES + (i * NPL)) - (S00P1DEF + (i * NPL)) != (NPL - 1))
			catastro(spr("GE:ERR:Sect00 Table Error %d Msg # %d", i + 1, S00P1DEF + (i * NPL)));
		classbase = S00P1DEF + (i * NPL);
		n = ynopt(classbase);
		if (!n)
			catastro(spr("GE:ERR:Sect00 Table Error %d Msg # %d", i + 1, S00P1DEF + (i * NPL)));

		s00[i].name = stgopt(++classbase);
		s00[i].owner = stgopt(++classbase);
		s00[i].type = numopt(++classbase, 0, 3);

		s00[i].xcoord = (double)numopt(++classbase, 100, 9900);
		s00[i].xcoord = s00[i].xcoord / 10000.0;
		s00[i].ycoord = (double)numopt(++classbase, 100, 9900);
		s00[i].ycoord = s00[i].ycoord / 10000.0;

		s00[i].env = numopt(++classbase, 0, 3);
		s00[i].res = numopt(++classbase, 0, 3);

		geshocst(1, spr("GE:INF:I/S00 %d %s", s00[i].type, s00[i].name));
	}

	/* Load the team table from disk */
	load_team_tab();

	/* tell everyone that we are up */
	geshocst(0, spr("Galactic Empire %s", VERSION));
	geshocst(0, spr("Registration # %s", stgopt(REGNO)));

	#ifdef PHARLAP
	rtkick(TICKTIME, pwarrti);
	rtkick(TICKTIME2, pwarrti2);
	rtkick(60, pwarrti3);
	rtkick(30, pplarti);
	rtkick(1, pautorti);
	#else

	rtkick(TICKTIME, warrti);
	rtkick(TICKTIME2, warrti2);
	rtkick(60, warrti3);
	rtkick(10, plarti);
	rtkick(1, autorti);
	#endif

	/* find the module number (state) of the FSE for later use */
	fse_state = -1;
	for (i = 0; i < nmods; i++) {
		if ((sameas((char *)(module[i]->descrp), "Editor")) == TRUE)
			fse_state = i;
	}
}

/**************************************************************************
** User loged in routine                                                 **
**************************************************************************/

int FUNC gelogon(void)
{
	int i;
	/* if classic GE is installed, don't send an additional login msg */
	/* this might need rethinking when there are other GE variants */
	int other_ge_present = FALSE;

	for (i = 0; i < nmods; ++i) {
		if (module[i] == &mpoge)	/* we see ourself */
			continue;

		if (sameas((char *)module[i]->descrp, "Galactic Empire"))	/* hello there */
			{
			other_ge_present = TRUE;
			break;
			}
	}

	setmbk(gemb);

	if (!hasmkey(PLAYKEY) || other_ge_present == TRUE)
		return 0;

	if (!geudb(GELOOKUP, usaptr->userid, warusroff(usrnum))) {
		/* you haven't played yet? seriously? */
		prfmsg(GECALLS);
		outprf(usrnum);
	} else if (gernd() % 10 == 1) {
		/* occasional reminder that this game is fun and you probably miss it */
		prfmsg(GECALLS2);
		outprf(usrnum);
	}
	return 0;
}

/**************************************************************************
** User deleted routine                                                  **
**************************************************************************/

#ifdef PHARLAP
void FUNC pgedelete(char *uid)
{
	gedeletea(uid);
}
#else
static int gedelete(char *uid)
{
	gedeletea(uid);
	return 0;
}
#endif

/* delete all GE data for a user account that MajorBBS is removing */
void FUNC gedeletea(char *uid)
{
	if (geudb(GELOOKUP, uid, &tmpusr)) {
		/* delete all ships owned by this user, then remove the GE user record */
		geudb(GEGET, uid, &tmpusr);
		while (gepdb(GELOOKUPNAME, uid, 0, &tmpshp)) {
			gcrbtv(&tmpshp, 0);
			gepdb(GEDELETE, tmpshp.userid, tmpshp.shipno, &tmpshp);
			logthis(spr("GE:Deleted %s ship %d", tmpshp.userid, tmpshp.shipno));
		}
		geudb(GEDELETE, tmpusr.userid, &tmpusr);
		logthis(spr("GE:Deleted %s user", tmpusr.userid));
		return;
	}
	geshocst(1, spr("GE:User %s not in DB", uid));
}


/**************************************************************************
** Midnight cleanup routine                                              **
**************************************************************************/

#ifdef PHARLAP
void FUNC pgemidnight(void)
{
	gemidnighta();
}
#else
int FUNC gemidnight(void)
{
	gemidnighta();
	return 0;
}
#endif

void FUNC gemidnighta(void)
{
	int i;
	int foundit;
	int intkey = PLTYPE_PLNT;

	setmbk(gemb);

	geshocst(0, spr("GE:INF:Begin Cleanup"));

	prune_stale_auto_records();

	/* clear out planet counter */
	geshocst(1, spr("GE:INF:Cleanup Phase-1"));
	setbtv(gebb5);
	if (qlobtv(0)) {
		do {
			gcrbtv(&tmpusr, 0);
			tmpusr.planets = 0;
			tmpusr.score = 0;
			tmpusr.plscore = 0;
			tmpusr.population = 0;
			/* faction dislike goes down by 40 per day */
			for (i = 0; i < 8; ++i)
				if (tmpusr.factions[i] > 40)
					tmpusr.factions[i] = tmpusr.factions[i] - 40;
				else
					tmpusr.factions[i] = 0;
			updbtv(&tmpusr);
			gcrbtv(&tmpusr, 0);	/* thank you BTRIEVE 5.00b */
		} while (qnxbtv());
	}

	geshocst(1, spr("GE:INF:Cleanup Phase-2"));

	setbtv(gebb2);

	/* if we didn't finish one full planet pass, recalc plantime tomorrow */
	if (passnum <= 1) {
		geshocst(1, spr("GE:INF:first planet cycle didn't complete, recalc tomorrow"));
		if (agebtv(&planet, &intkey, 2)) {
			/* force the scheduler to recompute the saved planet update timing */
			planet.plantimesave = 0;
			gesdb(GEUPDATE, (PKEY *)&planet, (GALSECT *)&planet);
		}
	}

	if (qlobtv(0)) {
		do {
			gcrbtv(&planet, 0);
			setbtv(gebb5);
			if (planet.type == PLTYPE_PLNT) {
				if (planet.userid[0] != 0) {
					/* roll each owned planet's value and population back into its owner */
					if (qeqbtv(planet.userid, 0)) {
						gcrbtv(&tmpusr, 0);
						plptr = &planet;
						calc_networth();
						++tmpusr.planets;
						tmpusr.population += (plptr->items[I_MEN].qty / 10000L);
						updbtv(&tmpusr);
						gcrbtv(&tmpusr, 0);

						/* now go create the Status Record */
						strncpy(tmpstat.userid, tmpusr.userid, UIDSIZ);
						tmpstat.userid[UIDSIZ - 1] = 0;
						tmpstat.class = MAIL_CLASS_PRODRPT;
						tmpstat.type = MESG20;
						tmpstat.stamp = cofdat(today());
						sprintf(tmpstat.dtime, "%s - %.5s", ncedat(today()), nctime(now()));
						strcpy(tmpstat.name1, planet.name);
						tmpstat.int1 = planet.xsect;
						tmpstat.int2 = planet.ysect;
						tmpstat.cash = planet.cash;
						tmpstat.timestamp = planet.timestamp;
						tmpstat.tax = planet.tax;
						for (i = 0; i < NUMITEMS; ++i)
							tmpstat.itemqty[i] = planet.items[i].qty;

						/* queue a production/status report mail record for the planet owner */
						memcpy(&mail, &tmpstat, sizeof(MAILSTAT));
						mailit(0);
					}
				}
			}
			setbtv(gebb2);
		} while (qnxbtv());
	}

	sprintf(gechrbuf, "%ld", (cntrbtv() / 2L));
	geshocst(0, spr("GE:INF:Plnt DB Size %sk", gechrbuf));

	if (cntrbtv() >= max_plrec)
		geshocst(0, "GE:INF:Max Sect Reached");

	geshocst(1, spr("GE:INF:Cleanup Phase-3"));

	/* purge mail older than 7 days */
	setbtv(gebb4);

	i = cofdat(today());
	i -= maildays; /* back up 1 week */
	if (qlobtv(0)) {
		do {
			gcrbtv(gemsg, 0);
			/* GE-owned mail uses nreply as its saved day stamp for retention checks */
			if (gemsg->nreply < i) /* we robbed nreply for the stamp */
				delbtv();
			/* purge mail addressed to deleted/non-live players */
			if (gemsg->userto[0] == '*') /* non-live player */
				delbtv();

			} while (qnxbtv());
	}

	geshocst(1, spr("GE:INF:Cleanup Phase-4"));
	setbtv(gebb5);

	/* zero out the team count */
	for (i = 0; i < MAXTEAMS; ++i) {
		teamtab[i].teamcount = 0;
	}

	/* first count up the members of a team */
	if (qlobtv(0)) {
		do {
			gcrbtv(&tmpusr, 0);
			foundit = FALSE;
			if (tmpusr.teamcode > 0) {
				/* verify that each saved team code still points to a live team entry */
				for (i = 0; i < MAXTEAMS; ++i) {
					if (teamtab[i].teamcode == tmpusr.teamcode
						&& teamtab[i].teamname[0] != '@') {
						foundit = TRUE;
						break;
					}
				}
				if (foundit == FALSE) {
					tmpusr.teamcode = 0;
					logthis(spr("Reset Teamcode to 0 [%s]", tmpusr.userid));
				} else {
					++teamtab[i].teamcount;
					sprintf(gechrbuf, "++Teamcnt %ld %s", tmpusr.teamcode, tmpusr.userid);
					logthis(gechrbuf);
				}
			}
			/* write back users whose stale team assignment was cleared */
			if (foundit == FALSE)
				updbtv(&tmpusr);
			gcrbtv(&tmpusr, 0);
		} while (qnxbtv());
	}

	/* update player scores */
	if (qlobtv(0)) {
		do {
			gcrbtv(&tmpusr, 0);
			/* total score is recomputed from planet score plus kill score */
			tmpusr.score = tmpusr.plscore + tmpusr.klscore;
			updbtv(&tmpusr);
			gcrbtv(&tmpusr, 0);
		} while (qnxbtv());
	}

	if (gerostxt)
		dump_roster_file();

	/* remove any teams with no players */
	for (i = 0; i < MAXTEAMS; ++i) {
		if (teamtab[i].teamcode > 0
			&& teamtab[i].teamname[0] != '@'
			&& teamtab[i].teamcount == 0) {
			geshocst(0, spr("GE:INF:Removed Team %s", teamtab[i].teamname));
			/* retain the slot as a deleted marker so team history stays coherent */
			strcpy(teamtab[i].teamname, "@DELETED@");
			teamtab[i].teamdeldate = cofdat(today());
			teamtab[i].password[0] = 0;
			teamtab[i].secret[0] = 0;
		}
	}

	/* update the team scores on disk */
	update_team_tab();

	geshocst(0, spr("GE:INF:End Cleanup"));
}

/**************************************************************************
** Write nightly top roster export                                       **
**************************************************************************/

void FUNC dump_roster_file(void)
{
	FILE *hdl;
	int i;

	hdl = fopen("MPOGEROS.TXT", "wt");
	if (hdl == NULL) {
		geshocst(0, "GE:ERR:MPOGEROS.TXT open failed");
		return;
	}

	setmbk(gemb);
	clrprf();
	prfmsg(ROSTER2, gemaxlist);
	fputs(prfbuf, hdl);
	clrprf();

	i = 0;
	setbtv(gebb5);
	if (qhibtv(1)) {
		do {
			gcrbtv(&tmpusr, 1);
			if (tmpusr.score > 0 && tmpusr.userid[0] != '@') {
				sprintf(gechrbuf, "%11lu", tmpusr.score);
				sprintf(gechrbuf2, " %10.2fm", ((float)tmpusr.population) / 100.0);
				fprintf(hdl, "%-29s%s%6u%6u%4u%s\n",
					tmpusr.userid,
					gechrbuf,
					tmpusr.kills,
					tmpusr.ukills,
					tmpusr.planets,
					gechrbuf2);
				++i;
			}
		} while (qprbtv() && i < gemaxlist);
	}

	fclose(hdl);
	geshocst(1, "GE:INF:MPOGEROS.TXT updated");
}

/* determine the net worth of a planet */
void FUNC calc_networth(void)
{
	unsigned long v;

	v = value_pl();
	tmpusr.plscore += v;
}

unsigned FUNC long value_pl(void)
{
	unsigned long v;
	int i;

	v = (plptr->cash + plptr->tax) / (1000000L / pltvcash);

	for (i = 0; i < NUMITEMS; ++i) {
		v += (value[i] * ((long)plptr->items[i].qty / pltvdiv));
	}

	return v;
}

/**************************************************************************
** User logged off                                                       **
**************************************************************************/

int FUNC pwarlof(void)
{
	warsptr = warshpoff(usrnum);
	waruptr = warusroff(usrnum);

	logthis(spr("WARLOF called 4 %s", waruptr->userid));
	return 0;
}

/**************************************************************************
** User hungup routine                                                   **
**************************************************************************/

#ifdef PHARLAP
void FUNC pwarhup(void)
{
	warhupa();
}

#else
int FUNC warhup(void)
{
	warhupa();
	return 0;
}
#endif

void FUNC warhupa(void)
{
	setbtv(gebb1);
	setmbk(gemb);

	warsptr = warshpoff(usrnum);
	waruptr = warusroff(usrnum);

	logthis(spr("WARHUP called 4 %s", waruptr->userid));

	if (warsptr->status == GESTAT_USER) {
		if (ingegame(usrnum)) {
		/* if modem hangup */
			logthis(spr("User Hungup Status = %d", status));
			if (warsptr->cantexit > 0)
#ifndef MBBSEMU
			{
				/* if we're in cleanup mode, don't killem */
				if (status == RING && rsmodes[usrnum] != NORMRS) {
					/* clear this user's projectile ownership from active torps and missiles */
					cleartm(usrnum);
					/* clear torps and missiles currently inbound to this user */
					clearitm(usrnum);
					gepdb(GEUPDATE, warsptr->userid, warsptr->shipno, warsptr);
					geudb(GEUPDATE, waruptr->userid, waruptr);
				}
				else
#endif
				{
					killem(warsptr, usrnum);
					warsptr->where = -1;
				}
#ifndef MBBSEMU
			}
#endif
			else {
				/* broadcast this user's departure to entry-message recipients */
				exit_entrymsg(usrnum);
				/* clear this user's projectile ownership from active torps and missiles */
				cleartm(usrnum);
				/* clear torps and missiles currently inbound to this user */
				clearitm(usrnum);
				gepdb(GEUPDATE, warsptr->userid, warsptr->shipno, warsptr);
				geudb(GEUPDATE, waruptr->userid, waruptr);
			}
			--numwar;
		}
	}

	warsptr->status = GESTAT_AVAIL;
}

/**************************************************************************
** System shutdown message                                               **
**************************************************************************/

#ifdef PHARLAP
void FUNC pclswar(void)
{
	clswara();
}
#else
int FUNC clswar(void)
{
	clswara();
	return 0;
}
#endif

void FUNC clswara(void)
{
	if (gemb != NULL) {
		clsmsg(gemb);
		gemb = NULL;
	}

	clsbtv(gebb1);
	clsbtv(gebb4);
	clsbtv(gebb2);
	clsbtv(gebb5);

	logthis("***GALACTIC EMPIRE SHUTDOWN***");
}

/**************************************************************************
** Main input loop                                                       **
**************************************************************************/

int FUNC galemp(void)
{
	int i, rtn;

	setbtv(gebb1);
	setmbk(gemb);
	warsptr = warshpoff(usrnum);
	waruptr = warusroff(usrnum);

	for (i = 0; i < MENUNUM; ++i) {
		if (menu[i].substt == usrptr->substt) {
			rtn = menu[i].func();
			clrprf();
			return rtn;
		}
	}
	return 1;
}

/**************************************************************************
** Send message to all ships                                             **
**************************************************************************/

void FUNC outwar(int filter, unsigned exclude, unsigned channel, int mode)
{
	int zothusn;

	for (zothusn = 0; zothusn < nships; ++zothusn) {
		if (zothusn != exclude && ingegame(zothusn)) {
			if (mode == 0) {
				/* send to every in-game ship except the excluded one */
				outprfge(filter, zothusn);
			}
			else if (mode == 1) {
				/* send only to ships tuned to the requested frequency */
				if (channel == warshpoff(zothusn)->freq) {
					outprfge(filter, zothusn);
				}
			}
			else if (mode == 2) {
				/* send only to members of the requested team */
				if (channel == warusroff(zothusn)->teamcode) {
					outprfge(filter, zothusn);
				}
			}
		}
	}
	clrprf();
}

/**************************************************************************
** Player/ship Database functions                                        **
**************************************************************************/

int FUNC gepdb(int func, char *usrname, int shipnum, WARSHP *geptr)
{
	int rtn;

	setbtv(gebb1);
	rtn = 0;

	strncpy(shpkey.userid, usrname, UIDSIZ);
	shpkey.shipno = shipnum;
	logthis(spr("GEPDB called: F=%d,%s,%d,%s", func, usrname, shipnum, geptr->userid));
	switch (func) {

	case GELOOKUP:
		if (qeqbtv(&shpkey, 1))
			rtn = 1;
		break;

	case GEADD:
#ifdef PHARLAP
		if (!dinsbtv(geptr))
#else
		if (!insbtv(geptr))
#endif
			geshocst(0, spr("GE:ERR:Ship ins Fail %s", usrname));
		else
			rtn = 1;

		break;

	case GEDELETE:
		if (acqbtv(NULL, &shpkey, 1)) {
			delbtv();
			rtn = 1;
		}
		else {
			geshocst(0, spr("GE:ERR:Ship Del Fail %s", usrname));
		}
		break;

	case GEUPDATE:
		if (acqbtv(NULL, &shpkey, 1)) {
			updbtv(geptr);
			rtn = 1;
		}
		else {
			geshocst(0, spr("GE:ERR:Ship Upd Fail %s", usrname));
		}
		break;

	case GEGET:
		if (acqbtv(geptr, &shpkey, 1))
			rtn = 1;
		break;

	case GENEXT:
		if (qnxbtv())
			rtn = 1;
		break;

	case GELOOKUPNAME:
		if (qeqbtv(usrname, 0))
			rtn = 1;
		break;

	default:
		rtn = 0;
	}
	return rtn;
}

/**************************************************************************
** User Database functions                                               **
**************************************************************************/

int FUNC geudb(int func, char *usrname, WARUSR *geptr)
{
	int rtn;

	setbtv(gebb5);
	rtn = 0;

	logthis(spr("GEUDB called: F=%d,%s,%s", func, usrname, geptr->userid));
	switch (func) {

	case GELOOKUP:
		if (qeqbtv(usrname, 0))
			rtn = 1;
		logthis(spr("GE: lookup *%s* f:%d", usrname, rtn));
		break;

	case GEADD:
#ifdef PHARLAP
		if (!dinsbtv(geptr))
#else
		if (!insbtv(geptr))
#endif
			geshocst(0, spr("GE:ERR:User ins Fail %s", usrname));
		else
			rtn = 1;

		break;

	case GEDELETE:
		if (acqbtv(NULL, usrname, 0)) {
			delbtv();
			rtn = 1;
		}
		else {
			geshocst(0, spr("GE:ERR:User Del Fail %s", usrname));
		}
		break;

	case GEUPDATE:
		logthis(spr("DEBUG <%s> <%s> update", usrname, geptr->userid));
		if (acqbtv(NULL, usrname, 0)) {
			updbtv(geptr);
			rtn = 1;
		}
		else {
			geshocst(0, spr("GE:ERR:User Upd Fail %s", usrname));
		}
		break;

	case GEGET:
		if (acqbtv(geptr, usrname, 0)) {
			rtn = 1;
		}
		else {
			geshocst(0, spr("GE:ERR:User Get Fail %s", usrname));
		}
		break;

	default:
		rtn = 0;
	}
	return rtn;
}


/**************************************************************************
** sector Database functions                                             **
**************************************************************************/

int FUNC gesdb(int func, PKEY *sect, GALSECT *geptr)
{
	int rtn;

	logthis(spr("Func GESDB, func = %d, sect*= %ld,geptr*=%ld", func, (long)sect, (long)geptr));
	logthis(spr("            xsect %d, ysect %d, plnum %d", sect->xsect, sect->ysect, sect->plnum));

	setbtv(gebb2);
	rtn = 0;

	switch (func) {

	case GELOOKUP:
		if (!qeqbtv(sect, 0))
			rtn = 1;
		break;

	case GEUPDATE:
		if (acqbtv(NULL, sect, 0)) {
			updbtv(geptr);
			rtn = 1;
		}
		else {
			geshocst(0, spr("GE:ERR:Plt Upd Fail x%d,y%d,p%d", sect->xsect,
				sect->ysect, sect->plnum));
		}
		break;

	case GEADD:
		logthis(spr("GE:DBG:Ins Sect %d %d %d", geptr->xsect, geptr->ysect, geptr->plnum));

#ifdef PHARLAP
		if (!dinsbtv(geptr))
#else
		if (!insbtv(geptr))
#endif
			geshocst(0, "GE:ERR:Sect/plt ins Fail");
		else {
			logthis("GE:DBG:Ins Sect suceeded");
			rtn = 1;
		}
		break;

	case GEGET:
		if ((geptr->xsect != sect->xsect)
			|| (geptr->ysect != sect->ysect)
			|| (geptr->plnum != sect->plnum)) {
			if (acqbtv(geptr, sect, 0)) {
				logthis("gesdb GEGET acqbtv found record");
				rtn = 1;
			}
		}
		else {
			logthis("gesdb GEGET record already in memory");
			rtn = 1;
		}
		break;

	case GEGETNOW:
		if (acqbtv(geptr, sect, 0))
			rtn = 1;
		break;

	default:
		rtn = 0;
	}
	return rtn;
}

int FUNC getplanetdat(int usrn)		/* plnum must already name the target planet slot */
{
	if (plnum <= 0 || plnum > MAXPLANETS)
		return FALSE;

	getsector(&(warshpoff(usrn)->coord));
	if (plnum > sector.numplan)
		return FALSE;

	if (!getplanet(&(warshpoff(usrn)->coord), plnum))
		return FALSE;

	plptr = &planet;
	if (plptr->type == PLTYPE_WORM)
		memcpy(&worm, &planet, sizeof(GALWORM)); /* make it the current wormhole */

	return TRUE;
}

static int load_admin_planet(void)
{
	plnum = warsptr->where - 10;
	if (!getplanetdat(usrnum)) {
		/* the player is in an orbit/admin flow but the backing record is gone */
		prfmsg(NOPLNT);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = FIGHTSUB;
		return FALSE;
	}
	return TRUE;
}

void FUNC fixplanetteam(void)
{
	int i;
	int valid = FALSE;

	/* only planets protected by a team password need team validation */
	if (!sameas(plptr->password, "team"))
		return;

	if (plptr->teamcode > 0) {
		/* verify that the planet still points at a live team entry */
		for (i = 0; i < MAXTEAMS; ++i) {
			if (teamtab[i].teamcode == plptr->teamcode
				&& teamtab[i].teamname[0] != '@') {
				valid = TRUE;
				break;
			}
		}
	}

	if (!valid) {
		/* clear stale team ownership and write the planet back to disk */
		plptr->teamcode = 0;
		plptr->password[0] = 0;
		setsect(warsptr); /* build PKEY */
		pkey.plnum = plnum;
		gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);
	}
}


/**************************************************************************
** Team Table Database functions                                         **
**************************************************************************/

void FUNC load_team_tab(void)
{
	char buffer[256];
	FILE *mzfp;
	int i;
	long val;

	/* clear out the memory team table */
	for (i = 0; i < MAXTEAMS; ++i) {
		teamtab[i].teamcode = 0;
		teamtab[i].teamname[0] = 0;
		teamtab[i].teamcount = 0;
		teamtab[i].teamdeldate = 0;
		teamtab[i].password[0] = 0;
		teamtab[i].secret[0] = 0;
	}

	logthis("Loading Team Table");

	/* reload the fixed-width team table saved in MPOGETEA.DAT */
	if ((mzfp = fopen("MPOGETEA.DAT", "r")) != NULL) {
		i = 0;
		while (fgets(buffer, sizeof(buffer), mzfp) != NULL) {
			/* valid records begin with TEAM| and include the final field delimiter */
			if (sameto("TEAM|", buffer) && strlen(buffer) > 80 && buffer[80] == '|') {
				/* unpack the fixed-position fields into the in-memory team table */
				strncpy(gechrbuf, &buffer[5], 5);
				gechrbuf[5] = 0;
				val = atol(gechrbuf);
				if (val < 0L)
					val = 0L;
				teamtab[i].teamcode = val;
				logthis(spr(" Team Code [%s]", gechrbuf));

				strncpy(teamtab[i].teamname, &buffer[11], 30);
				teamtab[i].teamname[30] = 0;
				stripb(teamtab[i].teamname);
				logthis(spr(" Team Name [%s]", teamtab[i].teamname));

				strncpy(gechrbuf, &buffer[42], 5);
				gechrbuf[5] = 0;
				val = atol(gechrbuf);
				if (val < 0L)
					val = 0L;
				else if (val > 65535L)
					val = 65535L;
				teamtab[i].teamcount = (unsigned int)val;
				logthis(spr(" Team Cnt [%s]", gechrbuf));

				strncpy(gechrbuf, &buffer[48], 10);
				gechrbuf[10] = 0;
				val = atol(gechrbuf);
				if (val < 0L)
					val = 0L;
				else if (val > 65535L)
					val = 65535L;
				teamtab[i].teamdeldate = (unsigned int)val;
				logthis(spr(" Team Del [%s]", gechrbuf));

				strncpy(teamtab[i].password, &buffer[59], 10);
				teamtab[i].password[10] = 0;
				stripb(teamtab[i].password);
				strncpy(teamtab[i].secret, &buffer[70], 10);
				teamtab[i].secret[10] = 0;
				stripb(teamtab[i].secret);

				++i;
				if (i >= MAXTEAMS)
					break;
			}
			else {
				geshocst(0, "GE:ERR Bad Team Rcd - Ignored");
			}
		}
		fclose(mzfp);
	}
}

void FUNC update_team_tab(void)
{
	FILE *hdl;
	int i;

	hdl = fopen("mpogetea.dat", "wt");

	if (hdl != (FILE *)0) {
		for (i = 0; i < MAXTEAMS; ++i) {
			if (teamtab[i].teamcode != 0) {
				fprintf(hdl, "TEAM|%5ld|%-30s|%5d|%10u|%-10s|%-10s|\n",
					teamtab[i].teamcode,
					teamtab[i].teamname,
					teamtab[i].teamcount,
					teamtab[i].teamdeldate,
					teamtab[i].password,
					teamtab[i].secret);
			}
		}
		fclose(hdl);
	}
}

/**************************************************************************
** Planet economic processing                                            **
**************************************************************************/
#ifdef PHARLAP
void FUNC pplarti(void)
{
	plartia();
}
#else
int FUNC plarti(void)
{
	plartia();
	return 0;
}
#endif

void FUNC plartia(void)
{
	static int foundpl = TRUE;
	static long fpos = 0;
	static long tocks = 0;
	double ftocktime, ftockfact;
	int i, tic, multnum;
	int intkey = PLTYPE_PLNT;
	static unsigned int plown = 0;
	static unsigned int plnob = 0;
	static unsigned int plemt = 0;
	long numrecs;

#define SECSADAY 82800L		/* 23 hours, for breathing room */
#define MAXTIC 25		/* max planets to scan in a go without finding one to update */

	setbtv(gebb2);

	numrecs = cntrbtv();	/* how many total records? sectors, planets, wormholes */

	sprintf(gechrbuf, "%ld", numrecs);
	logthis(spr("plartia entered, numrecs %s, passnum %d, plantime %u", gechrbuf, passnum, plantime));

	++tocks;	/* how many times routine has run this passnum */

	if (passnum > planupd) {	/* we've run all the day's updates */
		agebtv(&planet, &intkey, 2);
		planet.plantimesave = plantime;
		planet.timestamp = ((unsigned long)game_day << 4) | passnum;
		gesdb(GEUPDATE, (PKEY *)&planet, (GALSECT *)&planet);
		geshocst(1, spr("GE:INF:all planets updated for day %d", game_day));
		return;
	}

	if (passnum == 0) {	/* fresh boot */
		if (!agebtv(&planet, &intkey, 2)) {
			plantime = 10;
			logthis(spr("no planet records, wait %u seconds", plantime));
		}
		else {
			if (planet.xsect == 0 && planet.ysect == 0 && planet.plnum == 1) {
				/* Zygor record carries the saved scheduler state across restarts */
				logthis(spr("Checking Zygor timestamp, game day %d", game_day));

#ifdef FASTPLANET
				plantime = 3;
#else
				/* if no plantime saved, how fast do we want to start out? */
				if (planet.plantimesave <= 3) {
					if (numrecs < 50)
						numrecs = 50;
					else if (numrecs > 5000)
						numrecs = 5000;
					plantime = (unsigned int)((3997L * (5000L - numrecs)) / 4950L + 3);
					plantime /= planupd;
					logthis(spr("estimated plantime set to %u", plantime));
				}
				else
					plantime = planet.plantimesave;
#endif
				if (plantime < 3)
					plantime = 3;

				if ((planet.timestamp >> 4) == game_day) {	/* game has already been up today */
					passnum = (int)(planet.timestamp & 0xF);
					logthis(spr("resuming from planupd %d", planupd));
				}
				else
					passnum = 1;	/* different day than last time */
			}
			else
				catastro("GE:ERR:First planet record is not Zygor");
		}
	}
	else {
		if (foundpl == TRUE) {
			if (fpos == 0) {
				agebtv(&planet, &intkey, 2);	/* acquire first planet record */
				fpos = absbtv();		/* save position for next time through */
			}
			else
				gabbtv(&planet, fpos, 2);	/* get the planet we found last time */

			sprintf(gechrbuf, "%lu", fpos);

			/* foundpl/fpos point at the planet selected on the prior search pass */
			if (planet.xsect == 0 && planet.ysect == 0 && planet.plnum == 1) {
				logthis(spr("updating Zygor (%s)", gechrbuf));
				plptr = &planet;
				update_plan_1();
				planet.plantimesave = plantime;
			}
			else if (planet.xsect == 0 && planet.ysect == 0 && planet.plnum == 2) {
				logthis(spr("updating T-Station (%s)", gechrbuf));
				plptr = &planet;
				update_plan_2();
			}
			else if (planet.xsect == 0 && planet.ysect == 0 && planet.plnum == 3) {
				logthis(spr("updating Enforcer Planet (%s)", gechrbuf));
				plptr = &planet;
				update_plan_3();
			}
			else {
				logthis(spr("updating Planet %s (%s, %s)...", gechrbuf, planet.name, planet.userid));
				/* updated in the last 7 days? catch up */
				if ((planet.timestamp >> 4) >= game_day - 7 && (planet.timestamp >> 4) < game_day) {
					multnum = ((game_day - (int)(planet.timestamp >> 4) - 1) * planupd)
						+ (planupd - (int)(planet.timestamp & 0xF)) + passnum;
				}
				else if ((planet.timestamp >> 4) == game_day) {
					/* updated today but missed a pass? unlikely, but whatev */
					multnum = passnum - (int)(planet.timestamp & 0xF);
				}
				else
					/* otherwise update once */
					multnum = 1;
				if (multnum < 0)
					multnum = 0;
				logthis(spr("...%d times", multnum));
				plptr = &planet;
				/* replay missed passes quietly, then send mail only for the live pass */
				for (i = 0; i < multnum - 1; ++i)
					multiply(FALSE);	/* don't send mail for multiples */
				if (multnum > 0)
					multiply(TRUE);
				logthis("calling checkspy");
				check_spy();
				logthis("back from checkspy");
				setbtv(gebb2);
				setmbk(gemb);
			}

			planet.timestamp = (((unsigned long)game_day) << 4) | passnum;
			gesdb(GEUPDATE, (PKEY *)&planet, (GALSECT *)&planet);
		}

		/* mbm was right, see GEREADME 02/04/90 */
		/* at this point, we either just finished a planet update or are coming into this routine fresh */
		/* either way, we need to set the key and cursor again for query next to work */
		gabbtv(&planet, fpos, 2);

		foundpl = FALSE;
		tic = 0;

		do {
			tic++;

			if (!qnxbtv() || (int)(gebb2->key[0]) != PLTYPE_PLNT) {	/* hit a wormhole or no wormholes somehow? passnum done */
				sprintf(gechrbuf, "%ld", tocks);
				logthis(spr("tocks %s. planets updated %u, empty %u, unowned %u",
					gechrbuf, plown, plemt, plnob));

				/* recalibrate the tick interval so remaining passes fit inside the remaining day */
				ftocktime = ((double)(tocks * plantime)) + 1.0;
				ftockfact = ((double)(SECSADAY / planupd) / ftocktime);
				ftocktime = ((double)plantime * ftockfact);

				if (ftocktime < 3.0) {
					geshocst(1, "GE:INF:plarti:recalb tic forced to 3");
					plantime = 3;
				}
				else
#ifdef FASTPLANET
				{
					plantime = 3;
				}
#else
				{
					plantime = (int)ftocktime;
				}
#endif
				++passnum;
				foundpl = TRUE;
				fpos = 0;
				tocks = 0;
				plown = 0;
				plnob = 0;
				plemt = 0;
				break;
			}

			gcrbtv(&planet, 2);
			if (planet.userid[0] != 0 && ((planet.items[I_MEN].qty > 0 || planet.items[I_TROOPS].qty > 0)
				|| (planet.xsect == 0 && planet.ysect == 0))) {	/* owned and populated or in neut */
				fpos = absbtv();
				foundpl = TRUE;
				sprintf(gechrbuf, "%lu", fpos);
				++plown;
				logthis(spr("plartia: found next owned planet at %s", gechrbuf));
				break;
			}
			else {
				/* remember where the search left off even when this record needs no update */
				fpos = absbtv();
				sprintf(gechrbuf, "%lu", fpos);
				if (planet.userid[0] != 0)
					++plemt;
				else
					++plnob;
			}
			} while (tic < MAXTIC);
	}

	if (foundpl == FALSE)
		logthis("plartia: no planet this tock");

#ifdef PHARLAP
	rtkick(plantime, pplarti);
#else
	rtkick(plantime, plarti);
#endif
}

/**************************************************************************
** Real time kick routine                                                **
**************************************************************************/

#ifdef PHARLAP
void FUNC pwarrti(void)
{
	warrtia();
}
#else
int FUNC warrti(void)
{
	warrtia();
	return 0;
}
#endif

void FUNC warrtia(void)
{
	int zothusn;		/* general purpose other-user channel number */
	WARSHP *wptr;
	int cntr;

	logthis("TICK:Warrtia entered");

	cntr = 0;

	/* resolve global mine timers and detonations before per-ship processing */
	checkmines();		/* check for mines */
	/* stagger decoy expiry work so only one fifth of decoys are checked each tick */
	decpass = (decpass + 1) % 5;

	for (zothusn = 0; zothusn < nships; ++zothusn) {
		wptr = warshpoff(zothusn);
		if (ingegame(zothusn)) {
			logthis(spr("Chk Shp Stat %s", wptr->userid));
			setbtv(gebb1);
			setmbk(gemb);
			if (wptr->status == GESTAT_USER)
				++cntr;
			/* run each live ship through repair, lock, combat, and damage processing */
			repairship(wptr, zothusn);
			validate_lock(wptr, zothusn);
			if (wptr->damage < 100.0) {
				/* only ships that are still alive recharge and update shield/cloak state */
				shieldstat(wptr, zothusn);
				cloakstat(wptr, zothusn);
				recharge(wptr);
			}
			checktm(wptr, zothusn);		/* check torps, missl, and decoys */
			fireion(wptr, zothusn);
			checkdam(wptr, zothusn);
		}
	}
	/* keep the global in-game user count in sync with this tick's live user scan */
	numwar = cntr;
#ifdef PHARLAP
	rtkick(TICKTIME, pwarrti);
#else
	rtkick(TICKTIME, warrti);
#endif
}

/**************************************************************************
** Real time kick routine for all automatons                             **
**************************************************************************/

#ifdef PHARLAP
void FUNC pautorti(void)
{
	autortia();
}
#else
int FUNC autorti(void)
{
	autortia();
	return 0;
}
#endif

void FUNC autortia(void)
{
	int zothusn;		/* general purpose other-user channel number */
	WARSHP *wptr;

	static int ticktock1 = 0;
	static int ticktock2 = 0;
	int count, class, clscnt, i;

	logthis("TICK:autorti entered");

	setmbk(gemb);
	setbtv(gebb1);

	/* 12/19/91 spread out disk I/O over more time */

	if (ticktock1 == 0)
		ticktock1 = nterms;

	/* only attempt one vacant non-user slot every 30 ticks */
	++ticktock2;

	logthis(spr("ticktock1 = %d -- ticktock2 = %d", ticktock1, ticktock2));

	if (ticktock2 >= 30 && ticktock1 < nships) {
		logthis("ticktock2 >=30 and ticktock1 < nships");
		wptr = warshpoff(ticktock1);
		zothusn = ticktock1;

		if (wptr->status == GESTAT_AVAIL) {
			/* map this non-user slot back to its configured automaton class range */
			clscnt = ticktock1 - nterms;
			class = -1;
			logthis("Chan Stat = GESTAT_AVAIL");
			for (i = 0; i < tot_classes; ++i) {
				if (shipclass[i].max_type == CLASSTYPE_CYBORG ||
					shipclass[i].max_type == CLASSTYPE_DROID) {
					/* is this slot within class i */
					if (clscnt < shipclass[i].tot_to_create) {
						class = i;
						break;
					}
					/* no... check next class */
					clscnt -= shipclass[i].tot_to_create;
				}
			}

			logthis(spr("picked class - %d", class));

			/* initialize the non-user ship areas */
			if (class > -1 && shipclass[class].init_func != NULL) {
				logthis(spr("Calling init_func 4 cls %d", class));
				logthis(spr("   Name: %s", shipclass[i].typename));

				(*(shipclass[class].init_func))(wptr, zothusn, class);
			}
		}

		if (++ticktock1 >= nships)
			ticktock1 = nterms;
		ticktock2 = 0;
	}

	if (cybhaltflg <= 0) {
		cybhaltflg = 0;

		/* run the per-tick behavior callback for every live automaton */
		for (count = nterms; count < nships; ++count) {
			wptr = warshpoff(count);
			zothusn = count;

			if (wptr->status == GESTAT_AUTO) {
				if (!VALID_AUTOCLASS(wptr->shpclass)) {
					geshocst(0, spr("GE:ERR:BADAUTOCLS slot=%d cls=%d stat=%u uid=%s",
						count, wptr->shpclass, wptr->status, wptr->userid));
					wptr->status = GESTAT_AVAIL;
					continue;
				}

				if (wptr->tick == 0) {
					logthis(spr("Calling tick_func 4 usn %d", zothusn));
					logthis(spr("  Class %d", wptr->shpclass));

					if (shipclass[wptr->shpclass].tick_func != NULL)
						(*(shipclass[wptr->shpclass].tick_func))(wptr, zothusn);
					else {
						geshocst(0, spr("GE:ERR:NULLAUTOTICK slot=%d cls=%d uid=%s",
							count, wptr->shpclass, wptr->userid));
						wptr->status = GESTAT_AVAIL;
					}
				}
				else {
					--wptr->tick;
				}
			}
		}
	}
	else {
		--cybhaltflg;
	}

	logthis("Exiting AUTORTI");

#ifdef PHARLAP
	rtkick(1, pautorti);
#else
	rtkick(1, autorti);
#endif
}

/**************************************************************************
** Real time kick routine #2                                             **
**************************************************************************/

#ifdef PHARLAP
void FUNC pwarrti2(void)
{
	warrti2a();
}

#else
int FUNC warrti2(void)
{
	warrti2a();
	return 0;
}
#endif

void FUNC warrti2a(void)
{
	int zothusn;		/* general purpose other-user channel number */
	WARSHP *wptr;

	static int clicker = 0;

	logthis("TICK:PWarrti2 entered");

	if (beacontimer > 0)
		--beacontimer;

	/* advance delayed entry-message deliveries on the movement tick */
	tick_entrymsg();

	zothusn = clicker;

	while (zothusn < nships) {
		if (ingegame(zothusn)) {
			wptr = warshpoff(zothusn);
			setbtv(gebb1);
			setmbk(gemb);
			rotateship(wptr, zothusn);
			accel(wptr, zothusn);
			moveship(wptr, zothusn);
			destruct(wptr, zothusn);
		}
		zothusn += 3;
	}

	/* spread ship movement over three sub-passes to smooth out per-tick work */
	clicker = (clicker + 1) % 3;

#ifdef PHARLAP
	rtkick(TICKTIME2, pwarrti2);
#else
	rtkick(TICKTIME2, warrti2);
#endif
}

/**************************************************************************
** Real time kick routine #3                                             **
**************************************************************************/

#ifdef PHARLAP
void FUNC pwarrti3(void)
{
	warrti3a();
}

#else
int FUNC warrti3(void)
{
	warrti3a();
	return 0;
}
#endif

void FUNC warrti3a(void)
{
	COORD tmpcoord;

	logthis("TICK:Warrti3a entered");

	setmbk(gemb);
	prfmsg(ZAPHIM2);

	/* broadcast the periodic neutral-zone warning beacon */
	tmpcoord.xcoord = 0.0;
	tmpcoord.ycoord = 0.0;
	outsect(FLT_BEACON, &tmpcoord, 99);
#ifdef PHARLAP
	rtkick(120, pwarrti3);
#else
	rtkick(120, warrti3);
#endif
}


/*********************/
/* U T I L I T I E S */
/*********************/


/**************************************************************************
** OUTPRF special, apply filters, don't send to NPCs                     **
**************************************************************************/

void FUNC outprfge(int class, int shpno)
{
	byte msgfilter;

	if (shpno >= 0 && shpno < nterms) {
		if (user[shpno].state == gestt) {
			msgfilter = warusroff(shpno)->options[MSG_FILTER];
			switch (class) {
			case FLT_NONE:
				outprf(shpno);
				return;
			case FLT_CYB_ALL:
				if ((msgfilter & MSGF_CYBS_MASK) == 0x00) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_CYB_BAT:
				if ((msgfilter & MSGF_CYBS_MASK) == 0x00 ||
					(msgfilter & MSGF_CYBS_MASK) == 0x01) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_CYB_APP:
				if ((msgfilter & MSGF_CYBS_MASK) != 0x03) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_DISTRESS:
				if (!(msgfilter & MSGF_DISTRESS)) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_BEACON:
				if (!(msgfilter & MSGF_BEACON)) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_HAIL:
				if (!(msgfilter & MSGF_HAIL)) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_ENTRY:
				if ((msgfilter & MSGF_ENTRY_MASK) != 0x40) {
					outprf(shpno);
					return;
				}
				break;
			case FLT_SHIP:
				if (!(msgfilter & MSGF_SHIP)) {
					outprf(shpno);
					return;
				}
				break;
			}
		}
	}
	clrprf();
}


/**************************************************************************
** Send message to all ships in this sector                              **
**************************************************************************/

void FUNC outsect(int filter, COORD *coordptr, unsigned exclude)
{
	int zothusn;
	double ddist;
	byte src_neb, oth_neb;
	WARSHP *wptr;

	src_neb = (byte)innebula(coord1(coordptr->xcoord), coord1(coordptr->ycoord));

	for (zothusn = 0; zothusn < nterms; ++zothusn) {
		if (ingegame(zothusn) && zothusn != exclude) {
			wptr = warshpoff(zothusn);
			if (samesect(&(wptr->coord), coordptr)) {
				oth_neb = (byte)innebula(coord1(wptr->coord.xcoord), coord1(wptr->coord.ycoord));
				/* nebula messages only propagate when both endpoints are inside the same local nebula range */
				if (src_neb || oth_neb) {
					ddist = cdistance(coordptr, &wptr->coord);
					ddist *= 10000;
					if (!(src_neb && oth_neb && ddist < (double)NEBRNG))
						continue;
				}
				outprfge(filter, zothusn);
			}
		}
	}
	clrprf();
}

/**************************************************************************
** Send message to all ships in scanning range of this ship              **
**************************************************************************/

void FUNC outrange(int filter, COORD *coordptr)
{
	double ddist;
	int zothusn;
	byte src_neb, oth_neb;
	WARSHP *wptr;

	src_neb = (byte)innebula(coord1(coordptr->xcoord), coord1(coordptr->ycoord));

	for (zothusn = 0; zothusn < nships; ++zothusn) {
		wptr = warshpoff(zothusn);
		if (ingegame(zothusn) && shipclass[wptr->shpclass].max_type == CLASSTYPE_USER) {
			ddist = cdistance(coordptr, &wptr->coord);
			ddist *= 10000;
			oth_neb = (byte)innebula(coord1(wptr->coord.xcoord), coord1(wptr->coord.ycoord));
			if ((src_neb || oth_neb) && !(src_neb && oth_neb && ddist < (double)NEBRNG))
				continue;
			if (ddist > 1 && ddist < (double)ship_scanrange(wptr))
				outprfge(filter, zothusn);
		}
	}
	clrprf();
}


/**************************************************************************
** Check is user is in the game                                          **
**   Automatons are always in the game                                   **
**************************************************************************/

int FUNC ingegame(int shpno)
{
	if (shpno >= nships || shpno < 0)
		return FALSE;

	if (shpno < nterms)
		if (user[shpno].state == gestt && user[shpno].substt >= FIGHTSUB)
			return TRUE;

	if (shpno >= nterms && warshpoff(shpno)->status == GESTAT_AUTO)
		return TRUE;

	return FALSE;
}



/**************************************************************************
** SHOCST Replacement                                                    **
**************************************************************************/

void FUNC geshocst(int opt, char *str)
{
	char tmpbuf[40];

	/* wrap shocst() so GE can honor showopt and still preserve usrnum */
	if (opt == 0) {
		/* opt 0 always displays, regardless of showopt */
		tmp_usrnum = usrnum;
		usrnum = -1;
#ifdef PHARLAP
		strncpy(tmpbuf, str, 32);
		tmpbuf[31] = 0;
		shocst(tmpbuf, str);
#else
		shocst(0, str);
#endif
		usrnum = tmp_usrnum;
	}
	else if (opt <= showopt) {
		/* other messages display only when their level is within showopt */
		tmp_usrnum = usrnum;
		usrnum = -1;
#ifdef PHARLAP
		strncpy(tmpbuf, str, 32);
		tmpbuf[31] = 0;
		shocst(tmpbuf, str);
#else
		shocst(0, str);
#endif
		usrnum = tmp_usrnum;
	}
	/* optional console logging mirrors what was sent through shocst */
	if (logflag)
		logthis(spr("CON: %s", str));
}

/*****************************************************************************
** The following mnu functions are response handlers for input from the     **
** player while in a particular state. Each of these then typically results **
** in additional menus/messages being displayed to the player and the       **
** players state (substt) modified.                                         **
*****************************************************************************/

/* player selected GE from the main menu */

int FUNC mnu_main(void)
{
	prfmsg(INTRO, VERSION);
	disp_main_menu();
	outprfge(FLT_NONE, usrnum);
	usrptr->substt = 1;
	return 1;
}

/* player selected something from the main menu */

int FUNC mnu_main_ans(void)
{
	if (margc == 0 || margc > 1) {
		prfmsg(REPRMT);
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	else if (margc == 1) {
		if (sameas(input, "P")) {
#ifdef PHARLAP
			if (!hasmkey(PLAYKEY)) {
				prfmsg(FORPLAY);
				outprfge(FLT_NONE, usrnum);
				prfmsg(REPRMT);
				outprfge(FLT_NONE, usrnum);
			}
#else
			if (usrptr->class < PAYING && gefreebies == 0) {
				prfmsg(FORLIVE);
				outprfge(FLT_NONE, usrnum);
				prfmsg(REPRMT);
				outprfge(FLT_NONE, usrnum);
			}
#endif
			else {
				if (numwar < gemaxplrs) {
					lookupshp();
					clrprf();
					return 1;
				}
				else {
					prfmsg(NOSHPS);
					outprfge(FLT_NONE, usrnum);
					prfmsg(REPRMT);
					outprfge(FLT_NONE, usrnum);
					return 1;
				}
			}
		}
		else if (sameas(input, "G")) {
			prfmsg(EXPLAIN);
			outprfge(FLT_NONE, usrnum);
			prfmsg(REPRMT);
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		else if (sameas(input, "R")) {
			cmd_geroster();
			prfmsg(REPRMT);
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		else if (sameas(input, "M")) {
			disp_menu_d();
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		else if (sameas(input, "I")) {
			prfmsg(COINFO);
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		else if (sameas(input, "?")) {
			disp_main_menu();
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		else if (sameas(input, "X")) {
			prfmsg(EXIWAR);
			outprfge(FLT_NONE, usrnum);
			btupmt(usrnum, 0);
			return 0;
		}
		else if (optmenu) {
			input[0] = (char)toupper((unsigned char)input[0]);
			if (input[0] == optchr) {
				optdisp();
				return 1;
			}
		}
		prfmsg(REPRMT);
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	return 1;
}

/* player is playing the game and entered a command */

int FUNC mnu_fightsub(void)
{
	if (sameas(input, "x")) {
		/* only allow exit when not fighting or in neutral with no incoming projectiles */
		if (warsptr->cantexit == 0 || (neutral(&warsptr->coord) && chkitm(usrnum))) {
			warsptr->cantexit = 0;
			cleartm(usrnum);
			gepdb(GEUPDATE, warsptr->userid, warsptr->shipno, warsptr);
			geudb(GEUPDATE, waruptr->userid, waruptr);
			/* return the player to the GE main menu and mark the ship slot free */
			disp_main_menu();
			outprfge(FLT_NONE, usrnum);
			exit_entrymsg(usrnum);
			numwar = 0;
			usrptr->substt = 1;
			btupmt(usrnum, 0);
			warsptr->status = GESTAT_AVAIL;
		}
		else {
			prfmsg(CANTEXT);
			outprfge(FLT_NONE, usrnum);
		}
	}
	else {
		/* any non-empty input in fightsub is treated as an in-flight command */
		if (margc > 0)
			gwar();
		else {
			prfmsg(FORHELP);
			outprfge(FLT_NONE, usrnum);
		}
	}
	return 1;
}

/* player has asked to admin a planet they do not own, and has been prompted
   to respond with yes or no to the question "do you wish to claim this
   planet". */

int FUNC mnu_admenu1(void)
{
	int i;

	if (margc > 0) {
		if (sameto("y", margv[0])) {
			if (!load_admin_planet())
				return 1;

			strncpy(plptr->userid, warsptr->userid, UIDSIZ);
			plptr->userid[UIDSIZ - 1] = 0;
			++waruptr->planets;
			geudb(GEUPDATE, waruptr->userid, waruptr);

			if (strlen(plptr->name) == 0) {
				for (i = 0; i < NUMITEMS; ++i) {
					plptr->items[i].rate = 0;
					plptr->items[i].markup2a = baseprice[i];
				}

				plptr->items[I_MEN].rate = 50;
				plptr->items[I_FOOD].rate = 50;
			}

			setsect(warsptr); /* build PKEY */
			gesdb(GEUPDATE, &pkey, &sector);

			pkey.plnum = plnum;
			gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);

			prfmsg(ADMENU1A);
			outprfge(FLT_NONE, usrnum);
			usrptr->substt = ADMENU1A;
		}
		else if (sameto("n", margv[0])) {
			prfmsg(ADMIN3);
			outprfge(FLT_NONE, usrnum);
			usrptr->substt = FIGHTSUB;
		}
	}
	else {
		prfmsg(ADMENU1);
		outprfge(FLT_NONE, usrnum);
	}
	return 1;
}

/* player was asked to enter the name of the new planet and should have
   responded with a string. */

static int mnu_admenu1a(void)
{
	if (margc > 0) {
		if (!load_admin_planet())
			return 1;
		rstrin();

		*margv[0] = (char)toupper((unsigned char)*margv[0]);
		strncpy(plptr->name, margv[0], 19);
		plptr->name[19] = 0;

		setsect(warsptr); /* build PKEY */
		pkey.plnum = plnum;
		gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);

		if (warsptr->shipname[0] == 0)
			prfmsg(ADMNU1BO, plnum, plptr->name, warsptr->userid);
		else
			prfmsg(ADMENU1B, plnum, plptr->name, warsptr->userid, warsptr->shipname);

		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMENU2;
	}
	else {
		prfmsg(ADMENU1A);
		outprfge(FLT_NONE, usrnum);
	}
	return 1;
}

/* player was displayed the admin main menu and should have selected an
   item from it. */

int FUNC mnu_admenu2(void)
{
	int i;

	if (margc > 0) {
		if (*margv[0] >= '1' && *margv[0] <= '7') {
			if (!load_admin_planet())
				return 1;
			fixplanetteam();

			switch (*margv[0]) {
			case '1':
				prfmsg(ADMIN01, plptr->name);
				prfmsg(DASHESL);
				prfmsg(ADMIN02);
				for (i = 0; i < NUMITEMS; ++i) {
					sprintf(gechrbuf, "%-11s %5u %10lu %5u %5u %5c %8lu",
						item_name[i],
						plptr->items[i].rate,
						plptr->items[i].qty,
						plptr->items[i].markup2a,
						plptr->items[i].reserve,
						plptr->items[i].sell,
						plptr->items[i].sold2a);
					prf("%s\r", gechrbuf);
				}
				prfmsg(DASHESL);
				sprintf(gechrbuf, "%lu", plptr->cash);
				prfmsg(ADMIN04, gechrbuf);
				sprintf(gechrbuf, "%lu", plptr->tax);
				prfmsg(ADMIN04A, gechrbuf);
				prfmsg(ADMIN04B, plptr->taxrate);
				prfmsg(ADMIN06, plptr->password);
				prfmsg(DASHESL);
				prfmsg(PRESSKEY);
				usrptr->substt = ADMENU2;
				break;

			case '2':
				prfmsg(ADMENU2B);
				usrptr->substt = ADMENU2B;
				break;

			case '3':
				prfmsg(ADMENU2E);
				usrptr->substt = ADMENU2E;
				break;

			case '4':
				prfmsg(ADMENU2G);
				usrptr->substt = ADMENU1A;
				break;

			case '5':
				prfmsg(ADMENU2H);
				usrptr->substt = ADMENU2H;
				break;

			case '6':
				prfmsg(ADMENU2I);
				usrptr->substt = ADMENU2I;
				break;

			case '7':
				prfmsg(ADMENU2J);
				usrptr->substt = ADMENU2J;
				break;

			}

			outprfge(FLT_NONE, usrnum);
		}
		else if (sameas(input, "x")) {
			prfmsg(ADMIN3);
			outprfge(FLT_NONE, usrnum);
			usrptr->substt = FIGHTSUB;
		}
		else {
			prfmsg(ADMENU2);
			outprfge(FLT_NONE, usrnum);
		}
	}
	else {
		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
	}
	return 1;
}

/* player asked to transfer cash from planet and has was prompted to enter
   the amount to transfer */

int FUNC mnu_admenu2b(void)
{
	long val;
	unsigned long amt;

	if (!load_admin_planet())
		return 1;
	if (margc != 1) {
		prfmsg(ADMENU2B);
		outprfge(FLT_NONE, usrnum);
		return 1;
	}
	val = atol(margv[0]);
	if (val < 0L) {
		prfmsg(ADMENU2D);
		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMENU2;
		return 1;
	}
	amt = (unsigned long)val;

	if (amt <= plptr->tax) {
		if (waruptr->cash > ULCAP - amt) {
			sprintf(gechrbuf, "%lu", ULCAP);
			prfmsg(TOORICH, gechrbuf);
			outprfge(FLT_NONE, usrnum);
		}
		else {
			sprintf(gechrbuf, "%lu", amt);
			prfmsg(ADMENU2C, gechrbuf);
			outprfge(FLT_NONE, usrnum);
			waruptr->cash += amt;
			plptr->tax -= amt;
			setsect(warsptr); /* build PKEY */
			pkey.plnum = plnum;
			gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);
		}
	}
	else {
		prfmsg(ADMENU2D);
	}
	prfmsg(ADMENU2);
	outprfge(FLT_NONE, usrnum);
	usrptr->substt = ADMENU2;
	return 1;
}

/* player asked to modify the parameters on an item, and was asked to enter
   the name of the selected item */

int FUNC mnu_admenu2e(void)
{
	int i;

	if (!load_admin_planet())
		return 1;
	if (margc == 0) {
		prfmsg(ADMENU2E);
		outprfge(FLT_NONE, usrnum);
		return 1;
	}

	for (i = 0; i < NUMITEMS; ++i) { /* skip notused */
		if (sameto(kwrd[i], margv[0])) {
			warsptr->titem = i;
			prfmsg(ADMEN2F1, item_name[warsptr->titem], plptr->items[warsptr->titem].rate);
			outprfge(FLT_NONE, usrnum);
			usrptr->substt = ADMEN2F1;
			return 1;
		}
	}
	usrptr->substt = ADMENU2;
	prfmsg(ADMENU2);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player selected an item, was prompted to select the percent of effort
   on his selected item and should have responded with a percent. */

int FUNC mnu_admenu2f1(void)
{
	int amt;

	if (!load_admin_planet())
		return 1;

	if (margc == 0) {
		titems[usrnum].rate = plptr->items[warsptr->titem].rate;
		prfmsg(ADMEN2F2, item_name[warsptr->titem], plptr->items[warsptr->titem].markup2a);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMEN2F2;
		return 1;
	}
	amt = atoi(margv[0]);
	if (margc == 1 && amt >= 0 && amt <= 100) {
		titems[usrnum].rate = amt;
		prfmsg(ADMEN2F2, item_name[warsptr->titem], plptr->items[warsptr->titem].markup2a);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMEN2F2;
		return 1;
	}
	prfmsg(ADMEN2F1, item_name[warsptr->titem], plptr->items[warsptr->titem].rate);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player was asked how much to charge for his selected item */

int FUNC mnu_admenu2f2(void)
{
	int amt;

	if (!load_admin_planet())
		return 1;

	if (margc == 0) {
		titems[usrnum].markup2a = plptr->items[warsptr->titem].markup2a;
		prfmsg(ADMEN2F3, item_name[warsptr->titem], plptr->items[warsptr->titem].sell);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMEN2F3;
		return 1;
	}
	amt = atoi(margv[0]);
	if (margc == 1 && amt >= 0 && amt <= 32000) {
		titems[usrnum].markup2a = amt;
		prfmsg(ADMEN2F3, item_name[warsptr->titem], plptr->items[warsptr->titem].sell);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMEN2F3;
		return 1;
	}

	prfmsg(ADMEN2F2, item_name[warsptr->titem], plptr->items[warsptr->titem].markup2a);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player was asked if he wished to sell this item to other ships */

int FUNC mnu_admenu2f3(void)
{
	if (!load_admin_planet())
		return 1;

	if (margc == 0) {
		titems[usrnum].sell = plptr->items[warsptr->titem].sell;
		prfmsg(ADMEN2F4, item_name[warsptr->titem], plptr->items[warsptr->titem].reserve);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMEN2F4;
		return 1;
	}
	if (margc == 1 && (sameto("y", margv[0]) || sameto("n", margv[0]))) {
		titems[usrnum].sell = (char)toupper((unsigned char)*margv[0]);
		prfmsg(ADMEN2F4, item_name[warsptr->titem], plptr->items[warsptr->titem].reserve);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMEN2F4;
		return 1;
	}
	prfmsg(ADMEN2F3, item_name[warsptr->titem], plptr->items[warsptr->titem].sell);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player was asked hom much to reserve for stock piling */

int FUNC mnu_admenu2f4(void)
{
	int amt;

	if (!load_admin_planet())
		return 1;

	if (margc == 0) {
		titems[usrnum].reserve = plptr->items[warsptr->titem].reserve;
		update_items();
		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMENU2;
		return 1;
	}
	amt = atoi(margv[0]);
	if (margc == 1 && amt >= 0 && amt <= 32000) {
		titems[usrnum].reserve = amt;
		update_items();
		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMENU2;
		return 1;
	}
	prfmsg(ADMEN2F4, item_name[warsptr->titem], plptr->items[warsptr->titem].reserve);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player was asked how much to set taxes at */

int FUNC mnu_admenu2h(void)
{
	int amt;

	if (margc == 1) {
		amt = atoi(margv[0]);
		if (amt < 0 || amt > 100) {
			prfmsg(ADMENU2H);
			outprfge(FLT_NONE, usrnum);
			return 1;
		}
		if (!load_admin_planet())
			return 1;

		plptr->taxrate = amt;

		setsect(warsptr); /* build PKEY */
		pkey.plnum = plnum;
		gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);

		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMENU2;
		return 1;
	}
	prfmsg(ADMENU2H);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player was asked to specify the trade password for his planet */

int FUNC mnu_admenu2i(void)
{
	int i;

	if (margc == 1) {
		if (!load_admin_planet())
			return 1;

		strncpy(plptr->password, margv[0], sizeof(plptr->password) - 1);
		plptr->password[sizeof(plptr->password) - 1] = 0;

		if (sameas(plptr->password, "none")) {
			prfmsg(ADMEN2I2);
			plptr->teamcode = 0;
		}
		else if (sameas(plptr->password, "team")) {
			if (waruptr->teamcode > 0) {
				for (i = 0; i < MAXTEAMS; ++i) {
					if (teamtab[i].teamcode == waruptr->teamcode
						&& teamtab[i].teamname[0] != '@')
						break;
				}
				if (i < MAXTEAMS) {
					plptr->teamcode = waruptr->teamcode;
					prfmsg(ADMEN2I3);
				}
				else {
					plptr->teamcode = 0;
					plptr->password[0] = 0;
					prfmsg(ADMEN2I4);
				}
			}
			else {
				plptr->teamcode = 0;
				plptr->password[0] = 0;
				prfmsg(ADMEN2I4);
			}
		}
		else {
			prfmsg(ADMEN2I1, plptr->password);
			plptr->teamcode = 0;
		}

		setsect(warsptr); /* build PKEY */
		pkey.plnum = plnum;
		gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);

		prfmsg(ADMENU2);
		outprfge(FLT_NONE, usrnum);
		usrptr->substt = ADMENU2;
		return 1;
	}
	prfmsg(ADMENU2I);
	outprfge(FLT_NONE, usrnum);
	return 1;
}

/* player was asked to enter a beacon message should have
   responded with a string. */

int FUNC mnu_admenu2j(void)
{
	if (!load_admin_planet())
		return 1;

	if (margc == 0) {
		plptr->beacon[0] = 0;
		prfmsg(ADMEN2J1);
	}
	else {
		rstrin();
		strncpy(plptr->beacon, margv[0], BEACONMSGSZ);
		plptr->beacon[BEACONMSGSZ - 1] = 0;
		prfmsg(ADMEN2J2);
	}

	setsect(warsptr); /* build PKEY */
	pkey.plnum = plnum;
	gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);

	prfmsg(ADMENU2);
	outprfge(FLT_NONE, usrnum);
	usrptr->substt = ADMENU2;
	return 1;
}

/* player selected item 1 from the main menu */

int FUNC mnu_choosesh(void)
{
	selectship();
	return 1;
}

/* player selected read messages from main menu, was displayed the mail
   sub-menu, and was asked to select an option */

static int mnu_menug(void)
{
	if (margc > 0) {
		switch (tolower((unsigned char)*margv[0])) {

		case '1':

			if (mailread(usaptr->userid, MAIL_CLASS_DISTRESS)) {
				prfmsg(usrptr->substt = MENUG1);
				outprfge(FLT_NONE, usrnum);
			}
			else {
				disp_menu_d();
				outprfge(FLT_NONE, usrnum);
			}
			break;

		case '2':

			if (mailread(usaptr->userid, MAIL_CLASS_PRODRPT)) {
				prfmsg(usrptr->substt = MENUG2);
				outprfge(FLT_NONE, usrnum);
			}
			else {
				disp_menu_d();
				outprfge(FLT_NONE, usrnum);
			}
			break;

		case 'x':

			disp_main_menu();
			outprfge(FLT_NONE, usrnum);
			usrptr->substt = 1;
			break;

		default:

			disp_menu_d();
			outprfge(FLT_NONE, usrnum);
			break;
		}
	}
	else {
		disp_menu_d();
		outprfge(FLT_NONE, usrnum);
	}
	return 1;
}

/* player selected option 1 on the mail sub-menu and player was displayed the
   first message in the mail file and has been asked to press N for next or X
   to exit. */

int FUNC mnu_menug1(void)
{
	if (margc > 0) {
		switch (tolower((unsigned char)*margv[0])) {

		case 'n':

			if (mailread(usaptr->userid, MAIL_CLASS_DISTRESS)) {
				prfmsg(usrptr->substt = MENUG1);
				outprfge(FLT_NONE, usrnum);
			}
			else {
				disp_menu_d();
				outprfge(FLT_NONE, usrnum);
			}
			break;

		case 'x':

			disp_menu_d();
			outprfge(FLT_NONE, usrnum);
			break;

		default:

			prfmsg(MENUG1);
			outprfge(FLT_NONE, usrnum);
			break;
		}
	}
	else {
		prfmsg(MENUG1);
		outprfge(FLT_NONE, usrnum);
	}
	return 1;
}

/* player selected option 2 on the mail sub-menu and player was displayed the
   first message in the mail file and has been asked to press N for next or X
   to exit. */

int FUNC mnu_menug2(void)
{
	if (margc > 0) {
		switch (tolower((unsigned char)*margv[0])) {

		case 'n':

			if (mailread(usaptr->userid, MAIL_CLASS_PRODRPT)) {
				prfmsg(usrptr->substt = MENUG2);
				outprfge(FLT_NONE, usrnum);
			}
			else {
				disp_menu_d();
				outprfge(FLT_NONE, usrnum);
			}
			break;

		case 'x':

			disp_menu_d();
			outprfge(FLT_NONE, usrnum);
			break;

		default:

			prfmsg(MENUG2);
			outprfge(FLT_NONE, usrnum);
			break;
		}
	}
	else {
		prfmsg(MENUG2);
		outprfge(FLT_NONE, usrnum);
	}
	return 1;
}

/* re-displays the main menu */

void FUNC disp_main_menu(void)
{
	prfmsg(MENUA);

	if (mailscan(usaptr->userid, 0))
		prfmsg(MENUB2);
	else
		prfmsg(MENUB1);

	if (optmenu)
		prf("\r   %c ... %s", optchr, opttxt);

	prfmsg(MENUC);
}

/* re-displays the mail sub-menu */

void FUNC disp_menu_d(void)
{
	prfmsg(MENUD);
	if (mailscan(usaptr->userid, MAIL_CLASS_DISTRESS))
		prfmsg(MENUE2);
	else
		prfmsg(MENUE1);

	if (mailscan(usaptr->userid, MAIL_CLASS_PRODRPT))
		prfmsg(MENUF2);
	else
		prfmsg(MENUF1);
	prfmsg(usrptr->substt = MENUG);
}

void FUNC update_items(void)
{
	int i, pcnt = 0;

	if (!load_admin_planet())
		return;

	for (i = 0; i < NUMITEMS; ++i)
		if (i != warsptr->titem)
			pcnt += plptr->items[i].rate;

	if ((titems[usrnum].rate + pcnt) > 100) {
		titems[usrnum].rate = 100 - pcnt;
		if (titems[usrnum].rate > 100)
			titems[usrnum].rate = 0;

		prfmsg(ADMEN2FA, item_name[warsptr->titem], titems[usrnum].rate);
		outprfge(FLT_NONE, usrnum);
	}
	i = (titems[usrnum].rate + pcnt);
	if (i < 100) {
		i = 100 - i;
		prfmsg(ADMEN2FB, i);
		outprfge(FLT_NONE, usrnum);
	}
	plptr->items[warsptr->titem].rate = titems[usrnum].rate;
	plptr->items[warsptr->titem].sell = titems[usrnum].sell;
	plptr->items[warsptr->titem].reserve = titems[usrnum].reserve;
	plptr->items[warsptr->titem].markup2a = titems[usrnum].markup2a;
	setsect(warsptr); /* build PKEY */
	pkey.plnum = plnum;
	gesdb(GEUPDATE, (PKEY *)&pkey, (GALSECT *)&planet);

}

void FUNC optdisp(void)
{
	static FILE *hdl = (FILE *)0;

	/* open the optional text-menu file on first use and reuse the handle until EOF */
	if (hdl == (FILE *)0) {
		hdl = fopen("mpogemnu.txt", "rt");
		if (hdl == (FILE *)0)
			geshocst(0, "GE:ERR MPOGEMNU.TXT Open Failed");
		else
			logthis("optdisp: mpogemnu.txt opened");
	}

	if (hdl != (FILE *)0) {
		/* each user tracks their own current read offset in opttbl[] */
		if (fseek(hdl, opttbl[usrnum], 0) == 0) {
			if (fgets(gechrbuf, 85, hdl) != NULL) {
				logthis(gechrbuf);
				prf(gechrbuf);
				outprfge(FLT_NONE, usrnum);
			}
			else {
				/* hitting EOF rewinds this user back to the main menu on the next cycle */
				opttbl[usrnum] = 0;
				logthis("optdisp: hit eof - fpos set back to 0");
				usrptr->substt = OPTDISP2;
				btuinj(usrnum, CYCLE);
				fclose(hdl);		/* be kind rewind */
				hdl = (FILE *)0;
				return;
			}
			/* remember where this user should resume reading next time */
			opttbl[usrnum] = ftell(hdl);
			usrptr->substt = OPTDISP;
			btuinj(usrnum, CYCLE);
		}
		else {
			logthis(spr("optdisp: seek error fpos = %ld", opttbl[usrnum]));
			prf("\r*** Seek Err in text file ***\r");
			outprfge(FLT_NONE, usrnum);
		}
	}
	else {
		prf("*** FILE MISSING - Notify Sysop!! ***");
		outprfge(FLT_NONE, usrnum);
	}
}

void FUNC stshdlr(void)
{
	/* special-case the cycle-driven optional text-menu states, otherwise fall back to the default handler */
	if (status == CYCLE) {
		switch (usrptr->substt) {
		case OPTDISP:
			setmbk(gemb);
			/* only push another line when enough output buffer space is free */
			if (btuoba(usrnum) > (OUTSIZ / 2)) {
				optdisp();
			}
			else {
				btuinj(usrnum, CYCLE);
			}

			break;

		case OPTDISP2:

			/* EOF on the optional text file returns the user to the main menu */
			setmbk(gemb);
			usrptr->substt = 1;
			disp_main_menu();
			outprfge(FLT_NONE, usrnum);
			return;

		default:
			dfsthn();
			return;

		}
	}
	else {
		dfsthn();
	}
}
