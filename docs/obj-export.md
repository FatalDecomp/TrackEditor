# OBJ export conventions

E4-S1 retargeted the OBJ exporter onto ROLLER's canonical geometry
(`tEdVertex` / `tEdPrimitive` / `tEdMaterial`, delivered by
`RollerEd_QueryGeometrySizes` / `RollerEd_FillGeometry`). ROLLER is the
geometric authority (AD-6a); the editor's own CPU derivation is no longer part
of the OBJ path.

Every conversion below is stated **relative to**
`external/ROLLER/docs/adr/0003-canonical-geometry-conventions.md`, which is the
authoritative record of what the emitter publishes. Changing a convention there
is a breaking change for this exporter. E4-S2 (glTF) states its conversions
relative to that ADR as well, rather than re-deriving one against its own
importer, and any future exporter should too.

The writer is `TrackEditor/EditorObjExporter.{h,cpp}`. It owns no Qt type and
calls no `RollerEd_*` entry point, so it is exercised without
a render worker or a loaded track by `tests/editor_obj_exporter_test.cpp`.

## Coordinate system

ROLLER world space is right-handed with **+Z up** (ADR 0003, derived from
`transfrm.c`'s view basis). OBJ consumers expect right-handed **+Y up**, so the
exporter rotates -90 degrees about X:

```text
obj.x =  roller.x
obj.y =  roller.z
obj.z = -roller.y
```

The mapping preserves handedness (`newX × newY = newZ`), which is why no
winding flip accompanies it.

Positions are absolute ROLLER world coordinates. They are **not** relative to
chunk zero, which is where the editor's own `CTrackGeometry` derivation put its
origin. An export produced before E4-S1 and one produced after therefore do not
sit at the same place in the scene, even though both are +Y up at the same
scale.

## Scale

`ED_EXPORT_UNIT_SCALE` is `0.01`, preserved verbatim from the
pre-migration exporter, which divided by 100. The emitter itself applies no
scale at all (ADR 0003: "there is none"); exporters own their conversion.

Normals carry the rotation without the scale, so unit normals stay unit length.

## Winding

The emitted vertex order is preserved exactly. ROLLER's front face is the
right-hand-rule normal of `v0..v3`, which is the same side
`uiFrontMaterialId` describes, and OBJ's front face is counter-clockwise, which
is also the right-hand rule. Because the axis conversion preserves handedness,
front faces are emitted in their given order and no flip is applied.

Reverse-side faces are the same triangle wound the other way, with the normal
negated.

**Do not assume a globally coherent outside for the terrain skirt.** ADR 0003
records that the four outer-wall classes are not consistently wound in the
source data — retail `TRACK3` reverses twelve times where the terrain profile
crosses over. That is the data, not a defect, and the exporter does not try to
correct it.

## Reverse sides and the "backfaces" option

Complete exports explicitly double every authored quad. The reverse copy has
opposite winding and normals. If `uiBackMaterialId` names an authored reverse
texture or colour, that material is used and its material-local U coordinate
is mirrored for the reverse viewpoint. Otherwise the front material and UVs
are repeated, closing surfaces such as wheel backs that have no alternate.

The wizard's *Export backfaces as separate models* checkbox only chooses
whether those faces get their own `(Back)` object or are merged into the front
object — exactly what `eBackModeling::FRONTS` + `BACKS` versus
`FRONTS_AND_BACKS` did before.

## UV

`tEdVertex.fUV` is **material-local** with a top-left origin (AD-7b). The
exporter resolves it with the selected material's transform and does no tile
arithmetic of its own:

```text
atlas_uv = material_uv * fAtlasScale + fAtlasBias
```

OBJ's V origin is bottom-left while the exported atlas PNG's rows are
top-left, so the final coordinate is `(u, 1 - v)`. This is the same flip
`CShapeData::FlipTexCoordsForExport` used to perform.

The atlas PNGs are the ones `CTrackAssets::ExportTextures` writes:
`<name>.png` for `ROLLER_ED_TEXTURE_SET_TRACK` and `<name>_BLD.png` for
`ROLLER_ED_TEXTURE_SET_BUILDING_SIGN`. Their layout is what makes these UVs
resolve at all — see [texture-export.md](texture-export.md).

`ROLLER_ED_MATERIAL_FLAG_PAIR_WRAPS_ATLAS_ROW` marks a pair whose second tile
is the first tile of the next atlas row. The published transform runs past the
right edge of the atlas there, so the left half exports exactly and the wrapped
half is an approximation. The legacy renderer composes that pair from a flat
row-stride read, which no single scale/bias rectangle can describe.

## Materials

| Material kind | `newmtl` name | Body |
|---|---|---|
| `TEXTURED_TILE`, `TEXTURED_PAIR` | `<name>` or `<name>_BLD` | `map_Kd` / `map_d` pointing at the texture set's atlas PNG |
| `FLAT_PALETTE_COLOR` | `<name>_color_<index>` | `Ka` / `Kd` from the document's `PALETTE.PAL` entry |
| `SCREEN_DARKEN` | `<name>_darken_<level>` | black `Ka` / `Kd` plus a dissolve — see below |

Because UVs are already atlas-space, every tile of a texture set shares that
set's material; the `.mtl` declares it once. That is also the material set the
pre-migration exporter wrote, so an existing `.blend` keeps its material names.

## Screen darkening

Spec open item 2b, resolved here. A static format cannot express a framebuffer
darkening operation, and emitting it as an ordinary texture would be wrong, so
`SCREEN_DARKEN` becomes a documented semi-transparent black material.

ROLLER's renderer keeps `g_sceneGpuShadeFactor[level]` of whatever is behind
the surface (`scene_render_gpu.c`: `0.8, 0.6, 0.4, 0.2, 0.3`). Black
composited at dissolve `d` leaves `(1 - d)` of it, so:

```text
d = 1 - g_sceneGpuShadeFactor[level]
```

Levels past the table clamp to its last entry. This is exact for a black-over
composite; it is still an approximation of level 4, which the renderer blends
toward teal rather than toward black.

There is no option to omit these surfaces. They are real authored geometry, and
dropping them silently would leave gaps an importer could not explain.

## Scope

Authored content only (AD-6d), filtered on the `unContentClass` the producer
published (AD-6e) rather than inferred from the surface class. Editor helpers,
markers, selection overlays, the test car, and the reference model never reach
the canonical emitter at all; `RUNTIME_SCENERY` is filtered here.

### Signs and buildings

**Exported since E4A-S6**, governed by the *Include signs and buildings*
checkbox. They were unavailable from E4-S1 through E4-S5 because
`drawtrk3_emit_full_track` (E4A-S2) covered track chunks only; ROLLER's
`drawtrk3_emit_full_scenery` now walks every placed object with no camera
involved, and both canonical exporters gained them at once from that single
core change. The conventions are ROLLER's
`docs/adr/0005-camera-independent-scenery-traversal.md`; two of them show up
here:

- A billboard plan (an advert balloon) is exported at the **yaw the track file
  recorded**, not the viewer-facing yaw the editor viewport draws it at. A
  billboard has no orientation of its own, so the exported mesh and the
  viewport legitimately disagree about its facing.
- Nothing classified `RUNTIME_SCENERY` is published at all, so towers and the
  tree plan never arrive. No retail track places a tree.

The checkbox is deliberately **not** backed by the editor's own CPU
derivation, which works in a chunk-zero-relative frame and would place signs
off the exported track.

### What is not exported

- **Centerline and the four AI lines.** These are editor furniture derived by
  `editor_helpers.c`, kept out of the canonical stream on purpose (AD-6d), so
  they have no canonical representation to export.

No exporter reads legacy CPU geometry any more: **E4-S3 removed FBX outright**
rather than retargeting it, so the AI-line groups are gone from every format,
and WhipLib -- which held that geometry -- has since been deleted entirely.

## Objects in the file

With *Export track sections separately* on, one object per canonical surface
class, named exactly as the pre-migration exporter named them:

```text
Center, Left Shoulder, Right Shoulder, Left Wall, Right Wall, Roof,
Outer Wall Floor, Left Lower Outer Wall, Right Lower Outer Wall,
Left Upper Outer Wall, Right Upper Outer Wall
```

plus a `<name> (Back)` object for each when back faces are separate. With the
option off, a single `Track` object and, when back faces are separate, a
`Track (Back)`.

Signs and buildings keep their own grouping either way, because *Export track
sections separately* governs the track body:

```text
Sign 0, Sign 1, ... Sign N        one object per advert panel
Scenery                           every building and tower polygon
```

with `Sign N (Back)` and `Scenery (Back)` when back faces are separate. `Sign
N` restores the node names the pre-migration FBX exporter wrote. One object per
panel is exact rather than approximate: every building plan carries at most one
real-sign polygon (ROLLER `plans.c`), so a sign primitive and a panel are the
same thing. Buildings share one group because the canonical stream publishes no
per-object identity to split them on — only a chunk id, which two placed
buildings can share.

Faces are grouped by material inside each object, so the file carries one
`usemtl` per run rather than one per quad. Vertex, texture, and normal indices
run in lockstep and are file-global and one-based, as OBJ requires.
