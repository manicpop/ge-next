/***************************************************************************
 *                                                                         *
 *   GEPROTO.H                                                             *
 *                                                                         *
 *   Copyright (C) 1988, 89, 90, 91, 92 Michael B. Murdock                 *
 *                                                                         *
 *   This is the source for the Galactic Empire game module                *
 *                                                                         *
 *                                         M. Murdock     03/17/92         *
 *                                                                         *
 *   ge-next                                                               *
 *                                                                         *
 *   Copyright (C) 2024 Anthony Schmidt, anthony@manicpop.org              *
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

#ifdef __STDC__
# define P(s) s
#else
# define P(s) ()
#endif


/* gemain.c */
/*     384 */ void EXPORT init__galemp P((void ));
/*     398 */ int FUNC iniwar P((void ));
/*     405 */ void FUNC dummy P((void ));
/*     409 */ void FUNC iniwara P((void ));
/*     966 */ int FUNC gelogon P((void ));
/*     997 */ void FUNC pgedelete P((char *uid ));
/*    1004 */ int gedelete P((char *uid ));
/*    1012 */ void FUNC gedeletea P((char *uid ));
/*    1041 */ void FUNC pgemidnight P((void ));
/*    1047 */ int FUNC gemidnight P((void ));
/*    1054 */ void FUNC gemidnighta P((void ));
/*    1310 */ void FUNC calc_networth P((void ));
/*    1318 */ unsigned FUNC long value_pl P((void ));
/*    1337 */ int FUNC pwarlof P((void ));
/*    1353 */ void FUNC pwarhup P((void ));
/*    1360 */ int FUNC warhup P((void ));
/*    1367 */ void FUNC warhupa P((void ));
/*    1415 */ void FUNC pclswar P((void ));
/*    1421 */ int FUNC clswar P((void ));
/*    1428 */ void FUNC clswara P((void ));
/*    1453 */ int FUNC galemp P((void ));
/*    1487 */ void FUNC outwar P((int filter , unsigned exclude , unsigned freq ));
/*    1519 */ int FUNC gepdb P((int func , char *usrname , int shipnum , WARSHP *geptr ));
/*    1601 */ int FUNC geudb P((int func , char *usrname , WARUSR *geptr ));
/*    1676 */ int FUNC gesdb P((int func , PKEY *sect , GALSECT *geptr ));
/*    1761 */ int FUNC getplanetdat P((int usrn ));
/*    1818 */ void FUNC load_team_tab P((void ));
/*    1886 */ void FUNC update_team_tab P((void ));
/*    1919 */ void FUNC pplarti P((void ));
/*    1925 */ int FUNC plarti P((void ));
/*    1934 */ void FUNC plartia P((void ));
/*    2168 */ void FUNC plabld P((void ));
/*    2187 */ void FUNC pwarrti P((void ));
/*    2193 */ int FUNC warrti P((void ));
/*    2200 */ void FUNC warrtia P((void ));
/*    2253 */ void FUNC pautorti P((void ));
/*    2259 */ int FUNC autorti P((void ));
/*    2266 */ void FUNC autortia P((void ));
/*    2418 */ void FUNC pwarrti2 P((void ));
/*    2425 */ int FUNC warrti2 P((void ));
/*    2432 */ void FUNC warrti2a P((void ));
/*    2473 */ void FUNC pwarrti3 P((void ));
/*    2480 */ int FUNC warrti3 P((void ));
/*    2487 */ void FUNC warrti3a P((void ));
/*    2517 */ void FUNC outprfge P((int class , int shpno ));
/*    2553 */ void FUNC outsect P((int filter , COORD *coordptr , unsigned exclude , unsigned freq ));
/*    2591 */ void FUNC outrange P((int filter , COORD *coordptr ));
/*    2621 */ int FUNC ingegame P((int shpno ));
/*    2647 */ void FUNC geshocst P((int opt , char *str ));
/*    2696 */ int FUNC mnu_main P((void ));
/*    2707 */ int FUNC mnu_main_ans P((void ));
/*    2828 */ int FUNC mnu_fightsub P((void ));
/*    2869 */ int FUNC mnu_admenu1 P((void ));
/*    2919 */ int mnu_admenu1a P((void ));
/*    2958 */ int FUNC mnu_admenu2 P((void ));
/*    3058 */ int FUNC mnu_admenu2b P((void ));
/*    3090 */ int FUNC mnu_admenu2e P((void ));
/*    3114 */ int FUNC mnu_admenu2f1 P((void ));
/*    3134 */ int FUNC mnu_admenu2f2 P((void ));
/*    3155 */ int FUNC mnu_admenu2f3 P((void ));
/*    3173 */ int FUNC mnu_admenu2f4 P((void ));
/*    3194 */ int FUNC mnu_admenu2h P((void ));
/*    3222 */ int FUNC mnu_admenu2i P((void ));
/*    3277 */ int FUNC mnu_admenu2j P((void ));
/*    3309 */ int FUNC mnu_choosesh P((void ));
/*    3318 */ int mnu_menug P((void ));
/*    3381 */ int FUNC mnu_menug1 P((void ));
/*    3428 */ int FUNC mnu_menug2 P((void ));
/*    3473 */ void FUNC disp_main_menu P((void ));
/*    3494 */ void FUNC disp_menu_d P((void ));
/*    3509 */ void FUNC update_items P((void ));
/*    3548 */ void FUNC optdisp P((void ));
/*    3598 */ void FUNC stshdlr P((void ));

/* gefuncs.c */
/*      53 */ void FUNC lookupshp P((void ));
/*     123 */ void FUNC tossingegame P((void ));
/*     153 */ int FUNC initshp P((char *userid , int type ));
/*     253 */ int FUNC initusr P((char *userid ));
/*     280 */ int FUNC findships P((void ));
/*     330 */ void FUNC selectship P((void ));
/*     360 */ void FUNC repairship P((WARSHP *ptr , int usrn ));
/*     403 */ void FUNC rotateship P((WARSHP *ptr , int usrn ));
/*     439 */ void FUNC accel P((WARSHP *ptr , int usrn ));
/*     551 */ void FUNC hyperspace P((WARSHP *ptr , int usrn , int flag ));
/*     603 */ void FUNC moveship P((WARSHP *ptr , int usrn ));
/*     790 */ void FUNC telezip P((WARSHP *ptr , int usrn ));
/*     807 */ void FUNC gravity P((WARSHP *ptr , int usrn ));
/*     878 */ void FUNC checkdist P((WARSHP *ptr , int usrn ));
/*     903 */ void FUNC refresh P((WARSHP *ptr , int usrn ));
/*     942 */ void FUNC checkdam P((WARSHP *ptr , int usrn ));
/*    1058 */ void FUNC killem P((WARSHP *ptr , int usrn ));
/*    1261 */ void FUNC recharge P((WARSHP *ptr , int usrn ));
/*    1278 */ void FUNC fluxstat P((WARSHP *ptr , int usrn ));
/*    1305 */ void FUNC shieldstat P((WARSHP *ptr , int usrn ));
/*    1337 */ void FUNC cloakstat P((WARSHP *ptr , int usrn ));
/*    1374 */ void FUNC checkmines P((void ));
/*    1471 */ int FUNC useenergy P((WARSHP *ptr , int usrn , int amount ));
/*    1493 */ void FUNC checktm P((WARSHP *ptr , int usrn ));
/*    1701 */ void FUNC acctm P((WARSHP *ptr , int usrn , int mt , unsigned int channel ));
/*    1721 */ void FUNC cleartm P((int channel ));
/*    1756 */ void FUNC fireion P((WARSHP *ptr , int usrn ));
/*    1791 */ void FUNC destruct P((WARSHP *ptr , int usrn ));
/*    1877 */ int FUNC valpcnt P((char *ptr , unsigned minnum , unsigned maxnum ));
/*    1904 */ int FUNC valdegree P((char *ptr ));
/*    1927 */ void FUNC randamage P((WARSHP *ptr , int usrn ));
/*    2031 */ unsigned FUNC pdamage P((WARSHP *wptr , double dist , int foc ));
/*    2071 */ void FUNC setsect P((WARSHP *ptr ));
/*    2087 */ void FUNC movecoord P((COORD *pointb , COORD *pointa ));
/*    2100 */ int FUNC samesect P((COORD *pointb , COORD *pointa ));
/*    2118 */ int FUNC genearas P((char *str1 , char *str2 ));
/*    2133 */ int FUNC mailscan P((char *userid , int class ));
/*    2162 */ int mailread P((char *userid , int class ));
/*    2194 */ void FUNC mailit P((int flag ));
/*    2290 */ int FUNC sendit P((void ));
/*    2339 */ void FUNC prf2tx P((void ));
/*    2360 */ int FUNC sendgemsg P((struct message *msgptr , char *usrid ));
/*    2380 */ void FUNC shieldup P((WARSHP *wptr , int usrn ));
/*    2390 */ void FUNC shielddn P((WARSHP *wptr , int usrn ));
/*    2401 */ int FUNC shieldhit P((WARSHP *wptr , int usrn , int dam ));
/*    2444 */ void FUNC shieldrep P((WARSHP *wptr , int usrn ));
/*    2462 */ void FUNC shieldchg P((WARSHP *wptr , int usrn ));
/*    2501 */ void FUNC charge P((WARSHP *wptr , int *max , int *pct ));
/*    2512 */ int FUNC chkweight P((WARSHP *wptr , int itm , long amt ));
/*    2533 */ long FUNC calcweight P((WARSHP *wptr ));
/*    2549 */ char FUNC shpltr P((int usrn , int ship ));
/*    2567 */ char *FUNC username P((WARSHP *ptr ));
/*    2579 */ void FUNC logthis P((char *str ));
/*    2609 */ WARUSR *FUNC warusroff P((int usrn ));
/*    2620 */ WARSHP *FUNC warshpoff P((int usrn ));
/*    2632 */ double FUNC ton_fact P((WARSHP *ptr , double damfact ));
/*    2645 */ char *FUNC showarp P((double speed ));

/* gecmds.c */
/*     217 */ struct cmd *FUNC gesearch P((char *ptr , struct cmd tab [], int len ));
/*     245 */ void FUNC gwar P((void ));
/*     317 */ void FUNC warnop P((void ));
/*     325 */ void FUNC cmd_gehelp P((void ));
/*     450 */ void FUNC cmd_impulse P((void ));
/*     529 */ void FUNC cmd_warp P((void ));
/*     611 */ void FUNC cmd_rotate P((void ));
/*     703 */ void FUNC cmd_flux P((void ));
/*     726 */ void FUNC cmd_orbit P((void ));
/*     797 */ void FUNC cmd_phas P((void ));
/*     882 */ void FUNC firep P((WARSHP *ptr , int usrn ));
/*     988 */ void FUNC firehp P((WARSHP *ptr , int usrn ));
/*    1069 */ void FUNC cmd_torp P((void ));
/*    1146 */ void FUNC torp P((WARSHP *ptr , int usrn , int shpnum ));
/*    1183 */ void FUNC cmd_missl P((void ));
/*    1307 */ int FUNC lockon P((WARSHP *ptr , int type , int ship , int usrn ));
/*    1400 */ int FUNC findshp P((char *ptr , int type ));
/*    1493 */ void FUNC zaphim P((WARSHP *ptr , int usrn ));
/*    1506 */ void FUNC cmd_decoy P((void ));
/*    1561 */ void FUNC cmd_jammer P((void ));
/*    1592 */ void FUNC jam P((WARSHP *ptr , int usrn ));
/*    1626 */ void FUNC cmd_zipper P((void ));
/*    1657 */ void FUNC zip P((WARSHP *ptr , int usrn ));
/*    1690 */ void FUNC cmd_mine P((void ));
/*    1753 */ int FUNC laymine P((WARSHP *ptr , int usrn , int timer ));
/*    1793 */ void FUNC cmd_send P((void ));
/*    1853 */ void FUNC cmd_freq P((void ));
/*    1914 */ void FUNC cmd_report P((void ));
/*    2078 */ void FUNC damstr P((int damage ));
/*    2106 */ void FUNC cmd_scan P((void ));
/*    2158 */ void FUNC scan_sh P((void ));
/*    2263 */ void FUNC scan_pl P((void ));
/*    2452 */ void FUNC scan_ra P((void ));
/*    2548 */ void FUNC scan_se P((void ));
/*    2608 */ void FUNC scan_lo P((void ));
/*    2696 */ void scan_hy P((void ));
/*    2753 */ void FUNC update_scantab P((WARSHP *ptr , int usrn ));
/*    2863 */ void FUNC pick_letter P((SCANTAB *ptr ));
/*    2922 */ void FUNC map_planets P((void ));
/*    2938 */ void FUNC clearmap P((void ));
/*    2955 */ void FUNC printmap P((void ));
/*    2987 */ void FUNC printmapfull P((void ));
/*    3056 */ unsigned FUNC coord2 P((double dcoord ));
/*    3070 */ int FUNC coord1 P((double dcoord ));
/*    3082 */ void FUNC cmd_shields P((void ));
/*    3156 */ void FUNC cmd_cloak P((void ));
/*    3239 */ void FUNC cmd_transfer P((void ));
/*    3274 */ void trans_down P((int item ));
/*    3323 */ void trans_up P((int item ));
/*    3388 */ void FUNC cmd_abandon P((void ));
/*    3430 */ void FUNC cmd_admin P((void ));
/*    3483 */ void FUNC cmd_attack P((void ));
/*    3591 */ int FUNC attack_men P((unsigned long num ));
/*    3756 */ int FUNC attack_fig P((unsigned long num ));
/*    3920 */ void FUNC call_4_help P((int send_spy_mail , int won ));
/*    3964 */ void FUNC wonplnt P((void ));
/*    3976 */ void FUNC cmd_geroster P((void ));
/*    4031 */ void FUNC cmd_planet P((void ));
/*    4071 */ void FUNC cmd_sell P((void ));
/*    4115 */ void FUNC sell P((int item ));
/*    4169 */ void FUNC cmd_buy P((void ));
/*    4252 */ void FUNC cmd_price P((void ));
/*    4284 */ void FUNC buy P((int item ));
/*    4373 */ long FUNC amt4sale P((int item ));
/*    4398 */ long FUNC price P((unsigned item , unsigned long amt ));
/*    4420 */ void FUNC cmd_maint P((void ));
/*    4502 */ void FUNC cmd_new P((void ));
/*    4710 */ void FUNC cmd_sysop P((void ));
/*    4957 */ void done P((void ));
/*    4970 */ void FUNC cmd_rename P((void ));
/*    4993 */ void FUNC cmd_destruct P((void ));
/*    5012 */ void FUNC cmd_abort P((void ));
/*    5036 */ void FUNC cmd_lock P((void ));
/*    5077 */ void FUNC cmd_navigate P((void ));
/*    5130 */ void FUNC cmd_who P((void ));
/*    5145 */ void FUNC cmd_displ P((void ));
/*    5158 */ void FUNC cmd_set P((void ));
/*    5245 */ void FUNC cmd_team P((void ));
/*    5759 */ char *FUNC teamname P((WARUSR *ptr ));
/*    5776 */ void FUNC cmd_clear P((void ));
/*    5783 */ void FUNC ansifunc P((int func ));
/*    5797 */ void FUNC cmd_data P((void ));
/*    5911 */ void FUNC scan_data1 P((void ));
/*    5962 */ void FUNC scan_data2 P((void ));
/*    5989 */ char *FUNC gedots P((int numdots ));
/*    6008 */ void FUNC cmd_spy P((void ));
/*    6070 */ void FUNC cmd_jettison P((void ));
/*    6091 */ void FUNC jettison P((int item ));

/* gecybs.c */
/*      58 */ void FUNC cyb_init P((WARSHP *ptr , int usrn , int class ));
/*     168 */ void FUNC cyb_lives P((WARSHP *ptr , int usrn ));
/*     327 */ int FUNC notclaimed P((WARSHP *ptr , int usrn ));
/*     349 */ void FUNC cyb_annoy P((WARSHP *ptr , int usrn , int rnd , int first , int last ));
/*     364 */ int FUNC cybwhoops P((WARSHP *ptr , int zothusn ));
/*     386 */ int FUNC gebemean P((WARSHP *ptr , int usrn ));
/*     409 */ void FUNC db_update P((WARSHP *ptr , int usrn ));
/*     453 */ void FUNC cyb_attack P((WARSHP *ptr , int usrn , WARSHP *wptr , int zothusn ));
/*     552 */ void FUNC cyb_lay_decoys P((WARSHP *ptr , int zothusn ));
/*     573 */ void FUNC cyb_check_damage P((WARSHP *ptr , int usrn ));
/*     603 */ void FUNC cyb_check_lockon P((WARSHP *ptr , int usrn ));
/*     763 */ void FUNC cyb_won P((WARSHP *ptr , int usrn , WARSHP *wptr ));
/*     776 */ void FUNC cyb_died P((WARSHP *ptr , int usrn , WARSHP *wptr ));
/*     788 */ int FUNC isquad P((WARSHP *ptr ));

/* gedroids.c */
/*      68 */ void FUNC droid_init P((WARSHP *ptr , int usrn , int class ));
/*     151 */ void FUNC droid_lives P((WARSHP *ptr , int usrn ));
/*     202 */ void FUNC droid_annoy P((WARSHP *ptr , int usrn , int rnd , int first , int last ));
/*     223 */ void FUNC droid_act_class_10 P((WARSHP *ptr , int usrn ));
/*     272 */ void FUNC droid_act_class_11 P((WARSHP *ptr , int usrn ));
/*     380 */ void FUNC droid_act_class_12 P((WARSHP *ptr , int usrn ));
/*     504 */ void FUNC droid_won P((WARSHP *ptr ));
/*     512 */ void FUNC droid_died P((WARSHP *ptr ));
/*     520 */ int FUNC missl_attached P((WARSHP *ptr , int usrn ));

/* geplanet.c */
/*      54 */ void FUNC check_spy P((void ));
/*     165 */ void FUNC multiply P((void ));
/*     357 */ int FUNC getsector P((COORD *sect ));
/*     425 */ int FUNC xgetsector P((COORD *sect , int wormy ));
/*     635 */ void FUNC build_plan_1 P((int idx ));
/*     700 */ void FUNC build_plan_2 P((int idx ));
/*     741 */ void FUNC build_other P((int idx ));
/*     770 */ void FUNC build_worm P((int idx ));
/*     815 */ int FUNC getplanet P((COORD *sect , int plnt ));
/*     836 */ int FUNC neutral P((COORD *coord ));

/* gelib.c */
/*      46 */ void c4angle P((void ));
/*      72 */ unsigned smallest P((unsigned a1 , unsigned a2 ));
/*      91 */ double rndm P((double mod ));
/*     101 */ unsigned int gernd P((void ));
/*     111 */ double cbearing P((COORD *ptr1 , COORD *ptr2 , double heading ));
/*     141 */ double cdistance P((COORD *ptr1 , COORD *ptr2 ));
/*     167 */ double vector P((COORD *ptr1 , COORD *ptr2 ));
/*     210 */ double angleb P((COORD *ptr1 , COORD *ptr2 ));
/*     238 */ double anglec P((COORD *ptr1 , COORD *ptr2 ));
/*     267 */ double normal P((double angle ));
/*     288 */ double degtorad P((double value ));
/*     298 */ double radtodeg P((double value ));
/*     308 */ double absol P((double value ));

#undef P
