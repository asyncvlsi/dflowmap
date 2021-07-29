clear
./configure
mv ~/main.cc ./
mv ~/common.h ./
mv ~/helper.cc ./
mv ~/Constant.* ./
mv ~/Chp* ./
mv ~/Metrics.* ./
mv ~/fluid.metrics metrics/
mv ~/config.h ./
make clean
rm -f result_$1
rm -f lib_$1
rm -f conf_$1
rm -f dflowmap.*
rm -f $1.log
rm -f statistics
make
./dflowmap.* -v -m metrics/fluid.metrics $1 > $1.log 2>&1
mv statistics $1_statistics
