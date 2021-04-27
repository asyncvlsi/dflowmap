clear
mv ~/main.cc ./
mv ~/lib.cc ./
mv ~/common.h ./
mv ~/PreprocessingPass.* ./
mv ~/Chp* ./
mv ~/Metrics.* ./
mv ~/fluid.metrics metrics/
make clean
rm -f result_$1
rm -f lib_$1
rm -f dflowmap.*
make
./dflowmap.* $1
cp result_$1 ~
cp lib_$1 ~
cp conf_$1 ~
cp $1 ~
#clear
#actsim -cnf=conf_$1 result_$1 "main<>"

