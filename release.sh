#!/usr/bin/env bash

set -e

distdir=$PWD/dist
builddir=$distdir/build
destdir=$distdir/gekkoboot

version=$(git describe --always --tags --dirty)
distfile=gekkoboot-$version.zip

rm -rf "$distdir" "$distfile"

meson setup . "$builddir" --cross-file=devkitPPC.ini -Dfull_rom=enabled
meson install -C "$builddir" --skip-subprojects --destdir "$destdir"

7z a "$distfile" "$destdir/*"
