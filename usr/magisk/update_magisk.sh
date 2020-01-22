#!/bin/bash

set -e
set -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ "x$1" = "x--canary" ]
then
	ver="$(cat "$DIR/magisk_version" 2>/dev/null || echo -n 'none')"
	nver="canary"
	magisk_link="https://github.com/topjohnwu/magisk_files/raw/${nver}/magisk-release.zip"
else
	ver="$(cat "$DIR/magisk_version" 2>/dev/null || echo -n 'none')"
	nver="$(curl -s https://github.com/topjohnwu/Magisk/releases/latest | grep -o 'v[[:digit:].]*')"
	magisk_link="https://github.com/topjohnwu/Magisk/releases/download/${nver}/Magisk-${nver}.zip"
fi

if [ \( -n "$nver" \) -a \( "$nver" != "$ver" \) -o ! \( -f "$DIR/arm/magiskinit64" \) -o \( "$nver" = "canary" \) ]
then
	echo "Updating Magisk from $ver to $nver"
	curl -s --output "$DIR/magisk.zip" -L "$magisk_link"
	unzip -o "$DIR/magisk.zip" arm/magiskinit64 -d "$DIR"
	echo -n "$nver" > "$DIR/magisk_version"
	rm "$DIR/magisk.zip"
	touch "$DIR/initramfs_list"
else
	echo "Nothing to be done: Magisk version $nver"
fi
