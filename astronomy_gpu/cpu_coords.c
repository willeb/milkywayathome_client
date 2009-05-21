/*
 * Copyright 2008, 2009 Travis Desell, Dave Przybylo, Nathan Cole,
 * Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail
 * and Rensselaer Polytechnic Institute.
 *
 * This file is part of Milkway@Home.
 *
 * Milkyway@Home is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Milkyway@Home is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <stdlib.h>
#include <math.h>

#include "pi_constants.h"
#include "coords.h"

#include "../astronomy/parameters.h"


void gc_to_gal(int wedge, double amu_rad, double anu_rad, double *cpu__glong, double *cpu__glat) {
        double a_inc_rad = d_get_incl_rad(wedge);
	double cos_ainc = cos(a_inc_rad);
	double sin_ainc = sin(a_inc_rad);
	double x2 = cos(amu_rad - D_A_NODE_RAD) * cos(anu_rad);
	double y2 = sin(amu_rad - D_A_NODE_RAD) * cos(anu_rad);
	double z2 = sin(anu_rad);
	double x1 = x2;
	double y1 = (y2 * cos_ainc) - (z2 * sin_ainc);
	double z1 = (y2 * sin_ainc) + (z2 * cos_ainc);

	double ra = atan2(y1, x1) + D_A_NODE_RAD;
	double dec = asin(z1);

	double cosb = cos(dec);
	double v0 = cos(ra) * cosb;
	double v1 = sin(ra) * cosb;
	double v2 = sin(dec);

	double v0_temp = (rmat00 * v0) + (rmat01 * v1) + (rmat02 * v2);
	double v1_temp = (rmat10 * v0) + (rmat11 * v1) + (rmat12 * v2);
	double v2_temp = (rmat20 * v0) + (rmat21 * v1) + (rmat22 * v2);

	v0 = v0_temp;
	v1 = v1_temp;
	v2 = v2_temp;

	//slaDcc2s(v2, dl, db)
	double r = sqrt( (v0 * v0) + (v1 * v1) );
	/*
	 *      Might need to check if r is 0, then glong and glat are both 0
	**/
	*cpu__glong = fmod(atan2(v1, v0), D_2PI);
	*cpu__glat = fmod(atan2(v2, r), D_2PI);
}


void gc_to_lb(int wedge, double amu_rad, double anu_rad, double *cpu__lb) {
        double a_inc_rad = d_get_incl_rad(wedge);
	double cos_ainc = cos(a_inc_rad);
	double sin_ainc = sin(a_inc_rad);
	double x2 = cos(amu_rad - D_A_NODE_RAD) * cos(anu_rad);
	double y2 = sin(amu_rad - D_A_NODE_RAD) * cos(anu_rad);
	double z2 = sin(anu_rad);
	double x1 = x2;
	double y1 = (y2 * cos_ainc) - (z2 * sin_ainc);
	double z1 = (y2 * sin_ainc) + (z2 * cos_ainc);

	double ra = atan2(y1, x1) + D_A_NODE_RAD;
	double dec = asin(z1);

	double cosb = cos(dec);
	double v0 = cos(ra) * cosb;
	double v1 = sin(ra) * cosb;
	double v2 = sin(dec);

	double v0_temp = (rmat00 * v0) + (rmat01 * v1) + (rmat02 * v2);
	double v1_temp = (rmat10 * v0) + (rmat11 * v1) + (rmat12 * v2);
	double v2_temp = (rmat20 * v0) + (rmat21 * v1) + (rmat22 * v2);

	v0 = v0_temp;
	v1 = v1_temp;
	v2 = v2_temp;

	//slaDcc2s(v2, dl, db)
	double r = sqrt( (v0 * v0) + (v1 * v1) );
	/*
	 *      Might need to check if r is 0, then glong and glat are both 0
	**/
	double glong = fmod(atan2(v1, v0), D_2PI);
	double glat = fmod(atan2(v2, r), D_2PI);

	cpu__lb[0] = sin(glat);
	cpu__lb[1] = sin(glong);
	cpu__lb[2] = cos(glat);
	cpu__lb[3] = cos(glong);
}

void cpu__gc_to_lb(int wedge, INTEGRAL *integral, double **cpu__lb) {
	int i, j;
        double mu_min_rad = integral->mu_min * D_DEG2RAD;
        double mu_step_rad = integral->mu_step_size * D_DEG2RAD;
        double nu_min_rad = integral->nu_min * D_DEG2RAD;
        double nu_step_rad = integral->nu_step_size * D_DEG2RAD;

	*cpu__lb = (double*)malloc(4 * integral->mu_steps * integral->nu_steps * sizeof(double));

	double anu, amu;
	int pos;
	for (i = 0; i < integral->mu_steps; i++) {
		amu = mu_min_rad + ((i + 0.5) * mu_step_rad);
		for (j = 0; j < integral->nu_steps; j++) {
			anu = nu_min_rad + ((j + 0.5) * nu_step_rad);
       			pos = ((i * integral->nu_steps) + j) * 4;
			gc_to_lb(wedge, amu, anu, &( (*cpu__lb)[pos] ));
		}
	}
}
