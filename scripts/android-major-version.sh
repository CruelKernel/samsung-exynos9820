#!/bin/sh

MAJOR=$(echo $1 | cut -d '.' -f 1)
let MAJOR=MAJOR+103
printf "%b" "$(printf '\%03o' $MAJOR)"
