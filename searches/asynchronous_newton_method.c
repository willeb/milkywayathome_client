#include <stdio.h>
#include <stdlib.h>

#include "asynchronous_newton_method.h"
#include "gradient.h"
#include "hessian.h"
#include "population.h"
#include "recombination.h"
#include "search_parameters.h"

#include "../evaluation/search_manager.h"
#include "../util/settings.h"
#include "../util/matrix.h"
#include "../util/io_util.h"


int newton_generate_parameters(SEARCH* search, SEARCH_PARAMETERS* sp) {
	NEWTON_METHOD_SEARCH *nms = (NEWTON_METHOD_SEARCH*)(search->search_data);
	POPULATION *p = nms->current_population;

	if (nms->current_iteration < nms->maximum_iteration) {
		char metadata[METADATA_SIZE];
		sprintf(metadata, "iteration: %d, evaluation: %d", nms->current_iteration, nms->current_evaluation);
		new_search_parameters(&sp, search->search_name, p->number_parameters, random_recombination(nms->min_parameters, nms->max_parameters, p->number_parameters), metadata);
	}
	return 1;
}

int newton_insert_parameters(SEARCH* search, SEARCH_PARAMETERS* sp) {
	NEWTON_METHOD_SEARCH *nms = (NEWTON_METHOD_SEARCH*)(search->search_data);
	POPULATION *p = nms->current_population;

	/********
		*	Insert parameters into population.  If cutoff reached, calculate hessian
		*	and generate new population.
	 ********/
	
	if (nms->current_iteration < nms->maximum_iteration) {
		replace(p, nms->current_evaluation, sp->parameters, sp->fitness);
		nms->current_evaluation++;
		if (nms->current_evaluation >= nms->evaluations_per_iteration) {
			nms->current_evaluation = 0;
			nms->current_iteration++;
			if (nms->current_iteration < nms->maximum_iteration) {
				double **hessian, **inverse_hessian;
				double *gradient;
				int j, k;
				char filename[FILENAME_SIZE];

				randomized_hessian(p->individuals, p->fitness, p->size, p->number_parameters, &hessian, &gradient);

				printf("\n");
				print_double_array(stdout, "gradient: ", p->number_parameters, gradient);
				printf("\n");
				matrix_print(stdout, "hessian", hessian, p->number_parameters, p->number_parameters);
				printf("\n");

				/********
					*	Take the newton step:  y = -hessian^-1 * gradient
				 ********/
				matrix_invert(hessian, p->number_parameters, p->number_parameters, &inverse_hessian);
				for (j = 0; j < p->number_parameters; j++) {
					nms->parameters[j] = 0;
					for (k = 0; k < p->number_parameters; k++) nms->parameters[j] -= inverse_hessian[j][k] * gradient[j];
					nms->min_parameters[j] = nms->parameters[j] - nms->parameter_range[j];
					nms->max_parameters[j] = nms->parameters[j] + nms->parameter_range[j];
				}

				for (j = 0; j < p->number_parameters; j++) {
					free(inverse_hessian[j]);
					free(hessian[j]);
				}
				free(hessian);
				free(gradient);

				/********
					*	TODO
					*	Optional: use previous parameters with new point as a line search direction
				 ********/
				sprintf(filename, "%s/%s/population_%d", get_working_directory(), search->search_name, nms->current_iteration-1);
				write_population(filename, p);
				free_population(p);
				free(p);
				new_population(nms->evaluations_per_iteration, nms->number_parameters, &(nms->current_population));
			}
		}
	}
	return 1;
}

int fwrite_newton_method(FILE* file, NEWTON_METHOD_SEARCH *nms) {
	print_double_array(file, "parameters", nms->number_parameters, nms->parameters);
	print_double_array(file, "parameter_range", nms->number_parameters, nms->parameter_range);
	fprintf(file, "current_iteration: %d, maximum_iteration: %d\n", nms->current_iteration, nms->maximum_iteration);
	fprintf(file, "current_evaluation: %d, evaluations_per_iteration: %d\n", nms->current_evaluation, nms->evaluations_per_iteration);
	return 1;
}

int write_newton_method(char* file, NEWTON_METHOD_SEARCH *nms) {
	int result;
	FILE *f = fopen(file, "w+");
	if (f == NULL) return -1;
	else result = fwrite_newton_method(f, nms);
	fclose(f);
	return result;
}

int fread_newton_method(FILE* file, NEWTON_METHOD_SEARCH **nms) {
	int scanned, i, np;
	scanned = read_double_array(file, "parameters", &((*nms)->parameters));
	if (scanned < 1) return scanned;
	scanned = read_double_array(file, "parameter_range", &((*nms)->parameter_range));
	if (scanned < 1) return scanned;
	scanned = fscanf(file, "current_iteration: %d, maximum_iteration: %d\n", &((*nms)->current_iteration), &((*nms)->maximum_iteration));
	if (scanned < 2) return -1;
	scanned = fscanf(file, "current_evaluation: %d, evaluations_per_iteration: %d\n", &((*nms)->current_evaluation), &((*nms)->evaluations_per_iteration));
	if (scanned < 2) return -1;

	np = (*nms)->number_parameters;
	(*nms)->min_parameters = (double*)malloc(sizeof(double) * np);
	(*nms)->max_parameters = (double*)malloc(sizeof(double) * np);
	for (i = 0; i < np; i++) {
		(*nms)->min_parameters[i] = (*nms)->parameters[i] - (*nms)->parameter_range[i];
		(*nms)->max_parameters[i] = (*nms)->parameters[i] + (*nms)->parameter_range[i];
	}
	return 1;
}

int read_newton_method(char* file, NEWTON_METHOD_SEARCH **nms) {
	int result;
	FILE *f = fopen(file, "r");
	if (f == NULL) return -1;
	else result = fread_newton_method(f, nms);
	fclose(f);
	return result;
}

int init_newton_method(char* search_name, SEARCH* search) {
	NEWTON_METHOD_SEARCH* nms;
	char search_file[FILENAME_SIZE], population_file[FILENAME_SIZE];

	sprintf(search_file, "%s/%s/search", get_working_directory(), search_name);
	read_newton_method(search_file, &nms);

	sprintf(population_file, "%s/%s/population_%d", get_working_directory(), search_name, nms->current_iteration);
	read_population(population_file, (&nms->current_population));
	if (nms->current_population == NULL) new_population(nms->evaluations_per_iteration, nms->number_parameters, &(nms->current_population));
	nms->current_iteration = nms->current_population->size;

	search->search_data = nms;
	search->search_name = search_name;
	search->generate_parameters = newton_generate_parameters;
	search->insert_parameters = newton_insert_parameters;
	return 1;
}
