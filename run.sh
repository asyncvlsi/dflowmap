mv ~/main.cc ./
mv ~/lib.cc ./
make clean
make
./dflowmap.* $1
cp result_$1 ~
cp lib_$1 ~
actsim result_$1 "main<>"

