#ifndef TRACKEDITOR_EDITORTOWERMODEL_H
#define TRACKEDITOR_EDITORTOWERMODEL_H

#include "TrackModel.h"

// The five file-loadable camera modes stored in iSignType's low nibble.
enum class eEditorTowerMode
{
  STATIC = -1,
  FOLLOW_NEAR = -2,
  FOLLOW_AT_DISTANCE = -3,
  TRACK_SURFACE_TWO_BACK = -4,
  OVERHEAD_FOLLOW = -5
};

// Qt-free file-format codec and range editor used by CEditTowerWidget. Keeping
// these rules out of the widget makes preservation of non-canonical values and
// the sign/tower mutual lockout directly testable.
class CEditorTowerModel
{
public:
  static constexpr int TOWER_TYPE_BASE = 256;
  static constexpr int TOWER_LIMIT = 32;
  static constexpr int MAX_CANONICAL_ZOOM = 4;

  static bool IsTower(int iSignType);
  static bool IsSign(int iSignType);
  static eEditorTowerMode DecodeMode(int iSignType);
  // Returns 0 for both canonical "Unchanged" and unsupported zoom selectors,
  // because the runtime leaves VIEWDIST unchanged in either case.
  static int DecodeZoom(int iSignType);
  static int Encode(eEditorTowerMode mode, int iZoom);
  static bool IsCanonical(int iSignType);
  static bool UsesHorizontalOffset(eEditorTowerMode mode);
  static bool UsesVerticalOffset(eEditorTowerMode mode);
  static int VerticalOffsetScale(eEditorTowerMode mode);

  static int CountTowers(const CChunkAy &chunkAy);
  static int AddTowers(CChunkAy &chunkAy, int iFrom, int iTo);
  static int DeleteTowers(CChunkAy &chunkAy, int iFrom, int iTo);
  static int SetMode(CChunkAy &chunkAy, int iFrom, int iTo,
                     eEditorTowerMode mode);
  static int SetZoom(CChunkAy &chunkAy, int iFrom, int iTo, int iZoom);
  static int SetHorizontalOffset(CChunkAy &chunkAy, int iFrom, int iTo,
                                 int iOffset);
  static int SetVerticalOffset(CChunkAy &chunkAy, int iFrom, int iTo,
                               int iOffset);
  static int SetRawType(CChunkAy &chunkAy, int iFrom, int iTo,
                        int iSignType);

private:
  static bool IsValidRange(const CChunkAy &chunkAy, int iFrom, int iTo);
  static int ModeNibble(eEditorTowerMode mode);
};

#endif
