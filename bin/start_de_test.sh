sudo -u boinc ./milkyway_start_search -app milkyway -cwd /export/www/boinc/milkyway/results/milkyway -mw_stars $1 -mw_parameters $2 -s $3 -population_size 50 -redundancy_rate $4 -pair_weight 0.5 -crossover_rate 0.5 -recombination_pairs $5 -recombination_type binomial -parent_type $6 -gen 100