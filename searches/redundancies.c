#include "redundancies.h"
#include "../util/settings.h"

#include "time.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "string.h"

#include "../../mersenne_twister/dSFMT.h"

void new_redundancy(double fitness, int hostid, REDUNDANCY **redundancy) {
	(*redundancy) = (REDUNDANCY*)malloc(sizeof(REDUNDANCY));
	(*redundancy)->fitness = fitness;
	(*redundancy)->hostid = hostid;
	(*redundancy)->next = NULL;
}

void free_redundancy_list(REDUNDANCY_LIST **redundancy_list) {
	REDUNDANCY *current_redundancy, *temp;

	current_redundancy = (*redundancy_list)->first;
	while (current_redundancy != NULL) {
		temp = current_redundancy;
		current_redundancy = current_redundancy->next;
		temp->next = NULL;
		free(temp);
	}
	(*redundancy_list)->next = NULL;
	(*redundancy_list)->first = NULL;

	if ( (*redundancy_list)->parameters != NULL) {
		free( (*redundancy_list)->parameters );
	}
	if ( (*redundancy_list)->metadata != NULL) {
		free( (*redundancy_list)->metadata );
	}
	free( (*redundancy_list) );
}

void new_redundancy_list(int number_parameters, double fitness, double *parameters, char *metadata, int hostid, REDUNDANCY_LIST **redundancy_list) {
	int i;
	(*redundancy_list) = (REDUNDANCY_LIST*)malloc(sizeof(REDUNDANCY_LIST));
	(*redundancy_list)->first = NULL;
	(*redundancy_list)->next = NULL;

	(*redundancy_list)->number_parameters = number_parameters;
	(*redundancy_list)->parameters = (double*)malloc(sizeof(double) * number_parameters);
	for (i = 0; i < number_parameters; i++) (*redundancy_list)->parameters[i] = parameters[i];

	(*redundancy_list)->metadata = (char*)malloc(sizeof(char) * METADATA_SIZE);
	memcpy((*redundancy_list)->metadata, metadata, sizeof(char) * METADATA_SIZE);


	new_redundancy(fitness, hostid, &((*redundancy_list)->first));
}

void initialize_redundancies(int search_size, double redundancy_rate, REDUNDANCIES **redundancies) {
	(*redundancies) = (REDUNDANCIES*)malloc(sizeof(REDUNDANCIES));

	(*redundancies)->search_size = search_size;
	(*redundancies)->redundancy_rate = redundancy_rate;
	(*redundancies)->number_redundancies = 0 ;

	(*redundancies)->redundancy_list = NULL;
	(*redundancies)->last_list = NULL;

	dsfmt_gv_init_gen_rand((int)time(NULL));
}


int fwrite_redundancies(FILE *file, REDUNDANCIES *redundancies) {
	int i;
	REDUNDANCY_LIST *current_list;
	REDUNDANCY *current_redundancy;

	current_list = redundancies->redundancy_list;
	fprintf(file, "search_size: %d, redundancy_rate: %.15lf, number_redundancies: %d\n", redundancies->search_size, redundancies->redundancy_rate, redundancies->number_redundancies);
	fprintf(file, "redundancies:\n");
	while (current_list != NULL) {
		fprintf(file, "[%d] [%.20lf", current_list->number_parameters, current_list->parameters[0]);
		for (i = 1; i < current_list->number_parameters; i++) fprintf(file, " %.20lf", current_list->parameters[i]);
		fprintf(file, "] [%s] ", current_list->metadata);

		current_redundancy = current_list->first;
		while (current_redundancy != NULL) {
			fprintf(file, "[%.20lf %d]", current_redundancy->fitness, current_redundancy->hostid);
			current_redundancy = current_redundancy->next;
		}
		fprintf(file, "\n");
		current_list = current_list->next;
	}
	fprintf(file, "finished\n");
	return 0;
}

int fread_redundancy(FILE *file, REDUNDANCY **redundancy) {
	(*redundancy) = (REDUNDANCY*)malloc(sizeof(REDUNDANCY));
	(*redundancy)->next = NULL;

	if (2 == fscanf(file, "[%lf %d]", &((*redundancy)->fitness), &((*redundancy)->hostid))) {
		return 1;
	} else {
		free(*redundancy);
		*redundancy = NULL;
		return 0;
	}
}

int fread_redundancy_list(FILE *file, REDUNDANCY_LIST **redundancy_list) {
	int i;
	char c;
	REDUNDANCY *current_redundancy;

	(*redundancy_list) = (REDUNDANCY_LIST*)malloc(sizeof(REDUNDANCY_LIST));
	(*redundancy_list)->parameters = NULL;
	(*redundancy_list)->metadata = NULL;
	(*redundancy_list)->first = NULL;
	(*redundancy_list)->next = NULL;

	if (1 != fscanf(file, "[%d] [", &((*redundancy_list)->number_parameters))) {
		free_redundancy_list(redundancy_list);
		(*redundancy_list) = NULL;
		return 0;
	}

	(*redundancy_list)->parameters = (double*)malloc(sizeof(double) * (*redundancy_list)->number_parameters);
	for (i = 0; i < (*redundancy_list)->number_parameters; i++) {
		assert(1 == fscanf(file, "%lf", &((*redundancy_list)->parameters[i])));
		fgetc(file);
	}
	fscanf(file, " [");

	(*redundancy_list)->metadata = (char*)malloc(sizeof(char) * METADATA_SIZE);
	c = fgetc(file);
	i = 0;
	while (c != ']' && i < METADATA_SIZE) {
		(*redundancy_list)->metadata[i] = c;
		i++;
		c = fgetc(file);
	}
	(*redundancy_list)->metadata[i] = '\0';
	fgetc(file);

	fread_redundancy(file, &current_redundancy);
	(*redundancy_list)->first = current_redundancy;
	while (current_redundancy != NULL) {
		fread_redundancy(file, &(current_redundancy->next));
		current_redundancy = current_redundancy->next;
	}
	fscanf(file, "\n");
	return 0;
}


int fread_redundancies(FILE *file, REDUNDANCIES **redundancies) {
	REDUNDANCY_LIST *current_list;
	int search_size, number_redundancies;
	double redundancy_rate;

	fscanf(file, "search_size: %d, redundancy_rate: %lf, number_redundancies: %d\n", &search_size, &redundancy_rate, &number_redundancies);

	initialize_redundancies(search_size, redundancy_rate, redundancies);
	(*redundancies)->number_redundancies = number_redundancies;

	fscanf(file, "redundancies:\n");

	fread_redundancy_list(file, &current_list);
	(*redundancies)->redundancy_list = current_list;

	while (current_list != NULL) {
		(*redundancies)->last_list = current_list;
		fread_redundancy_list(file, &(current_list->next));
		current_list = current_list->next;
	}

	fscanf(file, "finished\n");

	printf("read the redundancies:\n");
	fwrite_redundancies(stdout, (*redundancies));

	return 0;
}

int read_redundancies(char *filename, REDUNDANCIES **redundancies) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) return -1;

	fread_redundancies(file, redundancies);
	fclose(file);
	return 0;
}

int write_redundancies(char *filename, REDUNDANCIES *redundancies) {
	FILE *file = fopen(filename, "w");
	if (file == NULL) return -1;

	fwrite_redundancies(file, redundancies);
	fclose(file);
	return 0;
}


int generate_redundancy(REDUNDANCIES *redundancies, int number_parameters, double *parameters, char *metadata) {
	int i;
	if (redundancies->redundancy_list == NULL) return 0;
	assert(redundancies->last_list != NULL);

	if (redundancies->redundancy_rate == 0) return 0;
	else if (redundancies->redundancy_rate == 1 && dsfmt_gv_genrand_close_open() >= (((double)redundancies->search_size)/((double)redundancies->number_redundancies))) return 0;
	else if (dsfmt_gv_genrand_close_open() >= redundancies->redundancy_rate) return 0;

	for (i = 0; i < number_parameters; i++) parameters[i] = redundancies->redundancy_list->parameters[i];
	memcpy(metadata, redundancies->redundancy_list->metadata, sizeof(char) * METADATA_SIZE);

	redundancies->last_list->next = redundancies->redundancy_list;			//set last->next to the first
	redundancies->redundancy_list = redundancies->redundancy_list->next;		//set first to first->next
	redundancies->last_list = redundancies->last_list->next;			//set last to last->next
	redundancies->last_list->next = NULL;						//set last->next to null
	return 1;
}

void append_redundancy(REDUNDANCY_LIST *redundancy_list, double fitness, int hostid) {
	REDUNDANCY *current;
	int count;

	if (redundancy_list->first == NULL) {
		new_redundancy(fitness, hostid, &(redundancy_list->first));
		return;
	}

	count = 0;
	current = redundancy_list->first;
	while (current->next != NULL) {
		count++;
		if (count > 1000) {
			printf("count > 1000 in append redundnacy\n");
			exit(0);
		}
		current = current->next;
	}
	new_redundancy(fitness, hostid, &(current->next));
}


int redundancy_match(REDUNDANCY_LIST *current, int number_parameters, double fitness, double *parameters, int hostid) {
	int i, count;
	REDUNDANCY *current_redundancy;

	if (number_parameters != current->number_parameters) return REDUNDANCY_NOT_EQUAL;
	
	for (i = 0; i < current->number_parameters; i++) {
//		printf("comparing parameters[%d]: %.20lf %.20lf\n", i, parameters[i], current->parameters[i]);
		if (fabs(parameters[i] - current->parameters[i]) > PARAMETER_THRESHOLD) {
			return REDUNDANCY_NOT_EQUAL;
		}
	}

	count = 0;
	current_redundancy = current->first;
	while (current_redundancy != NULL) {
		count++;
		if (count > 1000) {
			printf("count > 1000 in redundancy_match\n");
			exit(0);
		}

//		printf("comparing hostid: %d %d, fitness: %.20lf %.20lf\n", hostid, current_redundancy->hostid, fitness, current_redundancy->fitness);
		if (hostid == current_redundancy->hostid) return REDUNDANCY_DUPLICATE;
		else if (fabs(fitness - current_redundancy->fitness) < FITNESS_THRESHOLD) {
			return REDUNDANCY_MATCH;
		}
		current_redundancy = current_redundancy->next;
	}

	return REDUNDANCY_MISMATCH;
}


int verify_with_insert(REDUNDANCIES *redundancies, int number_parameters, double fitness, double *parameters, char *metadata, int hostid) {
	REDUNDANCY_LIST *current;
	REDUNDANCY_LIST *previous;
	REDUNDANCY_LIST *new_redundancy;
	int count;

	if (redundancies->redundancy_rate == 0) return VERIFY_VALID;

	current = redundancies->redundancy_list;
	if (current == NULL) {
		new_redundancy_list(number_parameters, fitness, parameters, metadata, hostid, &(redundancies->redundancy_list));
		redundancies->last_list = redundancies->redundancy_list;
		redundancies->number_redundancies++;
		return VERIFY_INSERT;
	}

	previous = NULL;

	count = 0;
	while (current != NULL) {
		count++;
		if (count > 5000) {
			printf("OVER 1000 IN VERIFY WITH INSERT LOOP\n");
			exit(0);
		}
		int match = redundancy_match(current, number_parameters, fitness, parameters, hostid);
		if (match == REDUNDANCY_MATCH) {
			/**
			 * Remove the redundancy
			 */
			if (previous == NULL) redundancies->redundancy_list = current->next;
			else previous->next = current->next;

			if (current->next == NULL) redundancies->last_list = previous;
			free_redundancy_list(&current);

			redundancies->number_redundancies--;

			return VERIFY_VALID;
		} else if (match == REDUNDANCY_MISMATCH) {
			/**
			 * Append to the redundancy list
			 */
			append_redundancy(current, fitness, hostid);

			return VERIFY_IN_PROGRESS;
		} else if (match == REDUNDANCY_DUPLICATE) {
			/**
			 * do nothing 
			 */
			return VERIFY_IN_PROGRESS;
		} else if (match == REDUNDANCY_NOT_EQUAL) {
			/**
			 *	Keep iterating
			 */
		}
		previous = current;
		current = current->next;
	}

	new_redundancy_list(number_parameters, fitness, parameters, metadata, hostid, &(new_redundancy));
	previous->next = new_redundancy;
	redundancies->last_list = new_redundancy;
	redundancies->number_redundancies++;

	return VERIFY_INSERT;
}

int verify_without_insert(REDUNDANCIES *redundancies, int number_parameters, double fitness, double *parameters, char *metadata, int hostid) {
	REDUNDANCY_LIST *current;
	REDUNDANCY_LIST *previous;
	int count;

	if (redundancies->redundancy_rate == 0) return VERIFY_VALID;

	previous = NULL;
	current = redundancies->redundancy_list;

	count = 0;
	while (current != NULL) {
		count++;
		if (count > 5000) {
			printf("OVER 1000 IN VERIFY WITHOUT INSERT LOOP\n");
			exit(0);
		}
		int match = redundancy_match(current, number_parameters, fitness, parameters, hostid);
		if (match == REDUNDANCY_MATCH) {
			/**
			 * Remove the redundancy
			 */
			if (previous == NULL) redundancies->redundancy_list = current->next;
			else previous->next = current->next;

			if (current->next == NULL) redundancies->last_list = previous;
			free_redundancy_list(&current);

			redundancies->number_redundancies--;

			return VERIFY_VALID;
		} else if (match == REDUNDANCY_MISMATCH) {
			/**
			 * Append to the redundancy list
			 */
			append_redundancy(current, fitness, hostid);

			return VERIFY_IN_PROGRESS;
		} else if (match == REDUNDANCY_DUPLICATE) {
			/**
			 * do nothing 
			 */
			return VERIFY_IN_PROGRESS;
		} else if (match == REDUNDANCY_NOT_EQUAL) {
			/**
			 *	Keep iterating
			 */
		}
		previous = current;
		current = current->next;
	}

	return VERIFY_NOT_INSERTED;
}
