#include "glew.h"
#include "TrackPreview.h"
#include "MainWindow.h"
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include "Camera.h"
#include "Shader.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "OpenGLDebug.h"
#include "Track.h"
#include "DisplaySettings.h"
#include "ShapeData.h"
#include "ShapeFactory.h"
#include "Texture.h"
#include "FBXExporter.h"
#include "CarHelpers.h"
#include "Entity.h"
#include "NoclipComponent.h"
#include "qevent.h"
#include "qdir.h"
#include "qmessagebox.h"
#include "qfiledialog.h"
#include "qfile.h"
#include "qtextstream.h"
#include <fstream>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

class CTrackPreviewPrivate
{
public:
  CTrackPreviewPrivate()
    : m_pShader(NULL)
    , m_pCenterSurf(NULL)
    , m_pCenterWire(NULL)
    , m_pLShoulderSurf(NULL)
    , m_pLShoulderWire(NULL)
    , m_pRShoulderSurf(NULL)
    , m_pRShoulderWire(NULL)
    , m_pLWallSurf(NULL)
    , m_pLWallWire(NULL)
    , m_pRWallSurf(NULL)
    , m_pRWallWire(NULL)
    , m_pRoofSurf(NULL)
    , m_pRoofWire(NULL)
    , m_pOWallFloorSurf(NULL)
    , m_pOWallFloorWire(NULL)
    , m_pLLOWallSurf(NULL)
    , m_pLLOWallWire(NULL)
    , m_pRLOWallSurf(NULL)
    , m_pRLOWallWire(NULL)
    , m_pLUOWallSurf(NULL)
    , m_pLUOWallWire(NULL)
    , m_pRUOWallSurf(NULL)
    , m_pRUOWallWire(NULL)
    , m_pSelection(NULL)
    , m_pAILine1(NULL)
    , m_pAILine2(NULL)
    , m_pAILine3(NULL)
    , m_pAILine4(NULL)
    , m_pEnvirFloor(NULL)
    , m_pAxes(NULL)
    , m_pCar(NULL)
    , m_pTestNormals(NULL)
    , m_noclipComponent(true)
  {
    m_entity.Init();
    m_noclipComponent.Init();
    m_entity.AddComponent(&m_noclipComponent);
  };
  ~CTrackPreviewPrivate()
  {
    m_entity.Shutdown();
    DeleteModels();
    if (m_pEnvirFloor) {
      delete m_pEnvirFloor;
      m_pEnvirFloor = NULL;
    }
    if (m_pCar) {
      delete m_pCar;
      m_pCar = NULL;
    }
    if (m_pShader) {
      delete m_pShader;
      m_pShader = NULL;
    }
    if (m_pAxes) {
      delete m_pAxes;
      m_pAxes = NULL;
    }
    if (m_pTestNormals) {
      delete m_pTestNormals;
      m_pTestNormals = NULL;
    }
  };
  void DeleteModels()
  {
    if (m_pCenterSurf) {
      delete m_pCenterSurf;
      m_pCenterSurf = NULL;
    }
    if (m_pCenterWire) {
      delete m_pCenterWire;
      m_pCenterWire = NULL;
    }
    if (m_pLShoulderSurf) {
      delete m_pLShoulderSurf;
      m_pLShoulderSurf = NULL;
    }
    if (m_pLShoulderWire) {
      delete m_pLShoulderWire;
      m_pLShoulderWire = NULL;
    }
    if (m_pRShoulderSurf) {
      delete m_pRShoulderSurf;
      m_pRShoulderSurf = NULL;
    }
    if (m_pRShoulderWire) {
      delete m_pRShoulderWire;
      m_pRShoulderWire = NULL;
    }
    if (m_pLWallSurf) {
      delete m_pLWallSurf;
      m_pLWallSurf = NULL;
    }
    if (m_pLWallWire) {
      delete m_pLWallWire;
      m_pLWallWire = NULL;
    }
    if (m_pRWallSurf) {
      delete m_pRWallSurf;
      m_pRWallSurf = NULL;
    }
    if (m_pRWallWire) {
      delete m_pRWallWire;
      m_pRWallWire = NULL;
    }
    if (m_pRoofSurf) {
      delete m_pRoofSurf;
      m_pRoofSurf = NULL;
    }
    if (m_pRoofWire) {
      delete m_pRoofWire;
      m_pRoofWire = NULL;
    }
    if (m_pOWallFloorSurf) {
      delete m_pOWallFloorSurf;
      m_pOWallFloorSurf = NULL;
    }
    if (m_pOWallFloorWire) {
      delete m_pOWallFloorWire;
      m_pOWallFloorWire = NULL;
    }
    if (m_pLLOWallSurf) {
      delete m_pLLOWallSurf;
      m_pLLOWallSurf = NULL;
    }
    if (m_pLLOWallWire) {
      delete m_pLLOWallWire;
      m_pLLOWallWire = NULL;
    }
    if (m_pRLOWallSurf) {
      delete m_pRLOWallSurf;
      m_pRLOWallSurf = NULL;
    }
    if (m_pRLOWallWire) {
      delete m_pRLOWallWire;
      m_pRLOWallWire = NULL;
    }
    if (m_pLUOWallSurf) {
      delete m_pLUOWallSurf;
      m_pLUOWallSurf = NULL;
    }
    if (m_pLUOWallWire) {
      delete m_pLUOWallWire;
      m_pLUOWallWire = NULL;
    }
    if (m_pRUOWallSurf) {
      delete m_pRUOWallSurf;
      m_pRUOWallSurf = NULL;
    }
    if (m_pRUOWallWire) {
      delete m_pRUOWallWire;
      m_pRUOWallWire = NULL;
    }
    if (m_pAILine1) {
      delete m_pAILine1;
      m_pAILine1 = NULL;
    }
    if (m_pAILine2) {
      delete m_pAILine2;
      m_pAILine2 = NULL;
    }
    if (m_pAILine3) {
      delete m_pAILine3;
      m_pAILine3 = NULL;
    }
    if (m_pAILine4) {
      delete m_pAILine4;
      m_pAILine4 = NULL;
    }
    if (m_pSelection) {
      delete m_pSelection;
      m_pSelection = NULL;
    }
    for (std::vector<CShapeData *>::iterator it = m_signAy.begin(); it != m_signAy.end(); ++it) {
      delete *it;
    }
    m_signAy.clear();
    for (std::vector<CShapeData *>::iterator it = m_audioAy.begin(); it != m_audioAy.end(); ++it) {
      delete *it;
    }
    m_audioAy.clear();
    for (std::vector<CShapeData *>::iterator it = m_stuntAy.begin(); it != m_stuntAy.end(); ++it) {
      delete *it;
    }
    m_stuntAy.clear();
  }

  CShapeData *m_pCenterSurf;
  CShapeData *m_pCenterWire;
  CShapeData *m_pLShoulderSurf;
  CShapeData *m_pLShoulderWire;
  CShapeData *m_pRShoulderSurf;
  CShapeData *m_pRShoulderWire;
  CShapeData *m_pLWallSurf;
  CShapeData *m_pLWallWire;
  CShapeData *m_pRWallSurf;
  CShapeData *m_pRWallWire;
  CShapeData *m_pRoofSurf;
  CShapeData *m_pRoofWire;
  CShapeData *m_pOWallFloorSurf;
  CShapeData *m_pOWallFloorWire;
  CShapeData *m_pLLOWallSurf;
  CShapeData *m_pLLOWallWire;
  CShapeData *m_pRLOWallSurf;
  CShapeData *m_pRLOWallWire;
  CShapeData *m_pLUOWallSurf;
  CShapeData *m_pLUOWallWire;
  CShapeData *m_pRUOWallSurf;
  CShapeData *m_pRUOWallWire;
  CShapeData *m_pSelection;
  CShapeData *m_pAILine1;
  CShapeData *m_pAILine2;
  CShapeData *m_pAILine3;
  CShapeData *m_pAILine4;
  CShapeData *m_pEnvirFloor;
  CShapeData *m_pCar;
  CShapeData *m_pAxes;
  CShapeData *m_pTestNormals;
  std::vector<CShapeData *> m_signAy;
  std::vector<CShapeData *> m_audioAy;
  std::vector<CShapeData *> m_stuntAy;

  CShader *m_pShader;
  CTrack m_track;
  CHistoryAy m_historyAy;
  CTexture m_carTex;
  CCamera m_camera;
  CEntity m_entity;
  CNoclipComponent m_noclipComponent;
};

//-------------------------------------------------------------------------------------------------

CTrackPreview::CTrackPreview(QWidget *pParent, const QString &sTrackFile)
  : QGLWidget(QGLFormat(QGL::SampleBuffers), pParent)
  , m_uiShowModels(0)
  , m_carModel(eWhipModel::CAR_XZIZIN)
  , m_carAILine(eShapeSection::AILINE1)
  , m_bMillionPlus(false)
  , m_bAttachLast(false)
  , m_iScale(1)
  , m_bUnsavedChanges(true)
  , m_bAlreadySaved(false)
  , m_sTrackFile(sTrackFile)
  , m_iSelFrom(0)
  , m_iSelTo(0)
  , m_bToChecked(false)
  , m_sLastCarTex("")
  , m_iHistoryIndex(0)
{
  p = new CTrackPreviewPrivate;

  if (!sTrackFile.isEmpty()) {
    p->m_track.m_sTrackFileFolder = sTrackFile.left(sTrackFile.lastIndexOf(QDir::separator()) + 1).toLatin1().constData();
  }
}

//-------------------------------------------------------------------------------------------------

CTrackPreview::~CTrackPreview()
{
  glUseProgram(0);
  if (p) {
    delete p;
    p = NULL;
  }
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateCameraPos()
{
  if (!hasFocus())
    return;

  //if (GetAsyncKeyState(0x57)) //W
  //  p->m_camera.MoveForward();
  //if (GetAsyncKeyState(0x41)) //A
  //  p->m_camera.StrafeLeft();
  //if (GetAsyncKeyState(0x53)) //S
  //  p->m_camera.MoveBackward();
  //if (GetAsyncKeyState(0x44)) //D
  //  p->m_camera.StrafeRight();
  //if (GetAsyncKeyState(0x52) //R
  //    || GetAsyncKeyState(0x45)) //E
  //  p->m_camera.MoveUp();
  //if (GetAsyncKeyState(0x46) //F
  //    || GetAsyncKeyState(0x51)) //Q
  //  p->m_camera.MoveDown();
  //if (GetAsyncKeyState(VK_LBUTTON)
  //    || GetAsyncKeyState(VK_RBUTTON)) {
  //  POINT mousePos;
  //  if (GetCursorPos(&mousePos)) {
  //    p->m_camera.MouseUpdate(glm::vec2(mousePos.x, mousePos.y));
  //  }
  //}
  p->m_entity.Update();
  p->m_camera.m_position = p->m_entity.m_position;
  p->m_camera.m_viewDirection = p->m_entity.m_orientation;

  repaint();
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::LoadTrack(const QString &sFilename)
{
  m_sTrackFile = sFilename;
  bool bSuccess = p->m_track.LoadTrack(sFilename.toLatin1().constData());
  if (bSuccess) {
    p->m_historyAy.clear();
    SaveHistory(sFilename + " loaded");
    m_bUnsavedChanges = false;
  }
  return bSuccess;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateTrack(bool bUpdatingStunt)
{
  if (!bUpdatingStunt)
    p->DeleteModels();

  if (!p->m_track.m_chunkAy.empty()) {
    p->m_pCenterSurf     = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pCenterSurf, p->m_pShader, &p->m_track, eShapeSection::CENTER, m_bAttachLast);
    p->m_pCenterWire     = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pCenterWire, p->m_pShader, &p->m_track, eShapeSection::CENTER, m_bAttachLast, true);
    p->m_pLShoulderSurf  = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLShoulderSurf, p->m_pShader, &p->m_track, eShapeSection::LSHOULDER, m_bAttachLast);
    p->m_pLShoulderWire  = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLShoulderWire, p->m_pShader, &p->m_track, eShapeSection::LSHOULDER, m_bAttachLast, true);
    p->m_pRShoulderSurf  = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRShoulderSurf, p->m_pShader, &p->m_track, eShapeSection::RSHOULDER, m_bAttachLast);
    p->m_pRShoulderWire  = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRShoulderWire, p->m_pShader, &p->m_track, eShapeSection::RSHOULDER, m_bAttachLast, true);
    p->m_pLWallSurf      = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLWallSurf, p->m_pShader, &p->m_track, eShapeSection::LWALL, m_bAttachLast);
    p->m_pLWallWire      = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLWallWire, p->m_pShader, &p->m_track, eShapeSection::LWALL, m_bAttachLast, true);
    p->m_pRWallSurf      = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRWallSurf, p->m_pShader, &p->m_track, eShapeSection::RWALL, m_bAttachLast);
    p->m_pRWallWire      = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRWallWire, p->m_pShader, &p->m_track, eShapeSection::RWALL, m_bAttachLast, true);
    p->m_pRoofSurf       = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRoofSurf, p->m_pShader, &p->m_track, eShapeSection::ROOF, m_bAttachLast);
    p->m_pRoofWire       = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRoofWire, p->m_pShader, &p->m_track, eShapeSection::ROOF, m_bAttachLast, true);
    p->m_pAILine1        = CShapeFactory::GetShapeFactory().MakeAILine(p->m_pAILine1, p->m_pShader, &p->m_track, eShapeSection::AILINE1, m_bAttachLast);
    p->m_pAILine2        = CShapeFactory::GetShapeFactory().MakeAILine(p->m_pAILine2, p->m_pShader, &p->m_track, eShapeSection::AILINE2, m_bAttachLast);
    p->m_pAILine3        = CShapeFactory::GetShapeFactory().MakeAILine(p->m_pAILine3, p->m_pShader, &p->m_track, eShapeSection::AILINE3, m_bAttachLast);
    p->m_pAILine4        = CShapeFactory::GetShapeFactory().MakeAILine(p->m_pAILine4, p->m_pShader, &p->m_track, eShapeSection::AILINE4, m_bAttachLast);
    if (!bUpdatingStunt) {
      p->m_pOWallFloorSurf = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pOWallFloorSurf, p->m_pShader, &p->m_track, eShapeSection::OWALLFLOOR, m_bAttachLast);
      p->m_pOWallFloorWire = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pOWallFloorWire, p->m_pShader, &p->m_track, eShapeSection::OWALLFLOOR, m_bAttachLast, true);
      p->m_pLLOWallSurf    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLLOWallSurf, p->m_pShader, &p->m_track, eShapeSection::LLOWALL, m_bAttachLast);
      p->m_pLLOWallWire    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLLOWallWire, p->m_pShader, &p->m_track, eShapeSection::LLOWALL, m_bAttachLast, true);
      p->m_pRLOWallSurf    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRLOWallSurf, p->m_pShader, &p->m_track, eShapeSection::RLOWALL, m_bAttachLast);
      p->m_pRLOWallWire    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRLOWallWire, p->m_pShader, &p->m_track, eShapeSection::RLOWALL, m_bAttachLast, true);
      p->m_pLUOWallSurf    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLUOWallSurf, p->m_pShader, &p->m_track, eShapeSection::LUOWALL, m_bAttachLast);
      p->m_pLUOWallWire    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pLUOWallWire, p->m_pShader, &p->m_track, eShapeSection::LUOWALL, m_bAttachLast, true);
      p->m_pRUOWallSurf    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRUOWallSurf, p->m_pShader, &p->m_track, eShapeSection::RUOWALL, m_bAttachLast);
      p->m_pRUOWallWire    = CShapeFactory::GetShapeFactory().MakeTrackSurface(p->m_pRUOWallWire, p->m_pShader, &p->m_track, eShapeSection::RUOWALL, m_bAttachLast, true);
      CShapeFactory::GetShapeFactory().MakeSigns(p->m_pShader, &p->m_track, p->m_signAy);
      CShapeFactory::GetShapeFactory().MakeAudio(p->m_pShader, &p->m_track, p->m_audioAy);
      CShapeFactory::GetShapeFactory().MakeStunts(p->m_pShader, &p->m_track, p->m_stuntAy);

      UpdateCar(m_carModel, m_carAILine, m_bMillionPlus);
      UpdateGeometrySelection();
    }
  }
  repaint();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::ShowModels(uint32 uiShowModels)
{
  m_uiShowModels = uiShowModels;
  repaint();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateGeometrySelection()
{
  p->m_pSelection = CShapeFactory::GetShapeFactory().MakeSelectedChunks(p->m_pSelection, p->m_pShader, &p->m_track, m_iSelFrom, m_iSelTo);
  p->m_pEnvirFloor = CShapeFactory::GetShapeFactory().MakeEnvirFloor(p->m_pEnvirFloor, p->m_pShader, &p->m_track, m_iSelFrom);

  if (p->m_pCar)
    CShapeFactory::GetShapeFactory().GetCarPos(&p->m_track, m_iSelFrom, m_carAILine, p->m_pCar->m_modelToWorldMatrix, m_bMillionPlus);
  //if (p->m_pTestNormals)
  //  CShapeFactory::GetShapeFactory().GetCarPos(&p->m_track, m_iSelFrom, m_carAILine, p->m_pTestNormals->m_modelToWorldMatrix, m_bMillionPlus);

  repaint();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::SaveHistory(const QString &sDescription)
{
  tTrackHistory history;
  history.sDescription = sDescription.toLatin1().constData();
  p->m_track.GetTrackData(history.byteAy);

  while (m_iHistoryIndex < (int)p->m_historyAy.size() - 1) {
    p->m_historyAy.pop_back();
  }

  p->m_historyAy.push_back(history);

  while ((int)p->m_historyAy.size() > g_pMainWindow->m_preferences.iHistoryMaxSize) {
    p->m_historyAy.erase(p->m_historyAy.begin());
  }

  m_iHistoryIndex = (int)p->m_historyAy.size() - 1;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Undo()
{
  if (p->m_historyAy.empty()) return;

  m_iHistoryIndex--;
  if (m_iHistoryIndex < 0)
    m_iHistoryIndex = 0;

  tTrackHistory *pHistory = &p->m_historyAy[m_iHistoryIndex];
  LoadHistory(pHistory);
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::Redo()
{
  if (p->m_historyAy.empty()) return;

  m_iHistoryIndex++;
  if (m_iHistoryIndex >= (int)p->m_historyAy.size())
    m_iHistoryIndex = (int)p->m_historyAy.size() - 1;

  tTrackHistory *pHistory = &p->m_historyAy[m_iHistoryIndex];
  LoadHistory(pHistory);
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::LoadHistory(const tTrackHistory *pHistory)
{
  p->DeleteModels();

  int iSize = (int)pHistory->byteAy.size();
  uint8 *byData = new uint8[iSize];
  for (int i = 0; i < iSize; ++i)
    byData[i] = pHistory->byteAy[i];

  p->m_track.ClearData();
  p->m_track.ProcessTrackData(byData, iSize);

  delete[] byData;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::UpdateCar(eWhipModel carModel, eShapeSection aiLine, bool bMillionPlus)
{
  m_carModel = carModel;
  m_carAILine = aiLine;
  m_bMillionPlus = bMillionPlus;

  if (p->m_pCar) {
    delete p->m_pCar;
    p->m_pCar = NULL;
  }
  if (p->m_pTestNormals) {
    delete p->m_pTestNormals;
    p->m_pTestNormals = NULL;
  }

  if (p->m_track.m_pPal && !p->m_track.m_sTrackFileFolder.empty()) {
    QString sTexName = CarHelpers::GetCarTextureFromModel(carModel).c_str();
    QString sTex = QString(p->m_track.m_sTrackFileFolder.c_str()) + QDir::separator() + sTexName;
    if (m_sLastCarTex.compare(sTex) != 0) {
      if (p->m_carTex.LoadTexture(sTex.toLatin1().constData(), p->m_track.m_pPal))
        m_sLastCarTex = sTex;
    }
    p->m_pCar = CShapeFactory::GetShapeFactory().MakeModel(p->m_pShader, &p->m_carTex, carModel);
    //p->m_pTestNormals = CShapeFactory::GetShapeFactory().MakeNormalsTest(*p->m_pCar, p->m_pShader);
  }

  if (p->m_pCar)
    CShapeFactory::GetShapeFactory().GetCarPos(&p->m_track, m_iSelFrom, m_carAILine, p->m_pCar->m_modelToWorldMatrix, m_bMillionPlus);
  //if (p->m_pTestNormals)
  //  CShapeFactory::GetShapeFactory().GetCarPos(&p->m_track, m_iSelFrom, m_carAILine, p->m_pTestNormals->m_modelToWorldMatrix, m_bMillionPlus);

  repaint();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::AttachLast(bool bAttachLast)
{
  m_bAttachLast = bAttachLast;
  UpdateTrack();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::paintGL()
{
  if (m_uiShowModels & SHOW_ENVIRONMENT)
    glClearColor(0.03125f, 0.296875f, 0.984375f, 1.0f);
  else
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, width(), height());

  glm::mat4 viewToProjectionMatrix = glm::perspective(glm::radians(60.0f), ((float)width()) / height(), 100.0f, 500000.0f);
  glm::mat4 worldToViewMatrix = p->m_camera.GetWorldToViewMatrix();
  glm::mat4 worldToProjectionMatrix = viewToProjectionMatrix * worldToViewMatrix;

  if (m_uiShowModels & SHOW_ENVIRONMENT && p->m_pEnvirFloor)
    p->m_pEnvirFloor->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  //environment floor should never clip through track even when it is higher than it
  //so we draw it first then clear depth buffer bit
  glClear(GL_DEPTH_BUFFER_BIT);

  if (m_uiShowModels & SHOW_CENTER_SURF_MODEL && p->m_pCenterSurf)
    p->m_pCenterSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_CENTER_WIRE_MODEL && p->m_pCenterWire)
    p->m_pCenterWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LSHOULDER_SURF_MODEL && p->m_pLShoulderSurf)
    p->m_pLShoulderSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LSHOULDER_WIRE_MODEL && p->m_pLShoulderWire)
    p->m_pLShoulderWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RSHOULDER_SURF_MODEL && p->m_pRShoulderSurf)
    p->m_pRShoulderSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RSHOULDER_WIRE_MODEL && p->m_pRShoulderWire)
    p->m_pRShoulderWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LWALL_SURF_MODEL && p->m_pLWallSurf)
    p->m_pLWallSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LWALL_WIRE_MODEL && p->m_pLWallWire)
    p->m_pLWallWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RWALL_SURF_MODEL && p->m_pRWallSurf)
    p->m_pRWallSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RWALL_WIRE_MODEL && p->m_pRWallWire)
    p->m_pRWallWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_ROOF_SURF_MODEL && p->m_pRoofSurf)
    p->m_pRoofSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_ROOF_WIRE_MODEL && p->m_pRoofWire)
    p->m_pRoofWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_OWALLFLOOR_SURF_MODEL && p->m_pOWallFloorSurf)
    p->m_pOWallFloorSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_OWALLFLOOR_WIRE_MODEL && p->m_pOWallFloorWire)
    p->m_pOWallFloorWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LLOWALL_SURF_MODEL && p->m_pLLOWallSurf)
    p->m_pLLOWallSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LLOWALL_WIRE_MODEL && p->m_pLLOWallWire)
    p->m_pLLOWallWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RLOWALL_SURF_MODEL && p->m_pRLOWallSurf)
    p->m_pRLOWallSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RLOWALL_WIRE_MODEL && p->m_pRLOWallWire)
    p->m_pRLOWallWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LUOWALL_SURF_MODEL && p->m_pLUOWallSurf)
    p->m_pLUOWallSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_LUOWALL_WIRE_MODEL && p->m_pLUOWallWire)
    p->m_pLUOWallWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RUOWALL_SURF_MODEL && p->m_pRUOWallSurf)
    p->m_pRUOWallSurf->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_RUOWALL_WIRE_MODEL && p->m_pRUOWallWire)
    p->m_pRUOWallWire->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_SELECTION_HIGHLIGHT && p->m_pSelection)
    p->m_pSelection->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_AILINE_MODELS
      && p->m_pAILine1
      && p->m_pAILine2
      && p->m_pAILine3
      && p->m_pAILine4) {
    p->m_pAILine1->Draw(worldToProjectionMatrix, p->m_camera.m_position);
    p->m_pAILine2->Draw(worldToProjectionMatrix, p->m_camera.m_position);
    p->m_pAILine3->Draw(worldToProjectionMatrix, p->m_camera.m_position);
    p->m_pAILine4->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  }
  if (m_uiShowModels & SHOW_TEST_CAR && p->m_pCar)
    p->m_pCar->Draw(worldToProjectionMatrix, p->m_camera.m_position);
  if (m_uiShowModels & SHOW_SIGNS) {
    for (std::vector<CShapeData *>::iterator it = p->m_signAy.begin(); it != p->m_signAy.end(); ++it) {
      (*it)->Draw(worldToProjectionMatrix, p->m_camera.m_position);
    }
  }
  if (m_uiShowModels & SHOW_AUDIO) {
    for (std::vector<CShapeData *>::iterator it = p->m_audioAy.begin(); it != p->m_audioAy.end(); ++it) {
      (*it)->Draw(worldToProjectionMatrix, p->m_camera.m_position);
    }
  }
  if (m_uiShowModels & SHOW_STUNTS) {
    for (std::vector<CShapeData *>::iterator it = p->m_stuntAy.begin(); it != p->m_stuntAy.end(); ++it) {
      (*it)->Draw(worldToProjectionMatrix, p->m_camera.m_position);
    }
  }
  //if (p->m_pAxes)
  //  p->m_pAxes->Draw(worldToProjectionMatrix);
  //if (p->m_pTestNormals)
  //  p->m_pTestNormals->Draw(worldToProjectionMatrix, p->m_camera.GetPosition());
}

//-------------------------------------------------------------------------------------------------

CTrack *CTrackPreview::GetTrack()
{
  return &p->m_track;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveChangesAndContinue()
{
  if (!m_bUnsavedChanges)
    return true;

  //init
  QString sTrackName = m_sTrackFile.right(m_sTrackFile.size() - m_sTrackFile.lastIndexOf(QDir::separator()) - 1);
  QMessageBox saveDiscardCancelBox(QMessageBox::Warning, "Unsaved Changes",
                                   "There are unsaved changes to " + sTrackName + ". Save them ? ",
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                   this);
  int iButton = saveDiscardCancelBox.exec();

  //cancel
  if (iButton == QMessageBox::Cancel || iButton == QMessageBox::NoButton)
    return false;

  //save
  QString sFilename = m_sTrackFile;
  if (iButton == QMessageBox::Save) {
    if (sFilename.isEmpty()) {
      sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
        this, "Save Track As", p->m_track.m_sTrackFileFolder.c_str(), "Track Files (*.TRK)"));
    }
    if (!SaveTrack_Internal(sFilename))
      return false;
    g_pMainWindow->m_sLastTrackFilesFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  }

  m_sTrackFile = sFilename;
  m_bUnsavedChanges = false;

  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveTrack()
{
  if (m_bAlreadySaved) {
    m_bUnsavedChanges = !SaveTrack_Internal(m_sTrackFile);
    g_pMainWindow->UpdateWindow();
    return true;
  } else {
    return SaveTrackAs();
  }
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveTrackAs()
{
  //save track
  QString sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
    this, "Save Track As", p->m_track.m_sTrackFileFolder.c_str(), "Track Files (*.TRK)"));
  if (!SaveTrack_Internal(sFilename))
    return false;

  //save successful, update app
  g_pMainWindow->m_sLastTrackFilesFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  m_sTrackFile = sFilename;
  m_bUnsavedChanges = false;
  m_bAlreadySaved = true;
  g_pMainWindow->UpdateWindow();
  return true;
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::ExportFBX()
{
  //save track
  QString sFilename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(
    this, "Export Track As", p->m_track.m_sTrackFileFolder.c_str(), "FBX Files (*.fbx)"));
  QString sFolder = sFilename.left(sFilename.lastIndexOf(QDir::separator()));
  QString sName = sFilename.right(sFilename.size() - sFilename.lastIndexOf(QDir::separator()) - 1);
  sName = sName.left(sName.lastIndexOf('.'));

  //make texture file
  QString sTexFile = sFolder + "\\" + sName + ".png";
  p->m_track.m_pTex->ExportToPngFile(sTexFile.toLatin1().constData());

  //make sign texture file
  QString sSignTexFile = sFolder + "\\" + sName + "_BLD.png";
  p->m_track.m_pBld->ExportToPngFile(sSignTexFile.toLatin1().constData());

  //generate models
  CShapeData *pExportTrack = NULL;
  std::vector<CShapeData *> signAy;
  pExportTrack = CShapeFactory::GetShapeFactory().MakeTrackSurface(pExportTrack,
                                                                   p->m_pShader,
                                                                   &p->m_track,
                                                                   eShapeSection::EXPORT,
                                                                   true);
  CShapeFactory::GetShapeFactory().MakeSigns(p->m_pShader, &p->m_track, signAy);
  //signs need to be moved to the right position on track, this is normally done in the shader
  for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it)
    (*it)->TransformVertsForExport();

  //export
  bool bExported = CFBXExporter::GetFBXExporter().ExportTrack(pExportTrack,
                                                              signAy,
                                                              sName.toLatin1().constData(),
                                                              sFilename.toLatin1().constData(),
                                                              sTexFile.toLatin1().constData(),
                                                              sSignTexFile.toLatin1().constData());

  //cleanup
  if (pExportTrack)
    delete pExportTrack;
  for (std::vector<CShapeData *>::iterator it = signAy.begin(); it != signAy.end(); ++it)
    delete *it;

  if (!bExported)
    return false;

  g_pMainWindow->m_sLastTrackFilesFolder = sFolder;
  return true;
}

//-------------------------------------------------------------------------------------------------

QString CTrackPreview::GetTitle(bool bFullPath)
{
  QString sTitle = m_sTrackFile;
  if (!bFullPath)
    sTitle = m_sTrackFile.right(m_sTrackFile.size() - m_sTrackFile.lastIndexOf(QDir::separator()) - 1);
  if (m_bUnsavedChanges)
    sTitle = QString("*") + sTitle;
  return sTitle;
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::initializeGL()
{
  setMouseTracking(false);
  if (glewInit() != GLEW_OK)
    assert(0);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(GLErrorCb, 0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.24f);
  glLineWidth(3.0f);

  if (!p->m_pShader)
    p->m_pShader = new CShader("Shaders/WhiplashVertexShader.glsl", "Shaders/WhiplashFragmentShader.glsl");

  //p->m_pAxes = DebugShapes::MakeAxes(p->m_pShader);
  UpdateCar(m_carModel, m_carAILine, m_bMillionPlus);
  p->m_track.LoadTextures();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::mouseClickEvent(QMouseEvent *pEvent)
{
  (void)(pEvent);
  setFocus();
}

//-------------------------------------------------------------------------------------------------

void CTrackPreview::mouseMoveEvent(QMouseEvent *pEvent)
{
  (void)(pEvent);
  setFocus();
}

//-------------------------------------------------------------------------------------------------

bool CTrackPreview::SaveTrack_Internal(const QString &sFilename)
{
  if (sFilename.isEmpty())
    return false;

  std::vector<uint8> data;
  std::vector<uint8> mangledData;
  p->m_track.GetTrackData(data);

  std::vector<uint8> *pOutData;
  //if (bIsMangled) {
  //  MangleFile(data, mangledData);
  //  pOutData = &mangledData;
  //} else {
  pOutData = &data;
//}

  QFile file(sFilename);
  file.resize(0);
  if (file.open(QIODevice::ReadWrite)) {
    QTextStream stream(&file);
    for (int i = 0; i < pOutData->size(); ++i) {
      stream << (char)((*pOutData)[i]);
    }
    file.close();
  }

  return true;
}

//-------------------------------------------------------------------------------------------------