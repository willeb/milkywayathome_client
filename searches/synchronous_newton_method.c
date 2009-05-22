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

#include "gradient.h"
#include "hessian.h"
#include "newton_method.h"
#include "line_search.h"
#include "regression.h"
#include "recombination.h"
#include "search_arguments.h"


#include "../evaluation/evaluator.h"

#include "../util/matrix.h"
#include "../util/io_util.h"

void synchronous_newton_method(int number_arguments, char** arguments, int number_parameters, double *point, double *range) {
	int i, j;
	double **hessian, *gradient, *step, *new_point;
	double current_fitness;
	int iterations, retval, evaluations;

	iterations = get_int_arg("-nm_iterations", number_arguments, arguments);
	if (iterations <= 0) {
		printf("argument: '-nm_iterations #' not specified, quitting.\n");
		return;
	}

	current_fitness = evaluate(point);

	printf("initial [fitness : point] -- %.15lf :", current_fitness);
	for (i = 0; i < number_parameters; i++) printf(" %.15lf", point[i]);
	printf("\n");

	new_point = (double*)malloc(sizeof(double) * number_parameters);
	step = (double*)malloc(sizeof(double) * number_parameters);
	gradient = (double*)malloc(sizeof(double) * number_parameters);
	hessian = (double**)malloc(sizeof(double*) * number_parameters);

	for (i = 0; i < number_parameters; i++) hessian[i] = (double*)malloc(sizeof(double) * number_parameters);

	for (i = 0; i < iterations; i++) {
		printf("iteration %d: current_fitness: %.20lf\n", i, current_fitness);

		printf("\tcalculating gradient.\n");
		get_gradient(number_parameters, point, range, gradient);

		printf("\tcalculating hessian.\n");
		get_hessian(number_parameters, point, range, hessian);

		printf("\tcalculating direction.\n");
		newton_step(number_parameters, hessian, gradient, step);
		for (j = 0; j < number_parameters; j++) step[j] = -step[j];
		fwrite_double_array(stdout, "\tdirection:", number_parameters, step);

		retval = line_search(point, current_fitness, step, number_parameters, new_point, &current_fitness, &evaluations);
		fwrite_double_array(stdout, "\tnew point:", number_parameters, new_point);
		printf("\tline search took: %d evaluations for new fitness: %.15lf, with result: [%s]\n", evaluations, current_fitness, LS_STR[retval]);

		if (retval != LS_SUCCESS) break;
		if (evaluations < 0) break;

		for (j = 0; j < number_parameters; j++) point[j] = new_point[j];
	}
	free(new_point);
	free(step);
	free(gradient);
	for (i = 0; i < number_parameters; i++) free(hessian[i]);
	free(hessian);
}
