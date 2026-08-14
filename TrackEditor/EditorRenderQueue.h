#ifndef TRACKEDITOR_EDITORRENDERQUEUE_H
#define TRACKEDITOR_EDITORRENDERQUEUE_H

#include "editor_api.h"

#include <QImage>

#include <cstdint>
#include <memory>
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
  UNLOAD,
  RENDER_ONLY,
  // E4-S1. Copies the canonical extraction out of the worker for export. It
  // renders nothing, sets no camera or overlay, and touches neither the
  // geometry epoch nor the track generation.
  EXTRACT_GEOMETRY
};

// E4-S1. An owned copy of one canonical extraction. RollerEd_FillGeometry
// never lets core-owned pointers escape, so the worker copies into these
// vectors and the exporter reads them on the calling thread.
struct tEdGeometrySnapshot
{
  std::vector<tEdVertex> Vertices;
  std::vector<uint32_t> Indices;
  std::vector<tEdPrimitive> Primitives;
  std::vector<tEdMaterial> Materials;
  uint32_t uiGeometryEpoch = 0;
  uint32_t uiTrackGeneration = 0;
};

// AD-16 again, and for the same reason: the facade copies the mesh during the
// call, but the call happens on the worker thread long after the UI built it,
// so the command carries its own copy of every array. E3A-S7.
struct tEdReferenceMeshPayload
{
  std::vector<tEdReferenceVertex> Vertices;
  std::vector<uint32_t> Indices;
  float fPosition[3] = { 0.0f, 0.0f, 0.0f };
  float fRotation[3] = { 0.0f, 0.0f, 0.0f };
  float fScale[3] = { 1.0f, 1.0f, 1.0f };
  uint32_t uiFlags = 0;
};

// E4-S1. Defined by the render service: the caller blocks on it while the
// worker fills the snapshot, so it outlives the request by construction.
struct tEdGeometryExtraction;

struct tEdRenderRequest
{
  tEdRenderRequestTag Tag = {};
  eEdRenderCommandKind eKind = eEdRenderCommandKind::RENDER_ONLY;
  std::string sTrackPath;
  std::string sDocumentAssetRoot;
  std::string sFallbackAssetRoot;
  std::vector<uint8_t> SerializedTrackData;
  tEdCameraState Camera = {};
  bool bHasCamera = false;
  // AD-16: the overlay state is copied into the command like every other
  // pointer payload, so the worker never reads UI-owned storage.
  tEdOverlayState Overlay = {};
  bool bHasOverlay = false;
  // Graphics settings are process-wide renderer state, but each command
  // carries the revision it was queued against. The worker applies a changed
  // revision before touching the scene and never reads UI-owned storage.
  tEdGraphicsSettings GraphicsSettings = {};
  uint64_t ullGraphicsSettingsRevision = 0;
  // Fixed legacy game ticks to apply on the worker before this frame. Stunt
  // geometry is runtime preview state and never mutates the document model.
  uint32_t uiStuntTicks = 0;
  // Only set when the reference mesh actually changed: uploading it on every
  // camera nudge would copy the whole model through the queue each frame.
  tEdReferenceMeshPayload ReferenceMesh;
  bool bHasReferenceMesh = false;
  // Only set on EXTRACT_GEOMETRY. Every path that drops a queued request has
  // to complete it, or the blocked caller never wakes.
  std::shared_ptr<tEdGeometryExtraction> Extraction;
  uint32_t uiWidth = 0;
  uint32_t uiHeight = 0;
  double dDevicePixelRatio = 1.0;
};

struct tEdRenderResult
{
  tEdRenderResultTag Tag = {};
  uint32_t uiRenderedGeometryEpoch = 0;
  bool bLoadFailed = false;
  bool bSceneEmpty = false;
  // E7-S6. LOAD results replace the document's committed tower table. A
  // render-only result leaves it alone, so camera motion cannot accidentally
  // discard the positions obtained by the worker during the last load.
  std::vector<tEdTowerInfo> Towers;
  bool bHasTowerSnapshot = false;
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
