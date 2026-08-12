#include "EditorCarModel.h"

extern "C"
{
#include "carplans.h"
}

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>

namespace
{
void Require(bool bCondition)
{
  if (!bCondition)
    std::abort();
}
}

int main()
{
  Require(CEditorCarModel::Count() == ROLLER_ED_TEST_CAR_DESIGN_COUNT);
  Require(CEditorCarModel::ExportCount() == 22u);
  Require(std::string(CEditorCarModel::Name(0)) == "XAUTO");
  Require(std::string(CEditorCarModel::Name(0, true)) == "YAUTO");
  Require(std::string(CEditorCarModel::Name(6)) == "XZIZIN");
  Require(std::string(CEditorCarModel::Name(6, true)) == "YZIZIN");
  Require(std::string(CEditorCarModel::Name(12)) == "F1WACK");
  Require(std::string(CEditorCarModel::TextureFileName(12)) == "red28.bm");
  Require(std::string(CEditorCarModel::TextureFileName(6, true)) ==
          "yzizin.bm");
  Require(CEditorCarModel::HasAdvancedVariant(7));
  Require(!CEditorCarModel::HasAdvancedVariant(8));
  Require(CEditorCarModel::Name(8, true) == nullptr);
  Require(CEditorCarModel::Name(CEditorCarModel::Count()) == nullptr);

  for (uint32_t uiDesign = 0; uiDesign < CEditorCarModel::Count(); ++uiDesign) {
    tEdCarGeometry Owned;
    std::string sError;
    Require(CEditorCarModel::Build(uiDesign, false, 256u, Owned, sError));
    Require(sError.empty());
    Require(!Owned.Primitives.empty());
    Require(Owned.Primitives.size() <= CarDesigns[uiDesign].byNumPols);
    Require(Owned.Vertices.size() == Owned.Primitives.size() * 4u);
    Require(Owned.Indices.size() == Owned.Primitives.size() * 6u);
    Require(!Owned.Materials.empty());

    const tEdExportGeometry View = Owned.View();
    Require(CEditorExportConventions::ValidateGeometry(View, sError));
    for (const tEdPrimitive &Primitive : Owned.Primitives) {
      Require(Primitive.byTopology == ROLLER_ED_TOPOLOGY_TRIANGLE_LIST);
      Require(Primitive.uiFrontMaterialId < Owned.Materials.size());
    }
  }

  tEdCarGeometry Auto;
  std::string sError;
  Require(CEditorCarModel::Build(0u, false, 256u, Auto, sError));
  const tVec3 &Expected =
      CarDesigns[0].pCoords[CarDesigns[0].pPols[0].verts[0]];
  Require(std::fabs(Auto.Vertices[0].fPosition[0] - Expected.fX) < 0.001f);
  Require(std::fabs(Auto.Vertices[0].fPosition[1] - Expected.fY) < 0.001f);
  Require(std::fabs(Auto.Vertices[0].fPosition[2] - Expected.fZ) < 0.001f);

  tEdExportGrouping Grouping;
  Grouping.sSingleObjectName = "XAUTO";
  Grouping.bSeparateBackFaces = false;
  std::vector<tEdExportObject> Objects;
  Require(CEditorExportConventions::BuildObjects(
      Auto.View(), Grouping, Objects, sError));
  Require(Objects.size() == 1u);
  Require(Objects[0].sName == "XAUTO");
  Require(!Objects[0].Entries.empty());

  // 2X4B523P uses the PULSE plan. Its animated wheel fronts (25..28) must
  // retain the polygon's reverse-side flags and use textured tile 1 on the
  // back. Its mirror faces (33..34) use flat palette black on the back.
  tEdCarGeometry TwoByFour;
  Require(CEditorCarModel::Build(10u, false, 256u, TwoByFour, sError));
  Require(TwoByFour.Primitives.size() == CarDesigns[10].byNumPols);
  for (uint32_t i = 25u; i <= 28u; ++i) {
    const tEdPrimitive &Wheel = TwoByFour.Primitives[i];
    Require((Wheel.unFlags & ROLLER_ED_PRIMITIVE_FLAG_TWO_SIDED) != 0);
    Require(Wheel.uiBackMaterialId < TwoByFour.Materials.size());
    const tEdMaterial &Back = TwoByFour.Materials[Wheel.uiBackMaterialId];
    Require(CEditorExportConventions::IsTexturedKind(Back.uiKind));
    Require(Back.uiTileIndex == 1u);
  }
  for (uint32_t i = 33u; i <= 34u; ++i) {
    const tEdPrimitive &Mirror = TwoByFour.Primitives[i];
    Require(Mirror.uiBackMaterialId < TwoByFour.Materials.size());
    const tEdMaterial &Back = TwoByFour.Materials[Mirror.uiBackMaterialId];
    Require(Back.uiKind == ROLLER_ED_MATERIAL_FLAT_PALETTE_COLOR);
    Require(Back.uiPaletteColour == 0u);
  }

  tEdExportGrouping CompleteGrouping;
  CompleteGrouping.sSingleObjectName = "2X4B523P";
  CompleteGrouping.bGenerateAllReverseSides = true;
  std::vector<tEdExportObject> CompleteObjects;
  Require(CEditorExportConventions::BuildObjects(
      TwoByFour.View(), CompleteGrouping, CompleteObjects, sError));
  Require(CompleteObjects.size() == 2u);
  Require(CompleteObjects[0].Entries.size() == TwoByFour.Primitives.size());
  Require(CompleteObjects[1].Entries.size() == TwoByFour.Primitives.size());
  bool bFoundRepeatedFront = false;
  bool bFoundAlternateWheel = false;
  bool bFoundBlackMirror = false;
  for (const tEdExportEntry &Entry : CompleteObjects[1].Entries) {
    if (Entry.uiPrimitive == 0u) {
      bFoundRepeatedFront = Entry.uiMaterial
          == TwoByFour.Primitives[0].uiFrontMaterialId;
    } else if (Entry.uiPrimitive == 25u) {
      bFoundAlternateWheel = Entry.uiMaterial
          == TwoByFour.Primitives[25].uiBackMaterialId;
    } else if (Entry.uiPrimitive == 33u) {
      bFoundBlackMirror = Entry.uiMaterial
          == TwoByFour.Primitives[33].uiBackMaterialId;
    }
  }
  Require(bFoundRepeatedFront);
  Require(bFoundAlternateWheel);
  Require(bFoundBlackMirror);

  // Advanced cars share their authored geometry with the normal car but use
  // the Y texture bank and the game's flat-colour remap. DESILVA maps CF to C3.
  tEdCarGeometry NormalDesilva;
  tEdCarGeometry AdvancedDesilva;
  Require(CEditorCarModel::Build(
      1u, false, 256u, NormalDesilva, sError));
  Require(CEditorCarModel::Build(
      1u, true, 256u, AdvancedDesilva, sError));
  bool bNormalColour = false;
  bool bAdvancedColour = false;
  for (const tEdMaterial &Material : NormalDesilva.Materials) {
    bNormalColour |= Material.uiKind == ROLLER_ED_MATERIAL_FLAT_PALETTE_COLOR
        && Material.uiPaletteColour == 0xCFu;
  }
  for (const tEdMaterial &Material : AdvancedDesilva.Materials) {
    bAdvancedColour |= Material.uiKind == ROLLER_ED_MATERIAL_FLAT_PALETTE_COLOR
        && Material.uiPaletteColour == 0xC3u;
  }
  Require(bNormalColour);
  Require(bAdvancedColour);
  return 0;
}
