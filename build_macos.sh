#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"

if [ "$(uname -s)" != "Darwin" ]; then
  echo "build_macos.sh is intended for macOS" >&2
  exit 1
fi

QT_QMAKE=${QT_QMAKE:-/opt/homebrew/opt/qt@5/bin/qmake}
if [ ! -x "$QT_QMAKE" ]; then
  echo "Qt 5 qmake not found at $QT_QMAKE" >&2
  echo "Install with: brew install qt@5" >&2
  exit 1
fi

for formula in glew glm; do
  if ! brew --prefix "$formula" >/dev/null 2>&1; then
    echo "Missing Homebrew dependency: $formula" >&2
    echo "Install with: brew install qt@5 glew glm libxml2" >&2
    exit 1
  fi
done

MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-$(sw_vers -productVersion | awk -F. '{print $1 ".0"}')}
export MACOSX_DEPLOYMENT_TARGET

ENABLE_FBX=${ENABLE_FBX:-auto}
if [ "$ENABLE_FBX" = "auto" ]; then
  FBXSDK_LIB=$(find "$ROOT/external/FBX" -path '*/libfbxsdk.a' -type f 2>/dev/null | head -n 1 || true)
  if [ -n "$FBXSDK_LIB" ]; then
    ENABLE_FBX=1
    FBX_LIB_DIR=${FBX_LIB_DIR:-$(dirname "$FBXSDK_LIB")}
  else
    ENABLE_FBX=0
  fi
fi
FBX_LIB_DIR=${FBX_LIB_DIR:-$ROOT/external/FBX/lib/release}

if [ "$ENABLE_FBX" = "1" ]; then
  if [ ! -f "$FBX_LIB_DIR/libfbxsdk.a" ] || [ ! -f "$FBX_LIB_DIR/libalembic.a" ]; then
    echo "ENABLE_FBX=1 but FBX libraries were not found in $FBX_LIB_DIR" >&2
    echo "Set FBX_LIB_DIR to the Autodesk FBX SDK macOS static library directory." >&2
    exit 1
  fi
else
  echo "Autodesk FBX SDK libraries not found; building without FBX export support."
fi

make -C WhipLib debug CXX=clang++ ENABLE_FBX="$ENABLE_FBX" FBX_LIB_DIR="$FBX_LIB_DIR" MACOSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET"
make -C ModelExporter debug CXX=clang++ ENABLE_FBX="$ENABLE_FBX" FBX_LIB_DIR="$FBX_LIB_DIR" MACOSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET"

cd TrackEditor
ENABLE_FBX="$ENABLE_FBX" FBX_LIB_DIR="$FBX_LIB_DIR" "$QT_QMAKE" TrackEditor.macos.pro -spec macx-clang
make -j "$(sysctl -n hw.ncpu)"

echo "Built macOS artifacts in $ROOT/bin/TrackEditor"
