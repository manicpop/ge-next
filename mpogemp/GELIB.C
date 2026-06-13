/*****************************************************************************
 * ge-next GELIB.C                                                           *
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


#endif

#include "math.h"
#include "majorbbs.h"

#define GELIB 1

#include "gemain.h"
#include "geglobal.h"

/**************************************************************************
** Return the smaller angular separation between two headings            **
**************************************************************************/

unsigned FUNC smallest(unsigned a1, unsigned a2)
{
	int a;

	a = abs(a1 - a2);

	if (a > 180)
		return 360 - a;
	else
		return a;
}


/**************************************************************************
** Generate a floating-point random value from 0 up to mod              **
**************************************************************************/

double FUNC rndm(double mod)
{
	static double randmax = (double)RAND_MAX;

	return mod * (((double)((unsigned)rand())) / randmax);
}


/**************************************************************************
** Generate a raw integer random value                                   **
**************************************************************************/

unsigned int FUNC gernd(void)
{
	return rand();
}


/**************************************************************************
** Clamp a value into the valid acos() input range                       **
**************************************************************************/

static double clamp_acos(double v)
{
	if (v > 1.0)
		return 1.0;
	if (v < -1.0)
		return -1.0;
	return v;
}

/**************************************************************************
** Return the signed relative bearing from ptr1 to ptr2                  **
**************************************************************************/

int FUNC cbearing(COORD *ptr1, COORD *ptr2, double heading)
{
	double b;

	b = vector(ptr1, ptr2);
	b = normal(360.0 - heading + b);

	/* fold the absolute angle into the signed -180..180 display range */
	if (b > 180.0)
		b -= 360.0;
	else if (b < -180.0)
		b += 360.0;

	if (b > -0.75 && b < 0.75)
		b = 0.0;

	/* do the rounding here so we can cast to int cleanly elsewhere */
	if (b < 0.0)
		b -= 0.4999;
	else
		b += 0.4999;

	/* never show 180 as negative */
	if (b <= -180)
		b = 180;

	return (int)b;
}

/**************************************************************************
** Calculate the distance between two coordinates                        **
**************************************************************************/

double FUNC cdistance(COORD *ptr1, COORD *ptr2)
{
	double dx, dy;

	dx = ptr1->xcoord - ptr2->xcoord;
	dy = ptr1->ycoord - ptr2->ycoord;

	return sqrt((dx * dx) + (dy * dy));
}

/**************************************************************************
** Return the absolute heading from ptr1 to ptr2                         **
**************************************************************************/

double FUNC vector(COORD *ptr1, COORD *ptr2)
{
	double da, dx, dy, raw;

	/* handle exact alignment on an axis */
	if (fabs(ptr1->ycoord - ptr2->ycoord) <= VEC_EPS) {
		if (ptr1->xcoord < ptr2->xcoord)
			return 90.0;
		else if (ptr1->xcoord > ptr2->xcoord)
			return 270.0;
		else
			return 0.0;
	}

	if (fabs(ptr1->xcoord - ptr2->xcoord) <= VEC_EPS) {
		if (ptr1->ycoord < ptr2->ycoord)
			return 180.0;
		else
			return 0.0;
	}

	dx = ptr2->xcoord - ptr1->xcoord;
	dy = ptr2->ycoord - ptr1->ycoord;
	da = sqrt((dx * dx) + (dy * dy));

	/* derive the north-based world angle, then mirror into the west half-plane */
	raw = radtodeg(acos(clamp_acos((-dy) / da)));

	if (dx >= 0.0)
		return raw;
	else
		return 360.0 - raw;
}

/**************************************************************************
** Bring an angle back into the range 0 - 360                            **
**************************************************************************/

double FUNC normal(double angle)
{
	while (angle < 0.0)
		angle += 360.0;
	while (angle >= 360.0)
		angle -= 360.0;
	return angle;
}

/**************************************************************************
** Convert degrees to radians                                            **
**************************************************************************/

double FUNC degtorad(double value)
{
	return value * (PI / 180);
}

/**************************************************************************
** Convert radians to degrees                                            **
**************************************************************************/

double FUNC radtodeg(double value)
{
	return value * (180 / PI);
}

/**************************************************************************
** Convert a world coordinate to a subsector coordinate                  **
**************************************************************************/

unsigned FUNC coord2(double dcoord)
{
	double d1, d2;
	int d3;

	d2 = modf(1 + modf(dcoord, &d1), &d1);
	d3 = (d2 * SSMAX);

	return (unsigned)d3;
}


/**************************************************************************
** Convert a world coordinate to a sector coordinate                     **
**************************************************************************/

int FUNC coord1(double dcoord)
{
	return (int)floor(dcoord);
}
