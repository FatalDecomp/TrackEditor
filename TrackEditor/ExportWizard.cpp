#include "ExportWizard.h"
#include "QtHelpers.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CExportWizard::CExportWizard(QWidget *pParent)
  : QDialog(pParent)
  , m_bExportSeparate(true)
  , m_bExportBacks(true)
  , m_bExportSigns(true)
{
  setupUi(this);

  ckSections->setChecked(m_bExportSeparate);
  ckBacks->setChecked(m_bExportBacks);
  ckSigns->setChecked(m_bExportSigns);

  // E4A-S6. The option works again. It was disabled from E4-S1 through E4-S5
  // because the canonical geometry covered the eleven track surface classes
  // only - signs and buildings reached the emitter through the camera-driven
  // render path, and the editor's own CPU derivation works in a different,
  // chunk-zero relative frame that would have placed them off the exported
  // track. ROLLER's drawtrk3_emit_full_scenery closed that gap
  // (docs/adr/0005-camera-independent-scenery-traversal.md), so the checkbox
  // now governs advert panels *and* buildings, which is why it says so.
  ckSigns->setToolTip(
      "Export advert panels and buildings alongside the track.");

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  connect(ckSections, &QCheckBox::toggled,    this, &CExportWizard::OnSeparateChecked);
  connect(ckBacks,    &QCheckBox::toggled,    this, &CExportWizard::OnBacksChecked);
  connect(ckSigns,    &QCheckBox::toggled,    this, &CExportWizard::OnSignsChecked);
  connect(pbCancel,   &QPushButton::clicked,  this, &CExportWizard::close);
  connect(pbExport,   &QPushButton::clicked,  this, &CExportWizard::accept);
}

//-------------------------------------------------------------------------------------------------

CExportWizard::~CExportWizard()
{

}

//-------------------------------------------------------------------------------------------------

void CExportWizard::OnSeparateChecked(bool bChecked)
{
  m_bExportSeparate = bChecked;
}

//-------------------------------------------------------------------------------------------------

void CExportWizard::OnBacksChecked(bool bChecked)
{
  m_bExportBacks = bChecked;
}

//-------------------------------------------------------------------------------------------------

void CExportWizard::OnSignsChecked(bool bChecked)
{
  m_bExportSigns = bChecked;
}

//-------------------------------------------------------------------------------------------------