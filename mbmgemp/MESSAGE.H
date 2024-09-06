 /**************************************************************************
  *    MESSAGE.H based on GME.H Copyright (c) 1994-1995 GALACTICOMM, Inc.  *
  *                                                                        *
  *   ge-next                                                              *
  *                                                                        *
  *   Copyright (C) 2024 Anthony Schmidt, anthony@manicpop.org             *
  *                                                                        *
  *   https://manicpop.org/ge-next/  https://github.com/manicpop/ge-next   *
  *                                                                        *
  *************************************************************************/

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

#define MESSAGE_H
#define HSTSIZ   57
#define TPCSIZ   51

struct message {
    long msgno;
    char userto[UIDSIZ];
    char from[UIDSIZ];
    char to[UIDSIZ];
    char topic[TPCSIZ];
    char auxtpc[HSTSIZ];
    unsigned crdate;
    unsigned crtime;
    int nreply;
    int flags;
    char spare[5];
    char text[1];
};
