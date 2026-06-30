/*****************************************************************************
 * ge-next GEPROTO.H                                                         *
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

#ifdef __STDC__
# define P(s) s
#else
# define P(s) ()
#endif

/* gemain.c */
void EXPORT init__galemp P((void));
int FUNC iniwar P((void));
void FUNC dummy P((void));
void FUNC iniwara P((void));
int FUNC gelogon P((void));
void FUNC pgedelete P((char *uid));
void FUNC gedeletea P((char *uid));
void FUNC pgemidnight P((void));
int FUNC gemidnight P((void));
void FUNC gemidnighta P((void));
void FUNC dump_roster_file P((void));
void FUNC calc_networth P((void));
unsigned FUNC long value_pl P((void));
int FUNC pwarlof P((void));
void FUNC pwarhup P((void));
int FUNC warhup P((void));
void FUNC warhupa P((void));
void FUNC pclswar P((void));
int FUNC clswar P((void));
void FUNC clswara P((void));
int FUNC galemp P((void));
void FUNC outwar P((int filter , unsigned exclude , unsigned channel , int mode));
int FUNC gepdb P((int func , char *usrname , int shipnum , WARSHP *geptr));
int FUNC geudb P((int func , char *usrname , WARUSR *geptr));
int FUNC gesdb P((int func , PKEY *sect , GALSECT *geptr));
int FUNC getplanetdat P((int usrn));
void FUNC fixplanetteam P((void));
void FUNC load_team_tab P((void));
void FUNC update_team_tab P((void));
void FUNC pplarti P((void));
int FUNC plarti P((void));
void FUNC plartia P((void));
void FUNC pwarrti P((void));
int FUNC warrti P((void));
void FUNC warrtia P((void));
void FUNC pautorti P((void));
int FUNC autorti P((void));
void FUNC autortia P((void));
void FUNC pwarrti2 P((void));
int FUNC warrti2 P((void));
void FUNC warrti2a P((void));
void FUNC pwarrti3 P((void));
int FUNC warrti3 P((void));
void FUNC warrti3a P((void));
void FUNC outprfge P((int cls , int shpno));
void FUNC outsect P((int filter , COORD *coordptr , unsigned exclude));
void FUNC outrange P((int filter , COORD *coordptr));
int FUNC ingegame P((int shpno));
void FUNC geshocst P((int opt , char *str));
int FUNC mnu_main P((void));
int FUNC mnu_main_ans P((void));
int FUNC mnu_fightsub P((void));
int FUNC mnu_admenu1 P((void));
int FUNC mnu_admenu2 P((void));
int FUNC mnu_admenu2b P((void));
int FUNC mnu_admenu2e P((void));
int FUNC mnu_admenu2f1 P((void));
int FUNC mnu_admenu2f2 P((void));
int FUNC mnu_admenu2f3 P((void));
int FUNC mnu_admenu2f4 P((void));
int FUNC mnu_admenu2h P((void));
int FUNC mnu_admenu2i P((void));
int FUNC mnu_admenu2j P((void));
int FUNC mnu_choosesh P((void));
int FUNC mnu_menug1 P((void));
int FUNC mnu_menug2 P((void));
void FUNC disp_main_menu P((void));
void FUNC disp_menu_d P((void));
void FUNC update_items P((void));
void FUNC optdisp P((void));
void FUNC stshdlr P((void));

/* gefuncs.c */
int FUNC lockon P((WARSHP *ptr , int type , int ship , int usrn));
int FUNC repair_needed P((WARSHP *ptr));
unsigned FUNC repair_eta P((WARSHP *ptr));
void FUNC fullrepair P((WARSHP *ptr));
void FUNC lock_simple P((WARSHP *ptr , int usrn , int msg , int skipsame));
void FUNC lock_sector P((WARSHP *ptr , int usrn , int msg));
void FUNC lock_proj P((int usrn , int target , int msg , int msgn));
double FUNC ship_accel P((WARSHP *ptr));
long FUNC ship_scanrange P((WARSHP *ptr));
void FUNC npc_cruise P((WARSHP *ptr , int usrn , int call));
void FUNC npc_lay_decoys P((WARSHP *ptr));
void FUNC prf_cloak_blocked P((WARSHP *ptr , char *system));
void FUNC firep P((WARSHP *ptr , int usrn));
void FUNC firehp P((WARSHP *ptr , int usrn));
int FUNC torp P((WARSHP *ptr , int usrn , int shpnum));
int FUNC misl P((WARSHP *ptr , int usrnum , int shpnum , unsigned energy, unsigned eng_flu));
void FUNC lookupshp P((void));
void FUNC suddenappear P((WARSHP *ptr , int usrn));
void FUNC start_entrymsg P((int usrn));
void FUNC tick_entrymsg P((void));
void FUNC exit_entrymsg P((int usrn));
void FUNC tossingegame P((void));
int FUNC initshp P((char *userid , int type));
int FUNC initusr P((char *userid));
int FUNC valid_user_ship P((WARSHP *ptr));
int FUNC cyb_slot_class P((int usrn));
int FUNC cyb_user_slot P((char *userid));
void FUNC prune_stale_auto_records P((void));
int FUNC findships P((int direction , int quiet));
void FUNC selectship P((void));
void FUNC repairship P((WARSHP *ptr , int usrn));
void FUNC rotateship P((WARSHP *ptr , int usrn));
void FUNC accel P((WARSHP *ptr , int usrn));
void FUNC hyperspace P((WARSHP *ptr , int usrn , int flag));
void FUNC moveship P((WARSHP *ptr , int usrn));
void FUNC telezip P((WARSHP *ptr , int usrn));
void FUNC proximity P((WARSHP *ptr , int usrn));
void FUNC checkdist P((WARSHP *ptr , int usrn));
void FUNC refresh P((WARSHP *ptr , int usrn));
void FUNC checkdam P((WARSHP *ptr , int usrn));
void FUNC killem P((WARSHP *ptr , int usrn));
void FUNC recharge P((WARSHP *ptr));
int FUNC fluxstat P((WARSHP *ptr , int usrn , unsigned energy));
void FUNC shieldstat P((WARSHP *ptr , int usrn));
void FUNC cloakstat P((WARSHP *ptr , int usrn));
int FUNC isvisible P((WARSHP *ptr , WARSHP *wptr));
void FUNC checkmines P((void));
void FUNC checktm P((WARSHP *ptr , int usrn));
void FUNC validate_lock P((WARSHP *ptr , int usrn));
void FUNC acctm P((WARSHP *ptr , int usrn , int mt , byte channel , int count));
int FUNC chkitm P((int usrn));
void FUNC clearitm P((int usrn));
void FUNC cleartm P((int channel));
void FUNC fireion P((WARSHP *ptr , int usrn));
void FUNC destruct P((WARSHP *ptr , int usrn));
int FUNC valpcnt P((char *ptr , unsigned minnum , unsigned maxnum));
int FUNC valdegree P((char *ptr));
void FUNC randamage P((WARSHP *ptr , int usrn , double hitdam));
double FUNC pdamage P((WARSHP *wptr , double dist , int foc));
void FUNC setsect P((WARSHP *ptr));
void FUNC movecoord P((COORD *pointb , COORD *pointa));
int FUNC samesect P((COORD *pointb , COORD *pointa));
int FUNC mailscan P((char *userid , int cls));
int FUNC mailread P((char *userid , int cls));
void FUNC mailit P((int flag));
int FUNC sendit P((void));
void FUNC prf2tx P((void));
int FUNC sendgemsg P((struct message *msgptr));
void FUNC shieldup P((WARSHP *wptr , int usrn));
void FUNC shielddn P((WARSHP *wptr , int usrn));
int FUNC shieldhit P((WARSHP *wptr , int dam));
void FUNC shieldrep P((WARSHP *wptr , int usrn));
void FUNC shieldchg P((WARSHP *wptr , int usrn));
void FUNC charge P((WARSHP *wptr , int *max , int *pct));
unsigned long FUNC cargo_room_for_item P((WARSHP *wptr , int itm));
int FUNC chkweight P((WARSHP *wptr , int itm , unsigned long amt));
unsigned long FUNC calcweight P((WARSHP *wptr));
char FUNC shpltr P((int usrn , int ship));
char *FUNC username P((WARSHP *ptr));
void FUNC logthis P((char *str));
WARUSR *FUNC warusroff P((int usrn));
WARSHP *FUNC warshpoff P((int usrn));
double FUNC ton_fact P((WARSHP *ptr , double damfact));
char *FUNC showupg P((WARSHP *ptr));
char *FUNC showarp P((double speed));
void FUNC set_dislike P((WARUSR *wuptr , int facnum , int dislike));
void FUNC rospos P((WARUSR *losptr , WARUSR *winptr , int *lospos , int *winpos));
void FUNC damstr P((int damage));
void FUNC update_scantab P((WARSHP *ptr , int usrn));

/* gecmds.c */
struct cmd *FUNC gesearch P((char *ptr , struct cmd tab [], int len));
void FUNC gwar P((void));
void FUNC warnop P((void));
void FUNC cmd_gehelp P((void));
void FUNC cmd_impulse P((void));
void FUNC cmd_warp P((void));
void FUNC cmd_stop P((void));
void FUNC cmd_rotate P((void));
void FUNC cmd_flux P((void));
void FUNC cmd_orbit P((void));
void FUNC cmd_phas P((void));
void FUNC cmd_torp P((void));
void FUNC cmd_missl P((void));
int FUNC findshp P((char *ptr , int type));
void FUNC zaphim P((WARSHP *ptr , int usrn));
void FUNC cmd_decoy P((void));
void FUNC cmd_jammer P((void));
void FUNC jam P((WARSHP *ptr , int usrn));
void FUNC cmd_zipper P((void));
void FUNC zip P((WARSHP *ptr));
void FUNC cmd_mine P((void));
int FUNC laymine P((WARSHP *ptr , int usrn , int timer));
void FUNC cmd_send P((void));
void FUNC cmd_report P((void));
void FUNC show_rep_sysdam P((WARSHP *ptr , int showeta));
void FUNC cmd_shields P((void));
void FUNC cmd_cloak P((void));
void FUNC cmd_transfer P((void));
void FUNC cmd_abandon P((void));
void FUNC cmd_admin P((void));
void FUNC cmd_attack P((void));
void FUNC cmd_geroster P((void));
void FUNC cmd_planet P((void));
void FUNC cmd_sell P((void));
void FUNC cmd_buy P((void));
void FUNC cmd_price P((void));
void FUNC cmd_maint P((void));
void FUNC cmd_new P((void));
void FUNC cmd_sysop P((void));
void FUNC cmd_rename P((void));
void FUNC cmd_destruct P((void));
void FUNC cmd_abort P((void));
void FUNC cmd_lock P((void));
void FUNC cmd_navigate P((void));
void FUNC cmd_who P((void));
void FUNC cmd_set P((void));
void FUNC cmd_team P((void));
char *FUNC teamname P((WARUSR *ptr));
void FUNC cmd_clear P((void));
void FUNC cmd_data P((void));
char *FUNC gedots P((int numdots));
void FUNC cmd_spy P((void));
void FUNC cmd_jettison P((void));

/* gecybs.c */
void FUNC cyb_init P((WARSHP *ptr , int usrn , int cls));
void FUNC assign_cybs P((int usrnum , int call));
int FUNC cyb_fast P((WARSHP *ptr));
void FUNC cyb_lives P((WARSHP *ptr , int usrn));
void FUNC cyb_won P((WARSHP *ptr , int usrn , WARSHP *wptr));
void FUNC cyb_died P((WARSHP *ptr , int usrn , WARSHP *wptr));

/* gedroids.c */
void FUNC droid_init P((WARSHP *ptr , int usrn , int cls));
void FUNC droid_lives P((WARSHP *ptr , int usrn));
void FUNC droid_won P((WARSHP *ptr , int usrn , WARSHP *wptr));
void FUNC droid_died P((WARSHP *ptr , int usrn , WARSHP *wptr));

/* geplanet.c */
void FUNC check_spy P((void ));
void FUNC multiply P((int final_mult ));
void FUNC getsector P((COORD *sect));
int FUNC getplanet P((COORD *sect , int plnt));
int FUNC neutral P((COORD *coord));
int FUNC innebula P((int x , int y));
void FUNC update_plan_1 P((void));
void FUNC update_plan_2 P((void));
void FUNC update_plan_3 P((void));

/* gelib.c */
unsigned FUNC smallest P((unsigned a1 , unsigned a2));
double FUNC rndm P((double mod));
unsigned int FUNC gernd P((void));
int FUNC cbearing P((COORD *ptr1 , COORD *ptr2 , double heading));
double FUNC cdistance P((COORD *ptr1 , COORD *ptr2));
double FUNC vector P((COORD *ptr1 , COORD *ptr2));
double FUNC normal P((double angle));
double FUNC degtorad P((double value));
double FUNC radtodeg P((double value));
unsigned FUNC coord2 P((double dcoord));
int FUNC coord1 P((double dcoord));

/* gescan.c */
void FUNC cmd_scan P((void));

#undef P
