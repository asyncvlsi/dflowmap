LOC=rl538@grace.hpc.yale.edu:/gpfs/loomis/project/manohar/rl538/dflowmap
scp $LOC/lib_$1 ./
scp $LOC/result_$1 ./
scp $LOC/conf_$1 ./
scp $LOC/$1 ./
scp $LOC/output ./
scp $LOC/metrics/fluid.metrics ./
cp lib_$1 lib_2_$1
cp result_$1 result_2_$1
cp conf_$1 conf_2_$1
mv fluid.metrics metrics/

