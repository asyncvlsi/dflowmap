clear
mv ~/main.cc ./
mv ~/lib.cc ./
mv ~/fluid.act ./
mv ~/common.h ./
make clean
rm -f result_$1
rm -f lib_$1
make
./dflowmap.* $1
cp result_$1 ~
cp lib_$1 ~
cp $1 ~
actsim -cnf=conf_$1 result_$1 "main<>"

