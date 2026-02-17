#!/bin/bash
#SBATCH --job-name=bfs
#SBATCH --output=logs/bfs_%j.out
#SBATCH --error=logs/bfs_%j.err
#SBATCH --time=00:05:00
#SBATCH --cpus-per-task=8
#SBATCH --mem=1G

echo "Host: $(hostname)"
echo "Start: $(date)"

make clean
make

echo "Sequential:"
./bin/bfs_seq "Tom Hanks" 3

echo ""
echo "Parallel:"
./bin/bfs_par "Tom Hanks" 3 8

echo "End: $(date)"

