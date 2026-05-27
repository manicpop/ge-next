
/***************************************************************************
 *                                                                         *
 *   GEMAIN.H                                                              *
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

#define TRUE 1
#define FALSE 0

/* if this is the trainer version this define needs to be made */
/* #define GETRAINER 1 */

/* if FASTPLANET defined updates happen every 3 seconds */
/* #define FASTPLANET 1 */

/* the DATA command is currently in disrepair and disabled */
/* #define DATACMD 1 */

/* workarounds for mbbsemu math functions */
/* hopefully not necessary much longer */
/* #define MBBSEMU 1 */

#ifdef GETRAINER
#define VERSION "ge-next 0.3-rc2 Trainer"
#define GEHELP  "MPOG2HLP.MCV"
#define GEMSG   "MPOG2MSG.MCV"
#else
#define VERSION "ge-next 0.3-rc2"
#define GEHELP  "MPOGEHLP.MCV"
#define GEMSG   "MPOGEMSG.MCV"
#endif

#ifdef GETRAINER
#include "mpog2msg.h"
#include "mpog2hlp.h"
#include "mpogeshp.h"
#else
#include "mpogemsg.h"
#include "mpogehlp.h"
#include "mpogeshp.h"
#endif

/***************************************************************************/
/* The following defines have been carefully chosen and refined to provide */
/* a well balanced and entertaining game play. Should they be modified,    */
/* do so with caution and consideration                                    */
/***************************************************************************/

#define NEUTRAL_X 0		/* neutral-zone x sector                   */
#define NEUTRAL_Y 0		/* neutral-zone y sector                   */

#define ACCENGAMT 120		/* Acceleration energy usage               */

#define PRELOAD 10		/* Phaser reload rate / tick               */
#define PMINFIRE 60		/* Minimum amount to fire phaser           */
#define PENGUSE 57		/* Energy used by phaser charging          */
#define PHABIAS 2		/* Degrees to bias "spread" to better aim  */

#define HPFIRAMT 5000		/* hyperphaser energy cost                 */
#define HPBEAMW 5		/* hyperphaser beam width                  */

#define ENGYMAX 65000U		/* Maximum energy one can have             */
#define ENGRECHG 2		/* Energy recharge amount / tick           */

#define SHENGUSE 100		/* Energy used every tick for shields      */
#define SHMINCHG 5		/* Minimum charge to be effective          */

#define SSMAX 10000		/* sector size in coordinate units         */

#define PI 3.1415927		/* pi                                      */
#define SLCAP 2147483647L	/* maximum value of signed long            */
#define ULCAP 4294967295UL	/* maximum value of unsigned long          */
#define FARSPEED 54321L		/* speed constant for cyb long pursuit     */
#define VEC_EPS 1e-9		/* epsilon for vector()                    */

#define GELOOKUP 1		/* database lookup by key                  */
#define GEADD 2			/* database insert                         */
#define GEDELETE 3		/* database delete                         */
#define GEUPDATE 4		/* database update                         */
#define GEGET 5			/* database fetch if not already loaded    */
#define GENEXT 6		/* database next-record iteration          */
#define GELOOKUPNAME 7		/* database lookup by alternate name key   */
#define GEGETNOW 8		/* database fetch unconditionally          */

#define MAXPLANETS 9		/* max planets/wormholes per sector        */

#define MAXX 30			/* scan map width                          */
#define MAXY 15			/* scan map height                         */

/* do not change these as they effect structure sizes */
#define MAXTORPS 3
#define MAXMISSL 3
#define MAXDECOY 10

#define DECOYTIME 15		/* times TICKTIME how long a decoy lives   */
#define TICKTIME 6		/* main ship-state tick interval in seconds */
#define TICKTIME2 1		/* movement tick interval in seconds       */
#define CYBTICKTIME 5		/* Cyborgs do stuff this fast              */

#define FIRETICKS 10		/* normal cantexit delay when in battle    */

#define LOCKGRACE 5		/* ticks to retain a hidden lock target    */
#define CYBGRACE 10		/* ticks to retain hidden cyb target       */

#define NUMITEMS 14		/* number of items below                   */

#define I_MEN 0
#define I_MISSILE 1
#define I_TORPEDO 2
#define I_IONCANNON 3
#define I_FLUXPOD 4
#define I_FOOD 5
#define I_FIGHTER 6
#define I_DECOYS 7
#define I_TROOPS 8
#define I_ZIPPERS 9
#define I_JAMMERS 10
#define I_MINE 11
#define I_GOLD 12
#define I_SPY 13

#define SHIELDUP 1		/* shields are up                          */
#define SHIELDDN 2		/* shields are down                        */
#define SHIELDDM 3		/* shields are damaged                     */

#define SHIELD_FACTOR 4		/* shieldhit() factor                      */

#define COUNTDOWN 20		/* self destruct countdown                 */

#define CYB_MINDAM 75		/* % of damage when cyb jams and mines     */

#define TELEDAM 17		/* damage when hitting galactic barrier    */

#define MINERANGE 10000		/* maximum mine range                      */

#define DESTRUCTRANGE 10000	/* maximum self-destruct explosion range   */

#define NEBRNG 1200		/* SE scan visibility range inside nebulas */

#define TOPSHIELD 19		/* highest shield class index              */

#define TOPPHASOR 19		/* highest phaser class index              */

#define SECTYPE_NORMAL 1	/* normal sector record                    */
#define PLTYPE_PLNT 2		/* planet record                           */
#define PLTYPE_WORM 3		/* wormhole record                         */

#define GESTAT_AVAIL 0		/* unused ship slot                        */
#define GESTAT_USER 1		/* live user-controlled ship               */
#define GESTAT_AUTO 2		/* live automaton ship                     */

#define CLASSTYPE_USER 1	/* user ship class                         */
#define CLASSTYPE_CYBORG 2	/* cyborg ship class                       */
#define CLASSTYPE_DROID 3	/* droid ship class                        */
#define CLASSTYPE_NONE 4	/* undefined/inactive class slot           */

#define VALID_SHPCLASS(c) ((c) >= 0 && (c) < tot_classes)
#define VALID_AUTOCLASS(c) (VALID_SHPCLASS(c) && \
	(shipclass[(c)].max_type == CLASSTYPE_CYBORG || \
	 shipclass[(c)].max_type == CLASSTYPE_DROID))

#define MAIL_CLASS_DISTRESS 1	/* distress-message mail class             */
#define MAIL_CLASS_UNUSED1 2
#define MAIL_CLASS_PRODRPT 3	/* production-report mail class            */
#define MAIL_CLASS_UNUSED2 4
#define MAIL_CLASS_UNUSED3 5

/* user options */
#define SCANOPTS 0		/* scan display mode option slot           */
#define UNUSED1 1		/* reserved user option slot               */
#define UNUSED2 2		/* reserved user option slot               */
#define MSG_FILTER 3		/* message filter option slot              */
#define UNUSED4 4		/* reserved user option slot               */
#define UNUSED5 5		/* reserved user option slot               */

#define	MAXTEAMS 50		/* max team records in memory              */

#define BEACONMSGSZ 75		/* max planet beacon message length        */

#define	GEMSGSIZ 784		/* max GE mail payload size                */
#define FIXEDMSGSIZ sizeof(struct message)+GEMSGSIZ	/* fixed stored mail record size */

#define HSTSIZ 57		/* borrowed MajorBBS message topic size    */
#define TPCSIZ 51		/* borrowed MajorBBS message aux/history size */

/* message filter types */
#define	FLT_CYB_ALL 0
#define	FLT_CYB_BAT 1
#define	FLT_CYB_APP 2
#define	FLT_DISTRESS 3
#define	FLT_BEACON 4
#define	FLT_HAIL 5
#define	FLT_ENTRY 6
#define	FLT_SHIP 7
#define	FLT_NONE 8

/* how many seconds to delay 'reduced' entry messages */
#define	ENTRYWAIT 30

/* message filter bit masks */
#define	MSGF_CYBS_MASK 0x03
#define	MSGF_DISTRESS 0x04
#define	MSGF_BEACON 0x08
#define	MSGF_HAIL 0x10
#define	MSGF_ENTRY_MASK 0x60
#define	MSGF_SHIP 0x80

/* Cybertron message types */
#define APPROACH 0
#define LOATTACK 1
#define HIATTACK 2
#define IGNORE 3
#define NEUTRAL 4
#define TAUNT 5
#define CYBTORP 6
#define FLEE 7
#define CYBBASEA 8
#define CYBBASEB 9

/* map types */
#define SECTOR 0
#define SECTORFULL 1
#define RANGE 2
#define RANGEFULL 3
#define RANGENAMES 4
#define RANGEEXTRA 5
#define LONG 6
#define RANGENOMAP 7
#define SECTORNOMAP 8

/* scan settigns */
#define SIMPLE 0		/* just the map                           */
#define FULL 1			/* map with ship details on right         */
#define FULLNAMES 2		/* FULL but also including ship names     */
#define FULLEXTRA 3		/* FULLNAMES with add'l ships under map   */
#define NOMAP 4			/* two column text only, no map           */

/* colors */
#define CLR_RED1	"\33[0;31m"
#define CLR_RED2	"\33[1;31m"
#define CLR_GREEN1	"\33[0;32m"
#define CLR_GREEN2	"\33[1;32m"
#define CLR_YELLOW1	"\33[0;33m"
#define CLR_YELLOW2	"\33[1;33m"
#define CLR_BLUE1	"\33[0;34m"
#define CLR_BLUE2	"\33[1;34m"
#define CLR_PURPLE1	"\33[0;35m"
#define CLR_PURPLE2	"\33[1;35m"
#define CLR_CYAN1	"\33[0;36m"
#define CLR_CYAN2	"\33[1;36m"
#define CLR_WHITE1	"\33[0;37m"
#define CLR_WHITE2	"\33[1;37m"

/* ship upgrades */
#define ENHSCAN 1
#define ARMOR 2
#define ACCELBST 4
#define SCANBST 8
#define ENHLOCK 16
#define DAMCTRL 32
#define TPONDER 64
#define NCORE 128

#define NUMUPGRADES 8

/* transponder upgrade settings */
#define TPONNORM 0
#define TPONLOW 1
#define TPONHIGH 2

/***************************************************************************/
/* End of Defines                                                          */
/***************************************************************************/

#ifndef byte
#define byte unsigned char
#endif

/*
 * Message structure from GME.H, Copyright (c) 1994-1995 GALACTICOMM, Inc.
 * The mail Btrieve record is sizeof(MESSAGE)+GEMSGSIZ, which is 1000 bytes
 * in the 16-bit packed layout. MAIL and MAILSTAT below are GE payload formats
 * stored inside MESSAGE.text, not separate 1000-byte Btrieve records.
 */
typedef struct message {
	long msgno;		/* stored message number                   */
/*4*/
	char userto[UIDSIZ];	/* personal recipient userid or "** ALL **"*/
/*34*/
	char from[UIDSIZ];	/* message originator                      */
/*64*/
	char to[UIDSIZ];	/* recipient userid or signature name; Btrieve keys 0 and 1 */
/*94*/
	char topic[TPCSIZ];	/* editable message topic                  */
/*145*/
	char auxtpc[HSTSIZ];	/* aux topic/history/routing text          */
/*202*/
	unsigned crdate;	/* creation date                           */
/*204*/
	unsigned crtime;	/* creation time of day                    */
/*206*/
	int nreply;		/* reply count / reused by GE as day stamp */
/*208*/
	int flags;		/* message/attachment flag bits            */
/*210*/
	char spare[5];		/* reserved expansion space                */
/*215*/
	char text[1];		/* start of trailing message-text buffer   */
/* text up to GEMSGSIZ (784) follows, record size is 1000 bytes */
} MESSAGE;

typedef struct _coord {
	double xcoord;		/* x coordinate                            */
	double ycoord;		/* y coordinate                            */
} COORD;

typedef struct _mine {
	byte channel;		/* owning ship/user slot                   */
	unsigned timer;		/* countdown timer                         */
	COORD coord;		/* mine location                           */
} MINE;

typedef struct _missile {
	byte channel;		/* target ship/user slot                   */
	unsigned distance;	/* distance to target                      */
	unsigned energy;	/* missile energy payload                  */
} MISSILE;

typedef struct _torpedo {
	byte channel;		/* target ship/user slot                   */
	unsigned distance;	/* distance to target                      */
} TORPEDO;

typedef struct warusr {
	char userid[UIDSIZ];		/* userid of player; Btrieve key 0         */
/*30*/
	unsigned long score;		/* player score; Btrieve key 1             */
/*34*/
	unsigned noships;		/* number of ships owned currently         */
/*36*/
	unsigned topshipno;		/* highest ship number ever assigned       */
/*38*/
	unsigned kills;			/* number of total kills                   */
/*40*/
	unsigned ukills;		/* just user kills                         */
/*42*/
	unsigned long cash;		/* cash on hand                            */
/*46*/
	unsigned long plscore;		/* score from planets only                 */
/*50*/
	unsigned long klscore;		/* score from kills only                   */
/*54*/
	unsigned long population;	/* total population                        */
/*58*/
	byte options[30];		/* per-user options                        */
/*88*/
	byte factions[8];		/* do NPCs like me                         */
/*96*/
	unsigned planets;		/* planets owned                           */
/*98*/
	char key_filler[36];		/* reserved; keeps teamcode at key offset  */
/*134*/
	unsigned long teamcode;		/* current team code; Btrieve key 2        */
/*138*/
	char filler[108];		/* pad WARUSR to the real 246-byte record  */
} WARUSR;

typedef struct warshp {
	char userid[UIDSIZ];		/* userid of player; Btrieve keys 0 and 1  */
/*30*/
	int shipno;			/* ship id number; Btrieve key 1           */
/*32*/
	char shipname[20];		/* ship name                               */
/*52*/
	int shpclass;			/* ship class                              */
/*54*/
	byte status;			/* status of this record (AVAIL/USER/AUTO) */
/*55*/
	COORD coord;			/* ship coordinates                        */
/*71*/
	double heading;			/* heading of this ship                    */
/*79*/
	double head2b;			/* pending heading                         */
/*87*/
	double speed;			/* speed                                   */
/*95*/
	double speed2b;			/* pending speed                           */
/*103*/
	int topspeed;			/* top speed w/ damage                     */
/*105*/
	int where;			/* impulse / hyper / orbit                 */
/*107*/
	double damage;			/* ship damage                             */
/*115*/
	double energy;			/* flux energy available                   */
/*123*/
	double phasr;			/* phaser energy                           */
/*131*/
	byte phasrtype;			/* phaser class                            */
/*132*/
	byte shieldtype;		/* shield class                            */
/*133*/
	byte shieldstat;		/* shield status                           */
/*134*/
	int shield;			/* shield charge                           */
/*136*/
	int cloak;			/* cloak status                            */
/*138*/
	int tactical;			/* tactical damage                         */
/*140*/
	int helm;			/* helm damage                             */
/*142*/
	int lastfired;			/* last to fire on this ship               */
/*144*/
	int lock;			/* ship this ship is locked onto           */
/*146*/
	byte track_grace;		/* lock/NPC target invisibility grace      */
/*147*/
	unsigned kills;			/* kills in this ship                      */
/*149*/
	unsigned ukills;		/* just user kills                         */
/*151*/
	TORPEDO ltorps[MAXTORPS];	/* torps tracking this ship                */
/*160*/
	MISSILE lmissl[MAXMISSL];	/* misls tracking this ship                */
/*175*/
	unsigned decout[MAXDECOY];	/* decoys deployed                         */
/*195*/
	byte torps_fired;		/* torpedoes fired this tick               */
/*196*/
	byte missl_fired;		/* missiles fired this tick                */
/*197*/
	unsigned long items[NUMITEMS];	/* all the items                           */
/*253*/
	unsigned titem;			/* colony temp item (User-only)            */
/*255*/
	byte upgrade;			/* installed ship upgrades                 */
/*256*/
	byte tponder;			/* transponder control (User-only)         */
/*257*/
	int degrees;			/* degree workspace                        */
/*259*/
	int percent;			/* percent workspace (phaser / engines)    */
/*261*/
	unsigned hostile;		/* just attacked a planet (User-only)      */
/*263*/
	unsigned cantexit;		/* can't exit battle timer                 */
/*265*/
	unsigned repair;		/* repairs underway (User-only)            */
/*267*/
	unsigned hypha;			/* hyper-phaser cooldown                   */
/*269*/
	byte torpcntl;			/* torpedo damage flag                     */
/*270*/
	byte mislcntl;			/* missile damage flag                     */
/*271*/
	byte destruct;			/* self destruct countdown (User-only)     */
/*272*/
	byte minesnear;			/* set when mines are near                 */
/*273*/
	unsigned overspeed;		/* counter for over max warp (User-only)   */
/*275*/
	int zipload;			/* zipper throttle/damage                  */
/*277*/
	int jamload;			/* jam throttle/damage                     */
/*279*/
	int decload;			/* decoy throttle/damage                   */
/*281*/
	int mineload;			/* mine throttle/damage                    */
/*283*/
	byte jam_sev;			/* jamming severity                        */
/*284*/
	byte jam_time;			/* time left in jamming                    */
/*285*/
	unsigned freq;			/* encoded-message frequency (User-only)   */
/*287*/
	unsigned npcstate;		/* auxiliary state (NPC-only)              */
/*289*/
	byte npcmsg;			/* message throttle/state (NPC-only)       */
/*290*/
	byte warpmsg;			/* over-warp warning throttle (User-only)  */
/*291*/
	byte cybmine;			/* target ship (NPC-only)                  */
/*292*/
	byte cybupdate;			/* save/update counter (NPC-only)          */
/*293*/
	byte tick;			/* ticker (NPC-only)                       */
/*294*/
	byte distress;			/* distress target/state (Droid-only)      */
/*295*/
	byte holdcourse;		/* hold-course timer/state (NPC-only)      */
/*296*/
	char filler[205];		/* pad WARSHP to the real 501-byte record  */
} WARSHP;

typedef struct item {
	unsigned long qty;		/* quantity on hand                        */
	unsigned rate;			/* rate of manufacture                     */
	char sell;			/* sell to allies?                         */
	unsigned reserve;		/* quantity to reserve                     */
	unsigned markup2a;		/* markup charged to allies                */
	unsigned long sold2a;		/* quantity sold to allies                 */
} ITEM;

typedef struct _plntcoord {
	int type;			/* object type                             */
	COORD coord;			/* location                                */
} PLNTCOORD;

typedef struct planetab {
	PLNTCOORD planets[MAXPLANETS];	/* visible planet/wormhole table           */
} PLANETAB;

typedef struct galsect {
	int xsect;			/* sector x coord; Btrieve key 0           */
/*2*/
	int ysect;			/* sector y coord; Btrieve key 0           */
/*4*/
	int plnum;			/* always 0; Btrieve key 0                 */
/*6*/
	int type;			/* type of sector; Btrieve key 2           */
/*8*/
	int numplan;			/* number of planetary objects             */
/*10*/
	PLNTCOORD ptab[MAXPLANETS];	/* planet/worm coord table                 */
/*172*/
	char filler[321];		/* pad GALSECT to the real 493-byte record */
} GALSECT;

typedef struct _galplnt {
	int xsect;			/* sector x coord; Btrieve key 0           */
/*2*/
	int ysect;			/* sector y coord; Btrieve key 0           */
/*4*/
	int plnum;			/* planet number; Btrieve key 0            */
/*6*/
	int type;			/* type of sector; Btrieve key 2           */
/*8*/
	COORD coord;			/* planet coordinates                      */
/*24*/
	char userid[UIDSIZ];		/* owning userid; Btrieve key 1            */
/*54*/
	char name[20];			/* planet name                             */
/*74*/
	char enviorn;			/* environment factor                      */
/*75*/
	char resource;			/* resource factor                         */
/*76*/
	unsigned long cash;		/* cash on hand                            */
/*80*/
	unsigned long timestamp;	/* time of last update                     */
/*84*/
	unsigned long tax;		/* amount of tax collected                 */
/*88*/
	int taxrate;			/* rate to tax population                  */
/*90*/
	char password[11];		/* trade/team access password              */
/*101*/
	ITEM items[NUMITEMS];		/* planet inventory and trade settings     */
/*311*/
	char beacon[BEACONMSGSZ];	/* beacon message                          */
/*386*/
	char spyowner[UIDSIZ];		/* userid of current spy owner             */
/*416*/
	unsigned int plantimesave;	/* saved planet update tick interval       */
/*418*/
	unsigned long teamcode;		/* team lock/access code                   */
/*422*/
	unsigned long nebseed;		/* random seed for nebula map (Zygor only) */
/*426*/
	char filler[67];		/* pad GALPLNT to the real 493-byte record */
} GALPLNT;

typedef struct galworm {
	int xsect;			/* sector x coord; Btrieve key 0           */
/*2*/
	int ysect;			/* sector y coord; Btrieve key 0           */
/*4*/
	int plnum;			/* planet number; Btrieve key 0            */
/*6*/
	int type;			/* type of sector; Btrieve key 2           */
/*8*/
	COORD coord;			/* wormhole coordinates                    */
/*24*/
	int visible;			/* is the wormhole visible                 */
/*26*/
	COORD destination;		/* destination coords                      */
/*42*/
	char name[20];			/* wormhole name                           */
/*62*/
	char filler[431];		/* pad GALWORM to the real 493-byte record */
} GALWORM;

typedef struct _pkey {			/* star system data record key             */
	int xsect;			/* sector x coord                          */
	int ysect;			/* sector y coord                          */
	int plnum;			/* always 0                                */
} PKEY;

typedef struct _wormtab {
	COORD coord;			/* wormhole entry location                 */
	COORD dest;			/* wormhole destination                    */
} WORMTAB;

typedef struct _mailkey {
	char userid[UIDSIZ];		/* mail owner userid                       */
	int class;			/* mail class                              */
	long msgno;			/* mail message number                     */
} MAILKEY;

typedef struct _mail {
	char userid[UIDSIZ];		/* GE mail recipient userid                */
/*30*/
	int class;			/* GE mail class / stored message flags    */
/*32*/
	int type;			/* GE message template id                  */
/*34*/
	int stamp;			/* GE day stamp                            */
/*36*/
	char dtime[20];			/* display date/time string for mail body  */
/*56*/
	char topic[30];			/* mail subject/topic text                 */
/*86*/
	char string1[80];		/* aux text field (usually attacker userid) */
/*166*/
	char name1[20];			/* primary name field (usually planet name) */
/*186*/
	char filler1[5];		/* reserved for future primary-name growth */
/*191*/
	char name2[20];			/* secondary name field (usually ship name) */
/*211*/
	char filler2[5];		/* reserved for future secondary-name growth */
/*216*/
	int int1;			/* generic integer field 1 (usually sector x) */
/*218*/
	int int2;			/* generic integer field 2 (usually sector y) */
/*220*/
	int int3;			/* generic integer field 3                 */
/*222*/
	long long1;			/* generic long field 1 (usually quantity/count) */
/*226*/
	long long2;			/* generic long field 2                    */
/*230*/
	long long3;			/* generic long field 3                    */
/*234*/
	char filler[253-200-UIDSIZ];	/* pad to 257                              */
} MAIL;

typedef struct _mailstat {
	char userid[UIDSIZ];		/* production-report recipient userid      */
/*30*/
	int class;			/* GE mail class / stored message flags    */
/*32*/
	int type;			/* GE message template id                  */
/*34*/
	int stamp;			/* GE day stamp                            */
/*36*/
	char dtime[20];			/* display date/time string for mail body  */
/*56*/
	char topic[30];			/* mail subject/topic text                 */
/*86*/
	char name1[20];			/* planet name                             */
/*106*/
	char filler1[5];		/* reserved for future planet-name growth  */
/*111*/
	int int1;			/* planet sector x                         */
/*113*/
	int int2;			/* planet sector y                         */
/*115*/
	unsigned long cash;		/* planet cash on hand                     */
/*119*/
	unsigned long timestamp;	/* planet timestamp at report time         */
/*123*/
	unsigned long tax;		/* planet tax on hand                      */
/*127*/
	unsigned long itemqty[NUMITEMS];	/* per-item quantities at report time */
/*183*/
	char filler[253-97-(UIDSIZ)-(sizeof(long)*NUMITEMS)];	/* pad to 253      */
} MAILSTAT;

typedef struct _shpkey {
	char userid[UIDSIZ];		/* owning userid                           */
	int shipno;			/* ship number                             */
} SHPKEY;

typedef struct _ship {
	char *typename;			/* class display name                      */
	char *npcprefx;			/* NPC shipname prefix                     */
	byte max_shlds;			/* max shield class                        */
	byte max_phasr;			/* max phaser class                        */
	byte max_torps;			/* max torpedo tubes                       */
	byte max_missl;			/* max missile tubes                       */
	byte has_decoy;			/* decoy capability                        */
	byte has_jam;			/* jammer capability                       */
	byte has_zip;			/* zipper capability                       */
	byte has_mine;			/* mine capability                         */
	byte max_attk;			/* fighter capability                      */
	byte max_cloak;			/* cloak capability                        */
	unsigned int max_accel;		/* max acceleration                        */
	byte max_warp;			/* max warp cruising speed                 */
	long max_tons;			/* cargo capacity                          */
	long max_price;			/* ship class price                        */
	unsigned int max_points;	/* point value if destroyed                */
	unsigned int max_type;		/* class type (user/cyborg/droid/none)     */
	long scanrange;			/* scanner range                           */
	byte cybs_can_att;		/* cybs of this class attack unprovoked    */
	byte lowest_to_attk;		/* lowest ship class this NPC will pursue  */
	byte noclaim;			/* max cyborg claims / claim pressure      */
	unsigned int tot_to_create;	/* target live NPC count for this class    */
	byte tough_factor;		/* NPC toughness/aggression factor         */
	unsigned int damfact;		/* damage/price scaling factor             */
	unsigned int faction;		/* faction id used for NPC relations       */
	unsigned int loadout;		/* upgrade/loadout profile                 */

	int hlpmsg;			/* help message id                         */
	void (*init_func)(WARSHP *, int, int);		/* class initialization hook */
	void (*tick_func)(WARSHP *, int);		/* class per-tick hook       */
	void (*kill_func)(WARSHP *, int, WARSHP *);	/* class death hook          */
	void (*won_func)(WARSHP *, int, WARSHP *);	/* class victory hook        */
} SHIP;

struct cmd {
	char *command;			/* command keyword                         */
	void (*func)(void);		/* command handler                         */
	byte cando;			/* 0 = payers only, 1 = all players        */
};

typedef struct _shiptab {
	int shipno;			/* target ship slot                        */
	char letter;			/* scan letter                             */
	char flag;			/* visibility/state flag                   */
	double dist;			/* distance                                */
	int bearing;			/* bearing                                 */
	int heading;			/* heading                                 */
	double speed;			/* speed                                   */
} SHIPTAB;

#define NOSCANTAB 15		/* how many ships can be tracked by scanners at once */

typedef struct _scantab {
	SHIPTAB ship[NOSCANTAB];
} SCANTAB;

typedef struct _entrytab {
	byte active;			/* entry-message throttle active           */
	byte ticks;			/* ticks since entry state began           */
} ENTRYTAB;

typedef struct _s00 {
	char *name;			/* predefined object name                  */
	char *owner;			/* predefined owner userid                 */
	int type;			/* predefined object type                  */
	double xcoord;			/* x coordinate within sector 0,0          */
	double ycoord;			/* y coordinate within sector 0,0          */
	int env;			/* environment setting                     */
	int res;			/* resource setting                        */
} S00;

typedef struct _team {
	long teamcode;			/* team identifier                         */
	char teamname[31];		/* team name                               */
	unsigned int teamcount;		/* current member count                    */
	unsigned int teamdeldate;	/* deletion date                           */
	char password[11];		/* join password                           */
	char secret[11];		/* team secret                             */
	int flag;			/* unused                                  */
} TEAM;


#define FUNC _export
