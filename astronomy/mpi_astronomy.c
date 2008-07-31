/********
	*	Includes for astronomy
 ********/
#include "mpi.h"

#include "parameters.h"
#include "star_points.h"
#include "evaluation.h"

#include "../searches/synchronous_search.h"
#include "../searches/genetic_search.h"
#include "../searches/differential_evolution.h"
#include "../searches/particle_swarm.h"
#include "../searches/newton_method.h"
#include "../evaluation/mpi_evaluator.h"
#include "../evaluation/evaluator.h"

#define max_iterations			35000
#define astronomy_parameters_file	"parameters.txt"
#define star_points_file		"stars_convolved_82.txt"
#define population_file_name		"population.txt"

ASTRONOMY_PARAMETERS *ap;
STAR_POINTS *sp;
EVALUATION_STATE *es;
int total_number_stars;

void read_data(int rank, int max_rank) {
	/********
		*	READ THE ASTRONOMY PARAMETERS
	 ********/
	int retval = read_astronomy_parameters(astronomy_parameters_file, ap);
	if (retval) {
		fprintf(stderr, "APP: error reading astronomy parameters: %d\n", retval);
		exit(1);
	}
	split_astronomy_parameters(ap, rank, max_rank);

	/********
		*	READ THE STAR POINTS
	 ********/
	retval = read_star_points(star_points_file, sp);
	if (retval) {
		fprintf(stderr, "APP: error reading star points: %d\n", retval);
		exit(1);
	}
	total_number_stars = sp->number_stars;
	split_star_points(sp, rank, max_rank);

	/********
		*	INITIALIZE THE EVALUATION STATE
	 ********/
	initialize_state(es, ap->number_streams);
}

double* integral_f(double* parameters) {
	int i;
	/********
		*	CALCULATE THE INTEGRALS
	 ********/
	set_astronomy_parameters(ap, parameters);

	es->r_step_current = 0;
	es->mu_step_current = 0;
	es->nu_step_current = 0;

	es->background_integral = 0.0;
	for (i = 0; i < es->number_streams; i++) {
		es->stream_integrals[i] = 0.0;
	}
	es->current_star_point = 0;
	es->num_zero = 0;
	es->bad_jacobians = 0;
	es->prob_sum = 0.0;

	int retval = calculate_integrals(ap, es, sp);
	if (retval) {
		fprintf(stderr, "APP: error calculating integrals: %d\n", retval);
		exit(retval);
	}
	double *results = (double*)malloc(sizeof(double) * 2);
	results[0] = es->background_integral;
	results[1] = es->stream_integrals[0];
//	printf("calculated integrals: %lf, %lf\n", results[0], results[1]);
	return results;
}

double* integral_compose(double* integral_results, int num_results) {
	int i;
	double *results = (double*)malloc(sizeof(double) * 2);
	results[0] = 0.0;
	results[1] = 0.0;
	for (i = 0; i < num_results; i++) {
		results[0] += integral_results[(2*i)];
		results[1] += integral_results[(2*i)+1];
	}
//	printf("composed integrals: %lf, %lf\n", results[0], results[1]);
	return results;
}

double* likelihood_f(double* integrals) {
//	printf("calculating likelihood function\n");
	es->background_integral = integrals[0];
	es->stream_integrals[0] = integrals[1];
	/********
		*	CALCULATE THE LIKELIHOOD
	 ********/
	int retval = calculate_likelihood(ap, es, sp);
	if (retval) {
		fprintf(stderr, "APP: error calculating likelihood: %d\n", retval);
		exit(retval);
	}
	double *results = (double*)malloc(sizeof(double) * 2);
	results[0] = es->prob_sum;
	results[1] = es->bad_jacobians;
//	printf("calculated likelihood: %lf, bad_jacobs: %lf\n", results[0], results[1]);
	return results;
}

double likelihood_compose(double* results, int num_results) {
	double prob_sum = 0.0;
	double bad_jacobians = 0.0;
	int i;
	for (i = 0; i < num_results; i++) {
		prob_sum += results[(2*i)];
		bad_jacobians += results[(2*i)+1];
	}
	prob_sum /= (total_number_stars - bad_jacobians);
	prob_sum *= -1;
//	printf("composed likelihood: %lf\n", prob_sum);
	return prob_sum;
}

int main(int number_arguments, char **arguments){
	int integral_parameter_length, integral_results_length;
	int likelihood_parameter_length, likelihood_results_length;
	double *min_parameters;
	double *max_parameters;
	double *point;
	double *step;

	integral_parameter_length = 8;
	integral_results_length = 2;
	likelihood_parameter_length = 2;
	likelihood_results_length = 2;

	evaluator__init_data(read_data);
	evaluator__init_integral(integral_f, integral_parameter_length, integral_compose, integral_results_length);
	evaluator__init_likelihood(likelihood_f, likelihood_parameter_length, likelihood_compose, likelihood_results_length);
	mpi_evaluator__start(number_arguments, arguments);

	get_min_parameters(ap, &min_parameters);
	get_max_parameters(ap, &max_parameters);
	get_search_parameters(ap, &point);
	get_step(ap, &step);

        if (arguments[2][0] == 'g') {
                synchronous_search(arguments[1], arguments[2], min_parameters, max_parameters, ap->number_parameters, start_genetic_search);
        } else if (arguments[2][0] == 'd') {
                synchronous_search(arguments[1], arguments[2], min_parameters, max_parameters, ap->number_parameters, start_differential_evolution);
        } else if (arguments[2][0] == 'p') {
                synchronous_search(arguments[1], arguments[2], min_parameters, max_parameters, ap->number_parameters, start_particle_swarm);
        } else if (arguments[2][0] == 'n') {
                synchronous_newton_method(arguments[1], arguments[2], point, step, ap->number_parameters);
        }

	return 0;
}
