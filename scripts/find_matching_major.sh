#!/bin/bash

# find_matching_version src_path version
TOP_DIR="$1"
SRC_PATH="$2"
FULL_SRC_PATH="$1/$2"
INPUT_VERSION="$3"

if [[ -d "${FULL_SRC_PATH}_${INPUT_VERSION}" ]]
then
  printf "${SRC_PATH}_${INPUT_VERSION}"
else
  printf "${SRC_PATH}"
fi
