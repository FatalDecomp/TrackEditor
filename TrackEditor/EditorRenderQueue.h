#ifndef TRACKEDITOR_EDITORRENDERQUEUE_H
#define TRACKEDITOR_EDITORRENDERQUEUE_H

#include "editor_api.h"

#include <QImage>

#include <cstdint>
#include <string>
#include <vector>

enum
{
  ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH = 1u << 0
};

typedef struct
{
  uint64_t ullRequestId;
  uint64_t ullDocumentId;
  uint64_t ullDocumentRevision;
  uint32_t uiExpectedGeometryEpoch;
  uint32_t uiFlags;
} tEdRenderRequestTag;

typedef struct
{
  uint64_t ullRequestId;
  uint64_t ullDocumentId;
  uint64_t ullDocumentRevision;
  uint32_t uiActualGeometryEpoch;
  eRollerEdResult eResult;
} tEdRenderResultTag;

enum class eEdRenderCommandKind
{
  LOAD_AND_RENDER,
  LOAD_SERIALIZED_AND_RENDER,
  RENDER_ONLY
};

struct tEdRenderRequest
{
  tEdRenderRequestTag Tag = {};
  eEdRenderCommandKind eKind = eEdRenderCommandKind::RENDER_ONLY;
  std::string sTrackPath;
  std::string sDocumentAssetRoot;
  std::vector<uint8_t> SerializedTrackData;
  tEdCameraState Camera = {};
  bool bHasCamera = false;
  uint32_t uiWidth = 0;
  uint32_t uiHeight = 0;
  double dDevicePixelRatio = 1.0;
};

struct tEdRenderResult
{
  tEdRenderResultTag Tag = {};
  uint32_t uiRenderedGeometryEpoch = 0;
  bool bLoadFailed = false;
  QImage Image;
  std::string sErrorText;
};

enum class eEdFrameDisplayState
{
  PLACEHOLDER,
  CURRENT,
  STALE_AFTER_LOAD_FAILURE
};

class CEditorRenderIds
{
public:
  static uint64_t NextRequestId();
  static uint64_t NextDocumentId();
};

class CDocumentFrameState
{
public:
  explicit CDocumentFrameState(uint64_t ullDocumentId);

  void BeginRequest(uint64_t ullRequestId);
  void MarkDocumentEdited();
  void Invalidate();
  bool ApplyResult(const tEdRenderResult &Result);

  uint64_t GetDocumentId() const { return m_ullDocumentId; }
  uint64_t GetDocumentRevision() const { return m_ullDocumentRevision; }
  uint64_t GetLatestRequestId() const { return m_ullLatestRequestId; }
  uint32_t GetInstalledGeometryEpoch() const { return m_uiInstalledGeometryEpoch; }
  bool IsValid() const { return m_bValid; }
  bool CanExport() const { return m_bExportEnabled; }
  eEdFrameDisplayState GetDisplayState() const { return m_eDisplayState; }
  const QImage &GetImage() const { return m_Image; }
  const std::string &GetErrorText() const { return m_sErrorText; }

private:
  bool MatchesCurrentRequest(const tEdRenderResultTag &Tag) const;

  uint64_t m_ullDocumentId;
  uint64_t m_ullDocumentRevision;
  uint64_t m_ullLatestRequestId;
  uint32_t m_uiInstalledGeometryEpoch;
  bool m_bValid;
  bool m_bExportEnabled;
  eEdFrameDisplayState m_eDisplayState;
  QImage m_Image;
  std::string m_sErrorText;
};

#endif
