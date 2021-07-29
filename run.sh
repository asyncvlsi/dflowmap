clear
#rm -f log
rm -f output
rm -f $1_energy
actsim -cnf=conf_$1 result_$1 "main_test<>"

