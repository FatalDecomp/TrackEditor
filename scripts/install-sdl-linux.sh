#!/usr/bin/env bash
#
# Build the pinned SDL3 and SDL3_image from source into a prefix.
#
# Why not apt: an AppImage runs only on a glibc at least as new as the one it
# was built against, so it has to be built on an *old* distro to be portable at
# all. Building on ubuntu-26.04 produced an AppImage that demanded GLIBC_2.43
# and therefore ran on ubuntu-26.04 and nothing else. No distro old enough to
# be a sensible AppImage base packages SDL3, so it gets built here.
#
# The versions must match ROLLER's ROLLER_SDL3_MIN_VERSION and
# ROLLER_SDL3_IMAGE_MIN_VERSION, which in turn track build.zig.zon.
#
# usage: install-sdl-linux.sh <prefix>
#
# The prefix is an argument, not a return value: the caller already knows it.
# Everything this prints goes to stderr, so the script has no stdout contract
# for anyone to capture by mistake -- CMake writes its configure progress to
# stdout, and $(...) around this would pick that up too.

set -euo pipefail

SDL_VERSION=3.2.22
SDL_IMAGE_VERSION=3.2.4

prefix="${1:?usage: $0 <prefix>}"
mkdir -p "$prefix"
prefix="$(cd "$prefix" && pwd)"

# Already populated by a restored cache: nothing to do.
if [ -f "$prefix/lib/cmake/SDL3/SDL3Config.cmake" ] \
   && [ -f "$prefix/lib/cmake/SDL3_image/SDL3_imageConfig.cmake" ]; then
    echo "SDL already present in $prefix" >&2
    exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

build() {
    local repo="$1" tag="$2" name="$3"
    shift 3
    echo "building $name $tag" >&2
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$tag" \
        "https://github.com/libsdl-org/${repo}.git" "$work/$name"
    # CMake writes progress to stdout. Send it to stderr so it is still in the
    # CI log but never lands in a caller's $(...) -- capturing it is what put
    # "-- Detecting C compiler ABI info" into a GITHUB_ENV entry.
    cmake -S "$work/$name" -B "$work/$name-build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_PREFIX_PATH="$prefix" \
        -DBUILD_SHARED_LIBS=ON \
        "$@" >&2
    cmake --build "$work/$name-build" --parallel >&2
    cmake --install "$work/$name-build" >&2
}

build SDL "release-${SDL_VERSION}" SDL3 \
    -DSDL_TEST_LIBRARY=OFF \
    -DSDL_EXAMPLES=OFF

# roller-core uses exactly one thing from SDL_image: IMG_SavePNG, in
# png_writer.c. (IMG_Load lives in roller.c, which is the game, not the core.)
# So every other decoder is dead weight here.
#
# That matters because SDLIMAGE_VENDORED=ON expects the upstream submodules to
# be present -- a --depth 1 clone without --recurse-submodules leaves
# external/dav1d, external/aom, and external/libavif empty and configure fails
# on them. Fetching them would mean building two AV1 codecs from source on
# every cold cache, to decode formats this project never opens. Turning the
# formats off is both the faster and the more honest fix.
build SDL_image "release-${SDL_IMAGE_VERSION}" SDL3_image \
    -DSDLIMAGE_VENDORED=OFF \
    -DSDLIMAGE_AVIF=OFF \
    -DSDLIMAGE_JXL=OFF \
    -DSDLIMAGE_TIF=OFF \
    -DSDLIMAGE_WEBP=OFF \
    -DSDLIMAGE_SAMPLES=OFF \
    -DSDLIMAGE_TESTS=OFF

echo "SDL installed into $prefix" >&2
