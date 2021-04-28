scp $hpcDst/dflowmap/lib_$1 ./
scp $hpcDst/dflowmap/result_$1 ./
scp $hpcDst/dflowmap/conf_$1 ./
scp $hpcDst/dflowmap/$1 ./
scp $hpcDst/dflowmap/output ./
cp lib_$1 lib_2_$1
cp result_$1 result_2_$1
cp conf_$1 conf_2_$1
