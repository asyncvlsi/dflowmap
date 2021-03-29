scp $hpcDst/lib_$1 ./
scp $hpcDst/result_$1 ./
scp $hpcDst/conf_$1 ./
scp $hpcDst/$1 ./
cp lib_$1 lib_2_$1
cp result_$1 result_2_$1
cp conf_$1 conf_2_$1
