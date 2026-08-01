# macOS build spike

This spike adds a small macOS build path without replacing the existing Visual Studio or Linux Make/qmake setup.

## Dependencies

Install the non-vendored dependencies with Homebrew:

```sh
brew install qt@5 glew glm libxml2
```

Qt 5 is keg-only and deprecated upstream, but it is the smallest fit for the existing `QGLWidget`-based editor code.

## Build

```sh
./build_macos.sh
```

Outputs are written to:

- `bin/TrackEditor/ModelExporter`
- `bin/TrackEditor/TrackEditor.app`

## FBX support

Autodesk FBX SDK libraries are not vendored here. If the script cannot find `libfbxsdk.a`, it builds with `WHIPLIB_ENABLE_FBX=0` so the OBJ exporter and editor can compile on macOS without adding large binary dependencies.

To build with FBX support, install the Autodesk FBX SDK for macOS and run:

```sh
ENABLE_FBX=1 FBX_LIB_DIR=/path/to/fbx/lib ./build_macos.sh
```

`FBX_LIB_DIR` must contain both:

- `libfbxsdk.a`
- `libalembic.a`

## Known limitations

- The current verified macOS build is arm64 and links against Homebrew Qt/GLEW rather than producing a redistributable `.app` bundle.
- `macdeployqt` packaging has not been spiked yet.
- FBX export is disabled unless the Autodesk FBX SDK static libraries are installed locally.
- Runtime behavior has only been smoke-tested at the binary level; interactive editor QA still needs a real Whiplash data set.
