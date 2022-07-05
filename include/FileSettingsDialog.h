#ifndef BR_FILESETTINGSDIALOG_H
#define BR_FILESETTINGSDIALOG_H

#include <QDialog>
#include <brlib_common.h>
#include <ui_filesettings.h>

#include "helpers.h"

namespace br_ui {

enum SettingFor { Bank, Books };
using Cols = brlib::ManualParseSettings::Cols;

class FileSettingsDialog : public QDialog, public Ui_fileSettingsDialog {
  Q_OBJECT
  friend class BR_MainWindow;

public:
  explicit FileSettingsDialog(QWidget *parent, SettingFor settingsFor,
                              brlib::ManualParseSettings settings);

private:
  SettingFor m_settingsFor;
  brlib::ManualParseSettings m_settings;

  void populateDateFormats();
  void populateDelims();
  void setupColIndices();
  void populateColIndices();
  void populateColIndicesDefaults();
  void connectSignals();

  QComboBox *colCombos[5];

private slots:
  void dateFormatChanged(int idx);
  void delimChanged(int idx);
  void colIndexChanged(int idx, brlib::ManualParseSettings::Cols col);
  void singleAmountColChanged(bool state);
};

} // namespace br_ui

#endif // BR_FILESETTINGSDIALOG_H
