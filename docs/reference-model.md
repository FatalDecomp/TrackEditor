# Reference model import

The editor can load a Wavefront OBJ and draw it alongside the track, so a
modeller can line new geometry up against what the track actually looks like.

- **Reading the file** is `EditorObjImporter` (`OpenReferenceModel` in
  `TrackPreview.cpp`), carried over from the pre-modernization editor's
  `CObjImporter`. It emits flat float positions, normals, and a triangle-list
  index buffer; the glm-typed `CShapeData` it used to return went away with
  WhipLib, since the caller discarded it immediately anyway.
- **Drawing it** is roller-core's reference mesh (`RollerEd_SetReferenceMesh`,
  AD-13), added by E3A-S7.
- **The conversion between the two** is E4-S6, and is what this page records.

## Coordinate system

`tEdReferenceMesh` inherits ADR 0003's conventions: **ROLLER world space, +Z
up**, in legacy track units. A Wavefront OBJ is **+Y up**. The importer
therefore rotates the file into world space on the way in:

```text
roller.x =  file.x / ED_EXPORT_UNIT_SCALE
roller.y = -file.z / ED_EXPORT_UNIT_SCALE
roller.z =  file.y / ED_EXPORT_UNIT_SCALE
```

That is the **exact algebraic inverse** of the export conversion in
[obj-export.md](obj-export.md), and it lives in the same file
(`CEditorExportConventions::ImportPosition` / `ImportDirection`) for that
reason: two halves of one mapping in two places is how they drift apart.

Normals get the same rotation without the scale, so unit normals stay unit
length. Handedness is preserved in both directions, so neither reading nor
writing flips winding.

## Units

`ED_EXPORT_UNIT_SCALE` is `0.01`, so one metre in the file is 100 track units.
`CObjImporter` returns the file's **raw** numbers — it applied a hard-coded
×100 of its own until E4-S6, which would have compounded with the conversion.

## Normals

`ROLLER_ED_REFERENCE_HAS_NORMALS` is set **only when the file actually
supplied normals**. An OBJ with no `vn` lines leaves every normal at zero, and
AD-13 says the core generates them when the flag is clear — so claiming the
flag over zeros produces a flat, wrongly-shaded model, while clearing it gets
correct generated normals.

`CEditorReferenceMesh::SetGeometry` decides this from the geometry rather than
trusting the caller: any vertex with a non-zero normal means the file carried
them.

## UVs

Discarded. The legacy editor overwrote every reference-model UV with a flat
colour and roller-core draws the mesh the same way, so there is nothing for a
UV to address.

## The round trip

Because the import is the exact inverse of the export, **a track exported by
this editor re-imports as a reference model that lands back on itself**. That
is the sharpest available check on both conversions at once, and it is how
E4-S6 was accepted.

On retail `TRACK3`: 15,204 exported vertices, re-imported as 26,676 (the
importer expands faces into a flat triangle list), with identical bounding
boxes on all six extremes and a worst distance to the nearest original vertex
of **0.016 track units over a 365,000-unit track** — a relative error of
6.3 × 10⁻⁸, about half a `float` ULP at that magnitude. A wrong axis or scale
would be off by whole percentages.

## Transform and display

The reference-model dialog's X/Y/Z, yaw/pitch/roll and scale are **ROLLER
world values** on ADR 0003's axes: +Z is up, yaw turns about Z, pitch tilts
about Y, roll banks about X. See `CEditorReferenceMesh`'s header comment.

Wireframe is a property of the mesh, not one of the `SHOW_*` overlay flags.

> **Offsets typed before E4-S6 will not carry over.** They were compensating
> for a model loaded on its side, so the numbers that used to look right are
> wrong now that the model arrives upright.
