"""E4-S2 acceptance: import the exported glTF into Blender and check it.

The story's acceptance criterion is that the output "imports natively in
Blender with named per-surface meshes, correct UVs and normals". Asserting
that against Blender's own importer is the only way to know it, so this drives
a real Blender in background mode over the samples
tests/editor_gltf_exporter_test.cpp writes.

Registered by CMake only when a Blender is found, because hosted CI has none
and the rest of the suite must not depend on one.

Usage: check_gltf_in_blender.py <blender-executable> <directory-with-samples>
"""

from __future__ import annotations

import pathlib
import subprocess
import sys


# Runs inside Blender. Keep it to Blender's bundled Python and its own API.
BLENDER_SCRIPT = r'''
import sys
import bpy

path = sys.argv[sys.argv.index("--") + 1]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=path)

objects = [o for o in bpy.context.scene.objects if o.type == "MESH"]
names = sorted(o.name for o in objects)
print("E4S2 objects:", names)

# Named per-surface meshes: the canonical surface-class names survive the trip
# rather than arriving as "mesh.001".
expected = {"Center", "Roof", "Left Wall"}
missing = expected - set(names)
if missing:
    raise SystemExit("missing named meshes: %s (got %s)" % (sorted(missing), names))

for obj in objects:
    mesh = obj.data
    if len(mesh.polygons) == 0:
        raise SystemExit("%s imported with no polygons" % obj.name)
    if not mesh.uv_layers:
        raise SystemExit("%s imported with no UV layer" % obj.name)

    uv_layer = mesh.uv_layers[0].data
    for loop in uv_layer:
        u, v = loop.uv
        if not (-0.001 <= u <= 1.001) or not (-0.001 <= v <= 1.001):
            raise SystemExit("%s has a UV outside the atlas: %f %f" % (obj.name, u, v))

    # Normals survive as unit vectors rather than being regenerated flat-zero.
    for poly in mesh.polygons:
        length = poly.normal.length
        if abs(length - 1.0) > 0.01:
            raise SystemExit("%s has a non-unit normal: %f" % (obj.name, length))

    if not mesh.materials or mesh.materials[0] is None:
        raise SystemExit("%s imported without a material" % obj.name)

# The two-sided surface must arrive as a double-sided material rather than as
# duplicated reverse geometry, which is the whole point of using glTF's own
# way of saying it.
roof = bpy.context.scene.objects["Roof"]
if roof.data.materials[0].use_backface_culling:
    raise SystemExit("Roof's material culls backfaces; double_sided was lost")
if len(roof.data.polygons) != 2:
    raise SystemExit("Roof should be one quad (2 tris), got %d" % len(roof.data.polygons))

print("E4S2 blender import OK:", path)
'''


def check(blender: str, path: pathlib.Path, script: pathlib.Path) -> None:
    result = subprocess.run(
        [
            blender,
            "--background",
            "--factory-startup",
            "--python-exit-code",
            "1",
            "--python",
            str(script),
            "--",
            str(path),
        ],
        capture_output=True,
        text=True,
    )
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    if result.returncode != 0:
        raise SystemExit(
            "blender failed to import %s (exit %d)" % (path, result.returncode)
        )


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    blender = sys.argv[1]
    directory = pathlib.Path(sys.argv[2])

    script = directory / "e4s2_blender_import.py"
    script.write_text(BLENDER_SCRIPT, encoding="utf-8")

    samples = ["e4s2-sample.gltf", "e4s2-sample-full.glb"]
    for name in samples:
        path = directory / name
        if not path.is_file():
            raise SystemExit(
                "missing %s; run trackeditor-e4-s2-gltf-exporter first" % path
            )
        check(blender, path, script)

    print("E4-S2 Blender import check passed for %d samples" % len(samples))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
