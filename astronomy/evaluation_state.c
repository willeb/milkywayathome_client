/*
 *  integral_function.c
 *  Astronomy
 *
 *  Created by Travis Desell on 2/21/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

/****
        *       BOINC includes
*****/

#ifdef GMLE_BOINC
	#ifdef _WIN32
		#include "boinc_win.h"
	#else
		#include "config.h"
	#endif

	#ifndef _WIN32
		#include <cstdio>
		#include <cctype>
		#include <cstring>
		#include <cstdlib>
		#include <csignal>
	#endif

	#ifdef BOINC_APP_GRAPHICS
		#include "graphics_api.h"
		#include "graphics_lib.h"
	#endif

	#include "diagnostics.h"
	#include "util.h"
	#include "filesys.h"
	#include "boinc_api.h"
	#include "mfile.h"
#endif

#define CHECKPOINT_FILE "astronomy_checkpoint"

/****
	*	Astronomy includes
*****/
#include <math.h>
#include <time.h>
#include <stdio.h>

#include "evaluation_optimized.h"
#include "parameters.h"
#include "probability.h"
#include "stCoords.h"
#include "atSurveyGeometry.h"
#include "star_points.h"
#include "numericalIntegration.h"
#include "../util/io_util.h"

#ifndef _WIN32
	#define pi M_PI
#else
	#define pi 3.14159265358979323846
#endif

#define deg (180.0/pi)


void fwrite_integral_area(FILE *file, INTEGRAL_AREA *ia) {
	fprintf(file, "mu[min,max,steps,current_step]: %.10lf, %.10lf, %d, %d\n", ia->mu_min, ia->mu_max, ia->mu_steps, ia->mu_step_current);
	fprintf(file, "nu[min,max,steps,current_step]: %.10lf, %.10lf, %d, %d\n", ia->nu_min, ia->nu_max, ia->nu_steps, ia->nu_step_current);
	fprintf(file, " r[min,max,steps,current_step]: %.10lf, %.10lf, %d, %d\n", ia->r_min, ia->r_max, ia->r_steps, ia->r_step_current);
	fprintf(file, "background integral: %.10lf\n", ia->background_integral);
	print_double_array(file, "stream_integrals", ia->number_streams, ia->stream_integrals);
}

void fread_integral_area(FILE *file, INTEGRAL_AREA *ia) {
	int i;

	fscanf(file, "mu[min,max,steps,current_step]: %lf, %lf, %d, %d\n", &(ia->mu_min), &(ia->mu_max), &(ia->mu_steps), &(ia->mu_step_current));
	ia->mu_step_size = (ia->mu_max - ia->mu_min) / ia->mu_steps;
	fscanf(file, "nu[min,max,steps,current_step]: %lf, %lf, %d, %d\n", &(ia->nu_min), &(ia->nu_max), &(ia->nu_steps), &(ia->nu_step_current));
	ia->nu_step_size = (ia->nu_max - ia->nu_min) / ia->nu_steps;
	fscanf(file, " r[min,max,steps,current_step]: %lf, %lf, %d, %d\n", &(ia->r_min), &(ia->r_max), &(ia->r_steps), &(ia->r_step_current));
	ia->r_step_size = (ia->r_max - ia->r_min) / ia->r_steps;
	fscanf(file, "background_integral: %lf\n", &(ia->background_integral));
	fscanf(file, "stream_integrals[%d]: ", &(ia->number_streams));
	for (i = 0; i < ia->number_streams; i++) {
		fscanf(file, "%lf", &(ia->stream_integrals[i]));
		if (i != ia->number_streams-1) fscanf(file, ", ");
	}
}

void initialize_integal_area(INTEGRAL_AREA *ia, double mu_min, double mu_max, int mu_steps, double nu_min, double nu_max, int nu_steps, double r_min, double r_max, int r_steps, int number_streams) {
	int i;

	ia->mu_min		= mu_min;
	ia->mu_max		= mu_max;
	ia->mu_steps		= mu_steps;
	ia->mu_step_current	= 0;
	ia->nu_min		= nu_min;
	ia->nu_max		= nu_max;
	ia->nu_steps		= nu_steps;
	ia->nu_step_current	= 0;
	ia->r_min		= r_min;
	ia->r_max		= r_max;
	ia->r_steps		= r_steps;
	ia->r_step_current	= 0;

	ia->mu_step_size = (ia->mu_max - ia->mu_min) / ia->mu_steps;
	ia->nu_step_size = (ia->nu_max - ia->nu_min) / ia->nu_steps;
	ia->r_step_size = (ia->r_max - ia->r_min) / ia->r_steps;

	ia->number_streams = number_streams;
	ia->background_integral	= 0;
	ia->stream_integrals	= (double*)malloc(sizeof(double) * number_streams);
	for (i = 0; i < number_streams; i++) {
		ia->stream_integrals[i] = 0;
	}
}

void initialize_state(ASTRONOMY_PARAMETERS *ap, STAR_POINTS *sp, EVALUATION_STATE *es) {
	int i;

	es->current_cut = -1;
	es->background_integral = 0;
	es->stream_integrals = (double*)malloc(sizeof(double) * ap->number_streams);
	for (i = 0; i < ap->number_streams; i++) es->stream_integrals[i] = 0;

	es->number_streams = ap->number_streams;
	es->total_stars = sp->number_stars;
	es->current_star_point = 0;
	es->num_zero = 0;
	es->bad_jacobians = 0;
	es->prob_sum = 0;

	es->main_integral = (INTEGRAL_AREA*)malloc(sizeof(INTEGRAL_AREA));
	initialize_integal_area(es->main_integral, ap->mu_min, ap->mu_max, ap->mu_steps, ap->nu_min, ap->nu_max, ap->nu_steps, ap->r_min, ap->r_max, ap->r_steps, ap->number_streams);

	es->number_cuts = ap->number_cuts;
	es->cuts = (INTEGRAL_AREA**)malloc(sizeof(INTEGRAL_AREA*) * ap->number_cuts);
	for (i = 0; i < ap->number_cuts; i++) {
		es->cuts[i] = (INTEGRAL_AREA*)malloc(sizeof(INTEGRAL_AREA));
		initialize_integal_area(es->cuts[i], ap->mu_cut[i][0], ap->mu_cut[i][1], (int)ap->mu_cut[i][2], ap->nu_cut[i][0], ap->nu_cut[i][1], (int)ap->nu_cut[i][2], ap->r_cut[i][0], ap->r_cut[i][1], (int)ap->r_cut[i][2], ap->number_streams);
	}
}

void reset_evaluation_state(EVALUATION_STATE *es) {
	int i, j;

	es->current_cut = -1;
	es->background_integral = 0;
	es->current_star_point = 0;
	es->num_zero = 0;
	es->bad_jacobians = 0;
	es->prob_sum = 0;

	es->main_integral->mu_step_current = 0;
	es->main_integral->nu_step_current = 0;
	es->main_integral->r_step_current = 0;
	es->main_integral->background_integral = 0;
	for (i = 0; i < es->number_streams; i++) {
		es->stream_integrals[i] = 0;
		es->main_integral->stream_integrals[i] = 0;
	}

	for (i = 0; i < es->number_cuts; i++) {
		es->cuts[i]->mu_step_current = 0;
		es->cuts[i]->nu_step_current = 0;
		es->cuts[i]->r_step_current = 0;
		es->cuts[i]->background_integral = 0;
		for (j = 0; j < es->number_streams; j++) {
			es->cuts[i]->stream_integrals[i] = 0;
		}
	}
}

void free_integral_area(INTEGRAL_AREA *ia) {
	free(ia->stream_integrals);
}

void free_state(EVALUATION_STATE* es) {
	int i;
	free(es->stream_integrals);
	free_integral_area(es->main_integral);
	for (i = 0; i < es->number_cuts; i++) {
		free_integral_area(es->cuts[i]);
	}
	free(es->main_integral);
	free(es->cuts);
}

#ifdef GMLE_BOINC
	int write_checkpoint(EVALUATION_STATE* es) {
		int i, retval;
		char output_path[512];
		FILE *file;

		boinc_resolve_filename(CHECKPOINT_FILE, output_path, sizeof(output_path));

		file = boinc_fopen(output_path, "w+");
		if (!file) {
			fprintf(stderr, "APP: error writing checkpoint (opening checkpoint file)\n");
			return 1;
		}

		fprintf(file, "background_integral: %lf\n", es->background_integral);
		fprintf(file, "stream_integrals[%d]: ", es->number_streams);
		for (i = 0; i < es->number_streams; i++) {
			fprintf(file, "%lf", es->stream_integrals[i]);
			if (i != (es->number_streams-1)) fprintf(file, ", ");
		}
		fprintf(file, "\n");

		fprintf(file, "prob_sum: %lf, num_zero: %d, bad_jacobians: %d\n", es->prob_sum, es->num_zero, es->bad_jacobians);
		fprintf(file, "current_star_point: %d\n", es->current_star_point);
		fprintf(file, "current_cut: %d\n", es->current_cut);

		fprintf(file, "main_volume:\n");
		fwrite_integral_area(file, es->main_integral);
		fprintf(file, "cuts: %d\n", es->number_cuts);
		for (i = 0; i < es->number_cuts; i++) {
			fwrite_integral_area(file, es->cuts[i]);
		}

		if ((retval = fclose(file))) {
	                fprintf(stderr, "APP: error writing checkpoint (closing checkpoint file) %d\n", retval);
	                return retval;
		}

		return 0;
	}

	int read_checkpoint(EVALUATION_STATE* es) {
		int i;
		char input_path[512];
		int retval = boinc_resolve_filename(CHECKPOINT_FILE, input_path, sizeof(input_path));
		if (retval) {
			return 0;
		}

	        FILE* file = boinc_fopen(input_path, "r");
	        if (file == NULL) {
	                return 0;
	        }

		if (1 > fscanf(file, "background_integral: %lf\n", &(es->background_integral))) return 1;
		es->number_streams = read_double_array(file, "stream_integrals", &(es->stream_integrals));

		fscanf(file, "prob_sum: %lf, num_zero: %d, bad_jacobians: %d\n", &(es->prob_sum), &(es->num_zero), &(es->bad_jacobians));
		fscanf(file, "current_star_point: %d\n", &(es->current_star_point));
		fscanf(file, "current_cut: %d\n", &(es->current_cut));
		fscanf(file, "main_volume:\n");

		fread_integral_area(file, es->main_integral);

		fscanf(file, "cuts: %d\n", &es->number_cuts);
		for (i = 0; i < es->number_cuts; i++) {
			fread_integral_area(file, es->cuts[i]);
		}

	        fclose(file);
	        return 0;
	}
#endif
