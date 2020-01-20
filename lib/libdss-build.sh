#!/bin/bash
if [[ ${CC} = *"clang" ]]; then
CC_DIR=$(dirname "${CC}")
export PATH=$PATH:${CC_DIR}
rm -rf lib/libdss.c
python lib/make_libdss.py &> lib/libdss.c
${CC} \
  --target=aarch64-linux-gnu \
  -Ilib/libdss-include \
  -Iinclude \
  -mlittle-endian \
  -Qunused-arguments \
  -fno-strict-aliasing \
  -fno-common \
  -fshort-wchar \
  -std=gnu89 \
  -DDSS_ANALYZER \
  -fno-PIE \
  -mno-implicit-float \
  -DCONFIG_BROKEN_GAS_INST=1 \
  -fno-asynchronous-unwind-tables \
  -fno-pic \
  -Oz \
  -Wframe-larger-than=4096 \
  -fno-stack-protector \
  -mno-global-merge \
  -fno-omit-frame-pointer \
  -fno-optimize-sibling-calls \
  -c \
  -static \
  -fno-strict-overflow \
  -fno-merge-all-constants \
  -fno-stack-check \
  -g lib/libdss.c -o lib/libdss.o &> /dev/null

rm -f libdss.a &> /dev/null
${CROSS_COMPILE}ar -rc libdss.a lib/libdss.o &> /dev/null
fi
