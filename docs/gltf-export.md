# glTF export conventions

E4-S2 added a glTF 2.0 exporter over the vendored [cgltf](https://github.com/jkuhlmann/cgltf)
writer, reading the same canonical ROLLER geometry
(`tEdVertex` / `tEdPrimitive` / `tEdMaterial`) the OBJ exporter reads. ROLLER is
the geometric authority (AD-6a).

Everything the exporters must agree on — axis, scale, scope, grouping, the
reverse-side rules, the darkening approximation — lives in
`TrackEditor/EditorExportCommon.{h,cpp}` and is stated relative to ROLLER's
`external/ROLLER/docs/adr/0003-canonical-geometry-conventions.md`. Only the
glTF document itself lives in `TrackEditor/EditorGltfExporter.{h,cpp}`. See
also [obj-export.md](obj-export.md); where the two formats differ, this page
says why.

The exporter owns no Qt type and calls no `RollerEd_*` entry
point, so it is exercised without a render worker or a loaded track by
`tests/editor_gltf_exporter_test.cpp`, which asserts against the document read
back through cgltf's own parser rather than against the JSON text.

## Vendored cgltf

`external/cgltf/` carries **v1.15**, commit
`360db1a95480fe102ae9c69b27c5d101167ff5ba`, comprising `cgltf.h` (parser),
`cgltf_write.h` (writer — a separate header; `cgltf.h` alone cannot serialize),
and its MIT `LICENSE`. `cgltf_impl.c` is the single translation unit that
instantiates both.

Do not edit the vendored headers. Re-vendor a newer tag instead, so the pinned
version stays a fact about the tree rather than a claim;
`tests/test_e4_s2_gltf_export.py` asserts the version and commit.

cgltf's writer serializes the glTF document but deliberately does **not** write
buffer or image contents — that stays this exporter's job, which is why the
binary payload and the GLB container are built here.

## Coordinate system

glTF fixes this, so there is nothing to choose: right-handed, **+Y up**, -Z
forward. The shared conversion from ROLLER's right-handed +Z-up world lands
exactly on it:

```text
gltf.x =  roller.x
gltf.y =  roller.z
gltf.z = -roller.y
```

The mapping preserves handedness, which is why no winding flip accompanies it.

Positions are absolute ROLLER world coordinates, not relative to chunk zero.

## Units

glTF declares its unit to be the metre. `ED_EXPORT_UNIT_SCALE` is `0.01`,
preserved verbatim from the pre-migration exporter, so one legacy track unit
exports as one centimetre and a track arrives in Blender at the same scale the
OBJ path produces.

## Winding

glTF's front face is counter-clockwise, the same rule as OBJ's, and the same
rule ADR 0003 records for ROLLER's emitted vertex order. Front faces are
therefore emitted in their given order with no flip; a generated reverse-side
primitive is the same triangle wound the other way with its normals negated.

The ADR's caveat applies here too: the four outer-wall classes are **not**
consistently wound in the source data, and the exporter does not try to correct
it.

## Normals

Taken from `tEdVertex.fNormal`, which the emitter generated (E4A-S4) and
`RollerEd_FillGeometry` copies through. Nothing here generates normals, and
`NORMAL` is always written, so an importer never has to guess them.

## Alpha mode and reverse-side geometry

**Alpha.** The atlas alpha is binary: `CTexture` decodes palette index 0 to
alpha 0 and every other index to 255. A transparent surface is therefore a
cut-out, not a blend.

| Canonical material | glTF `alphaMode` |
|---|---|
| Textured, `ROLLER_ED_MATERIAL_FLAG_ALPHA_BLEND` set | `MASK`, `alphaCutoff` 0.5 |
| Textured, flag clear | `OPAQUE` |
| `FLAT_PALETTE_COLOR` | `OPAQUE` |
| `SCREEN_DARKEN` | `BLEND` |

`OPAQUE` matters as much as `MASK` here: it makes the viewer ignore the alpha
channel, so palette index 0 keeps its colour on a surface the game draws
opaquely, which is what the renderer does. This is deliberately **more faithful
than the OBJ path**, whose `.mtl` writes `map_d` for every textured material and
so makes index 0 transparent everywhere — legacy behaviour E4-S1 preserved
rather than changed.

**Reverse sides.** Complete glTF exports explicitly double every authored
quad, just like OBJ. Each reverse copy has opposite winding and normals. An
authored `uiBackMaterialId` supplies its texture or colour using its authored
UV orientation; without one, the front material and UVs are repeated.
Materials remain single-sided so the front cannot leak through the alternate
reverse geometry.

The low-level exporter retains a compact mode that can use glTF
`doubleSided: true`, but the Track Editor does not use that mode for user
exports.

## How canonical materials collapse

The canonical table interns one material per atlas **tile** — retail `TRACK3`
has 92 — but the exported UVs are already atlas space, so every tile of a bank
resolves through the same image with the same settings. Emitting one glTF
material per canonical material would hand an importer 92 identically named
entries to rename to `TRACK3.001` and up.

The glTF materials are therefore keyed on their **resolved name**, which is a
pure function of everything that reaches the material: texture set, alpha
mode, palette colour, darkening level, and double-sidedness. Names carry
`_cutout` for `MASK` and, in compact mode, `_two_sided`, so materials that must
stay apart never collide.

The *Export backfaces as separate models* checkbox chooses whether the
explicit reverse geometry is placed in `(Back)` nodes or merged into each
front node.

## AI lines and centre line

**Not represented**, as lines or as thin geometry. They are `editor_helpers.c`
furniture kept out of the canonical stream on purpose (AD-6d), so there is no
canonical geometry to export — the same position E4-S1 took. If they are ever
wanted, they need a canonical representation in ROLLER first, not a
reconstruction in each exporter.

## Textures

Referenced or embedded, following the container:

- **`.gltf`** — images carry a `uri` pointing at `<name>.png` /
  `<name>_BLD.png`, which the export writes beside the model. This is the same
  pair the OBJ path writes, so the two exports can share a folder.
- **`.glb`** — images carry a `bufferView` into the binary chunk with
  `mimeType: image/png`. The PNGs are staged into a temporary directory,
  embedded, and discarded, so a `.glb` export leaves no loose files: being
  self-contained is the only reason to choose it.

Only atlases the export actually references become textures; the
building/sign bank is skipped when nothing uses it. Their layout is what makes
these UVs resolve at all — see [texture-export.md](texture-export.md).

The sampler is **nearest** and **clamp-to-edge**. The atlas packs tiles edge to
edge, so bilinear filtering would bleed neighbouring tiles into every seam, and
nearest is what the legacy renderer did anyway. UVs never leave the atlas, so
wrapping could only ever be a bug.

`ROLLER_ED_MATERIAL_FLAG_PAIR_WRAPS_ATLAS_ROW` carries the same caveat as in
OBJ: the published transform runs past the right edge of the atlas, so the left
half exports exactly and the wrapped half is an approximation.

## Materials

`pbrMetallicRoughness` with `metallicFactor` 0 and `roughnessFactor` 1: these
are unlit palette textures from 1997, and a metallic, glossy surface is not
something the source data can describe.

`baseColorFactor` is **linear** while the palette and the PNGs are sRGB. A PNG
is decoded as sRGB by the viewer, but a factor is taken at face value, so
`FLAT_PALETTE_COLOR` runs the palette entry through the sRGB transfer function
first. Skipping that step makes flat colours read far too bright.

## Screen darkening

The same approximation E4-S1 chose (spec open item 2b): a black
`baseColorFactor` whose alpha is `1 - g_sceneGpuShadeFactor[level]`, with
`alphaMode: BLEND`. Never a texture. See [obj-export.md](obj-export.md) for the
derivation.

## Output

The container follows the extension the user picks in the save dialog:

- **`<name>.glb`** — one file: a 12-byte header, a JSON chunk padded to four
  bytes with spaces, and a BIN chunk padded with zeros. Self-contained.
- **`<name>.gltf`** — JSON referencing `<name>.bin` and the two atlas PNGs
  beside it.

Both share one buffer laid out as, per mesh and per material batch, positions,
normals, texture coordinates, and indices, each run starting on a four-byte
boundary; the embedded images follow in a `.glb`. Indices are `UNSIGNED_INT`,
attributes are `FLOAT`, and `POSITION` carries the `min`/`max` bounds glTF
requires.

## Scope

Authored content only (AD-6d), filtered on the `unContentClass` the producer
published (AD-6e). Identical to OBJ, because the filter lives in the shared
layer.

**Signs and buildings are exported since E4A-S6**, under the same *Include
signs and buildings* checkbox OBJ uses and with the same conventions, recorded
in [obj-export.md](obj-export.md) and in ROLLER's
`docs/adr/0005-camera-independent-scenery-traversal.md`. The filter and the
grouping both live in the shared layer, so neither format has a rule of its
own. Towers and anything else ROLLER classifies `RUNTIME_SCENERY` are not
published at all.

One visible consequence for glTF specifically: the building/sign atlas is now
referenced, so an export that used to carry one image carries two.

## Objects in the file

One glTF mesh and one node per canonical surface class, named exactly as the
pre-migration exporter named its objects, so a Blender import produces
recognisable objects rather than `mesh.001`:

```text
Center, Left Shoulder, Right Shoulder, Left Wall, Right Wall, Roof,
Outer Wall Floor, Left Lower Outer Wall, Right Lower Outer Wall,
Left Upper Outer Wall, Right Upper Outer Wall
```

plus a `<name> (Back)` node for every non-empty front group. With *Export track
sections separately* off, `Track` and `Track (Back)` nodes. Empty groups are
dropped rather than written as empty meshes. Within a mesh, one primitive per
material.

## Validation

`tests/editor_gltf_exporter_test.cpp` round-trips every document through
`cgltf_parse` + `cgltf_validate` and checks the GLB container bytes directly.

The story's acceptance criterion is that the output imports natively in
Blender, so `tests/check_gltf_in_blender.py` drives a real Blender in
background mode over both sample containers and asserts the named meshes, UV
ranges, unit normals, materials, and the surviving double-sided flag. CMake
registers that test only when `TRACKEDITOR_BLENDER_EXECUTABLE` resolves —
hosted CI has no Blender, and the rest of the suite must not depend on one.
