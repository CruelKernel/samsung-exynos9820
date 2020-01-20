#!/bin/sh
echo $1 | awk -F. '{ printf "%d%02d%02d", $1, $2, $3 }'
