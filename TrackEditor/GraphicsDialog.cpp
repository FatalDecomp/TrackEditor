#include "TrackEditor.h"
#include "GraphicsDialog.h"
#include "MainWindow.h"

CGraphicsDialog::CGraphicsDialog(QWidget *pParent)
  : QDialog(pParent)
{
  setupUi(this);

  slDrawDistance->setValue(g_pMainWindow->m_graphics.iDrawDistancePercent);
  lblDrawDistanceValue->setText(
      QString("%1%").arg(g_pMainWindow->m_graphics.iDrawDistancePercent));
  ckHardwareRendering->setChecked(
      g_pMainWindow->m_graphics.bHardwareRendering);
  cbSoftwareDisplay->setCurrentIndex(
      g_pMainWindow->m_graphics.iSoftwareDisplay);
  cbAntiAliasing->setCurrentIndex(g_pMainWindow->m_graphics.iAntiAliasing);
  cbAnisotropy->setCurrentIndex(g_pMainWindow->m_graphics.iAnisotropy);
  cbTextureFilter->setCurrentIndex(g_pMainWindow->m_graphics.iTextureFilter);
  ckTrilinear->setChecked(g_pMainWindow->m_graphics.bTrilinear);
  dsbLodBias->setValue(g_pMainWindow->m_graphics.dLodBias);
  ckEmulateTransparentBorders->setChecked(
      g_pMainWindow->m_graphics.bEmulateTransparentBorders);

  connect(pbClose, &QPushButton::clicked, this, &CGraphicsDialog::reject);
  connect(slDrawDistance, &QSlider::valueChanged,
          this, &CGraphicsDialog::DialogEdited);
  connect(ckHardwareRendering, &QCheckBox::toggled,
          this, &CGraphicsDialog::HardwareRenderingToggled);
  connect(cbSoftwareDisplay, &QComboBox::currentIndexChanged,
          this, &CGraphicsDialog::DialogEdited);
  connect(cbAntiAliasing, &QComboBox::currentIndexChanged,
          this, &CGraphicsDialog::DialogEdited);
  connect(cbAnisotropy, &QComboBox::currentIndexChanged,
          this, &CGraphicsDialog::DialogEdited);
  connect(cbTextureFilter, &QComboBox::currentIndexChanged,
          this, &CGraphicsDialog::DialogEdited);
  connect(ckTrilinear, &QCheckBox::toggled,
          this, &CGraphicsDialog::DialogEdited);
  connect(dsbLodBias, &QDoubleSpinBox::valueChanged,
          this, &CGraphicsDialog::DialogEdited);
  connect(ckEmulateTransparentBorders, &QCheckBox::toggled,
          this, &CGraphicsDialog::DialogEdited);

  UpdateHardwareControls();
}

CGraphicsDialog::~CGraphicsDialog() = default;

void CGraphicsDialog::HardwareRenderingToggled(bool bChecked)
{
  (void)bChecked;
  UpdateHardwareControls();
  DialogEdited();
}

void CGraphicsDialog::DialogEdited()
{
  g_pMainWindow->m_graphics.iDrawDistancePercent = slDrawDistance->value();
  lblDrawDistanceValue->setText(QString("%1%").arg(slDrawDistance->value()));
  g_pMainWindow->m_graphics.bHardwareRendering =
      ckHardwareRendering->isChecked();
  g_pMainWindow->m_graphics.iSoftwareDisplay =
      cbSoftwareDisplay->currentIndex();
  g_pMainWindow->m_graphics.iAntiAliasing = cbAntiAliasing->currentIndex();
  g_pMainWindow->m_graphics.iAnisotropy = cbAnisotropy->currentIndex();
  g_pMainWindow->m_graphics.iTextureFilter = cbTextureFilter->currentIndex();
  g_pMainWindow->m_graphics.bTrilinear = ckTrilinear->isChecked();
  g_pMainWindow->m_graphics.dLodBias = dsbLodBias->value();
  g_pMainWindow->m_graphics.bEmulateTransparentBorders =
      ckEmulateTransparentBorders->isChecked();
  g_pMainWindow->ApplyGraphicsSettings();
}

void CGraphicsDialog::UpdateHardwareControls()
{
  gbHardwareOptions->setEnabled(ckHardwareRendering->isChecked());
  gbSoftwareOptions->setEnabled(!ckHardwareRendering->isChecked());
}
