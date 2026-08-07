# Building TrackEditor

TrackEditor uses CMake 3.24 or newer. The supported editor build uses a dynamic Qt 6.8 LTS installation and binary development packages for SDL3 and SDL3_image. Qt and SDL binaries are not stored in this repository. CMake requires Qt 6.8 or newer; CI provisions 6.8.3 exactly.

Always initialize the pinned ROLLER submodule:

```sh
git clone --recursive https://github.com/FatalDecomp/TrackEditor.git
cd TrackEditor
```

For an existing clone, run `git submodule update --init --recursive`.

## Windows

Install the dynamic MSVC 2022 64-bit Qt 6.8 component (`win64_msvc2022_64`) with the official Qt installer and install CMake. The repository script downloads the official prebuilt SDL 3.2.22 and SDL_image 3.2.4 Visual C++ development archives and verifies their SHA-256 hashes:

```powershell
$sdlPrefix = .\scripts\install-sdl-windows.ps1
cmake -S . -B out\build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="$env:Qt6_DIR;$sdlPrefix"
cmake --build out\build --config Release --parallel
ctest --test-dir out\build -C Release --output-on-failure
```

If Qt is not already discoverable, set `Qt6_DIR` to the installation's `lib\cmake\Qt6` directory. Run TrackEditor from the Qt developer terminal so the Qt DLL directory is on `PATH`.

Keep the Visual Studio generator. Plain Ninja without an initialized MSVC developer environment lets CMake select MinGW and `windres`, which cannot consume the UTF-16 `TrackEditor.rc` and is ABI-incompatible with the MSVC Qt package.

## Ubuntu Linux

Ubuntu 26.04 provides sufficiently recent binary Qt 6 and SDL packages. Any `qt6-base-dev` at 6.8 or newer works; install Qt 6.8 LTS with the official installer or `aqtinstall` if the distribution package is older:

```sh
sudo apt update
sudo apt install cmake ninja-build qt6-base-dev libsdl3-dev libsdl3-image-dev
cmake -S . -B out/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

## macOS

Install a dynamic Qt 6.8 package with the official Qt installer. Homebrew bottles provide the remaining binary dependencies:

```sh
brew install --force-bottle sdl3 sdl3_image ninja
cmake -S . -B out/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.3/macos"
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

Adjust the Qt prefix for the version and location selected in the installer.

## Run

The executable is `TrackEditor` (`TrackEditor.exe` on Windows). The viewport is a plain Qt widget that blits worker-rendered `QImage` frames, so it does not create a Qt OpenGL context or require runtime GLSL files. The noninteractive `--cmake-smoke-test` switch initializes Qt and exits successfully; CTest uses it on all three CI platforms.

## Qt modules

The editor links Qt Core, Gui, and Widgets only. `cmake/TrackEditorNoOpenGL.cmake` walks each Qt-linked target's link closure at configure time and fails the build if Qt OpenGL reaches it, and the `trackeditor-e5-s2-linked-qt-modules` test reads the module names back out of the built executable. Deploying the editor therefore needs those three Qt libraries and the platform plugin, nothing more.

