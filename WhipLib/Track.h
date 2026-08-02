#ifndef _WHIPLIB_TRACK_H
#define _WHIPLIB_TRACK_H
//-------------------------------------------------------------------------------------------------
#include "TrackModel.h"
#include "TrackAssets.h"
#include "Types.h"
//-------------------------------------------------------------------------------------------------
class CTrack : public CTrackModel
{
public:
  CTrack();
  ~CTrack() override;

  bool ShouldShowChunkSection(int i, eShapeSection section);
  bool HasPitchedStunt();

  static bool ShouldDrawSurfaceType(int iSurfaceType);

  CTrackAssets m_assets;
};

//-------------------------------------------------------------------------------------------------
#endif
