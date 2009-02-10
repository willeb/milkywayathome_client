#ifndef FGDO_SEARCH_H
#define FGDO_SEARCH_H

#include "../searches/search_parameters.h"

/********
	*	Generic search data.
 ********/
#define AS_CREATE_SUCCESS 0;
#define AS_CREATE_FAIL 1;

extern char AS_VERIFY_MSG[1024];
#define AS_VERIFY_VALID 0
#define AS_VERIFY_INVALID 1
#define AS_VERIFY_IN_PROGRESS 2

extern const char *AS_INSERT_STR[];
#define AS_INSERT_SUCCESS 0
#define AS_INSERT_OVER 1
#define AS_INSERT_FITNESS_NAN 2
#define AS_INSERT_FITNESS_INVALID 3
#define AS_INSERT_PARAMETERS_NAN 4
#define AS_INSERT_OUT_OF_BOUNDS 5
#define AS_INSERT_ERROR 6
#define AS_INSERT_OUT_OF_RANGE 7
#define AS_INSERT_OUT_OF_ITERATION 8
#define AS_INSERT_BAD_METADATA 9
#define AS_INSERT_NOT_UNIQUE 10
#define AS_INSERT_OUTLIER 11
#define AS_INSERT_INVALID_METADATA 12

extern const char *AS_GEN_STR[];
#define AS_GEN_SUCCESS 0
#define AS_GEN_OVER 1
#define AS_GEN_ERROR 2
#define AS_GEN_FAIL 3

extern const char *AS_CP_STR[];
#define AS_CP_SUCCESS 0
#define AS_CP_OVER 1
#define AS_CP_ERROR 2

#define AS_READ_SUCCESS 0
#define AS_READ_ERROR 1

extern char AS_MSG[1024];

typedef int (*create_search_type)(char*, int, char**, int, double*, double*, double*, double*);
typedef int (*read_search_type)(char*, void**);
typedef int (*checkpoint_search_type)(char*, void*);
typedef int (*generate_parameters_type)(char*, void*, SEARCH_PARAMETERS*);
typedef int (*insert_parameters_type)(char*, void*, SEARCH_PARAMETERS*);

typedef struct asynchronous_search {
	char*				search_qualifier;

	create_search_type		create_search;
	read_search_type		read_search;
	checkpoint_search_type		checkpoint_search;
	generate_parameters_type	generate_parameters;
	insert_parameters_type		insert_parameters;
} ASYNCHRONOUS_SEARCH;

void asynchronous_search__init(int number_arguments, char** arguments, int number_parameters, double *point, double *range, double *min_bound, double *max_bound);

void asynchronous_search(int number_arguments, char** arguments, int number_parameters, double *point, double *range, double *min_bound, double *max_bound);

#endif
