#!/bin/bash

#strong scalability

#MPI VERSION

#size = 101
for (( procs=1; procs<=48; procs++ ));do
   echo "executing on ", ${procs}, "  processors"
   /usr/bin/time 2>>mpi.strong.size.101.proc.${procs} mpirun --mca btl '^openib' -np ${procs} ./blur.mpi 1 101 0.2 earth-large.pgm >mpi.strong.size.101.proc.${procs}
done
