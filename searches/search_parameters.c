#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "search_parameters.h"
#include "../util/settings.h"

/****
         *     BOINC includes
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
		#include <ctime>
		#include <cstring>
		#include <cstdlib>
		#include <csignal>
		#include <unistd.h>
	#endif

	#include "diagnostics.h"
	#include "util.h"
	#include "filesys.h"
	#include "boinc_api.h"
	#include "mfile.h"

	using std::string;
#endif


void free_search_parameters(SEARCH_PARAMETERS *parameters) {
	free(parameters->search_name);
	free(parameters->parameters);
	free(parameters->metadata);
}

void init_search_parameters(SEARCH_PARAMETERS **p, int number_parameters) {
	(*p) = (SEARCH_PARAMETERS*)malloc(sizeof(SEARCH_PARAMETERS));
	(*p)->search_name = (char*)malloc(sizeof(char) * 1024);
	(*p)->metadata = (char*)malloc(sizeof(char) * METADATA_SIZE);
	(*p)->number_parameters = number_parameters;
	(*p)->parameters = (double*)malloc(sizeof(double) * number_parameters);
}

void set_search_parameters(SEARCH_PARAMETERS *p, char *search_name, int number_parameters, double* parameters, char* metadata) {
	strcpy(p->search_name, search_name);
	strcpy(p->metadata, metadata);

	if (p->number_parameters != number_parameters) {
		p->number_parameters = number_parameters;
		p->parameters = (double*)realloc(p->parameters, sizeof(double) * number_parameters);
	}
	memcpy(p->parameters, parameters, sizeof(double) * number_parameters);
}

int fread_search_parameters(FILE* file, SEARCH_PARAMETERS *p) {
	int i, c, number_parameters;
	if (fscanf(file, "%s\n", p->search_name) != 1) return 1;
	if (fscanf(file, "parameters [%d]:", &number_parameters) != 1) return 1;

	if (p->number_parameters != number_parameters) {
		p->number_parameters = number_parameters;
		p->parameters = (double*)realloc(p->parameters, sizeof(double) * number_parameters);
	}
	for (i = 0; i < number_parameters; i++) {
		if (fscanf(file, " %lf", &(p->parameters[i])) != 1) return 1;
	}
	if (fscanf(file, "\n") != 0) return 1;

	fscanf(file, "metadata: ");
	c = fgetc(file);
	i = 0;
	while (i < METADATA_SIZE && c != '\n' && c != '\0') {
		if (c == 13) {
			c = fgetc(file);
			continue;
		}
		p->metadata[i] = c;
		c = fgetc(file);
		i++;
	}
	p->metadata[i] = '\0';
	if (c == '\0' || i == METADATA_SIZE) return 1;
	return 0;
}

int fwrite_search_parameters(FILE* file, SEARCH_PARAMETERS *parameters) {
	int i;
	if (fprintf(file, "%s\n", parameters->search_name) < 0) return 1;
	if (fprintf(file, "parameters [%d]:", parameters->number_parameters) < 0) return 1;

	for (i = 0; i < parameters->number_parameters; i++) {
		if (fprintf(file, " %.15lf", parameters->parameters[i]) < 0) return 1;
	}
	if (fprintf(file, "\n") < 0) return 1;
	if (fprintf(file, "metadata: %s\n", parameters->metadata) < 0) return 1;

	return 0;
}

int read_search_parameters(const char* filename, SEARCH_PARAMETERS *parameters) {
	int retval;
	FILE* data_file = fopen(filename, "r");

	if (!data_file) {
		fprintf(stderr, "Error reading search parameters: Couldn't find input file %s.\n", filename);
		return 1;
	}
        
	retval = fread_search_parameters(data_file, parameters);
	fclose(data_file);
	return retval;
}

int write_search_parameters(const char* filename, SEARCH_PARAMETERS *parameters) {
	int retval;
	FILE* data_file = fopen(filename, "w+");

	if (!data_file) {
		fprintf(stderr, "Error writing search parameters: Couldn't find input file %s.\n", filename);
		return 1;
	}
        
	retval = fwrite_search_parameters(data_file, parameters);
	fclose(data_file);
	return retval;
}

#ifdef GMLE_BOINC
	int boinc_read_search_parameters(const char* filename, SEARCH_PARAMETERS* parameters) {
		char input_path[512];
		int retval = boinc_resolve_filename(filename, input_path, sizeof(input_path));
		if (retval) {
			fprintf(stderr, "APP: error resolving search parameters file (for read): %d\n", retval);
			fprintf(stderr, "\tfilename: %s\n", filename);
			fprintf(stderr, "\tresolved input path: %s\n", input_path);
			return retval;
		}

		FILE* data_file = boinc_fopen(input_path, "r");
		if (data_file == NULL) {
			fprintf(stderr, "APP: error reading search parameters file (for read): data_file == NULL\n");
			return 1;
		}
		retval = fread_search_parameters(data_file, parameters);
		fscanf(data_file, "fitness: %lf\n", &(parameters->fitness));
		fclose(data_file);
		return retval;
	}

	int boinc_write_search_parameters(const char* filename, SEARCH_PARAMETERS* parameters, double fitness) {
		char output_path[512];
		int retval = boinc_resolve_filename(filename, output_path, sizeof(output_path));
		if (retval) {
			fprintf(stderr, "APP: error resolving search parameters file (for write): %d\n", retval);
			fprintf(stderr, "\tfilename: %s\n", filename);
			fprintf(stderr, "\tresolved output path: %s\n", output_path);
			return retval;
		}

		FILE* data_file = boinc_fopen(output_path, "w");
		retval = fwrite_search_parameters(data_file, parameters);
		fprintf(data_file, "fitness: %0.15lf\n", fitness);
		fprintf(data_file, "stock: 0.9\n");
		fclose(data_file);
		return retval;
	}
#endif
