/*
Copyright 2008, 2009 Travis Desell, Dave Przybylo, Nathan Cole,
Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail
and Rensselaer Polytechnic Institute.

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

#ifndef FGDO_SEARCH_PARAMETERS_H
#define FGDO_SEARCH_PARAMETERS_H

#include <stdio.h>

typedef struct search_parameters {
	char *search_name;

	int number_parameters;
	double* parameters;
	char* metadata;
	char app_version[128];
	char host_os[256];

	int has_result;
	double fitness;
} SEARCH_PARAMETERS;

void free_search_parameters(SEARCH_PARAMETERS *parameters);
void init_search_parameters(SEARCH_PARAMETERS **p, int number_parameters);
void set_search_parameters(SEARCH_PARAMETERS *p, char *search_name, int number_parameters, double* parameters, char* metadata);

int fread_search_parameters(FILE* file, SEARCH_PARAMETERS *parameters);
int fwrite_search_parameters(FILE* file, SEARCH_PARAMETERS *parameters);

int read_search_parameters(const char* filename, SEARCH_PARAMETERS *parameters);
int write_search_parameters(const char* filename, SEARCH_PARAMETERS *parameters);

#ifdef BOINC_APPLICATION 
	int boinc_read_search_parameters(const char* filename, SEARCH_PARAMETERS* parameters);
	int boinc_read_search_parameters2(const char* filename, SEARCH_PARAMETERS* parameters);
	int boinc_write_search_parameters(const char* filename, SEARCH_PARAMETERS* parameters, double fitness);
#endif

#endif
