#include "EditorRenderQueue.h"

#include <QColor>

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>

#ifdef assert
#undef assert
#endif
#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "assertion failed: " #condition << " (" << __FILE__       \
                << ':' << __LINE__ << ")\n";                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (false)

namespace
{
tEdRenderResult SuccessfulFrame(const CDocumentFrameState &State,
                                uint64_t ullRequestId,
                                uint32_t uiGeometryEpoch,
                                const QColor &Colour)
{
  tEdRenderResult Result;
  Result.Tag.ullRequestId = ullRequestId;
  Result.Tag.ullDocumentId = State.GetDocumentId();
  Result.Tag.ullDocumentRevision = State.GetDocumentRevision();
  Result.Tag.uiActualGeometryEpoch = uiGeometryEpoch;
  Result.Tag.eResult = ROLLER_ED_RESULT_OK;
  Result.uiRenderedGeometryEpoch = uiGeometryEpoch;
  Result.Image = QImage(2, 2, QImage::Format_RGBA8888);
  Result.Image.fill(Colour);
  return Result;
}

tEdRenderResult FailedLoad(const CDocumentFrameState &State,
                           uint64_t ullRequestId,
                           const std::string &sError)
{
  tEdRenderResult Result;
  Result.Tag.ullRequestId = ullRequestId;
  Result.Tag.ullDocumentId = State.GetDocumentId();
  Result.Tag.ullDocumentRevision = State.GetDocumentRevision();
  Result.Tag.eResult = ROLLER_ED_RESULT_LOAD_FAILED;
  Result.bLoadFailed = true;
  Result.sErrorText = sError;
  return Result;
}

void TestPerDocumentFailureNeverBorrowsAnotherFrame()
{
  CDocumentFrameState DocumentA(CEditorRenderIds::NextDocumentId());
  CDocumentFrameState DocumentB(CEditorRenderIds::NextDocumentId());
  assert(DocumentA.GetDocumentId() != DocumentB.GetDocumentId());

  const uint64_t ullRequestA = CEditorRenderIds::NextRequestId();
  DocumentA.BeginRequest(ullRequestA);
  assert(DocumentA.ApplyResult(
      SuccessfulFrame(DocumentA, ullRequestA, 11, Qt::red)));

  const uint64_t ullFirstRequestB = CEditorRenderIds::NextRequestId();
  DocumentB.BeginRequest(ullFirstRequestB);
  tEdRenderResult FailureB = FailedLoad(
      DocumentB, ullFirstRequestB, "B could not load");
  assert(!DocumentA.ApplyResult(FailureB));
  assert(DocumentA.GetDisplayState() == eEdFrameDisplayState::CURRENT);
  assert(DocumentA.GetImage().pixelColor(0, 0) == QColor(Qt::red));
  assert(DocumentB.ApplyResult(FailureB));
  assert(DocumentB.GetDisplayState() == eEdFrameDisplayState::PLACEHOLDER);
  assert(DocumentB.GetImage().isNull());

  const uint64_t ullGoodRequestB = CEditorRenderIds::NextRequestId();
  DocumentB.BeginRequest(ullGoodRequestB);
  assert(DocumentB.ApplyResult(
      SuccessfulFrame(DocumentB, ullGoodRequestB, 22, Qt::green)));
  const uint64_t ullFailedReloadB = CEditorRenderIds::NextRequestId();
  DocumentB.BeginRequest(ullFailedReloadB);
  assert(DocumentB.ApplyResult(
      FailedLoad(DocumentB, ullFailedReloadB, "B reload failed")));
  assert(DocumentB.GetDisplayState()
      == eEdFrameDisplayState::STALE_AFTER_LOAD_FAILURE);
  assert(DocumentB.GetImage().pixelColor(0, 0) == QColor(Qt::green));
  assert(!DocumentB.CanExport());
}

void TestOlderRequestCannotReplaceNewerFrame()
{
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  const uint64_t ullOlder = CEditorRenderIds::NextRequestId();
  const uint64_t ullNewer = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullOlder);
  Document.BeginRequest(ullNewer);

  assert(Document.ApplyResult(
      SuccessfulFrame(Document, ullNewer, 33, Qt::blue)));
  assert(!Document.ApplyResult(
      SuccessfulFrame(Document, ullOlder, 33, Qt::yellow)));
  assert(Document.GetImage().pixelColor(0, 0) == QColor(Qt::blue));
}

void TestRevisionRejectsInflightCompletionBeforeReplacementIsQueued()
{
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  const uint64_t ullRequest = CEditorRenderIds::NextRequestId();
  const uint64_t ullQueuedRevision = Document.GetDocumentRevision();
  Document.BeginRequest(ullRequest);
  Document.MarkDocumentEdited();

  tEdRenderResult OldRevision = SuccessfulFrame(Document, ullRequest, 44, Qt::red);
  OldRevision.Tag.ullDocumentRevision = ullQueuedRevision;
  assert(!Document.ApplyResult(OldRevision));
  assert(Document.GetDisplayState() == eEdFrameDisplayState::PLACEHOLDER);
}

void TestEpochAndExplicitExpectedEpochFlag()
{
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  const uint64_t ullRequest = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullRequest);
  tEdRenderResult WrongEpoch = SuccessfulFrame(
      Document, ullRequest, 55, Qt::cyan);
  WrongEpoch.uiRenderedGeometryEpoch = 54;
  assert(!Document.ApplyResult(WrongEpoch));

  tEdRenderRequestTag Tag = {};
  Tag.uiExpectedGeometryEpoch = 0;
  assert((Tag.uiFlags & ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH) == 0);
  Tag.uiFlags = ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH;
  assert((Tag.uiFlags & ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH) != 0);
}

void TestRenderFailureDoesNotMasqueradeAsLoadFailure()
{
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  const uint64_t ullGoodRequest = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullGoodRequest);
  assert(Document.ApplyResult(
      SuccessfulFrame(Document, ullGoodRequest, 66, Qt::magenta)));

  const uint64_t ullRenderRequest = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullRenderRequest);
  tEdRenderResult RenderFailure;
  RenderFailure.Tag.ullRequestId = ullRenderRequest;
  RenderFailure.Tag.ullDocumentId = Document.GetDocumentId();
  RenderFailure.Tag.ullDocumentRevision = Document.GetDocumentRevision();
  RenderFailure.Tag.eResult = ROLLER_ED_RESULT_GPU_FAILED;
  RenderFailure.sErrorText = "render failed without replacing the scene";
  assert(!Document.ApplyResult(RenderFailure));
  assert(Document.GetDisplayState() == eEdFrameDisplayState::CURRENT);
  assert(Document.CanExport());
  assert(Document.GetImage().pixelColor(0, 0) == QColor(Qt::magenta));
}

void TestEmptyDocumentClearsItsStaleFrame()
{
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  const uint64_t ullGoodRequest = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullGoodRequest);
  assert(Document.ApplyResult(
      SuccessfulFrame(Document, ullGoodRequest, 77, Qt::darkGreen)));

  Document.MarkDocumentEdited();
  const uint64_t ullEmptyRequest = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullEmptyRequest);
  tEdRenderResult EmptyResult;
  EmptyResult.Tag.ullRequestId = ullEmptyRequest;
  EmptyResult.Tag.ullDocumentId = Document.GetDocumentId();
  EmptyResult.Tag.ullDocumentRevision = Document.GetDocumentRevision();
  EmptyResult.Tag.uiActualGeometryEpoch = 78;
  EmptyResult.Tag.eResult = ROLLER_ED_RESULT_OK;
  EmptyResult.bSceneEmpty = true;
  EmptyResult.sErrorText = "Track has no geometry chunks";

  assert(Document.ApplyResult(EmptyResult));
  assert(Document.GetDisplayState() == eEdFrameDisplayState::PLACEHOLDER);
  assert(Document.GetImage().isNull());
  assert(!Document.CanExport());
  assert(Document.GetErrorText() == "Track has no geometry chunks");
}

void TestOwnedErrorAndCloseInvalidation()
{
  CDocumentFrameState Document(CEditorRenderIds::NextDocumentId());
  const uint64_t ullRequest = CEditorRenderIds::NextRequestId();
  Document.BeginRequest(ullRequest);
  std::string FacadeStorage = "persistent copied error";
  tEdRenderResult Failure = FailedLoad(Document, ullRequest, FacadeStorage);
  FacadeStorage.assign("next facade call replaced the buffer");
  assert(Failure.sErrorText == "persistent copied error");
  assert(Document.ApplyResult(Failure));
  assert(Document.GetErrorText() == "persistent copied error");

  Document.Invalidate();
  assert(!Document.IsValid());
  assert(!Document.ApplyResult(Failure));
  assert(Document.GetImage().isNull());
}
}

int main()
{
  TestPerDocumentFailureNeverBorrowsAnotherFrame();
  TestOlderRequestCannotReplaceNewerFrame();
  TestRevisionRejectsInflightCompletionBeforeReplacementIsQueued();
  TestEpochAndExplicitExpectedEpochFlag();
  TestRenderFailureDoesNotMasqueradeAsLoadFailure();
  TestEmptyDocumentClearsItsStaleFrame();
  TestOwnedErrorAndCloseInvalidation();
  std::cout << "E3-S1 editor render queue tests passed\n";
  return 0;
}
