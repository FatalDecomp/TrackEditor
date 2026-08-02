#include "TrackGeometry.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool bCondition, const char *szMessage)
{
  if (!bCondition) {
    std::cerr << "requirement failed: " << szMessage << '\n';
    std::exit(1);
  }
}

void TestGeometryIsDerivedOnDemand()
{
  CTrackModel Track;
  tGeometryChunk Chunk;
  Chunk.Default();
  Chunk.iLength = 1000;
  Track.m_chunkAy.push_back(Chunk);
  Track.m_chunkAy.push_back(Chunk);

  const CTrackGeometry Geometry(Track);
  const CDerivedTrackChunkAy &Chunks = Geometry.GetChunks();
  Require(Chunks.size() == Track.m_chunkAy.size(), "one derived row per scalar chunk");
  Require(Chunks.front().center == glm::vec3(0.0f), "first chunk starts at the model origin");
  Require(std::isfinite(Chunks[1].center.x)
              && std::isfinite(Chunks[1].center.y)
              && std::isfinite(Chunks[1].center.z),
          "derived geometry is finite");

  const glm::vec3 OriginalSecondCenter = Chunks[1].center;
  Track.m_chunkAy[0].iLength = 1750;
  const CTrackGeometry UpdatedGeometry(Track);
  Require(UpdatedGeometry.GetChunks()[1].center != OriginalSecondCenter,
          "scalar edits affect newly derived geometry");
}
}

int main()
{
  TestGeometryIsDerivedOnDemand();
  std::cout << "E3-S5c on-demand geometry tests passed\n";
  return 0;
}
