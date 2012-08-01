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


#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <Constants.h>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QComboBox;
class QSpinBox;
class QTextEdit;
class QCheckBox;
class QDoubleSpinBox;
class QListWidget;
QT_END_NAMESPACE


class PreferencesDialog : public QDialog
{
   Q_OBJECT

public:
   PreferencesDialog(PreferenceData& prefs);

   PreferenceData getPreferences() const;

protected:
   void keyPressEvent(QKeyEvent *event);
   void showEvent(QShowEvent* event);
   void closeEvent(QCloseEvent* event);

signals:
   void emitBackgroundColorChanged(const QColor& color);
   void emitExtruderColorChanged(const QColor& color, int index);

public slots:
   //// Editor Tab.
   void onSaveConfigPressed();
   void onLoadConfigPressed();
   void onUseDisplayListsChanged(int state);
   void onDrawQualityChanged(int value);
   void onLayerSkipChanged(int value);
   void onBackgroundColorPressed();

   //// Splicing Tab.
   void onPrefixChanged();
   void onExportCommentsChanged(int state);
   void onExportAllAxesChanged(int state);
   void onPrintSkirtChanged(int state);
   void onSkirtDistanceChanged(double value);

   //// Printer Tab.
   void onExtruderSelected(int index);
   void onExtruderAddPressed();
   void onExtruderRemovePressed();
   void onExtruderOffsetXChanged(double value);
   void onExtruderOffsetYChanged(double value);
   void onExtruderOffsetZChanged(double value);
   void onExtruderFlowChanged(double value);
   void onExtruderIdleTempChanged(double value);
   void onExtruderPrintTempChanged(double value);
   void onExtruderColorPressed();
   void onPlatformWidthChanged(double value);
   void onPlatformHeightChanged(double value);

   //// Advanced Tab.

   //// Dialog buttons.
   void onDefaultPressed();

private:
   void setupUI();
   void setupConnections();

   void setBackgroundColor(const QColor& color);
   void setExtruderColor(const QColor& color);

   void storeWindowState();
   void restoreWindowState();

   PreferenceData    mPrefs;

   QTabWidget*       mTabWidget;

   //// Editor Tab
   QPushButton*      mSaveConfigurationButton;
   QPushButton*      mLoadConfigurationButton;
   QCheckBox*        mUseDisplayListsCheckbox;
   QComboBox*        mDrawQualityCombo;
   QSpinBox*         mLayerSkipSpin;
   QPushButton*      mBackgroundColorButton;

   //// Splicing Tab
   QTextEdit*        mGCodePrefixEdit;
   QCheckBox*        mExportCommentsCheckbox;
   QCheckBox*        mExportAllAxesCheckbox;
   QCheckBox*        mPrintSkirtCheckbox;
   QDoubleSpinBox*   mSkirtDistanceSpin;

   //// Printer Tab.
   QListWidget*      mExtruderList;
   QPushButton*      mAddExtruderButton;
   QPushButton*      mRemoveExtruderButton;
   QDoubleSpinBox*   mExtruderOffsetXSpin;
   QDoubleSpinBox*   mExtruderOffsetYSpin;
   QDoubleSpinBox*   mExtruderOffsetZSpin;
   QDoubleSpinBox*   mExtruderFlowSpin;
   QDoubleSpinBox*   mExtruderIdleTempSpin;
   QDoubleSpinBox*   mExtruderPrintTempSpin;
   QPushButton*      mExtruderColorButton;
   QDoubleSpinBox*   mPlatformWidthSpin;
   QDoubleSpinBox*   mPlatformHeightSpin;
   int               mCurrentExtruder;

   //// Advanced Tab.

   //// Dialog Buttons.
   QPushButton*      mOkButton;
   QPushButton*      mCancelButton;
   QPushButton*      mDefaultButton;
};

#endif // PREFERENCES_DIALOG_H
