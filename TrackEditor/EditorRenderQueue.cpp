#include "EditorRenderQueue.h"

#include <QtGlobal>

#include <atomic>
#include <exception>
#include <limits>

namespace
{
std::atomic<uint64_t> g_ullNextRequestId(1);
std::atomic<uint64_t> g_ullNextDocumentId(1);

uint64_t TakeMonotonicId(std::atomic<uint64_t> &Counter)
{
  const uint64_t ullId = Counter.fetch_add(1, std::memory_order_relaxed);
  if (ullId == 0 || ullId == std::numeric_limits<uint64_t>::max())
    std::terminate();
  return ullId;
}
}

uint64_t CEditorRenderIds::NextRequestId()
{
  return TakeMonotonicId(g_ullNextRequestId);
}

uint64_t CEditorRenderIds::NextDocumentId()
{
  return TakeMonotonicId(g_ullNextDocumentId);
}

CDocumentFrameState::CDocumentFrameState(uint64_t ullDocumentId)
  : m_ullDocumentId(ullDocumentId)
  , m_ullDocumentRevision(1)
  , m_ullLatestRequestId(0)
  , m_uiInstalledGeometryEpoch(0)
  , m_bValid(ullDocumentId != 0)
  , m_bExportEnabled(false)
  , m_eDisplayState(eEdFrameDisplayState::PLACEHOLDER)
{
  Q_ASSERT(ullDocumentId != 0);
}

void CDocumentFrameState::BeginRequest(uint64_t ullRequestId)
{
  if (!m_bValid || ullRequestId <= m_ullLatestRequestId)
    return;
  m_ullLatestRequestId = ullRequestId;
}

void CDocumentFrameState::MarkDocumentEdited()
{
  if (!m_bValid)
    return;
  if (m_ullDocumentRevision == std::numeric_limits<uint64_t>::max())
    std::terminate();
  ++m_ullDocumentRevision;
}

void CDocumentFrameState::Invalidate()
{
  m_bValid = false;
  m_bExportEnabled = false;
  m_eDisplayState = eEdFrameDisplayState::PLACEHOLDER;
  m_Image = QImage();
  m_sErrorText.clear();
}

bool CDocumentFrameState::MatchesCurrentRequest(const tEdRenderResultTag &Tag) const
{
  return m_bValid
      && Tag.ullDocumentId == m_ullDocumentId
      && Tag.ullRequestId == m_ullLatestRequestId
      && Tag.ullDocumentRevision == m_ullDocumentRevision;
}

bool CDocumentFrameState::ApplyResult(const tEdRenderResult &Result)
{
  if (!MatchesCurrentRequest(Result.Tag))
    return false;

  if (Result.Tag.eResult != ROLLER_ED_RESULT_OK) {
    if (!Result.bLoadFailed)
      return false;

    m_bExportEnabled = false;
    m_sErrorText = Result.sErrorText;
    m_eDisplayState = m_Image.isNull()
        ? eEdFrameDisplayState::PLACEHOLDER
        : eEdFrameDisplayState::STALE_AFTER_LOAD_FAILURE;
    return true;
  }

  if (Result.bSceneEmpty) {
    m_Image = QImage();
    m_uiInstalledGeometryEpoch = Result.Tag.uiActualGeometryEpoch;
    m_bExportEnabled = false;
    m_eDisplayState = eEdFrameDisplayState::PLACEHOLDER;
    m_sErrorText = Result.sErrorText;
    return true;
  }

  if (Result.Image.isNull()
      || Result.Tag.uiActualGeometryEpoch != Result.uiRenderedGeometryEpoch) {
    return false;
  }

  m_Image = Result.Image;
  m_uiInstalledGeometryEpoch = Result.Tag.uiActualGeometryEpoch;
  m_bExportEnabled = true;
  m_eDisplayState = eEdFrameDisplayState::CURRENT;
  m_sErrorText.clear();
  return true;
}
