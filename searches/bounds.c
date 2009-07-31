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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "bounds.h"
#include "../util/io_util.h"

void new_bounds(BOUNDS **bounds, int number_parameters, double *min_bound, double *max_bound, int *in_radians) {
	int i;
	(*bounds) = (BOUNDS*)malloc(sizeof(BOUNDS));
	BOUNDS *b = (*bounds);

	b->number_parameters = number_parameters;
	b->min_bound = (double*)malloc(sizeof(double) * number_parameters);
	b->max_bound = (double*)malloc(sizeof(double) * number_parameters);
	b->in_radians = (int*)malloc(sizeof(int) * number_parameters);

	memcpy(b->min_bound, min_bound, sizeof(double) * number_parameters);
	memcpy(b->max_bound, max_bound, sizeof(double) * number_parameters);

	if (in_radians != NULL) {
		memcpy(b->in_radians, in_radians, sizeof(int) * number_parameters);
	} else {
		for (i = 0; i < number_parameters; i++) b->in_radians[i] = 0;
	}

	for (i = 0; i < number_parameters; i++) {
		if (b->in_radians[i]) {
			b->min_bound[i] = -M_PI;
			b->max_bound[i] = M_PI;
		}
	}
}

void free_bounds(BOUNDS **bounds) {
	BOUNDS *b = (*bounds);
	free(b->min_bound);
	free(b->max_bound);
	free(b->in_radians);
	free((*bounds));
	(*bounds) = NULL;
}

void bound_parameters(double* parameters, BOUNDS *b) {
	int i;
	if (b->in_radians == NULL) {
		for (i = 0; i < b->number_parameters; i++) {
			if (parameters[i] > b->max_bound[i]) parameters[i] = b->max_bound[i];
			if (parameters[i] < b->min_bound[i]) parameters[i] = b->min_bound[i];
		}       
	} else {
		for (i = 0; i < b->number_parameters; i++) {
			if (b->in_radians[i]) {
				while (parameters[i] > M_PI || parameters[i] < -M_PI) {
					if (parameters[i] > M_PI) parameters[i] -= (M_PI + M_PI);
					if (parameters[i] < -M_PI) parameters[i] += (M_PI + M_PI);
				}
			} else {
				if (parameters[i] > b->max_bound[i]) parameters[i] = b->max_bound[i];
				if (parameters[i] < b->min_bound[i]) parameters[i] = b->min_bound[i];
			}
		}       
	}
}

void bound_velocity(double *parameters, double *velocity, BOUNDS *b) {
	int i;
	if (b->in_radians == NULL) {
		for (i = 0; i < b->number_parameters; i++) {
			if (parameters[i] + velocity[i] > b->max_bound[i]) velocity[i] = b->max_bound[i] - parameters[i];
			if (parameters[i] + velocity[i] < b->min_bound[i]) velocity[i] = b->min_bound[i] - parameters[i];
		}       
	} else {
		for (i = 0; i < b->number_parameters; i++) {
			if (b->in_radians[i]) {
				if (velocity[i] > M_PI) velocity[i] = M_PI;
				if (velocity[i] < -M_PI) velocity[i] = -M_PI;
			} else {
				if (parameters[i] + velocity[i] > b->max_bound[i]) velocity[i] = b->max_bound[i] - parameters[i];
				if (parameters[i] + velocity[i] < b->min_bound[i]) velocity[i] = b->min_bound[i] - parameters[i];
			}
		}       
	}
}

void bound_step(double *parameters, double *direction, double step, BOUNDS *b) {
}

void fwrite_bounds(FILE *file, BOUNDS *b) {
	fwrite_double_array(file, "min_bound", b->number_parameters, b->min_bound);
	fwrite_double_array(file, "max_bound", b->number_parameters, b->max_bound);
	fwrite_int_array(file, "in_radians", b->number_parameters, b->in_radians);
}

void fread_bounds(FILE *file, BOUNDS **bounds) {
	(*bounds) = (BOUNDS*)malloc(sizeof(BOUNDS));
	BOUNDS *b = (*bounds);

	b->number_parameters = fread_double_array(file, "min_bound", &(b->min_bound));
	fread_double_array(file, "max_bound", &(b->max_bound));
	fread_int_array(file, "in_radians", &(b->in_radians));
}

