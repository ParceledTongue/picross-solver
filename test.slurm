#!/bin/bash
#SBATCH --nodes=1
#SBATCH --cpus-per-task=4
#SBATCH --output=out.txt 
nproc=$(expr $SLURM_JOB_CPUS_PER_NODE \* $SLURM_NNODES)
echo $nproc cores available
cp -r src puzzles makefile $PFSDIR/.
cd $PFSDIR
make
./bin/solver puzzles/birds.txt
