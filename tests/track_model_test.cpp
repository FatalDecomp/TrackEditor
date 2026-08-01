#include "TrackModel.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
void PopulateTrack(CTrackModel &track)
{
  track.m_header.iHeaderUnk1 = 17;
  track.m_header.iHeaderUnk2 = 23;
  track.m_header.iFloorDepth = 4096;

  tGeometryChunk chunk;
  chunk.Default();
  chunk.iLength = 1750;
  chunk.dYaw = -15.0;
  chunk.dPitch = 12.5;
  chunk.dRoll = 370.0;
  chunk.iSignType = 0;
  chunk.iSignTexture = 9;
  track.m_chunkAy.push_back(chunk);

  tStunt stunt{};
  stunt.iChunkCount = 1;
  stunt.iNumTicks = 20;
  stunt.iHeight = 300;
  stunt.iFlags = 0x0c;
  track.m_stuntMap[0] = stunt;
  track.m_backsMap[4] = 12;

  track.m_sTextureFile = "ROAD.TEX";
  track.m_sBuildingFile = "SIGNS.TEX";
  track.m_raceInfo.iTrackNumber = 7;
  track.m_raceInfo.iImpossibleLaps = 2;
  track.m_raceInfo.iHardLaps = 3;
  track.m_raceInfo.iTrickyLaps = 4;
  track.m_raceInfo.iMediumLaps = 5;
  track.m_raceInfo.iEasyLaps = 6;
  track.m_raceInfo.iGirlieLaps = 7;
  track.m_raceInfo.dTrackMapSize = 1.25;
  track.m_raceInfo.iTrackMapFidelity = 8;
  track.m_raceInfo.dPreviewSize = 2.5;
}

void TestRoundTripAndCopyPaste()
{
  CTrackModel source;
  PopulateTrack(source);

  std::vector<std::uint8_t> serialized;
  source.GetTrackData(serialized);
  assert(!serialized.empty());

  CTrackModel loaded;
  assert(loaded.ProcessTrackData(serialized.data(), serialized.size()));
  assert(loaded.m_header.iNumChunks == 1);
  assert(loaded.m_header.iHeaderUnk1 == 17);
  assert(loaded.m_header.iHeaderUnk2 == 23);
  assert(loaded.m_header.iFloorDepth == 4096);
  assert(loaded.m_chunkAy.size() == 1);
  assert(loaded.m_chunkAy[0].iLength == 1750);
  assert(loaded.m_chunkAy[0].dYaw == 345.0);
  assert(loaded.m_chunkAy[0].dPitch == 12.5);
  assert(loaded.m_chunkAy[0].dRoll == 10.0);
  assert(loaded.m_chunkAy[0].iSignTexture == 9);
  assert(loaded.m_stuntMap.at(0).iHeight == 300);
  assert(loaded.m_backsMap.at(4) == 12);
  assert(loaded.m_sTextureFile == "ROAD.TEX");
  assert(loaded.m_sBuildingFile == "SIGNS.TEX");
  assert(loaded.m_raceInfo.iTrackNumber == 7);
  assert(loaded.m_raceInfo.dTrackMapSize == 1.25);

  std::vector<std::uint8_t> serializedAgain;
  loaded.GetTrackData(serializedAgain);
  assert(serializedAgain == serialized);

  CTrackModel target;
  target.m_chunkAy.push_back(loaded.m_chunkAy[0]);
  assert(target.m_chunkAy.size() == 1);
  assert(target.m_chunkAy[0].iLength == loaded.m_chunkAy[0].iLength);
  assert(target.m_chunkAy[0].iSignTexture == loaded.m_chunkAy[0].iSignTexture);
}

void TestMangledFileLoad()
{
  CTrackModel source;
  PopulateTrack(source);
  std::vector<std::uint8_t> serialized;
  source.GetTrackData(serialized);

  std::vector<std::uint8_t> mangled(4);
  const std::uint32_t serializedSize = static_cast<std::uint32_t>(serialized.size());
  for (int i = 0; i < 4; ++i)
    mangled[static_cast<size_t>(i)] = static_cast<std::uint8_t>(serializedSize >> (i * 8));
  for (size_t offset = 0; offset < serialized.size();) {
    const size_t literalCount = std::min<size_t>(0x3f, serialized.size() - offset);
    mangled.push_back(static_cast<std::uint8_t>(literalCount));
    mangled.insert(mangled.end(), serialized.begin() + offset,
                   serialized.begin() + offset + literalCount);
    offset += literalCount;
  }

  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "trackeditor-e3-s5a-track-model.trk";
  {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(mangled.data()),
               static_cast<std::streamsize>(mangled.size()));
    assert(file.good());
  }

  CTrackModel loaded;
  const bool loadedSuccessfully = loaded.LoadTrack(path.string());
  std::filesystem::remove(path);

  assert(loadedSuccessfully);
  assert(loaded.m_chunkAy.size() == 1);
  assert(loaded.m_chunkAy[0].iLength == 1750);
  assert(loaded.m_sTrackFile == path.string());
}

void TestUndoRedoHistory()
{
  CTrackModel track;
  PopulateTrack(track);
  CTrackHistory history;

  history.Save(track, "initial", 8);
  track.m_chunkAy[0].iLength = 2500;
  history.Save(track, "changed length", 8);
  track.m_chunkAy[0].iLength = 3000;

  assert(history.GetEntryCount() == 2);
  assert(history.Undo(track));
  assert(track.m_chunkAy[0].iLength == 1750);
  assert(history.GetCurrentEntry()->sDescription == "initial");

  assert(history.Redo(track));
  assert(track.m_chunkAy[0].iLength == 2500);
  assert(history.GetCurrentEntry()->sDescription == "changed length");

  assert(history.Undo(track));
  track.m_chunkAy[0].iLength = 4000;
  history.Save(track, "new branch", 8);
  assert(history.GetEntryCount() == 2);
  assert(history.GetCurrentEntry()->sDescription == "new branch");
  assert(history.Redo(track));
  assert(track.m_chunkAy[0].iLength == 4000);
}
}

int main()
{
  TestRoundTripAndCopyPaste();
  TestMangledFileLoad();
  TestUndoRedoHistory();
  return 0;
}
