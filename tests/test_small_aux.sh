rm -f boinc_finish_called
rm -f init_data.xml
rm -f boinc_lockfile
rm -f astronomy_checkpoint
rm -f out
rm -f stderr.txt
touch out
touch stderr.txt

cp stars-11.txt stars.txt
cp astronomy_parameters-11-small-aux.txt astronomy_parameters.txt

./$1 -np 23 -p 0.571713 12.312119 -3.305187 148.010257 22.453902 0.42035 -0.468858 0.760579 -1.361644 177.884238 23.882892 1.210639 -1.611974 8.534378 -1.361644 177.884238 10.882892 1.210639 -1.611974 8.534378 100 50 25

echo "finished test on 11 aux"

cp stars-12.txt stars.txt
cp astronomy_parameters-12-small-aux.txt astronomy_parameters.txt

./$1 -np 23 -p 0.571713 12.312119 -3.305187 148.010257 22.453902 0.42035 -0.468858 0.760579 -1.361644 177.884238 23.882892 1.210639 -1.611974 8.534378 -1.361644 177.884238 10.882892 1.210639 -1.611974 8.534378 100 50 25 
echo "finished test on 12 aux"

cp stars-20.txt stars.txt
cp astronomy_parameters-20-small-aux.txt astronomy_parameters.txt

./$1 -np 17 -p 0.571713 12.312119 -3.305187 148.010257 22.453902 0.42035 -0.468858 0.760520 -1.361644 177.884238 23.882892 1.210639 -1.611974 8.534378 100 50 25 
echo "finished test on 20 aux"

cp stars-21.txt stars.txt
cp astronomy_parameters-21-small-aux.txt astronomy_parameters.txt

./$1 -np 17 -p 0.571713 12.312119 -3.305187 148.010257 22.453902 0.42035 -0.468858 0.760579 -1.361644 177.884238 23.882892 1.210639 -1.611974 8.534378 100 50 25 
echo "finished test on 21 aux"

cp stars-79.txt stars.txt
cp astronomy_parameters-79-small-aux.txt astronomy_parameters.txt

./$1 -np 11 -p 0.34217373320392042 25.9517910846623 -2.1709414738826602 38.272511356953906 30.225190442596112 2.2149060013372885 0.32316169064291655 2.7740244716285285 100 50 25 
echo "finished test on 79 aux"

cp stars-82.txt stars.txt
cp astronomy_parameters-82-small-aux.txt astronomy_parameters.txt

./$1 -np 11 -p 0.40587961154742185 17.529961843393409 -1.8575145272144837 29.360893891378243 31.228263575178566 -1.551741065334 .064096152599308373 2.5542820991278 100 50 251
echo "finished test on 82 aux"

cp stars-86.txt stars.txt
cp astronomy_parameters-86-small-aux.txt astronomy_parameters.txt

./$1 -np 11 -p 0.73317163557524425 14.657212876628332 -1.7054653473950408 16.911711745343633 28.077212666463502 -1.2032908515814611 3.5273606439247287 2.2248214505875008 100 50 25 
echo "finished test on 86 aux"
