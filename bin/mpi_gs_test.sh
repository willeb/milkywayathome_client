mpirun -np 38 -machinefile machinefiles/machinefile_38_high ./mpi_astronomy -stars $1 -parameters $2 -asynch -gs -s $3 -gs_type simplex -gs_parents 4 -gs_population_size 100 -no_redundancy
