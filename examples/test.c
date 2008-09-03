#include <stdio.h>
#include <stdlib.h>

#include "../searches/gradient_descent.h"
#include "../searches/differential_evolution.h"
#include "../searches/genetic_search.h"
#include "../searches/particle_swarm.h"
#include "../searches/newton_method.h"
#include "../searches/synchronous_search.h"

#include "../evaluation/simple_evaluator.h"


double *min_parameters, *max_parameters;
int number_parameters;


double sum_of_squares(double* parameters) {
	int i;
	double sum;

	sum = 0.0;
	for (i = 0; i < number_parameters; i++) {
		sum += parameters[i] * parameters[i];
	}
	return sum;
}



int main(int number_arguments, char** arguments) {
	int i;
	double* step;
	double* point;

	if (number_arguments != 4) {
		fprintf(stderr, "Invalid Arguments, proper usage:\n");
		fprintf(stderr, "\ttest <number_parameters> <search_path> <search_parameters>\n");

		return 0;
	}

	srand48(23);

	number_parameters = atoi(arguments[1]);
	min_parameters = (double*)malloc(sizeof(double) * number_parameters);
	max_parameters = (double*)malloc(sizeof(double) * number_parameters);
	step = (double*)malloc(sizeof(double) * number_parameters);
	point = (double*)malloc(sizeof(double) * number_parameters);
	for (i = 0; i < number_parameters; i++) {
		min_parameters[i] = -100.0;
		max_parameters[i] = 100.0;
		step[i] = 2.0;
		point[i] = (drand48() * (double)(max_parameters[i] - min_parameters[i])) + (double)min_parameters[i];
	}

	init_simple_evaluator(sum_of_squares);
        printf("searching...\n");
        if (arguments[3][0] == 'g' && arguments[3][1] == 'd') {
                synchronous_gradient_descent(arguments[2], arguments[3], point, step, number_parameters);
        } else if (arguments[3][0] == 'c') {
                synchronous_conjugate_gradient_descent(arguments[2], arguments[3], point, step, number_parameters);
        } else if (arguments[3][0] == 'n') {
                synchronous_newton_method(arguments[2], arguments[3], point, step, number_parameters);
	} else if (arguments[3][0] == 'r') {
                randomized_newton_method(arguments[2], arguments[3], point, step, number_parameters);
        } else if (arguments[3][0] == 'g' && arguments[3][1] == 's') {         
                synchronous_search(arguments[2], arguments[3], min_parameters, max_parameters, number_parameters, start_genetic_search);
        } else if (arguments[3][0] == 'd') {
                synchronous_search(arguments[2], arguments[3], min_parameters, max_parameters, number_parameters, start_differential_evolution);
        } else if (arguments[3][0] == 'p') {
                synchronous_search(arguments[2], arguments[3], min_parameters, max_parameters, number_parameters, start_particle_swarm);
        }

	return 0;
}
