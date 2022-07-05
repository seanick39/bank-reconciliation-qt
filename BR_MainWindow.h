#ifndef BR_MAINWINDOW_H
#define BR_MAINWINDOW_H

#include <QMainWindow>
#include <QObject>

/* this header is available as QObject's meta-object compilation on project
 * build */
#include <ui_mainwindow.h>

#include <EntryMatch.h>
#include <reconcile.h>

#include "AboutDialog.h"
#include "EntryDataModel.h"
#include "EntryMatchModel.h"
#include "FileSettingsDialog.h"
#include "MissingEntryModel.h"
#include "helpers.h"

namespace br_ui {

struct SelectionState {
  enum Init { SelBank, SelBooks };
  enum Side { SelDebit, SelCredit };
  SelectionState(const Init _init, const Side _side)
      : init(_init), side(_side) {}

  Init init;
  Side side;
};

class BR_MainWindow : public QMainWindow, public Ui_MainWindow {

  Q_OBJECT

public:
  explicit BR_MainWindow(QWidget *parent = nullptr);
  void tblRowSelectionChanged(br_ui::SettingFor settingFor,
                              const QItemSelection &deselected);

public slots:
  static void onExit();
  void openAboutDialog();

private:
  QMetaObject::Connection m_dialogConnection;
  QMetaObject::Connection m_bankSelConnection;
  QMetaObject::Connection m_booksSelConnection;

  brlib::ParseSettings m_options;

  QString m_bankFile, m_bookFile;
  brlib::passedAndFailedVecs m_bankVecs, m_bookVecs;
  brlib::results_t m_results;

  /* m_bankTableModel manages tblMissingInBank which displays books entries
   * missing in bank */
  MissingEntryModel m_bankTableModel;
  /* m_bookTableModel manages tblMissingInBooks which displays bank entries
   * missing in books */
  MissingEntryModel m_bookTableModel;

  EntryMatchModel m_matchesTableModel;

  EntryDataModel m_bankDataModel, m_booksDataModel;

  void readBankFile(const QString &fileName);
  void readBookFile(const QString &fileName);
  void setUpTables();
  void connectSignals();

  /* checks whether both files have been picked */
  void updateBtnReconcile();

  /* calls updateVec on table models so they can update their rowCount */
  void updateTablesData(const bool afterSaveMatch = false);

  /* write settings to file to pick on next boot */
  void updateSettingsFile() const;

  /* read key value from settings file for key param */
  static std::pair<str, str> findSetting(std::ifstream &fs, const str &key);
  void setTitle();

  void clearBankData();
  void clearBooksData();
  void showErrorMessage(const QString &title, const QString &message);

  brlib::sp<brlib::EntryMatch> currEntryMatch;

  /* tblMissingInBank's selection model */
  QItemSelectionModel *bankSelModel;

  /* tblMissingInBooks' selection model */
  QItemSelectionModel *booksSelModel;

  SelectionState selState;
  void
  updateSelState(SelectionState::Init init = SelectionState::Init::SelBank,
                 SelectionState::Side side = SelectionState::Side::SelDebit);
  void updateMatchesText();
  void updateBtnSaveMatch();
  void deselectSelection(QItemSelectionModel *sel);
  void deselectSelection(QItemSelectionModel *selModel, const QModelIndex &idx);
  void toggleSelectionConnections(bool mode = true);

  //  void loadFileProperties();

  //  static constexpr char bankFileHeaderAt[] = "bankFileHeaderAt";
  //  static constexpr char booksFileHeaderAt[] = "booksFileHeaderAt";
  //  static constexpr char bankFileNumCols[] = "bankFileNumCols";
  //  static constexpr char booksFileNumCols[] = "booksFileNumCols";

private slots:
  /* common dialog for picking files */
  void openFileDialog(br_ui::SettingFor settingFor);

  void openFileSettingsDialog(const br_ui::SettingFor settingsSFor);
  void btnSaveMatchClicked();

  /* common slot to open picked file; */
  void openFile(const QString &fileName, br_ui::SettingFor settingFor);
  void btnReconcileClicked();
  void btnClearClicked();
  void updateAutoParseSetting(bool state);
  void updateDates(const brlib::entry_vec &entryVec);
};
} // namespace br_ui

#endif // BR_MAINWINDOW_H
