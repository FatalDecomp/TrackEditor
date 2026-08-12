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
  Require(std::string(CEditorCarModel::Name(0)) == "AUTO");
  Require(std::string(CEditorCarModel::Name(12)) == "F1WACK");
  Require(std::string(CEditorCarModel::TextureFileName(12)) == "red28.bm");
  Require(CEditorCarModel::Name(CEditorCarModel::Count()) == nullptr);

  for (uint32_t uiDesign = 0; uiDesign < CEditorCarModel::Count(); ++uiDesign) {
    tEdCarGeometry Owned;
    std::string sError;
    Require(CEditorCarModel::Build(uiDesign, 256u, Owned, sError));
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
  Require(CEditorCarModel::Build(0u, 256u, Auto, sError));
  const tVec3 &Expected =
      CarDesigns[0].pCoords[CarDesigns[0].pPols[0].verts[0]];
  Require(std::fabs(Auto.Vertices[0].fPosition[0] - Expected.fX) < 0.001f);
  Require(std::fabs(Auto.Vertices[0].fPosition[1] - Expected.fY) < 0.001f);
  Require(std::fabs(Auto.Vertices[0].fPosition[2] - Expected.fZ) < 0.001f);

  tEdExportGrouping Grouping;
  Grouping.sSingleObjectName = "AUTO";
  Grouping.bSeparateBackFaces = false;
  std::vector<tEdExportObject> Objects;
  Require(CEditorExportConventions::BuildObjects(
      Auto.View(), Grouping, Objects, sError));
  Require(Objects.size() == 1u);
  Require(Objects[0].sName == "AUTO");
  Require(!Objects[0].Entries.empty());
  return 0;
}
