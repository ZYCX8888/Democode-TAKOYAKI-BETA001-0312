#!/bin/sh

pause(){
  #read -n 1 -p "$*" INP
  INP=`read -n 1 -p "$*" INP`
  #if [ $INP != '' ] ; then
  if [ "$INP" != "" ] ; then
    echo -ne '\b \n'
  fi
}

if [ "${PATH}" == "/sbin:/bin:/config" ]; then
  export PATH=/config:/sbin:/usr/sbin:/bin:/usr/bin
  echo -e "\e[31mDefault path is not set correctly!\nChange PATH to ${PATH}\e[0m"
  echo "Please read readme.md for environment setup."
fi

#record script name and use it as script set version terminator
script=$(basename "$0")
script="${script%.*}"
if [ "${ver}" == "" ]; then
  ver=`echo $script | cut -c1`
#if [[ "${script}" == "i*" ]]; then ver=1; does not work
  if [ "${ver}" == "i" ]; then ver=1;
  elif [ "${ver}" == "c" ]; then ver=2;
  else ver=3;
  fi
fi

. ./_config.sh
ret_config=$?
if [ "${ret_config}" != "0" ]; then return ${ret_config} ; fi

if [ ! -s ${exe} ]; then
  echo "Cannot find executable:${exe} Revise variable 'exe' in _config.sh";
  echo "or export exe=YOUR_EXECUTABLE_NAME before run this script."
  #pause "Press enter to continue..."
  return 1
fi

#out_path=/tmp
#dbg_level=/sys/module/mi_venc/parameters/debug_level

PASS="\e[32mPass\e[0m"
FAIL="\e[31mFail\e[0m"
WARN="\e[33mNG as warning\e[0m"

total=0
pass=0
fail=0
warning=0

#$1: Title string of this test. $2:condition to be checked.
test() {
	#echo ts $1, $2 ${LINENO}}
    echo -e "\e[33massert is recommended.\e[0m"
	total=`expr $total + 1`
	if [ "$2" == "0" ];then (>&2 echo -e $1 "==>" $FAIL); fail=`expr $fail + 1`;
	else (>&2 echo -e $1 "==>1" $PASS); pass=`expr $pass + 1`; fi
}

#new name of test()
#false condition is regarded as error
#$1: Title string of this test. $2:condition to be checked.
assert() {
	#echo ts $1, $2 ${LINENO}}
	total=`expr $total + 1`
	if [ "$2" == "0" ];then (>&2 echo -e $1 "==>" $FAIL); fail=`expr $fail + 1`; return 1;
	else (>&2 echo -e $1 "==>" $PASS); pass=`expr $pass + 1`; return 0; fi
}

#$1: Title string of this test. $2:condition to be checked.
#false condition is regarded as warning
check() {
	total=`expr $total + 1`
	if [ "$2" == "0" ];then (>&2 echo -e $1 "==>" $WARN); warning=`expr $warning + 1`; return 1;
	else (>&2 echo -e $1 "==>" $PASS); pass=`expr $pass + 1`; return 0; fi
}

#used for standard checking
#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $ret
#input $1: prefix of output file, output:$md5, $answer
run_std_case_check() {
  assert "The program returns ${ret}." `expr "${ret}" == "${expected_ret}"`
  [ ! -s ${of} ]; exist=$?
  assert "Check any output file exists ${of}" ${exist}
  md5=`md5sum ${of} | cut -d' ' -f1`
  echo "md5:${md5}  $1.${codec}"
  mv ${of} ${to}/$1.${codec};
  if [ "$?" != "0" ]; then echo -e $FAIL; fi
  if [ "{$unset_script}" != "" ]; then . $unset_script; fi
  #eval answer=\$${ans_ver}${1}
}

#output ES only exists while the main program success.
#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $ret
#input $1: prefix of output file, output:$md5, $answer
run_es_on_ok_check() {
  assert "The program returns ${ret}." `expr "${ret}" == "${expected_ret}"`
  if [ "${ret}" == "0" ]; then
    [ ! -s ${of} ]; exist=$?
    assert "Check any output file exists ${of}" ${exist}
    md5=`md5sum ${of} | cut -d' ' -f1`
    echo "md5:${md5}  $1.${codec}"
    mv ${of} ${to}/$1.${codec};
    if [ "$?" != "0" ]; then echo -e $FAIL; fi
  fi
  if [ "{$unset_script}" != "" ]; then . $unset_script; fi
  #eval answer=\$${ans_ver}${1}
}

#must run after run_std_case_check
#needed predefined variable in this script: $codec, $md5, $ans_ver
#input $1: prefix of output file, output:none
run_std_md5_check() {
  eval answer=\$${ans_ver}${1}
  check "Check $1.${codec} MD5" `expr "${md5}" == "${answer}"`
  if [ "$?" != "0" ]; then echo "expected MD5: ${answer}"; fi
  (>&2 echo -e "")       #Add extra line into summary
}

#must run after run_std_case_check
#needed predefined variable in this script: $codec, $md5, $ans_ver
#input $1: prefix of output file, output:none
run_2_md5_check() {
  eval answer1=\$${ans_ver}${1}_1
  eval answer2=\$${ans_ver}${1}_2
  check "Check $1.${codec} MD5" `expr "${md5}" == "${answer1}" \| "${md5}" == "${answer2}"`
  if [ "$?" != "0" ]; then echo "expected MD5: ${answer1} or ${answer2}"; fi
  (>&2 echo -e "")       #Add extra line into summary
}

print_result() {
	(>&2 printf "--------------------------------\nTotal $total tests, ")
	#if [ "$fail" == "0" ]; then (>&2 echo -e "\e[32m${fail} Failed\e[0m");
	if [ "$fail" == "0" ]; then (>&2 echo -e "\e[32mAll passed.\e[0m");
	else (>&2 echo -e "\e[31m${fail} Failed\e[0m"); fi
    if [ "${warning}" != "0" ]; then (>&2 echo -e "\e[33m${warning} Warnings\e[0m"); fi
}

log() {
	echo -e "\n\n""\e[33m""$1""\e[0m""\n\n"
	(>&2 echo -e "\e[36m""$1""\e[0m")
	#(>&2 printf $1)
	#(>&2 echo -e "1\neee\n" "$1")
}