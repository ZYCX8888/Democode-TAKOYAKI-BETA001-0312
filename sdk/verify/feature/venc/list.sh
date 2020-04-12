if [ "${1}" == "1" ]; then
  grep log i2*.sh | cut -d' ' -f1,3- | cut -d'\' -f1 | cut -c-18,22-
elif [ "${1}" == "e" ]; then
  grep log exception??.sh | cut -d' ' -f1,3- | cut -d'\' -f1 | cut -c-15,19-
else
  grep log case??.sh | cut -d' ' -f1,3- | cut -d'\' -f1 | cut -c-10,14-
fi
