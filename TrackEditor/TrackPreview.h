#ifndef _TRACKEDITOR_TRACKPREVIEW_H
#define _TRACKEDITOR_TRACKPREVIEW_H
//-------------------------------------------------------------------------------------------------
#include <QWidget>
#include <vector>
#include "EditorCameraController.h"
#include "EditorExportCommon.h"
#include "EditorExportFormat.h"
#include "EditorOverlaySettings.h"
#include "EditorReferenceMesh.h"
#include "EditorRenderQueue.h"
#include "Types.h"
//-------------------------------------------------------------------------------------------------
#define DEFAULT_HISTORY_MAX_SIZE 256 //approx 200KB per saved track
//-------------------------------------------------------------------------------------------------
// eExportType and the format table live in EditorExportFormat.h (E4-S5).
//-------------------------------------------------------------------------------------------------
class CTrackPreviewPrivate;
class CTrack;
class CEditorRenderService;
class QTimer;
//-------------------------------------------------------------------------------------------------
class CTrackPreview : public QWidget
{
  Q_OBJECT

public:
  CTrackPreview(QWidget *pParent, CEditorRenderService *pRenderService,
                const QString &sTrackFile = "");
  ~CTrackPreview();

  void UpdateCameraPos(float fDeltaSeconds);
  bool LoadTrack(const QString &sFilename);
  void DeleteEnvirFloor();
  void UpdateTrack(bool bUpdatingStunt = false);
  void ShowModels(uint32 uiShowModels);
  void UpdateCar(eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus);
  void AttachLast(bool bAttachLast);
  void OpenReferenceModel();
  CTrack *GetTrack();
  bool SaveChangesAndContinue();
  bool SaveTrack();
  bool SaveTrackAs();
  bool Export(eExportType exportType);
  QString GetTitle(bool bFullPath);
  const QString &GetFilename() { return m_sTrackFile; };
  void UpdateGeometrySelection();
  void SaveHistory(const QString &sDescription, bool bDocumentEdit = true);
  void Undo();
  void Redo();
  void UpdateReferenceModelPos(double dYaw, double dPitch, double dRoll,
                               int iX, int iY, int iZ,
                               double dScale);
  void UpdateReferenceModelTexture();
  // E3A-S7. Whether the reference mesh should be drawn as edges. Follows the
  // dialog rather than the file.
  void UpdateReferenceModelWireframe(bool bWireframe);
  void Activate();
  void MarkDocumentEdited();
  bool CanExport() const { return m_FrameState.CanExport(); }

  bool m_bUnsavedChanges;
  int m_iSelFrom;
  int m_iSelTo;
  bool m_bToChecked;
  QString m_sReferenceModelFile;
  double m_dRefYaw;
  double m_dRefPitch;
  double m_dRefRoll;
  int m_iRefX;
  int m_iRefY;
  int m_iRefZ;
  double m_dRefScale;

protected:
  void paintEvent(QPaintEvent *pEvent) override;
  void resizeEvent(QResizeEvent *pEvent) override;
  void showEvent(QShowEvent *pEvent) override;
  void mousePressEvent(QMouseEvent *pEvent) override;
  void mouseReleaseEvent(QMouseEvent *pEvent) override;
  void mouseMoveEvent(QMouseEvent *pEvent) override;
  void keyPressEvent(QKeyEvent *pEvent) override;
  void keyReleaseEvent(QKeyEvent *pEvent) override;

signals:
  void ReferenceModelChanged();
  void FrameStateChanged();

private:
  bool SaveTrack_Internal(const QString &sFilename);
  // E4-S1. Writes the OBJ/MTL pair from ROLLER's canonical geometry rather
  // than from WhipLib's CPU derivation.
  bool ExportObj_Internal(const QString &sFolder, const QString &sName,
                          const QString &sFilename, bool bExportScenery,
                          bool bSeparateSections, bool bSeparateBackFaces);
  // E4-S2. Same canonical geometry, glTF 2.0 instead of OBJ.
  bool ExportGltf_Internal(const QString &sFolder, const QString &sName,
                           const QString &sFilename, bool bExportScenery,
                           bool bSeparateSections, bool bSeparateBackFaces);
  // Both canonical exporters need the extraction; neither may call the facade
  // from UI code, so both go through the render worker.
  bool ExtractCanonicalGeometry(tEdGeometrySnapshot &SnapshotOut);
  std::vector<tEdExportPaletteEntry> BuildExportPalette() const;
  void UpdateReferenceModelPos_Internal();
  // Hands the worker the mesh exactly once per change: null on every other
  // frame, so a camera nudge does not copy the whole model through the queue.
  const tEdReferenceMeshPayload *TakePendingReferenceMesh();
  void ScheduleReferenceMeshUpload();
  void QueueLoadAndRender();
  void QueueEditedTrackReload();
  void QueueResizeRender();
  void ScheduleCameraRender();
  void ArmCameraRenderTimer();
  void QueueCameraRender();
  void OnRenderCompleted(const tEdRenderResult &Result);
  QSize DevicePixelSize() const;

  CTrackPreviewPrivate *p;
  uint32 m_uiShowModels;
  eWhipModel m_carModel;
  eShapeSection m_carAILine;
  bool m_bMillionPlus;
  bool m_bAttachLast;
  int m_iScale;
  bool m_bAlreadySaved;
  QString m_sTrackFile;
  QString m_sDocumentAssetRoot;
  QString m_sLastCarTex;
  CEditorRenderService *m_pRenderService;
  uint64_t m_ullDocumentId;
  CDocumentFrameState m_FrameState;
  CEditorCameraController m_CameraController;
  CEditorOverlaySettings m_OverlaySettings;
  CEditorReferenceMesh m_ReferenceMesh;
  tEdReferenceMeshPayload m_PendingReferenceMesh;
  bool m_bReferenceMeshDirty = false;
  QTimer *m_pResizeTimer;
  QTimer *m_pEditTimer;
  QTimer *m_pCameraRenderTimer;
  uint64_t m_ullCameraRequestId;
  bool m_bCameraRenderPending;
  bool m_bReloadPending;
};

//-------------------------------------------------------------------------------------------------
#endif
