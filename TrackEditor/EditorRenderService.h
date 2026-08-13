#ifndef TRACKEDITOR_EDITORRENDERSERVICE_H
#define TRACKEDITOR_EDITORRENDERSERVICE_H

#include "EditorRenderQueue.h"

#include <QMutex>
#include <QObject>
#include <QSize>
#include <QString>
#include <QWaitCondition>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

class CEditorRenderThread;

// E4-S1. The export path needs the canonical geometry on the calling thread,
// but RollerEd_QueryGeometrySizes / RollerEd_FillGeometry may only be called
// on the render worker. The caller blocks on this slot rather than on the Qt
// event loop, so a modal export can wait for it from the UI thread without
// deadlocking against its own queued result delivery.
struct tEdGeometryExtraction
{
  QMutex Mutex;
  QWaitCondition Completed;
  bool bComplete = false;
  eRollerEdResult eResult = ROLLER_ED_RESULT_INTERNAL_ERROR;
  std::string sErrorText;
  tEdGeometrySnapshot Snapshot;
};

class CEditorRenderService : public QObject
{
  Q_OBJECT

public:
  explicit CEditorRenderService(const QString &sAssetRoot, QObject *pParent = nullptr);
  ~CEditorRenderService() override;

  void Start();
  void Stop();
  void SetGraphicsSettings(const tEdGraphicsSettings &Settings);
  void RegisterDocument(uint64_t ullDocumentId);
  void InvalidateDocument(uint64_t ullDocumentId);

  uint64_t EnqueueLoadAndRender(uint64_t ullDocumentId,
                                uint64_t ullDocumentRevision,
                                const QString &sTrackPath,
                                const QString &sDocumentAssetRoot,
                                const QSize &DevicePixelSize,
                                double dDevicePixelRatio,
                                const tEdCameraState &Camera,
                                const tEdOverlayState &Overlay);
  uint64_t EnqueueSerializedLoadAndRender(
      uint64_t ullDocumentId,
      uint64_t ullDocumentRevision,
      const std::vector<uint8_t> &SerializedTrackData,
      const QString &sDocumentAssetRoot,
      const QSize &DevicePixelSize,
      double dDevicePixelRatio,
      const tEdCameraState &Camera,
      const tEdOverlayState &Overlay);
  uint64_t EnqueueUnload(uint64_t ullDocumentId,
                         uint64_t ullDocumentRevision);
  // pReferenceMesh is null on every frame the mesh did not change (E3A-S7).
  // Uploading it on each camera nudge would copy the whole model through the
  // queue every frame, and the core keeps the last one it was given.
  uint64_t EnqueueRender(uint64_t ullDocumentId,
                         uint64_t ullDocumentRevision,
                         uint32_t uiExpectedGeometryEpoch,
                         const QSize &DevicePixelSize,
                         double dDevicePixelRatio,
                         const tEdCameraState &Camera,
                         const tEdOverlayState &Overlay,
                         const tEdReferenceMeshPayload *pReferenceMesh = nullptr,
                         uint32_t uiStuntTicks = 0);

  // E4-S1. Blocks the calling thread until the worker has copied the
  // extraction for the document that currently owns the worker scene.
  // SnapshotOut is written only on ROLLER_ED_RESULT_OK.
  eRollerEdResult ExtractGeometry(uint64_t ullDocumentId,
                                  uint64_t ullDocumentRevision,
                                  tEdGeometrySnapshot &SnapshotOut,
                                  std::string &sErrorOut);

signals:
  void FrameCompleted(const tEdRenderResult &Result);

private:
  friend class CEditorRenderThread;
  void PublishResult(tEdRenderResult Result);
  bool IsDocumentRegistered(uint64_t ullDocumentId) const;

  CEditorRenderThread *m_pThread;
  std::unordered_set<uint64_t> m_RegisteredDocuments;
  tEdGraphicsSettings m_GraphicsSettings;
  uint64_t m_ullGraphicsSettingsRevision;
};

#endif
