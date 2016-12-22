#!/bin/bash
#SBATCH --nodes=1
#SBATCH --cpus-per-task=8
#SBATCH --output=slurm-output/8-out.txt 
nproc=$(expr $SLURM_JOB_CPUS_PER_NODE \* $SLURM_NNODES)
echo $nproc cores available
cp -r src puzzles makefile $PFSDIR/.
cd $PFSDIR
make
./bin/solver puzzles/birds.txt
