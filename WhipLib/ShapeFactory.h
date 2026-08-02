#ifndef _WHIPLIB_SHAPEFACTORY_H
#define _WHIPLIB_SHAPEFACTORY_H
//-------------------------------------------------------------------------------------------------
#include "Types.h"
#include <vector>
#include <glm.hpp>
//-------------------------------------------------------------------------------------------------
class CShapeData;
class CTexture;
class CTrack;
struct tVertex;
struct tAnimation;
struct tPolygon;
//-------------------------------------------------------------------------------------------------
enum eBackModeling
{
  FRONTS = 0,
  BACKS,
  FRONTS_AND_BACKS
};
enum eVertOrder
{
  SURFACE = 0,
  LWALL,
  RWALL,
  OWALL
};
//-------------------------------------------------------------------------------------------------

class CShapeFactory
{
public:
  static CShapeFactory &GetShapeFactory();
  ~CShapeFactory();
  CShapeFactory(CShapeFactory const &) = delete;
  void operator=(CShapeFactory const &) = delete;

  void MakeAxes(CShapeData **pShape, CTexture *pTexture);
  void MakeDebugTri(CShapeData **pShape, CTexture *pTexture, const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2);
  void MakeDebugLine(CShapeData **pShape, CTexture *pTexture, const glm::vec3 &p0, const glm::vec3 &p1);
  void MakeModel(CShapeData **pShape, CTexture *pTexture, eWhipModel model, int iSignSurfaceType = -1, eBackModeling backModeling = FRONTS_AND_BACKS);
  void MakeAudioMarker(CShapeData **pShape, CTexture *pTexture);
  void MakeStuntMarker(CShapeData **pShape, CTexture *pTexture);
  void MakeTrackSurface(CShapeData **pShape, CTrack *pTrack, eShapeSection section, bool bAttachLast, bool bWireframe = false, eBackModeling backModeling = FRONTS_AND_BACKS);
  void MakeEnvirFloor(CShapeData **pShape, CTrack *pTrack, int iIndex);
  void MakeAILine(CShapeData **pShape, CTrack *pTrack, eShapeSection section, bool bAttachLast);
  void MakeSelectedChunks(CShapeData **pShape, CTrack *pTrack, int iStart, int iEnd);
  void MakeSigns(CTrack *pTrack, std::vector<CShapeData*> &signAy, eBackModeling backModeling = FRONTS_AND_BACKS);
  void MakeAudio(CTrack *pTrack, std::vector<CShapeData *> &audioAy);
  void MakeStunts(CTrack *pTrack, std::vector<CShapeData *> &stuntAy);
  CShapeData *MakeNormalsTest(const CShapeData &data);
  void GetCarPos(CTrack *pTrack, int iChunk, eShapeSection aiLine, glm::mat4 &modelToWorldMatrix, bool bMillionPlus = false);

private:
  CShapeFactory();

  tVertex *MakeModelVerts(uint32 &numVertices, CTexture *pTexture, eWhipModel model, int iSignSurfaceType, eBackModeling backModeling);
  uint32 *MakeModelIndices(uint32 &numIndices, eWhipModel model, eBackModeling backModeling);

  tVertex *MakeVertsAudioMarker(uint32 &uiNumVerts, CTexture *pTexture);
  uint32 *MakeIndicesAudioMarker(uint32 &uiNumIndices);
  tVertex *MakeVertsStuntMarker(uint32 &uiNumVert, CTexture *pTextures);
  uint32 *MakeIndicesStuntMarker(uint32 &uiNumIndices);

  float *GetCoords(eWhipModel model);
  int GetCoordsCount(eWhipModel model);
  tPolygon *GetPols(eWhipModel model);
  int GetPolsCount(eWhipModel model);
  tAnimation *GetAnms(eWhipModel model);
  int GetAnmsCount(eWhipModel model);
  uint32 *GetBacks(eWhipModel model);
  int GetBacksCount(eWhipModel model);

  tVertex *MakeVerts(uint32 &numVertices, eShapeSection section, CTrack *pTrack, CTexture *pTexture, eBackModeling backModeling = FRONTS);
  tVertex *MakeVertsEnvirFloor(uint32 &numVertices, CTrack *pTrack, CTexture *pTexture, int iIndex);

  uint32 *MakeIndicesEnvirFloor(uint32 &numIndices);
  uint32 *MakeIndicesCenterline(uint32 &numIndices, CTrack *pTrack, bool bAttachLast);
  uint32 *MakeIndicesSurface(uint32 &numIndices, CTrack *pTrack, bool bAttachLast);
  uint32 *MakeIndicesSurfaceWireframe(uint32 &numIndices, CTrack *pTrack, bool bAttachLast);
  uint32 *MakeIndicesSingleSection(uint32 &numIndices, eShapeSection section, CTrack *pTrack, bool bAttachLast, eBackModeling backModeling);
  uint32 *MakeIndicesExport(uint32 &numIndices, CTrack *pTrack, eBackModeling backModeling);
  void MakeIndicesHelper(uint32 *indices, uint32 i, uint32 uiNumIndicesPerChunk, int &iIndicesRunner, uint32 uiNumVertsPerChunk, int iPos, eBackModeling backModeling);
  uint32 *MakeIndicesSelectedChunks(uint32 &numIndices, int iStart, int iEnd, CTrack *pTrack);
  uint32 *MakeIndicesSingleSectionWireframe(uint32 &numIndices, eShapeSection section, CTrack *pTrack, bool bAttachLast);

  void ApplyVerticesSingleSection(int i, tVertex *vertices,
                                  uint32 uiNumVertsPerChunk,
                                  const glm::vec3 &v0,
                                  const glm::vec3 &v1,
                                  const glm::vec3 &v2,
                                  const glm::vec3 &v3,
                                  eBackModeling backModeling,
                                  int iOffset = 0);
  void ApplyNormalsAndTexCoords(int i,
                                int iChunkIndex,
                                tVertex *vertices,
                                uint32 uiNumVertsPerChunk,
                                CTrack *pTrack,
                                CTexture *pTexture,
                                int iSurfaceType,
                                eVertOrder vertOrder,
                                eBackModeling backModeling,
                                int iOffset = 0);
  void MakeNormals(tVertex &topLeft, tVertex &topRight, tVertex &bottomLeft, tVertex &bottomRight);

  static CShapeFactory s_shapeFactory;
};

//-------------------------------------------------------------------------------------------------
#endif
