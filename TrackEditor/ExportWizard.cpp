#include "ExportWizard.h"
#include "QtHelpers.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

CExportWizard::CExportWizard(QWidget *pParent, eExportType exportType)
  : QDialog(pParent)
  , m_bExportSeparate(true)
  , m_bExportBacks(true)
  , m_bExportSigns(true)
  , m_exportType(exportType)
{
  setupUi(this);

  ckSections->setChecked(m_bExportSeparate);
  ckBacks->setChecked(m_bExportBacks);
  ckSigns->setChecked(m_bExportSigns);

  // E4-S1. OBJ export now reads ROLLER's canonical geometry, which covers the
  // eleven track surface classes only: signs and buildings reach the emitter
  // through the camera-driven render path, so there is no camera-independent
  // sign traversal to extract them from yet (ROLLER
  // docs/adr/0003-canonical-geometry-conventions.md). Rather than fall back to
  // the editor's own CPU derivation - which works in a different, chunk-zero
  // relative frame and would place signs off the exported track - the option
  // is disabled for OBJ until that traversal exists. FBX still uses the legacy
  // path and keeps the option.
  if (m_exportType == eExportType::EXPORT_OBJ) {
    m_bExportSigns = false;
    ckSigns->setChecked(false);
    ckSigns->setEnabled(false);
    ckSigns->setToolTip(
        "Signs are not yet available from ROLLER's canonical geometry.");
  }

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