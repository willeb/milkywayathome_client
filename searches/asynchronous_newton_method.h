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

#ifndef FGDO_ASYNCHRONOUS_NEWTON_H
#define FGDO_ASYNCHRONOUS_NEWTON_H

#include <stdio.h>

#include "../evaluation/search_manager.h"
#include "population.h"
#include "asynchronous_search.h"
#include "bounds.h"

#define NEWTON_ERROR_RANGE 1
#define NEWTON_UPDATE_RANGE 2
#define NEWTON_LINE_SEARCH 3

#define NMS_HESSIAN 1
#define NMS_LINE_SEARCH 2

typedef struct line_search {
	double *direction;
	double center, min_range, max_range;
} LINE_SEARCH;

typedef struct newton_method_search {
	int type, mode, remove_outliers;

	int number_parameters;
	double *current_point;
	double *initial_range;
	double *parameter_range;

	BOUNDS *bounds;

	int current_iteration, maximum_iteration;
	int current_evaluation, evaluations_per_iteration;

	LINE_SEARCH* line_search;

	POPULATION *population;
} NEWTON_METHOD_SEARCH;

ASYNCHRONOUS_SEARCH* get_asynchronous_newton_method();

int create_newton_method(char* search_name, int number_arguments, char** arguments, int number_parameters, double *point, double *range, BOUNDS *bounds);
int read_newton_method(char* search_name, void** search_data);
int checkpoint_newton_method(char* search_name, void* search_data);
int newton_generate_parameters(char* search_name, void* search_data, SEARCH_PARAMETERS *sp);
int newton_insert_parameters(char* search_name, void* search_data, SEARCH_PARAMETERS *sp);

#endif
