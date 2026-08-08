#!/usr/bin/env bash
#
# Turn the per-OS CI artifacts into the assets a release publishes.
#
# Shared by the tagged-release workflow and the nightly, because those two must
# ship identically shaped downloads: an asset that only appears on one of them
# is a difference nobody discovers until they need it.
#
# usage: assemble-release-assets.sh <label> <artifacts-dir> <output-dir>
#
#   label          goes in each filename, e.g. "v1.2.0" or "nightly"
#   artifacts-dir  where actions/download-artifact put TrackEditor-{Windows,macOS,Linux}
#   output-dir     created if missing; receives exactly three files

set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <label> <artifacts-dir> <output-dir>" >&2
    exit 2
fi

label="$1"
artifacts="$(cd "$2" && pwd)"
mkdir -p "$3"
output="$(cd "$3" && pwd)"

# Windows is the one platform whose payload is a directory, so it is the one
# that needs archiving. The other two already ship as single files and are
# better left that way: re-zipping an AppImage would only make a user unpack
# and chmod it again.
( cd "$artifacts/TrackEditor-Windows" \
  && zip -qr "$output/TrackEditor-${label}-windows-x64.zip" . )
cp "$artifacts/TrackEditor-macOS/TrackEditor-macOS.tar.gz" \
   "$output/TrackEditor-${label}-macos.tar.gz"
cp "$artifacts/TrackEditor-Linux/TrackEditor-x86_64.AppImage" \
   "$output/TrackEditor-${label}-linux-x86_64.AppImage"

# A missing platform must fail the release rather than publish a partial one.
count="$(ls "$output" | wc -l)"
if [ "$count" -ne 3 ]; then
    echo "expected 3 release assets, found $count:" >&2
    ls -l "$output" >&2
    exit 1
fi

ls -l "$output"
