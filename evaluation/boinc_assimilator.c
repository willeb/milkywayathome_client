// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// This is a framework for an assimilator.
// You need to link with with an (application-specific) function
// assimilate_handler()
// in order to make a complete program.
//

#include "config.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <vector>

#include "boinc_db.h"
#include "parse.h"
#include "util.h"
#include "str_util.h"

#include "sched_config.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "assimilate_handler.h"

#include "time.h"

/********
	*	FGDO includes
 ********/
#include "search_manager.h"
#include "boinc_add_wu.h"
#include "../searches/search_parameters.h"


using std::vector;

#define LOCKFILE "assimilator.out"
#define PIDFILE  "assimilator.pid"

SCHED_CONFIG config;
bool update_db = true;
bool noinsert = false;

int wu_id_modulus=0, wu_id_remainder=0;

#define SLEEP_INTERVAL 10

int sleep_interval = SLEEP_INTERVAL;

int one_pass_N_WU=0;

time_t start_time = -1;
long processed_wus = 0;
const int unsent_wu_buffer = 400;
const int wus_to_generate = 500;


// assimilate all WUs that need it
// return nonzero if did anything
//
bool do_pass(APP& app) {
	DB_WORKUNIT wu;
	DB_RESULT canonical_result, result;
	bool did_something = false;
	char buf[256];
	char mod_clause[256];
	int retval;
	int num_assimilated = 0;

	check_stop_daemons();

	if (wu_id_modulus) {
		sprintf(mod_clause, " and workunit.id %% %d = %d ", wu_id_modulus, wu_id_remainder);
	} else {
		strcpy(mod_clause, "");
	}

	sprintf(buf, "where appid=%d and assimilate_state=%d %s limit %d", app.id, ASSIMILATE_READY, mod_clause, one_pass_N_WU ? one_pass_N_WU : 1000);

	/********
		*	FGDO: Track the start time
	 ********/
	if (start_time == -1) {
		time(&start_time);
		log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "Starting at: %d\n", (int)start_time);
	}

	while (!wu.enumerate(buf)) {
		vector<RESULT> results;     // must be inside while()!

		// for testing purposes, pretend we did nothing
		if (update_db) {
			did_something = true;
		}
		log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG, "[%s] assimilating boinc WU %d; state=%d\n", wu.name, wu.id, wu.assimilate_state);

		sprintf(buf, "where workunitid=%d", wu.id);
		while (!result.enumerate(buf)) {
			results.push_back(result);
			if (result.id == wu.canonical_resultid) {
				canonical_result = result;
			}
		}

		retval = assimilate_handler(wu, results, canonical_result);
		if (retval) {
			log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "[%s] handler returned error %d; exiting\n", wu.name, retval);
			exit(retval);
		}

		if (update_db) {
			sprintf(buf, "assimilate_state=%d, transition_time=%d", ASSIMILATE_DONE, (int)time(0));

			retval = wu.update_field(buf);
			if (retval) {
				log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "[%s] update failed: %d\n", wu.name, retval);
				exit(1);
			}
		}
		num_assimilated++;
	}

    
	if (did_something) boinc_db.commit_transaction();

	/********
		*	FGDO: Generate new workunits if any WUs were assimilated.
	 ********/
	if (num_assimilated)  {
		log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "Assimilated %d workunits.\n", num_assimilated);
		int unsent_wus;
		count_unsent_results(unsent_wus, 0);

		processed_wus += num_assimilated;
		time_t current_time;
		time(&current_time);
		double wus_per_second = (double)processed_wus/((double)current_time-(double)start_time);
		log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "wus/sec: %lf, unsent wus: %d\n", wus_per_second, unsent_wus);

		if (unsent_wus < unsent_wu_buffer) {
			SEARCH_PARAMETERS **parameters;
			int num_generated = generate_workunits(wus_to_generate, &parameters);
			for (int i = 0; i < num_generated; i++) {
				add_workunit(parameters[i]);
				free_search_parameters(parameters[i]);
				free(parameters[i]);
			}
			free(parameters);

			log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "Generated %d new workunits.\n", num_generated);
		}
	}
	return did_something;
}

int main(int argc, char** argv) {
	int retval;
	bool one_pass = false;
	DB_APP app;
	int i;
	char buf[256];

	check_stop_daemons();
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-one_pass_N_WU")) {
			one_pass_N_WU = atoi(argv[++i]);
			one_pass = true;
		} else if (!strcmp(argv[i], "-sleep_interval")) {
			sleep_interval = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-one_pass")) {
			one_pass = true;
		} else if (!strcmp(argv[i], "-d")) {
			log_messages.set_debug_level(atoi(argv[++i]));
		} else if (!strcmp(argv[i], "-app")) {
			strcpy(app.name, argv[++i]);
		} else if (!strcmp(argv[i], "-dont_update_db")) {
			// This option is for testing your assimilator.  When set,
			// it ensures that the assimilator does not actually modify
			// the assimilate_state of the workunits, so you can run
			// your assimilator over and over again without affecting
			// your project.
			update_db = false;
		} else if (!strcmp(argv[i], "-noinsert")) {
			// This option is also for testing and is used to 
			// prevent the inserting of results into the *backend*
			// (as opposed to the boinc) DB.
			noinsert = true;
		} else if (!strcmp(argv[i], "-mod")) {
			wu_id_modulus   = atoi(argv[++i]);
			wu_id_remainder = atoi(argv[++i]);
		} else {
			log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "Unrecognized arg: %s\n", argv[i]);
		}
	}

	if (wu_id_modulus) {
		log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG, "Using mod'ed WU enumeration.  modulus = %d  remainder = %d\n", wu_id_modulus, wu_id_remainder);
	}

	retval = config.parse_file("..");
	if (retval) {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "Can't parse ../config.xml: %s\n", boincerror(retval));
		exit(1);
	}

	log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "Starting\n");

	retval = boinc_db.open(config.db_name, config.db_host, config.db_user, config.db_passwd);
	if (retval) {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "Can't open DB\n");
		exit(1);
	}
	sprintf(buf, "where name='%s'", app.name);
	retval = app.lookup(buf);
	if (retval) {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "Can't find app\n");
		exit(1);
	}

	install_stop_signal_handler();
	init_workunit_generator(app, config);
	while (1) {
		if (!do_pass(app)) {
			if (one_pass) break;
			sleep(sleep_interval);
		}
	}
}


const char *BOINC_RCSID_7841370789 = "$Id: boinc_assimilator.c,v 1.1 2008/10/30 21:07:02 deselt Exp $";