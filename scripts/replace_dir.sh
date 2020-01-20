#!/bin/sh
# Support selinux version
# replace_directory dst src
DST="$1/$2"
SRC="$1/$3"

if [ "x${DST}" == "x${SRC}" ]
then
  #echo "${DST} and ${SRC} is same"
  exit 0
else
  if [ -d ${SRC} ]
  then
    if [ -L ${DST} ]
    then
      rm -f ${DST}
    fi
    ln -s $(basename ${SRC}) ${DST}
  else
    echo "${SRC} does not exit"
    exit -1
  fi
fi

