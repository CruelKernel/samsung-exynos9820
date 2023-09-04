#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

ver="$(cat "$DIR/magisk_version" 2>/dev/null || echo -n 'none')"

if [ "x$1" = "xcanary" ]
then
	nver="canary"
	magisk_link="https://github.com/topjohnwu/magisk-files/raw/${nver}/app-debug.apk"
elif [ "x$1" = "xalpha" ]
then
	nver="alpha"
	magisk_link="https://github.com/vvb2060/magisk_files/raw/${nver}/app-release.apk"
else
	dash='-'
	if [ "x$1" = "x" ]; then
		nver="$(curl -s https://github.com/topjohnwu/Magisk/releases | grep -m 1 -Poe 'Magisk v[\d\.]+' | cut -d ' ' -f 2)"
	else
		nver="$1"
	fi
	if [ "$nver" = "v26.3" ]; then
		dash='.'
	fi
	magisk_link="https://github.com/topjohnwu/Magisk/releases/download/${nver}/Magisk${dash}${nver}.apk"
fi

if [ \( -n "$nver" \) -a \( "$nver" != "$ver" \) -o ! \( -f "$DIR/magiskinit" \) -o \( "$nver" = "canary" \) -o \( "$nver" = "alpha" \) ]
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
	elif unzip -o "$DIR/magisk.zip" lib/armeabi-v7a/libmagiskinit.so lib/armeabi-v7a/libmagisk32.so lib/armeabi-v7a/libmagisk64.so -d "$DIR"; then
		mv -f "$DIR/lib/armeabi-v7a/libmagiskinit.so" "$DIR/magiskinit"
		mv -f "$DIR/lib/armeabi-v7a/libmagisk32.so" "$DIR/magisk32"
		mv -f "$DIR/lib/armeabi-v7a/libmagisk64.so" "$DIR/magisk64"
		xz --force --check=crc32 "$DIR/magisk32" "$DIR/magisk64"
	elif unzip -o "$DIR/magisk.zip" lib/arm64-v8a/libmagiskinit.so lib/armeabi-v7a/libmagisk32.so lib/arm64-v8a/libmagisk64.so assets/stub.apk -d "$DIR"; then
		mv -f "$DIR/lib/arm64-v8a/libmagiskinit.so" "$DIR/magiskinit"
		mv -f "$DIR/lib/armeabi-v7a/libmagisk32.so" "$DIR/magisk32"
		mv -f "$DIR/lib/arm64-v8a/libmagisk64.so" "$DIR/magisk64"
		mv -f "$DIR/assets/stub.apk" "$DIR/stub"
		xz --force --check=crc32 "$DIR/magisk32" "$DIR/magisk64" "$DIR/stub"
	else
		unzip -o "$DIR/magisk.zip" lib/arm64-v8a/libmagiskinit.so lib/armeabi-v7a/libmagisk32.so lib/arm64-v8a/libmagisk64.so -d "$DIR"
		mv -f "$DIR/lib/arm64-v8a/libmagiskinit.so" "$DIR/magiskinit"
		mv -f "$DIR/lib/armeabi-v7a/libmagisk32.so" "$DIR/magisk32"
		mv -f "$DIR/lib/arm64-v8a/libmagisk64.so" "$DIR/magisk64"
		xz --force --check=crc32 "$DIR/magisk32" "$DIR/magisk64"
	fi
	echo -n "$nver" > "$DIR/magisk_version"
	rm "$DIR/magisk.zip"
	touch "$DIR/initramfs_list"
else
	echo "Nothing to be done: Magisk version $nver"
fi
