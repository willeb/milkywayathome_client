mpirun -np 32 -machinefile machinefiles/mpi_machinefile ./mpi_astronomy -synch -stars stars-$1.txt -parameters parameters-$1.txt -gd -gd_min_threshold 0.00001 -gd_iterations $2
