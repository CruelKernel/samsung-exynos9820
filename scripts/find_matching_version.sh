#!/bin/bash

# find_matching_version src_path version
TOP_DIR="$1"
SRC_PATH="$2"
FULL_SRC_PATH="$1/$2"
INPUT_VERSION="$3"

if [[ -d "${FULL_SRC_PATH}_v${INPUT_VERSION}" ]]
then
  printf "${SRC_PATH}_v${INPUT_VERSION}"
else
  LIST=$(ls -d ${FULL_SRC_PATH}_v*)
  PREV_VERSION=${INPUT_VERSION}
  for i in $LIST
  do
    VERSION=${i//${FULL_SRC_PATH}_v/}
    if [ $VERSION -lt $INPUT_VERSION ]
    then
      PREV_VERSION=$VERSION
    fi
  done
  if [ "x$PREV_VERSION" == "x$INPUT_VERSION" ]
  then
    printf "${SRC_PATH}"
  else
    printf "${SRC_PATH}_v${PREV_VERSION}"
  fi
fi
