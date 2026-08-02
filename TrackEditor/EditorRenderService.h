#ifndef TRACKEDITOR_EDITORRENDERSERVICE_H
#define TRACKEDITOR_EDITORRENDERSERVICE_H

#include "EditorRenderQueue.h"

#include <QObject>
#include <QSize>
#include <QString>

#include <cstdint>
#include <unordered_set>
#include <vector>

class CEditorRenderThread;

class CEditorRenderService : public QObject
{
  Q_OBJECT

public:
  explicit CEditorRenderService(const QString &sAssetRoot, QObject *pParent = nullptr);
  ~CEditorRenderService() override;

  void Start();
  void Stop();
  void RegisterDocument(uint64_t ullDocumentId);
  void InvalidateDocument(uint64_t ullDocumentId);

  uint64_t EnqueueLoadAndRender(uint64_t ullDocumentId,
                                uint64_t ullDocumentRevision,
                                const QString &sTrackPath,
                                const QString &sDocumentAssetRoot,
                                const QSize &DevicePixelSize,
                                double dDevicePixelRatio,
                                const tEdCameraState &Camera);
  uint64_t EnqueueSerializedLoadAndRender(
      uint64_t ullDocumentId,
      uint64_t ullDocumentRevision,
      const std::vector<uint8_t> &SerializedTrackData,
      const QString &sDocumentAssetRoot,
      const QSize &DevicePixelSize,
      double dDevicePixelRatio,
      const tEdCameraState &Camera);
  uint64_t EnqueueUnload(uint64_t ullDocumentId,
                         uint64_t ullDocumentRevision);
  uint64_t EnqueueRender(uint64_t ullDocumentId,
                         uint64_t ullDocumentRevision,
                         uint32_t uiExpectedGeometryEpoch,
                         const QSize &DevicePixelSize,
                         double dDevicePixelRatio,
                         const tEdCameraState &Camera);

signals:
  void FrameCompleted(const tEdRenderResult &Result);

private:
  friend class CEditorRenderThread;
  void PublishResult(tEdRenderResult Result);
  bool IsDocumentRegistered(uint64_t ullDocumentId) const;

  CEditorRenderThread *m_pThread;
  std::unordered_set<uint64_t> m_RegisteredDocuments;
};

#endif
