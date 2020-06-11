#!/bin/bash

git clone git://git.zx2c4.com/wireguard-linux-compat
rm -fr net/wireguard/*

perl -i -ne 'BEGIN{$print=1;} $print = 0 if m/cat/; print $_ if $print;' wireguard-linux-compat/kernel-tree-scripts/create-patch.sh

wireguard-linux-compat/kernel-tree-scripts/create-patch.sh | patch -p1

rm -fr wireguard-linux-compat

git checkout net/wireguard/crypto/zinc/chacha20/.gitignore \
	     net/wireguard/crypto/zinc/poly1305/.gitignore \
	     net/wireguard/Kconfig
	     
