# Building TrackEditor

TrackEditor uses CMake 3.24 or newer. The supported editor build uses a dynamic Qt 5.15 installation and binary development packages for SDL3, SDL3_image, and GLEW. Qt, SDL, and GLEW binaries are not stored in this repository.

Always initialize the pinned ROLLER submodule:

```sh
git clone --recursive https://github.com/FatalDecomp/TrackEditor.git
cd TrackEditor
```

For an existing clone, run `git submodule update --init --recursive`.

## Windows

Install the dynamic MSVC 64-bit Qt 5.15 component with the official Qt installer. Install CMake, Ninja, and GLEW (for example, `vcpkg install glew:x64-windows`). The repository script downloads the official prebuilt SDL 3.2.22 and SDL_image 3.2.4 Visual C++ development archives and verifies their SHA-256 hashes:

```powershell
$sdlPrefix = .\scripts\install-sdl-windows.ps1
cmake -S . -B out\build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="$env:Qt5_DIR;$sdlPrefix" `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DTRACKEDITOR_ENABLE_FBX=OFF
cmake --build out\build
ctest --test-dir out\build --output-on-failure
```

If Qt is not already discoverable, set `Qt5_DIR` to the installation's `lib\cmake\Qt5` directory. Run TrackEditor from the Qt developer terminal so the Qt DLL directory is on `PATH`.

## Ubuntu Linux

Ubuntu 26.04 provides sufficiently recent binary SDL packages:

```sh
sudo apt update
sudo apt install cmake ninja-build qtbase5-dev libqt5opengl5-dev \
  libglew-dev libsdl3-dev libsdl3-image-dev
cmake -S . -B out/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DTRACKEDITOR_ENABLE_FBX=OFF
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

## macOS

Install a dynamic Qt 5.15 package with the official Qt installer. Homebrew bottles provide the remaining binary dependencies:

```sh
brew install --force-bottle glew sdl3 sdl3_image ninja
cmake -S . -B out/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/clang_64" \
  -DTRACKEDITOR_ENABLE_FBX=OFF
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

Adjust the Qt prefix for the version and location selected in the installer.

## Run

The executable is `TrackEditor` (`TrackEditor.exe` on Windows). CMake copies its required GLSL files to a `Shaders` directory beside the executable. The noninteractive `--cmake-smoke-test` switch initializes Qt and exits successfully; CTest uses it on all three CI platforms.

## Optional FBX export

FBX export is disabled by default and is always disabled in CI. Normal builds neither compile `FBXExporter.cpp` nor search for the Autodesk SDK.

To enable it locally, install the Autodesk FBX SDK and point CMake at the SDK root containing `include/fbxsdk.h`:

```sh
cmake -S . -B out/fbx \
  -DTRACKEDITOR_ENABLE_FBX=ON \
  -DTRACKEDITOR_FBX_SDK_ROOT=/path/to/fbx-sdk
cmake --build out/fbx --config Release
```

The FBX menu action is omitted from default builds and becomes available only when this option is enabled.
