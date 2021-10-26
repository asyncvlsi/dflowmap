LOC=rl538@grace.hpc.yale.edu:/gpfs/loomis/project/manohar/rl538/dflowmap$2
scp $LOC/lib_$1 ./
scp $LOC/result_$1 ./
scp $LOC/conf_$1 ./
scp $LOC/$1 ./
scp $LOC/$1.log ./
#scp $LOC/output ./
#scp $LOC/$1_statistics ./
#scp $LOC/$1_merge ./
#scp $LOC/$1_split ./
#scp $LOC/$1_mem ./
#scp $LOC/$1_energy ./
#scp $LOC/$1_fu ./
#cp lib_$1 lib_2_$1
#cp result_$1 result_2_$1
#cp conf_$1 conf_2_$1

