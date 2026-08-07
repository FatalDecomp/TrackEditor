# Roller Track Editor
Whiplash/Fatal Racing track editor, OBJ and glTF resource converter, and library for loading Whiplash assets.

## Build

Clone recursively so the pinned ROLLER editor core is available:

```sh
git clone --recursive https://github.com/FatalDecomp/TrackEditor.git
cd TrackEditor
cmake -S . -B out/build
cmake --build out/build --config Release
```

The CMake build uses dynamic Qt 6.8 LTS and system/prebuilt SDL3 and SDL3_image packages. It builds ROLLER's `ROLLER::core` target with the game disabled, so WildMidi and libcdio are not required. See [docs/building.md](docs/building.md) for Windows, Linux, macOS, and run instructions.

## Track Editor Features: 
* Open, render, edit, and save Whiplash tracks
* Multiple tracks open at once in tabs
* Export tracks to OBJ and glTF 2.0 format (`.gltf` or self-contained `.glb`)
* Track geometry data can be edited and track chunks can be added and removed
* Additional surface data such as grip level and AI data can be edited
* Surface textures can be changed
* Signs can be added, removed, repositioned, and edited
* Audio triggers can be added, removed, and edited
* Moving stunts can be added, removed, and edited
* Track global settings such as texture files and lap data can be edited
* Surface backface textures can be changed
* Undo/Redo and Cut/Copy/Paste between tracks
* Fine control over Copy/Paste behavior
* Toggle display of each individual section of track

![Track Editor](TrackEditor/images/screenshot.png)

![Model exported to Blender](TrackEditor/images/blender.png)

![Model exported to Unreal](TrackEditor/images/unreal.png)

## External dependencies used:
* Dynamic Qt 6.8 LTS
* SDL 3.2.22 or newer
* SDL_image 3.2.4 or newer
* stb_image 2.30: https://github.com/nothings/stb
* stb_image_write 1.16: https://github.com/nothings/stb
* cgltf 1.15 (vendored): https://github.com/jkuhlmann/cgltf
