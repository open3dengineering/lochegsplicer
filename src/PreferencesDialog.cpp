/*
 * LocheGSplicer
 * Copyright (C) 2012 Jeff P. Houde (Lochemage)
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include <PreferencesDialog.h>

#include <QtGui>
#include <QVariant>


////////////////////////////////////////////////////////////////////////////////
PreferencesDialog::PreferencesDialog(PreferenceData& prefs)
   : mPrefs(prefs)
   , mCurrentExtruder(-1)
{
   setupUI();
   setupConnections();
}

////////////////////////////////////////////////////////////////////////////////
PreferenceData PreferencesDialog::getPreferences() const
{
   return mPrefs;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::keyPressEvent(QKeyEvent *e)
{
   if (e->key() == Qt::Key_Escape)
      close();
   else
      QWidget::keyPressEvent(e);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::showEvent(QShowEvent* event)
{
   restoreWindowState();
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::closeEvent(QCloseEvent* event)
{
   storeWindowState();

   QWidget::closeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onSaveConfigPressed()
{
   QMessageBox::information(this, "Save!", "Not implemented yet.", QMessageBox::Ok);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onLoadConfigPressed()
{
   QMessageBox::information(this, "Load!", "Not implemented yet.", QMessageBox::Ok);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onUseDisplayListsChanged(int state)
{
   mPrefs.useDisplayLists = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onDrawQualityChanged(int value)
{
   mPrefs.drawQuality = (DrawQuality)value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onLayerSkipChanged(int value)
{
   mPrefs.layerSkipSize = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onBackgroundColorPressed()
{
   QColor newColor = QColorDialog::getColor(mPrefs.backgroundColor, this, "Background Color");

   if (newColor.isValid())
   {
      setBackgroundColor(newColor);
   }
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onPrefixChanged()
{
   mPrefs.customPrefixCode = mGCodePrefixEdit->toPlainText();
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExportImportedStartCodeChanged(int state)
{
   mPrefs.exportImportedStartCode = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExportCommentsChanged(int state)
{
   mPrefs.exportComments = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExportAllAxesChanged(int state)
{
   mPrefs.exportAllAxes = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onPrintSkirtChanged(int state)
{
   mPrefs.printSkirt = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onSkirtDistanceChanged(double value)
{
   mPrefs.skirtDistance = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderSelected(int index)
{
   mCurrentExtruder = index;

   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   mExtruderOffsetXSpin->setEnabled(true);
   mExtruderOffsetXSpin->setValue(data.offset[X]);
   mExtruderOffsetYSpin->setEnabled(true);
   mExtruderOffsetYSpin->setValue(data.offset[Y]);
   mExtruderOffsetZSpin->setEnabled(true);
   mExtruderOffsetZSpin->setValue(data.offset[Z]);
   mExtruderFlowSpin->setEnabled(true);
   mExtruderFlowSpin->setValue(data.flow);
   mExtruderIdleTempSpin->setEnabled(true);
   mExtruderIdleTempSpin->setValue(data.idleTemp);
   mExtruderPrintTempSpin->setEnabled(true);
   mExtruderPrintTempSpin->setValue(data.printTemp);
   mExtruderColorButton->setEnabled(true);
   QPixmap pix = QPixmap(QSize(16, 16));
   pix.fill(data.color);
   mExtruderColorButton->setIcon(QIcon(pix));
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderAddPressed()
{
   mPrefs.extruderList.push_back(ExtruderData());
   int index = (int)mPrefs.extruderList.size();

   mExtruderList->addItem("Extruder: " + QString::number(index));
   mExtruderList->setCurrentRow(index - 1);

   mRemoveExtruderButton->setEnabled(true);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderRemovePressed()
{
   if (mPrefs.extruderList.size() > 1)
   {
      mPrefs.extruderList.pop_back();
   }

   if (mExtruderList->count() > 1)
   {
      if (mExtruderList->currentRow() == mExtruderList->count() - 1)
      {
         mExtruderList->setCurrentRow(mExtruderList->count() - 2);
      }

      mExtruderList->takeItem(mExtruderList->count() - 1);

      if (mExtruderList->count() <= 1)
      {
         mRemoveExtruderButton->setEnabled(false);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderOffsetXChanged(double value)
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   data.offset[X] = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderOffsetYChanged(double value)
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   data.offset[Y] = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderOffsetZChanged(double value)
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   data.offset[Z] = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderFlowChanged(double value)
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   data.flow = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderIdleTempChanged(double value)
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   data.idleTemp = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderPrintTempChanged(double value)
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];

   data.printTemp = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExtruderColorPressed()
{
   if (mCurrentExtruder < 0 || mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   ExtruderData& data = mPrefs.extruderList[mCurrentExtruder];
   
   QColor newColor = QColorDialog::getColor(data.color, this, "Background Color");

   if (newColor.isValid())
   {
      setExtruderColor(newColor);
   }
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onPlatformWidthChanged(double value)
{
   mPrefs.platformWidth = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onPlatformHeightChanged(double value)
{
   mPrefs.platformHeight = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExportAbsoluteModeChanged(int state)
{
   mPrefs.exportAbsoluteMode = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onExportAbsoluteEModeChanged(int state)
{
   mPrefs.exportAbsoluteEMode = (state == Qt::Checked);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onImportRetractionChanged(double value)
{
   mPrefs.importRetraction = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onImportPrimerChanged(double value)
{
   mPrefs.importPrimer = value;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onOkPressed()
{
   storeLastPreferences();
   accept();
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::onDefaultPressed()
{
   mPrefs = PreferenceData();

   updateUI();
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::setupUI()
{
   setWindowTitle(tr("Preferences"));
   resize(400, 300);

   QVBoxLayout* mainLayout = new QVBoxLayout();
   mainLayout->setMargin(0);

   mTabWidget = new QTabWidget();
   mainLayout->addWidget(mTabWidget);

   //// Editor Tab.
   QWidget* editorTab = new QWidget();
   mTabWidget->addTab(editorTab, "Editor");
   {
      mTabWidget->setTabToolTip(0, "Preferences related to this editor.");
      QGridLayout* editorLayout = new QGridLayout();
      editorTab->setLayout(editorLayout);

      // File Group.
      QGroupBox* fileGroup = new QGroupBox("File");
      editorLayout->addWidget(fileGroup, 0, 0, 1, 1);

      QHBoxLayout* fileLayout = new QHBoxLayout();
      fileGroup->setLayout(fileLayout);

      mSaveConfigurationButton = new QPushButton("Save Config...");
      mSaveConfigurationButton->setToolTip("Save all current preferences into a data file.");
      fileLayout->addWidget(mSaveConfigurationButton);

      mLoadConfigurationButton = new QPushButton("Load Config...");
      mLoadConfigurationButton->setToolTip("Restore all preferences from a data file.");
      fileLayout->addWidget(mLoadConfigurationButton);
      editorLayout->setRowStretch(0, 0);

      // Rendering Group.
      QGroupBox* renderingGroup = new QGroupBox("Rendering");
      editorLayout->addWidget(renderingGroup, 1, 0, 1, 1);

      QGridLayout* renderingLayout = new QGridLayout();
      renderingGroup->setLayout(renderingLayout);

      QLabel* useDisplayListsLabel = new QLabel("Use Display Lists: ");
      useDisplayListsLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      renderingLayout->addWidget(useDisplayListsLabel, 0, 0, 1, 1);

      mUseDisplayListsCheckbox = new QCheckBox();
      mUseDisplayListsCheckbox->setToolTip("Depending on hardware configurations of your device, this may improve your rendering speed at the cost of memory.");
      renderingLayout->addWidget(mUseDisplayListsCheckbox, 0, 1, 1, 2);

      QLabel* drawQualityLabel = new QLabel("Draw Quality: ");
      drawQualityLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      renderingLayout->addWidget(drawQualityLabel, 1, 0, 1, 1);

      mDrawQualityCombo = new QComboBox();
      mDrawQualityCombo->addItem("Low");
      mDrawQualityCombo->addItem("Medium");
      mDrawQualityCombo->addItem("High");
      mDrawQualityCombo->setToolTip("Set the quality of the geometry to be rendered.");
      renderingLayout->addWidget(mDrawQualityCombo, 1, 1, 1, 2);

      QLabel* layerSkipLabel = new QLabel("Layer Skip: ");
      layerSkipLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      renderingLayout->addWidget(layerSkipLabel, 2, 0, 1, 1);

      mLayerSkipSpin = new QSpinBox();
      mLayerSkipSpin->setToolTip("This will skip the generation of geometry for every # of layers.");
      renderingLayout->addWidget(mLayerSkipSpin, 2, 1, 1, 2);

      mBackgroundColorButton = new QPushButton("Background Color");
      mBackgroundColorButton->setToolTip("The background color of the visualizer window.");
      renderingLayout->addWidget(mBackgroundColorButton, 3, 0, 1, 3);
      renderingLayout->setRowStretch(1, 0);

      renderingLayout->setColumnStretch(0, 1);
      renderingLayout->setColumnStretch(1, 1);
      renderingLayout->setColumnStretch(2, 1);

      editorLayout->setRowStretch(2, 1);
   }

   //// Splicing Tab.
   QWidget* splicingTab = new QWidget();
   mTabWidget->addTab(splicingTab, "Splicing");
   {
      mTabWidget->setTabToolTip(1, "Preferences related to the splicing output.");
      QVBoxLayout* splicingLayout = new QVBoxLayout();
      splicingTab->setLayout(splicingLayout);

      // Custom GCode Prefix.
      QGroupBox* gcodeGroup = new QGroupBox("Export");
      splicingLayout->addWidget(gcodeGroup);

      QVBoxLayout* gcodeLayout = new QVBoxLayout();
      gcodeGroup->setLayout(gcodeLayout);

      // Export start code from imported objects.
      mExportImportedStartCodeCheckbox = new QCheckBox("Export Imported Start Code");
      mExportImportedStartCodeCheckbox->setToolTip("This option will include the start code found in the first imported object.  The code\nwill appear at the beginning of the file followed by the below GCode Prefix.");
      gcodeLayout->addWidget(mExportImportedStartCodeCheckbox);

      QLabel* prefixLabel = new QLabel("GCode Prefix:");
      gcodeLayout->addWidget(prefixLabel);

      mGCodePrefixEdit = new QTextEdit();
      mGCodePrefixEdit->setToolTip("GCode that will be inserted at the beginning of the spliced output file.");
      gcodeLayout->addWidget(mGCodePrefixEdit);

      // Export comments and all axes.
      QHBoxLayout* exportLayout = new QHBoxLayout();
      gcodeLayout->addLayout(exportLayout);

      mExportCommentsCheckbox = new QCheckBox("Export Comments");
      mExportCommentsCheckbox->setToolTip("This will include comments in the spliced output file.");
      mExportAllAxesCheckbox = new QCheckBox("Export All Axes");
      mExportAllAxesCheckbox->setToolTip("This option will include gcode for all axes even if they are unchanged.");
      exportLayout->addWidget(mExportCommentsCheckbox);
      exportLayout->addWidget(mExportAllAxesCheckbox);

      // Skirt.
      QGroupBox* skirtGroup = new QGroupBox("Skirt");
      splicingLayout->addWidget(skirtGroup);

      QHBoxLayout* skirtLayout = new QHBoxLayout();
      skirtGroup->setLayout(skirtLayout);

      mPrintSkirtCheckbox = new QCheckBox("Enable Skirt");
      mPrintSkirtCheckbox->setLayoutDirection(Qt::RightToLeft);
      mPrintSkirtCheckbox->setToolTip("If enabled, a custom skirt will be generated in the spliced output file, one skirt loop per extruder.");
      QLabel* skirtDistanceLabel = new QLabel("Skirt Distance: ");
      skirtDistanceLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      mSkirtDistanceSpin = new QDoubleSpinBox();
      mSkirtDistanceSpin->setSuffix("mm");
      mSkirtDistanceSpin->setMinimum(0.0);
      mSkirtDistanceSpin->setMaximum(100.0);
      mSkirtDistanceSpin->setToolTip("The distance (in mm) that the skirt will be generated.");
      skirtLayout->addWidget(mPrintSkirtCheckbox);
      skirtLayout->addWidget(skirtDistanceLabel);
      skirtLayout->addWidget(mSkirtDistanceSpin);
   }

   //// Printer Tab.
   QWidget* printerTab = new QWidget();
   mTabWidget->addTab(printerTab, "Printer");
   {
      mTabWidget->setTabToolTip(2, "Preferences that define the properties of your printer.");
      QVBoxLayout* printerLayout = new QVBoxLayout();
      printerTab->setLayout(printerLayout);

      // Extruder List.
      QGroupBox* extruderGroup = new QGroupBox("Extruders");
      printerLayout->addWidget(extruderGroup);

      QGridLayout* extruderLayout = new QGridLayout();
      extruderGroup->setLayout(extruderLayout);

      mExtruderList = new QListWidget();
      mExtruderList->setToolTip("List of all extruders that the printer controls.");
      extruderLayout->addWidget(mExtruderList, 0, 0, 6, 2);

      // Extruder Properties.
      QLabel* extruderOffsetXLabel = new QLabel("Offset X: ");
      extruderOffsetXLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      QLabel* extruderOffsetYLabel = new QLabel("Y: ");
      extruderOffsetYLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      QLabel* extruderOffsetZLabel = new QLabel("Z: ");
      extruderOffsetZLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      QLabel* extruderFlowLabel = new QLabel("Flow %: ");
      extruderFlowLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      QLabel* extruderIdleTempLabel = new QLabel("Idle Temp: ");
      extruderIdleTempLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      QLabel* extruderPrintTempLabel = new QLabel("Print Temp: ");
      extruderPrintTempLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

      mExtruderOffsetXSpin = new QDoubleSpinBox();
      mExtruderOffsetXSpin->setMinimum(0.0);
      mExtruderOffsetXSpin->setMaximum(1000.0);
      mExtruderOffsetXSpin->setDecimals(4);
      mExtruderOffsetXSpin->setToolTip("The X coordinate offset of this extruder relative to the primary.");
      mExtruderOffsetYSpin = new QDoubleSpinBox();
      mExtruderOffsetYSpin->setMinimum(0.0);
      mExtruderOffsetYSpin->setMaximum(1000.0);
      mExtruderOffsetYSpin->setDecimals(4);
      mExtruderOffsetYSpin->setToolTip("The Y coordinate offset of this extruder relative to the primary.");
      mExtruderOffsetZSpin = new QDoubleSpinBox();
      mExtruderOffsetZSpin->setMinimum(0.0);
      mExtruderOffsetZSpin->setMaximum(1000.0);
      mExtruderOffsetZSpin->setDecimals(4);
      mExtruderOffsetZSpin->setToolTip("The Z coordinate offset of this extruder relative to the primary.");
      mExtruderFlowSpin = new QDoubleSpinBox();
      mExtruderFlowSpin->setMinimum(0.0);
      mExtruderFlowSpin->setMaximum(100.0);
      mExtruderFlowSpin->setSingleStep(0.1);
      mExtruderFlowSpin->setDecimals(4);
      mExtruderFlowSpin->setToolTip("A multiplier to apply to the extrusion flow values of the spliced output file.");
      mExtruderIdleTempSpin = new QDoubleSpinBox();
      mExtruderIdleTempSpin->setMinimum(0.0);
      mExtruderIdleTempSpin->setMaximum(1000.0);
      mExtruderIdleTempSpin->setToolTip("The temperature when this extruder is idle (0 = Keep current temp).");
      mExtruderPrintTempSpin = new QDoubleSpinBox();
      mExtruderPrintTempSpin->setMinimum(0.0);
      mExtruderPrintTempSpin->setMaximum(1000.0);
      mExtruderPrintTempSpin->setToolTip("The temperature when this extruder is printing (0 = Keep current temp).");
      mExtruderColorButton = new QPushButton("Color");
      QPixmap pix = QPixmap(QSize(16, 16));
      pix.fill(Qt::darkGray);
      mExtruderColorButton->setIcon(QIcon(pix));
      mExtruderColorButton->setToolTip("The color to render the geometry for anything printed with this extruder.");

      QFrame* line = new QFrame();
      line->setObjectName(QString::fromUtf8("line"));
      line->setGeometry(QRect(0, 0, 3, 1000));
      line->setFrameShape(QFrame::VLine);
      line->setFrameShadow(QFrame::Sunken);
      extruderLayout->addWidget(line,                  0, 2, 7, 1);

      extruderLayout->addWidget(extruderOffsetXLabel,  0, 3, 1, 1);
      extruderLayout->addWidget(mExtruderOffsetXSpin,  0, 4, 1, 1);
      extruderLayout->addWidget(extruderOffsetYLabel,  1, 3, 1, 1);
      extruderLayout->addWidget(mExtruderOffsetYSpin,  1, 4, 1, 1);
      extruderLayout->addWidget(extruderOffsetZLabel,  2, 3, 1, 1);
      extruderLayout->addWidget(mExtruderOffsetZSpin,  2, 4, 1, 1);
      extruderLayout->addWidget(extruderFlowLabel,     3, 3, 1, 1);
      extruderLayout->addWidget(mExtruderFlowSpin,     3, 4, 1, 1);
      extruderLayout->addWidget(extruderIdleTempLabel, 4, 3, 1, 1);
      extruderLayout->addWidget(mExtruderIdleTempSpin, 4, 4, 1, 1);
      extruderLayout->addWidget(extruderPrintTempLabel,5, 3, 1, 1);
      extruderLayout->addWidget(mExtruderPrintTempSpin,5, 4, 1, 1);
      extruderLayout->addWidget(mExtruderColorButton,  6, 3, 1, 2);

      // Add/Remove buttons.
      mAddExtruderButton = new QPushButton("Add");
      mAddExtruderButton->setToolTip("Add an extruder to the list.");
      mRemoveExtruderButton = new QPushButton("Remove");
      mRemoveExtruderButton->setToolTip("Remove the last extruder from the list.");
      extruderLayout->addWidget(mAddExtruderButton, 6, 0, 1, 1);
      extruderLayout->addWidget(mRemoveExtruderButton, 6, 1, 1, 1);

      if (mPrefs.extruderList.empty())
      {
         mRemoveExtruderButton->setEnabled(false);
      }

      // Platform Dimensions.
      QGroupBox* platformGroup = new QGroupBox("Platform");
      printerLayout->addWidget(platformGroup);

      QHBoxLayout* platformLayout = new QHBoxLayout();
      platformGroup->setLayout(platformLayout);

      QLabel* platformWidthLabel = new QLabel("Width (X): ");
      platformWidthLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      QLabel* platformHeightLabel = new QLabel("Height (Y): ");
      platformHeightLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

      mPlatformWidthSpin = new QDoubleSpinBox();
      mPlatformWidthSpin->setMinimum(0.0);
      mPlatformWidthSpin->setMaximum(1000.0);
      mPlatformWidthSpin->setToolTip("The width of your build platform.");

      mPlatformHeightSpin = new QDoubleSpinBox();
      mPlatformHeightSpin->setMinimum(0.0);
      mPlatformHeightSpin->setMaximum(1000.0);
      mPlatformHeightSpin->setToolTip("The height of your build platform.");

      platformLayout->addWidget(platformWidthLabel);
      platformLayout->addWidget(mPlatformWidthSpin);
      platformLayout->addWidget(platformHeightLabel);
      platformLayout->addWidget(mPlatformHeightSpin);
   }

   //// Advanced Tab.
   QWidget* advancedTab = new QWidget();
   mTabWidget->addTab(advancedTab, "Advanced");
   {
      mTabWidget->setTabToolTip(3, "Advanced preferences.");
      QGridLayout* advancedLayout = new QGridLayout();
      advancedTab->setLayout(advancedLayout);

      // Export group.
      QGroupBox* exportGroup = new QGroupBox("Export");
      advancedLayout->addWidget(exportGroup, 0, 0, 1, 1);

      QHBoxLayout* exportLayout = new QHBoxLayout();
      exportGroup->setLayout(exportLayout);

      mExportAbsoluteModeCheckbox = new QCheckBox("Export Absolute Axes");
      mExportAbsoluteModeCheckbox->setToolTip("If enabled, this will export all extruder movement axes in absolute\ncoordinates. If disabled, coordinates will be relative.");
      exportLayout->addWidget(mExportAbsoluteModeCheckbox);

      mExportAbsoluteEModeCheckbox = new QCheckBox("Export Absolute E Axis");
      mExportAbsoluteEModeCheckbox->setToolTip("If enabled, this will export all extrusions in absolute coordinates.\nIf disabled, coordinates will be relative.");
      exportLayout->addWidget(mExportAbsoluteEModeCheckbox);

      // Import group.
      QGroupBox* importGroup = new QGroupBox("Import");
      advancedLayout->addWidget(importGroup, 1, 0, 1, 1);

      QGridLayout* importLayout = new QGridLayout();
      importGroup->setLayout(importLayout);

      QLabel* retractionLabel = new QLabel("Retraction: ");
      retractionLabel->setAlignment(Qt::AlignRight);
      importLayout->addWidget(retractionLabel, 0, 0, 1, 1);

      QLabel* primerLabel = new QLabel("Primer: ");
      primerLabel->setAlignment(Qt::AlignRight);
      importLayout->addWidget(primerLabel, 1, 0, 1, 1);

      mImportRetractionSpin = new QDoubleSpinBox();
      mImportRetractionSpin->setToolTip("When importing objects, this defines the amount of retraction to expect in that file (use -1 to auto calculate).");
      mImportRetractionSpin->setMinimum(-1.0);
      mImportRetractionSpin->setMaximum(1000.0);
      mImportRetractionSpin->setDecimals(4);
      importLayout->addWidget(mImportRetractionSpin, 0, 1, 1, 1);

      mImportPrimerSpin = new QDoubleSpinBox();
      mImportPrimerSpin->setToolTip("When importing objects, this defines the amount of retraction priming to expect in that file (use -1 to use the same value as retraction).\nYou really only need to change this if you use a different prime length than retraction.");
      mImportPrimerSpin->setMinimum(-1.0);
      mImportPrimerSpin->setMaximum(1000.0);
      mImportPrimerSpin->setDecimals(4);
      importLayout->addWidget(mImportPrimerSpin, 1, 1, 1, 1);

      importLayout->setColumnStretch(0, 1);
      importLayout->setColumnStretch(1, 2);

      advancedLayout->setRowStretch(0, 0);
      advancedLayout->setRowStretch(1, 0);
      advancedLayout->setRowStretch(2, 1);
   }

   //// Dialog buttons.
   QHBoxLayout* buttonLayout = new QHBoxLayout();
   mainLayout->addLayout(buttonLayout);

   mOkButton = new QPushButton("Ok");
   mOkButton->setToolTip("Save and close this preference dialog.");
   buttonLayout->addWidget(mOkButton);

   mCancelButton = new QPushButton("Cancel");
   mCancelButton->setToolTip("Close this preference dialog and discard any changes made.");
   buttonLayout->addWidget(mCancelButton);

   mDefaultButton = new QPushButton("Reset to Defaults");
   mDefaultButton->setToolTip("Restore all preferences to their default settings.");
   buttonLayout->addWidget(mDefaultButton);

   setLayout(mainLayout);

   restoreLastPreferences(mPrefs);
   updateUI();
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::setupConnections()
{
   //// Editor Tab.
   connect(mSaveConfigurationButton,         SIGNAL(pressed()),                  this, SLOT(onSaveConfigPressed()));
   connect(mLoadConfigurationButton,         SIGNAL(pressed()),                  this, SLOT(onLoadConfigPressed()));
   connect(mUseDisplayListsCheckbox,         SIGNAL(stateChanged(int)),          this, SLOT(onUseDisplayListsChanged(int)));
   connect(mDrawQualityCombo,                SIGNAL(currentIndexChanged(int)),   this, SLOT(onDrawQualityChanged(int)));
   connect(mLayerSkipSpin,                   SIGNAL(valueChanged(int)),          this, SLOT(onLayerSkipChanged(int)));
   connect(mBackgroundColorButton,           SIGNAL(pressed()),                  this, SLOT(onBackgroundColorPressed()));

   //// Splicing Tab.
   connect(mGCodePrefixEdit,                 SIGNAL(textChanged()),              this, SLOT(onPrefixChanged()));
   connect(mExportImportedStartCodeCheckbox, SIGNAL(stateChanged(int)),          this, SLOT(onExportImportedStartCodeChanged(int)));
   connect(mExportCommentsCheckbox,          SIGNAL(stateChanged(int)),          this, SLOT(onExportCommentsChanged(int)));
   connect(mExportAllAxesCheckbox,           SIGNAL(stateChanged(int)),          this, SLOT(onExportAllAxesChanged(int)));
   connect(mPrintSkirtCheckbox,              SIGNAL(stateChanged(int)),          this, SLOT(onPrintSkirtChanged(int)));
   connect(mSkirtDistanceSpin,               SIGNAL(valueChanged(double)),       this, SLOT(onSkirtDistanceChanged(double)));

   //// Printer Tab.
   connect(mExtruderList,                    SIGNAL(currentRowChanged(int)),     this, SLOT(onExtruderSelected(int)));
   connect(mAddExtruderButton,               SIGNAL(pressed()),                  this, SLOT(onExtruderAddPressed()));
   connect(mRemoveExtruderButton,            SIGNAL(pressed()),                  this, SLOT(onExtruderRemovePressed()));
   connect(mExtruderOffsetXSpin,             SIGNAL(valueChanged(double)),       this, SLOT(onExtruderOffsetXChanged(double)));
   connect(mExtruderOffsetYSpin,             SIGNAL(valueChanged(double)),       this, SLOT(onExtruderOffsetYChanged(double)));
   connect(mExtruderOffsetZSpin,             SIGNAL(valueChanged(double)),       this, SLOT(onExtruderOffsetZChanged(double)));
   connect(mExtruderFlowSpin,                SIGNAL(valueChanged(double)),       this, SLOT(onExtruderFlowChanged(double)));
   connect(mExtruderIdleTempSpin,            SIGNAL(valueChanged(double)),       this, SLOT(onExtruderIdleTempChanged(double)));
   connect(mExtruderPrintTempSpin,           SIGNAL(valueChanged(double)),       this, SLOT(onExtruderPrintTempChanged(double)));
   connect(mExtruderColorButton,             SIGNAL(pressed()),                  this, SLOT(onExtruderColorPressed()));
   
   connect(mPlatformWidthSpin,               SIGNAL(valueChanged(double)),       this, SLOT(onPlatformWidthChanged(double)));
   connect(mPlatformHeightSpin,              SIGNAL(valueChanged(double)),       this, SLOT(onPlatformHeightChanged(double)));

   //// Advanced Tab.
   connect(mExportAbsoluteModeCheckbox,      SIGNAL(stateChanged(int)),          this, SLOT(onExportAbsoluteModeChanged(int)));
   connect(mExportAbsoluteEModeCheckbox,     SIGNAL(stateChanged(int)),          this, SLOT(onExportAbsoluteEModeChanged(int)));
   connect(mImportRetractionSpin,            SIGNAL(valueChanged(double)),       this, SLOT(onImportRetractionChanged(double)));
   connect(mImportPrimerSpin,                SIGNAL(valueChanged(double)),       this, SLOT(onImportPrimerChanged(double)));

   //// Dialog Buttons.
   connect(mOkButton,                        SIGNAL(pressed()),                  this, SLOT(onOkPressed()));
   connect(mCancelButton,                    SIGNAL(pressed()),                  this, SLOT(close()));
   connect(mDefaultButton,                   SIGNAL(pressed()),                  this, SLOT(onDefaultPressed()));
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::updateUI()
{
   // Editor Tab.
   mUseDisplayListsCheckbox->setChecked(mPrefs.useDisplayLists);
   mDrawQualityCombo->setCurrentIndex((int)mPrefs.drawQuality);
   mLayerSkipSpin->setValue(mPrefs.layerSkipSize);
   setBackgroundColor(mPrefs.backgroundColor);

   // Splicing Tab.
   mGCodePrefixEdit->setText(mPrefs.customPrefixCode);
   mExportImportedStartCodeCheckbox->setChecked(mPrefs.exportImportedStartCode);
   mExportCommentsCheckbox->setChecked(mPrefs.exportComments);
   mExportAllAxesCheckbox->setChecked(mPrefs.exportAllAxes);
   mPrintSkirtCheckbox->setChecked(mPrefs.printSkirt);
   mSkirtDistanceSpin->setValue(mPrefs.skirtDistance);

   // Printer Tab.
   mExtruderList->clear();
   int extruderCount = (int)mPrefs.extruderList.size();
   for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
   {
      mExtruderList->addItem("Extruder: " + QString::number(extruderIndex + 1));
   }
   mExtruderOffsetXSpin->setEnabled(false);
   mExtruderOffsetYSpin->setEnabled(false);
   mExtruderOffsetZSpin->setEnabled(false);
   mExtruderFlowSpin->setEnabled(false);
   mExtruderIdleTempSpin->setEnabled(false);
   mExtruderPrintTempSpin->setEnabled(false);
   mExtruderColorButton->setEnabled(false);
   mExtruderOffsetXSpin->setValue(0.0);
   mExtruderOffsetYSpin->setValue(0.0);
   mExtruderOffsetZSpin->setValue(0.0);
   mExtruderFlowSpin->setValue(1.0);
   mExtruderIdleTempSpin->setValue(0.0);
   mExtruderPrintTempSpin->setValue(0.0);
   QPixmap pix = QPixmap(QSize(16, 16));
   pix.fill(Qt::darkGray);
   mExtruderColorButton->setIcon(QIcon(pix));
   mPlatformWidthSpin->setValue(mPrefs.platformWidth);
   mPlatformHeightSpin->setValue(mPrefs.platformHeight);
   mCurrentExtruder = -1;

   // Advanced Tab.
   mExportAbsoluteModeCheckbox->setChecked(mPrefs.exportAbsoluteMode);
   mExportAbsoluteEModeCheckbox->setChecked(mPrefs.exportAbsoluteEMode);
   mImportRetractionSpin->setValue(mPrefs.importRetraction);
   mImportPrimerSpin->setValue(mPrefs.importPrimer);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::setBackgroundColor(const QColor& color)
{
   mPrefs.backgroundColor = color;
   QPixmap pix = QPixmap(QSize(16, 16));
   pix.fill(mPrefs.backgroundColor);
   mBackgroundColorButton->setIcon(QIcon(pix));

   emit emitBackgroundColorChanged(mPrefs.backgroundColor);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::setExtruderColor(const QColor& color)
{
   if (mCurrentExtruder < 0 && mCurrentExtruder >= (int)mPrefs.extruderList.size())
   {
      return;
   }

   mPrefs.extruderList[mCurrentExtruder].color = color;
   QPixmap pix = QPixmap(QSize(16, 16));
   pix.fill(color);
   mExtruderColorButton->setIcon(QIcon(pix));

   emit emitExtruderColorChanged(color, mCurrentExtruder);
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::storeWindowState()
{
   QSettings settings(COMPANY_NAME, APPLICATION_NAME);
   settings.beginGroup("PreferencesDialogState");
   settings.setValue("geometry", saveGeometry());
   settings.endGroup();
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::storeLastPreferences()
{
   QSettings settings(COMPANY_NAME, APPLICATION_NAME);

   settings.beginGroup("LastPreferences");
   {
      // Editor properties.
      settings.setValue("BackgroundColor", mPrefs.backgroundColor);
      settings.setValue("UseDisplayLists", mPrefs.useDisplayLists);
      settings.setValue("DrawQuality", (int)mPrefs.drawQuality);
      // Splicing properties.
      settings.setValue("CustomPrefixCode", mPrefs.customPrefixCode);
      settings.setValue("ExportImportedStartCode", mPrefs.exportImportedStartCode);
      settings.setValue("ExportComments", mPrefs.exportComments);
      settings.setValue("ExportAllAxes", mPrefs.exportAllAxes);
      settings.setValue("PrintSkirt", mPrefs.printSkirt);
      settings.setValue("SkirtDistance", mPrefs.skirtDistance);
      // Printer properties.
      int extruderCount = (int)mPrefs.extruderList.size();
      settings.beginWriteArray("Extruders", extruderCount);
      for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
      {
         const ExtruderData& data = mPrefs.extruderList[extruderIndex];
         settings.setArrayIndex(extruderIndex);
         settings.setValue("OffsetX", data.offset[X]);
         settings.setValue("OffsetY", data.offset[Y]);
         settings.setValue("OffsetZ", data.offset[Z]);
         settings.setValue("Flow", data.flow);
         settings.setValue("IdleTemp", data.idleTemp);
         settings.setValue("PrintTemp", data.printTemp);
         settings.setValue("Color", data.color);
      }
      settings.endArray();
      settings.setValue("PlatformWidth", mPrefs.platformWidth);
      settings.setValue("PlatformHeight", mPrefs.platformHeight);
      // Advanced properties.
      settings.setValue("ExportAbsoluteMode", mPrefs.exportAbsoluteMode);
      settings.setValue("ExportAbsoluteEMode", mPrefs.exportAbsoluteEMode);
      settings.setValue("ImportRetraction", mPrefs.importRetraction);
      settings.setValue("ImportPrimer", mPrefs.importPrimer);
   }
   settings.endGroup();
}

////////////////////////////////////////////////////////////////////////////////
bool PreferencesDialog::restoreLastPreferences(PreferenceData& prefs)
{
   QSettings settings(COMPANY_NAME, APPLICATION_NAME);

   settings.beginGroup("LastPreferences");
   {
      if (!settings.contains("BackgroundColor"))
      {
         return false;
      }

      // Editor properties.
      PreferenceData defaults;
      prefs.backgroundColor = settings.value("BackgroundColor", defaults.backgroundColor).value<QColor>();
      prefs.useDisplayLists = settings.value("UseDisplayLists", defaults.useDisplayLists).toBool();
      prefs.drawQuality = (DrawQuality)settings.value("DrawQuality", (int)defaults.drawQuality).toInt();
      // Splicing properties.
      prefs.customPrefixCode = settings.value("CustomPrefixCode", defaults.customPrefixCode).toString();
      prefs.exportImportedStartCode = settings.value("ExportImportedStartCode", defaults.exportImportedStartCode).toBool();
      prefs.exportComments = settings.value("ExportComments", defaults.exportComments).toBool();
      prefs.exportAllAxes = settings.value("ExportAllAxes", defaults.exportAllAxes).toBool();
      prefs.printSkirt = settings.value("PrintSkirt", defaults.printSkirt).toBool();
      prefs.skirtDistance = settings.value("SkirtDistance", defaults.skirtDistance).toDouble();
      // Printer properties.
      ExtruderData defaultExtruder;
      int extruderCount = settings.beginReadArray("Extruders");
      prefs.extruderList.resize(extruderCount);
      for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
      {
         ExtruderData& data = prefs.extruderList[extruderIndex];
         settings.setArrayIndex(extruderIndex);
         data.offset[X] = settings.value("OffsetX", defaultExtruder.offset[X]).toDouble();
         data.offset[Y] = settings.value("OffsetY", defaultExtruder.offset[Y]).toDouble();
         data.offset[Z] = settings.value("OffsetZ", defaultExtruder.offset[Z]).toDouble();
         data.flow = settings.value("Flow", defaultExtruder.flow).toDouble();
         data.idleTemp = settings.value("IdleTemp", defaultExtruder.idleTemp).toDouble();
         data.printTemp = settings.value("PrintTemp", defaultExtruder.printTemp).toDouble();
         data.color = settings.value("Color", defaultExtruder.color).value<QColor>();
      }
      settings.endArray();
      prefs.platformWidth = settings.value("PlatformWidth", defaults.platformWidth).toDouble();
      prefs.platformHeight = settings.value("PlatformHeight", defaults.platformHeight).toDouble();
      // Advanced properties.
      prefs.exportAbsoluteMode = settings.value("ExportAbsoluteMode", defaults.exportAbsoluteMode).toBool();
      prefs.exportAbsoluteEMode = settings.value("ExportAbsoluteEMode", defaults.exportAbsoluteEMode).toBool();
      prefs.importRetraction = settings.value("ImportRetraction", defaults.importRetraction).toDouble();
      prefs.importPrimer = settings.value("ImportPrimer", defaults.importPrimer).toDouble();
   }
   settings.endGroup();

   return true;
}

////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::restoreWindowState()
{
   QSettings settings(COMPANY_NAME, APPLICATION_NAME);
   settings.beginGroup("PreferencesDialogState");
   if (settings.contains("geometry"))
   {
      restoreGeometry(settings.value("geometry").toByteArray());
   }
   settings.endGroup();
}

////////////////////////////////////////////////////////////////////////////////
