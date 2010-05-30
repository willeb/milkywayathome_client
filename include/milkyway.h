/* Copyright 2010 Matthew Arsenault, Travis Desell, Dave Przybylo,
Nathan Cole, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
Magdon-Ismail and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MILKYWAY_AT_HOME_
#define _MILKYWAY_AT_HOME_

#ifdef _WIN32
	#include <boinc_win.h>
	#include <str_util.h>
#else
	#include <stdio.h>
#endif

#include <boinc_api.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if BOINC_APP_GRAPHICS
	#include <graphics_api.h>
	#include <graphics_lib.h>
#endif

#include "config.h"

#include "evaluation_optimized.h"
#include "parameters.h"
#include "probability.h"
#include "stCoords.h"
#include "atSurveyGeometry.h"
#include "star_points.h"
#include "numericalIntegration.h"
#include "../util/io_util.h"

#ifndef M_PI
	#define pi 3.1415926535897932384626433832795028841971693993751
#else
	#define pi M_PI
#endif

#define deg (180.0/pi)

#define EPS 3.0e-11
#define PI (double) 3.1415926535897932384626433832795028841971693993751
#define D2PI (double) 6.2831853071795864769252867665590057683943387987502
#define DPI (double) 3.1415926535897932384626433832795028841971693993751

#endif /* _MILKYWAY_AT_HOME_ */

