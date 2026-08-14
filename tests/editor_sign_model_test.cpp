#include "EditorSignModel.h"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <type_traits>

#ifdef assert
#undef assert
#endif
#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "assertion failed: " #condition << " (" << __FILE__       \
                << ':' << __LINE__ << ")\n";                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (false)

namespace
{
void TestTypeBoundariesAndSafeTableIndexPredicate()
{
  assert(!CEditorSignModel::IsTower(-1));
  assert(!CEditorSignModel::IsTower(255));
  assert(CEditorSignModel::IsTower(256));
  assert(CEditorSignModel::IsTower(777));

  assert(!CEditorSignModel::IsSign(-1));
  assert(CEditorSignModel::IsSign(0));
  assert(CEditorSignModel::IsSign(255));
  assert(!CEditorSignModel::IsSign(256));

  assert(!CEditorSignModel::IsKnownSignIndex(-1, 17));
  assert(CEditorSignModel::IsKnownSignIndex(0, 17));
  assert(CEditorSignModel::IsKnownSignIndex(16, 17));
  assert(!CEditorSignModel::IsKnownSignIndex(17, 17));
  assert(!CEditorSignModel::IsKnownSignIndex(256, 17));
}

void TestMixedRangeLeavesEveryTowerByteIdentical()
{
  static_assert(std::is_trivially_copyable<tGeometryChunk>::value,
                "byte-identity regression requires a trivially copyable chunk");

  CChunkAy Chunks(4);
  for (tGeometryChunk &Chunk : Chunks)
    Chunk.Default();
  Chunks[0].iSignType = 9;
  Chunks[1].iSignType = 256;
  Chunks[2].iSignType = 777;
  Chunks[3].iSignType = -1;
  Chunks[1].iSignHorizOffset = 123;
  Chunks[1].iSignVertOffset = -456;
  Chunks[1].dSignYaw = 12.5;
  Chunks[1].dSignPitch = 34.5;
  Chunks[1].dSignRoll = 56.5;
  Chunks[1].iSignTexture = 0x12345678;
  Chunks[2].iSignHorizOffset = -321;
  Chunks[2].iSignTexture = 0x76543210;

  const tGeometryChunk TowerBefore = Chunks[1];
  const tGeometryChunk RawTowerBefore = Chunks[2];
  const int iEdited = CEditorSignModel::ApplyToRange(
      Chunks, 0, 3, [](tGeometryChunk &Chunk) {
        Chunk.iSignType = 4;
        Chunk.iSignHorizOffset = 999;
        Chunk.iSignVertOffset = 888;
        Chunk.dSignYaw = 111.0;
        Chunk.dSignPitch = 222.0;
        Chunk.dSignRoll = 333.0;
        Chunk.iSignTexture = 0x0badcafe;
      });

  assert(iEdited == 2);
  assert(Chunks[0].iSignType == 4);
  assert(Chunks[3].iSignType == 4);
  assert(std::memcmp(&Chunks[1], &TowerBefore, sizeof(TowerBefore)) == 0);
  assert(std::memcmp(&Chunks[2], &RawTowerBefore,
                     sizeof(RawTowerBefore)) == 0);
}

void TestInvalidRangesWriteNothing()
{
  CChunkAy Chunks(1);
  Chunks[0].Default();
  Chunks[0].iSignType = 9;
  const tGeometryChunk Before = Chunks[0];
  const CEditorSignModel::tEditOperation Edit = [](tGeometryChunk &Chunk) {
    Chunk.iSignType = 2;
  };

  assert(CEditorSignModel::ApplyToRange(Chunks, -1, 0, Edit) == 0);
  assert(CEditorSignModel::ApplyToRange(Chunks, 0, 1, Edit) == 0);
  assert(CEditorSignModel::ApplyToRange(Chunks, 0, 0, {}) == 0);
  assert(std::memcmp(&Chunks[0], &Before, sizeof(Before)) == 0);
}
}

int main()
{
  TestTypeBoundariesAndSafeTableIndexPredicate();
  TestMixedRangeLeavesEveryTowerByteIdentical();
  TestInvalidRangesWriteNothing();
  std::cout << "E7-S7 sign/tower exclusion tests passed\n";
  return 0;
}
