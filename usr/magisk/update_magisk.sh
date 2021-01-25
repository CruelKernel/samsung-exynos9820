#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

ver="$(cat "$DIR/magisk_version" 2>/dev/null || echo -n 'none')"

if [ "x$1" = "xcanary" ]
then
	nver="canary"
	magisk_link="https://github.com/topjohnwu/magisk_files/raw/${nver}/app-debug.apk"
else
	if [ "x$1" = "x" ]; then
		nver="$(curl -s https://github.com/topjohnwu/Magisk/releases | fgrep -m 1 'Magisk v' | cut -d '>' -f 2 | cut -d '<' -f 1 | cut -d ' ' -f 2)"
	else
		nver="$1"
	fi
	magisk_link="https://github.com/topjohnwu/Magisk/releases/download/${nver}/Magisk-${nver}.apk"
fi

if [ \( -n "$nver" \) -a \( "$nver" != "$ver" \) -o ! \( -f "$DIR/arm/magiskinit64" \) -o \( "$nver" = "canary" \) ]
then
	echo "Updating Magisk from $ver to $nver"
	curl -s --output "$DIR/magisk.zip" -L "$magisk_link"
	if fgrep 'Not Found' "$DIR/magisk.zip"; then
		curl -s --output "$DIR/magisk.zip" -L "${magisk_link%.apk}.zip"
	fi
	if unzip -o "$DIR/magisk.zip" arm/magiskinit64 -d "$DIR"; then
		mv -f "$DIR/arm/magiskinit64" "$DIR/magiskinit"
		: > "$DIR/magisk32.xz"
		: > "$DIR/magisk64.xz"
	else
		unzip -o "$DIR/magisk.zip" lib/armeabi-v7a/libmagiskinit.so lib/armeabi-v7a/libmagisk32.so lib/armeabi-v7a/libmagisk64.so -d "$DIR"
		mv -f "$DIR/lib/armeabi-v7a/libmagiskinit.so" "$DIR/magiskinit"
		mv -f "$DIR/lib/armeabi-v7a/libmagisk32.so" "$DIR/magisk32"
		mv -f "$DIR/lib/armeabi-v7a/libmagisk64.so" "$DIR/magisk64"
		xz --force --check=crc32 "$DIR/magisk32" "$DIR/magisk64"
	fi
	echo -n "$nver" > "$DIR/magisk_version"
	rm "$DIR/magisk.zip"
	touch "$DIR/initramfs_list"
else
	echo "Nothing to be done: Magisk version $nver"
fi
