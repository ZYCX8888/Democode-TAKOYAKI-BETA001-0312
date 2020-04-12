#!/bin/sh
target=$1
script=_run.sh~
echo "#!/bin/sh" > $script
chmod 755 $script
if [ "$target" == "" ]; then find . -name "i2_venc_test*.sh" > $script; else echo ./$target > $script; fi
#ls i2_venc_test*.sh > $script
#ls test03.sh > $script

./$script 2> /tmp/err

echo  ========================================
echo "              TEST RESULT"
echo  ========================================
cat /tmp/err
AnyErr=`grep -i fail /tmp/err`
echo -e "\n\n========================================\n"
if [ "$AnyErr" == "" ];then echo All pass; else echo some error has been found!; fi