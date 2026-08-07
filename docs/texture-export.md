# Texture export

Every export writes its atlases from `track-assets` (`CTrackAssets::ExportTextures`
→ `CTexture::ExportToPngFile`), which owns palette and texture decoding since
E3-S5b. No exporter decodes an image itself.

E4-S4 changed the **layout** those PNGs are written in. This page records what
that layout is and why, because it is the thing the canonical UVs are resolved
against — see [obj-export.md](obj-export.md) and [gltf-export.md](gltf-export.md).

## Atlas layout

The exported atlas is the layout **ROLLER addresses tiles in**:

```text
width       256 pixels (EXPORT_ATLAS_WIDTH)
tile        64 x 64
per row     4
tile N at   x = (N % 4) * 64,  y = (N / 4) * 64
height      ceil(tileCount / 4) * 64
```

This is not a choice the editor gets to make. `polytex.c` resolves a tile as
`row = index >> 2`, `col = index & 3` with a 256-byte stride, and
`drawtrk3_editor_texture_atlas` publishes `uiWidth = 256`,
`uiHeight = rows * tileSize` to the material table. `ed_build_material` derives
every `fAtlasScale` / `fAtlasBias` from those numbers, and the exporters apply
them verbatim (AD-7b forbids exporter-side tile arithmetic). An atlas laid out
any other way makes every textured UV land somewhere else.

Retail `TRACK3` produces a **256 × 2624** main atlas (164 tiles, 41 rows) and a
**256 × 384** sign atlas (24 tiles, 6 rows), matching
`drawtrk3_editor_texture_atlas` exactly.

The sign atlas was written unconditionally from E4-S4 onwards but referenced by
nothing, because no export produced a sign. **E4A-S6 made it live**: advert
panels and buildings now reach both exporters, 17 of `TRACK3`'s building-bank
materials among them, and glTF — which only embeds an atlas something uses —
went from one image to two.

## Which tiles

**Content tiles only** — `CTexture::GetNumTiles()`, not `GetAtlasTileCount()`.

The editor appends two synthetic tiles to every bank for its own pickers: a
palette swatch tile and a transparency tile. ROLLER's `uiTileCount` does not
count them, so including them would make the atlas taller than the height the
UVs were divided by, and every V would land a fraction of a row out.

A tile count that does not fill its last row leaves padding. The canonical UVs
never address it, and it is written as transparent black rather than as a
repeat of an earlier tile.

## Orientation

Rows top-first, columns left-to-right, no transpose and no vertical flip.
`tTile::data` is indexed `[column][row]`, and the writer walks rows outermost so
it goes into the row-major image upright.

ROLLER's UV origin is top-left (ADR 0003), which is also PNG's row order and
glTF's UV origin, so glTF needs no flip. OBJ's V origin is bottom-left, so the
OBJ writer emits `1 - v` — the flip belongs there, in one exporter, not in the
shared image.

## The legacy single-column bitmap is unchanged

`CTexture::GenerateBitmapData` still produces the old shape: `TILE_WIDTH` wide,
one tile per row-block, **every** tile including the synthetic ones, each tile
transposed and vertically flipped.

It used to be frozen because WhipLib's C API (`wlLoadTexture` and friends)
published that buffer to external callers, and `WhipLib::TextureMapping`
computed the legacy CPU UVs against it. **Both went away when WhipLib and
`ModelExporter` were deleted**, so nothing in the build calls
`GenerateBitmapData` any more; only the track-assets unit test does. It is kept
as a record of the legacy on-disk layout.

Those two consumers were self-consistent with each other, which is exactly why
the mismatch went unnoticed: before E4-S1 the exporters used WhipLib's mapping
too, so the wrong-looking atlas was addressed by matching wrong-looking UVs.

## Parity

**E4-S4 deliberately breaks byte-parity with pre-E4-S4 exports.** The story's
stated acceptance criterion was "parity with current texture export", but the
current export could not be addressed by the UVs E4-S1 and E4-S2 emit, so
preserving it would have preserved a defect. The maintainer chose the fix.

Practical consequence: a `.blend` (or any other scene) that references a PNG
exported before E4-S4 keeps its old, wrongly-laid-out image. Re-export the
model **and** the texture together.

## Verification

- `tests/track_assets_test.cpp` builds a six-tile texture with per-tile colours
  and orientation markers, then reproduces ROLLER's `scale`/`bias` arithmetic
  as the specification and asserts every tile sits at the rectangle that
  transform addresses — plus the two-row height, the transparent padding, and
  the untouched legacy bitmap.
- `tests/test_e4_s4_texture_output.py` pins the layout, the content-tile rule,
  the absent transpose and flip, the frozen legacy bitmap, and that ROLLER
  still describes a 256-wide four-per-row atlas at the current pin.
- Retail acceptance on `TRACK3`: all **91** textured materials resolve to
  exactly the 64×64 (128×64 for pairs) rectangle their `uiTileIndex` occupies,
  with zero mismatches, and Blender loads the 256 × 2624 atlas from both the
  `.gltf` and the `.glb` with every UV inside it.
