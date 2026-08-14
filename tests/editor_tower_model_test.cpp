#include "EditorTowerModel.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>

namespace
{
tGeometryChunk ChunkWithType(int iSignType)
{
  tGeometryChunk Chunk;
  Chunk.Default();
  Chunk.iSignType = iSignType;
  return Chunk;
}

void test_every_mode_and_zoom_round_trips_canonically()
{
  const eEditorTowerMode aModes[] = {
    eEditorTowerMode::STATIC,
    eEditorTowerMode::FOLLOW_NEAR,
    eEditorTowerMode::FOLLOW_AT_DISTANCE,
    eEditorTowerMode::TRACK_SURFACE_TWO_BACK,
    eEditorTowerMode::OVERHEAD_FOLLOW
  };

  for (const eEditorTowerMode mode : aModes) {
    for (int iZoom = 0; iZoom <= 4; ++iZoom) {
      const int iRaw = CEditorTowerModel::Encode(mode, iZoom);
      assert(iRaw >= 256 && iRaw < 336);
      assert(CEditorTowerModel::DecodeMode(iRaw) == mode);
      assert(CEditorTowerModel::DecodeZoom(iRaw) == iZoom);
      assert(CEditorTowerModel::IsCanonical(iRaw));
    }
  }

  assert(CEditorTowerModel::Encode(eEditorTowerMode::TRACK_SURFACE_TWO_BACK, 0)
         == 0x101);
  assert(CEditorTowerModel::Encode(eEditorTowerMode::FOLLOW_NEAR, 4) == 0x143);
  assert(CEditorTowerModel::Encode(eEditorTowerMode::OVERHEAD_FOLLOW, 2) == 0x124);
  assert(CEditorTowerModel::Encode(eEditorTowerMode::FOLLOW_AT_DISTANCE, 3)
         == 0x135);
}

void test_lossy_values_decode_best_effort_without_becoming_canonical()
{
  // Zoom selectors 5+ have the same runtime behavior as zero: VIEWDIST is
  // unchanged. Their raw spelling must nevertheless survive until a decoded
  // control is edited.
  const int iZoomNineFollowNear = 256 + 16 * 9 + 3;
  assert(CEditorTowerModel::DecodeMode(iZoomNineFollowNear)
         == eEditorTowerMode::FOLLOW_NEAR);
  assert(CEditorTowerModel::DecodeZoom(iZoomNineFollowNear) == 0);
  assert(!CEditorTowerModel::IsCanonical(iZoomNineFollowNear));

  assert(CEditorTowerModel::DecodeMode(512) == eEditorTowerMode::STATIC);
  assert(CEditorTowerModel::DecodeZoom(512) == 0);
  assert(!CEditorTowerModel::IsCanonical(512));
  assert(!CEditorTowerModel::IsCanonical(258)); // unknown low nibble
}

void test_mode_specific_offset_rules_match_the_runtime()
{
  assert(CEditorTowerModel::UsesHorizontalOffset(eEditorTowerMode::STATIC));
  assert(CEditorTowerModel::UsesVerticalOffset(eEditorTowerMode::STATIC));
  assert(!CEditorTowerModel::UsesHorizontalOffset(
      eEditorTowerMode::TRACK_SURFACE_TWO_BACK));
  assert(!CEditorTowerModel::UsesVerticalOffset(
      eEditorTowerMode::TRACK_SURFACE_TWO_BACK));
  assert(!CEditorTowerModel::UsesHorizontalOffset(
      eEditorTowerMode::OVERHEAD_FOLLOW));
  assert(CEditorTowerModel::UsesVerticalOffset(
      eEditorTowerMode::OVERHEAD_FOLLOW));
  assert(CEditorTowerModel::VerticalOffsetScale(
      eEditorTowerMode::OVERHEAD_FOLLOW) == 128);
  assert(CEditorTowerModel::VerticalOffsetScale(eEditorTowerMode::STATIC) == 32);
}

void test_range_lifecycle_skips_signs_and_obeys_the_budget()
{
  CChunkAy chunkAy = {
    ChunkWithType(-1), ChunkWithType(9), ChunkWithType(-1),
    ChunkWithType(CEditorTowerModel::Encode(eEditorTowerMode::STATIC, 0))
  };

  assert(CEditorTowerModel::AddTowers(chunkAy, 0, 3) == 2);
  assert(CEditorTowerModel::IsTower(chunkAy[0].iSignType));
  assert(chunkAy[1].iSignType == 9);
  assert(CEditorTowerModel::IsTower(chunkAy[2].iSignType));
  assert(CEditorTowerModel::CountTowers(chunkAy) == 3);

  assert(CEditorTowerModel::DeleteTowers(chunkAy, 0, 3) == 3);
  assert(chunkAy[0].iSignType == -1);
  assert(chunkAy[1].iSignType == 9);
  assert(chunkAy[2].iSignType == -1);
  assert(chunkAy[3].iSignType == -1);

  CChunkAy budgetAy(34, ChunkWithType(-1));
  for (int i = 0; i < 31; ++i)
    budgetAy[i].iSignType = 256;
  budgetAy[31].iSignType = 4;
  assert(CEditorTowerModel::AddTowers(budgetAy, 31, 33) == 1);
  assert(budgetAy[31].iSignType == 4);
  assert(CEditorTowerModel::CountTowers(budgetAy) == 32);
  assert(CEditorTowerModel::AddTowers(budgetAy, 31, 33) == 0);
}

void test_range_edits_touch_existing_towers_only()
{
  CChunkAy chunkAy = {
    ChunkWithType(CEditorTowerModel::Encode(eEditorTowerMode::STATIC, 2)),
    ChunkWithType(7),
    ChunkWithType(-1),
    ChunkWithType(256 + 16 * 9 + 3)
  };

  assert(CEditorTowerModel::SetMode(
      chunkAy, 0, 3, eEditorTowerMode::OVERHEAD_FOLLOW) == 2);
  assert(chunkAy[0].iSignType
         == CEditorTowerModel::Encode(eEditorTowerMode::OVERHEAD_FOLLOW, 2));
  // The unsupported zoom canonicalizes to its best-effort runtime behavior.
  assert(chunkAy[3].iSignType
         == CEditorTowerModel::Encode(eEditorTowerMode::OVERHEAD_FOLLOW, 0));
  assert(chunkAy[1].iSignType == 7);
  assert(chunkAy[2].iSignType == -1);

  assert(CEditorTowerModel::SetHorizontalOffset(chunkAy, 0, 3, -12) == 2);
  assert(CEditorTowerModel::SetVerticalOffset(chunkAy, 0, 3, 45) == 2);
  assert(chunkAy[0].iSignHorizOffset == -12);
  assert(chunkAy[3].iSignVertOffset == 45);
  assert(chunkAy[1].iSignHorizOffset == 0);

  assert(CEditorTowerModel::SetRawType(chunkAy, 0, 3, 777) == 2);
  assert(chunkAy[0].iSignType == 777);
  assert(chunkAy[3].iSignType == 777);
  assert(chunkAy[1].iSignType == 7);
  assert(CEditorTowerModel::SetRawType(chunkAy, 0, 3, 10) == 0);
}

void test_tower_fields_round_trip_and_history_restores_them()
{
  CTrackModel track;
  track.m_chunkAy = { ChunkWithType(-1), ChunkWithType(-1) };
  CTrackHistory history;
  history.Save(track, "before tower", 8);

  assert(CEditorTowerModel::AddTowers(track.m_chunkAy, 0, 0) == 1);
  assert(CEditorTowerModel::SetMode(
      track.m_chunkAy, 0, 0, eEditorTowerMode::FOLLOW_AT_DISTANCE) == 1);
  assert(CEditorTowerModel::SetZoom(track.m_chunkAy, 0, 0, 4) == 1);
  assert(CEditorTowerModel::SetHorizontalOffset(track.m_chunkAy, 0, 0, -123) == 1);
  assert(CEditorTowerModel::SetVerticalOffset(track.m_chunkAy, 0, 0, 456) == 1);
  history.Save(track, "edited tower", 8);

  std::vector<std::uint8_t> bytes;
  track.GetTrackData(bytes);
  CTrackModel loaded;
  assert(loaded.ProcessTrackData(bytes.data(), bytes.size()));
  assert(loaded.m_chunkAy[0].iSignType
         == CEditorTowerModel::Encode(eEditorTowerMode::FOLLOW_AT_DISTANCE, 4));
  assert(loaded.m_chunkAy[0].iSignHorizOffset == -123);
  assert(loaded.m_chunkAy[0].iSignVertOffset == 456);
  std::vector<std::uint8_t> roundTrip;
  loaded.GetTrackData(roundTrip);
  assert(roundTrip == bytes);

  assert(history.Undo(track));
  assert(track.m_chunkAy[0].iSignType == -1);
  assert(history.Redo(track));
  assert(track.m_chunkAy[0].iSignType
         == CEditorTowerModel::Encode(eEditorTowerMode::FOLLOW_AT_DISTANCE, 4));

  // An unsupported raw value is ordinary document data and survives when no
  // decoded control canonicalizes it.
  track.m_chunkAy[1].iSignType = 777;
  bytes.clear();
  track.GetTrackData(bytes);
  CTrackModel loadedRaw;
  assert(loadedRaw.ProcessTrackData(bytes.data(), bytes.size()));
  assert(loadedRaw.m_chunkAy[1].iSignType == 777);
}
}

int main()
{
  test_every_mode_and_zoom_round_trips_canonically();
  test_lossy_values_decode_best_effort_without_becoming_canonical();
  test_mode_specific_offset_rules_match_the_runtime();
  test_range_lifecycle_skips_signs_and_obeys_the_budget();
  test_range_edits_touch_existing_towers_only();
  test_tower_fields_round_trip_and_history_restores_them();
  return 0;
}
