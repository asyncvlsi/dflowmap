sort $1 > 1_sort.log
sort $2 > 2_sort.log
diff -a -b 1_sort.log 2_sort.log

