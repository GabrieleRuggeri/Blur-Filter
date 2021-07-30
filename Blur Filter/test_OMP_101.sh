#!/bin/bash

#strong scalability

#OMP VERSION

cd $PBS_O_WORKDIR
#size = 101
for (( procs = 1 ; procs <= 24 ; procs += 1 )); do
   export OMP_NUM_THREADS=${procs}
   echo "executing on ", ${procs}, "  threads" 
   /usr/bin/time 2>>omp.strong.size.101.threads.${procs} ./blur.omp 1 101 0.2 earth-large.pgm >omp.strong.size.101.threads.${procs}
done
