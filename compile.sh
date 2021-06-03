clear
mv ~/main.cc ./
mv ~/common.h ./
mv ~/Chp* ./
mv ~/Metrics.* ./
mv ~/fluid.metrics metrics/
mv ~/config.h ./
make clean
rm -f result_$1
rm -f lib_$1
rm -f dflowmap.*
make
./dflowmap.* -q -m metrics/fluid.metrics $1
cp result_$1 ~
cp lib_$1 ~
cp conf_$1 ~
cp $1 ~

