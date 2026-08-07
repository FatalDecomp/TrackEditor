# Building TrackEditor

TrackEditor uses CMake 3.24 or newer. The supported editor build uses a dynamic Qt 5.15 installation and binary development packages for SDL3 and SDL3_image. Qt and SDL binaries are not stored in this repository.

Always initialize the pinned ROLLER submodule:

```sh
git clone --recursive https://github.com/FatalDecomp/TrackEditor.git
cd TrackEditor
```

For an existing clone, run `git submodule update --init --recursive`.

## Windows

Install the dynamic MSVC 64-bit Qt 5.15 component with the official Qt installer and install CMake. The repository script downloads the official prebuilt SDL 3.2.22 and SDL_image 3.2.4 Visual C++ development archives and verifies their SHA-256 hashes:

```powershell
$sdlPrefix = .\scripts\install-sdl-windows.ps1
cmake -S . -B out\build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="$env:Qt5_DIR;$sdlPrefix"
cmake --build out\build --config Release --parallel
ctest --test-dir out\build -C Release --output-on-failure
```

If Qt is not already discoverable, set `Qt5_DIR` to the installation's `lib\cmake\Qt5` directory. Run TrackEditor from the Qt developer terminal so the Qt DLL directory is on `PATH`.

## Ubuntu Linux

Ubuntu 26.04 provides sufficiently recent binary SDL packages:

```sh
sudo apt update
sudo apt install cmake ninja-build qtbase5-dev libsdl3-dev libsdl3-image-dev
cmake -S . -B out/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

## macOS

Install a dynamic Qt 5.15 package with the official Qt installer. Homebrew bottles provide the remaining binary dependencies:

```sh
brew install --force-bottle sdl3 sdl3_image ninja
cmake -S . -B out/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/clang_64"
cmake --build out/build
ctest --test-dir out/build --output-on-failure
```

Adjust the Qt prefix for the version and location selected in the installer.

## Run

The executable is `TrackEditor` (`TrackEditor.exe` on Windows). The viewport is a plain Qt widget that blits worker-rendered `QImage` frames, so it does not create a Qt OpenGL context or require runtime GLSL files. The noninteractive `--cmake-smoke-test` switch initializes Qt and exits successfully; CTest uses it on all three CI platforms.

