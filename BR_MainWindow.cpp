#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>

#include <brlib_common.h>

#include "BR_MainWindow.h"
#include "parse.h"

namespace br_ui {
BR_MainWindow::BR_MainWindow(QWidget *parent)
    : QMainWindow(parent), m_bankVecs(), m_bookVecs(), m_results(),
      m_bankTableModel(parent, m_bookVecs.passed.get(),
                       &m_results.missingInBank),
      m_bookTableModel(parent, m_bankVecs.passed.get(),
                       &m_results.missingInBook),
      m_matchesTableModel(parent, &m_results.matches, m_bankVecs.passed.get(),
                          m_bookVecs.passed.get()),
      m_bankDataModel(parent, m_bankVecs.passed),
      m_booksDataModel(parent, m_bookVecs.passed), currEntryMatch(nullptr),
      selState(SelectionState::Init::SelBank, SelectionState::Side::SelDebit) {

  setupUi(this);
  setTitle();    // With version.
  setUpTables(); // Connect models.
  connectSignals();
  updateMatchesText(); // initial text for nullptr

  btnRunReconciliation->setEnabled(false); // enabled after picking files
  btnMatchSelected->setEnabled(false);
  btnBankFileSettings->setEnabled(!m_options.isAutoParseEnabled());
  btnBooksFileSettings->setEnabled(!m_options.isAutoParseEnabled());

  dtFrom->setDate(today);
  dtTo->setDate(today);

  /** construct a locale with brlib::indianmoneypunct, and let cout imbue it,
   * so ostrinstream in tablemodels' print members can later imbue from cout. */
  std::cout.imbue(std::locale(std::cout.getloc(), new brlib::indianMoneyPunct));
  //  loadFileProperties();
}

void BR_MainWindow::onExit() { QCoreApplication::quit(); }

void BR_MainWindow::openFileDialog(SettingFor dialogFor) {
  QString caption = "Pick ";
  if (dialogFor == SettingFor::Bank) {
    caption += "bank";
  } else {
    caption += "books";
  }
  caption += " file";
  const QString filter("Delimited Files (*.csv *.txt *.psv)");
  /** removed hard-coded $HOME in here */
  auto *fileDialog = new QFileDialog(this, caption, "", filter);
  fileDialog->setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
  fileDialog->setFileMode(QFileDialog::FileMode::ExistingFile);
  fileDialog->setViewMode(QFileDialog::ViewMode::Detail);
  fileDialog->open();
  m_dialogConnection = connect(
      fileDialog, &QFileDialog::fileSelected, this,
      [this, dialogFor](const QString &file) { openFile(file, dialogFor); });
}

void BR_MainWindow::openFile(const QString &file, SettingFor dialogFor) {
  if (m_dialogConnection) {
    QObject::disconnect(m_dialogConnection);
  }
  if (dialogFor == SettingFor::Bank) {
    m_bankFile = file;
    readBankFile(file);
  } else if (dialogFor == SettingFor::Books) {
    m_bookFile = file;
    readBookFile(file);
  } else {
    qDebug() << "Runtime error: unhandled case in openFile()";
  }
}

void BR_MainWindow::readBankFile(const QString &fileName) {
  std::fstream file(fileName.toStdString().data());
  if (file.is_open()) {
    try {
      brlib::parseEntries(brlib::EntryBase::EntryFrom::Bank, file, m_bankVecs,
                          m_options.isAutoParseEnabled(), m_options.bank);
      if (m_bankVecs.passed->empty()) {
        throw EmptyDataError("no data found in bank file.");
      }
      updateDates(*m_bankVecs.passed);
      lblBankFile->setText(m_bankFile);
      updateBtnReconcile();
    } catch (EmptyDataError &e) {
      qDebug() << "No data in bank file [" + fileName + ']';
      clearBankData();
      showErrorMessage("No data in bank file.", e.what());
      m_bankFile.clear();
    } catch (std::invalid_argument &e) {
      qWarning() << "Invalid argument in bank file [" + fileName + "]\n" +
                        e.what();
      clearBankData();
      showErrorMessage("Error parsing data", e.what());
      m_bankFile.clear();
    }
  } else {
    showErrorMessage("Error opening bank file.", "File could not be opened.");
    qDebug() << fileName + " couldn't be opened";
  }
}

void BR_MainWindow::readBookFile(const QString &fileName) {
  std::fstream file(fileName.toStdString().data());
  if (file.is_open()) {
    try {
      brlib::parseEntries(brlib::EntryBase::EntryFrom::Books, file, m_bookVecs,
                          m_options.isAutoParseEnabled(), m_options.books);
      if (m_bookVecs.passed->empty()) {
        throw EmptyDataError("no data found in books file.");
      }
      updateDates(*m_bookVecs.passed);
      lblBookFile->setText(m_bookFile);
      updateBtnReconcile();
    } catch (EmptyDataError &e) {
      qDebug() << "No data in books file [" + fileName + ']';
      clearBooksData();
      showErrorMessage("Error parsing data", e.what());
      m_bookFile.clear();
    } catch (std::invalid_argument &e) {
      qWarning() << "Invalid argument in books file [" + fileName + "]\n" +
                        e.what();
      clearBooksData();
      showErrorMessage("Error parsing data", e.what());
      m_bookFile.clear();
    }
  } else {
    showErrorMessage("Error opening books file.", "File could not be opened.");
    qWarning() << "Books file: " + fileName + " couldn't be opened.";
  }
}

void BR_MainWindow::updateBtnReconcile() {
  if (!m_bankVecs.passed->empty() && !m_bookVecs.passed->empty()) {
    btnRunReconciliation->setEnabled(true);
  } else {
    btnRunReconciliation->setEnabled(false);
  }
}

void BR_MainWindow::btnReconcileClicked() {
  bool needTableUpdating = false;
  if (!m_results.matches.empty()) {
    m_results.matches.clear();
    needTableUpdating = true;
  }
  if (!m_results.missingInBank.empty()) {
    m_results.missingInBank.clear();
    needTableUpdating = true;
  }
  if (!m_results.missingInBook.empty()) {
    m_results.missingInBook.clear();
    needTableUpdating = true;
  }
  if (needTableUpdating) {
    updateTablesData();
  }
  brlib::pr_vec_t pr = brlib::findLastMatchingBalance(m_bankVecs, m_bookVecs);
  brlib::entry_vec_sz_t bankBeg = 0;
  brlib::entry_vec_sz_t bookBeg = 0;
  if (pr.first && pr.second) {
    bankBeg = pr.first;
    bookBeg = pr.second;
  }
  runReconciliation(m_bankVecs, bankBeg, m_bookVecs, bookBeg, m_results);
  updateTablesData();
  currEntryMatch = std::make_shared<brlib::EntryMatch>(
      std::vector<brlib::EntryPointer>(), m_bankVecs.passed, m_bookVecs.passed,
      true);
  //  updateSettingsFile();
  /* find possible relations is a WIP. Will be implemented later */
  // vec<brlib::PossibleRelation> relns;
  // brlib::findRelatedRecords(m_results, m_bankVecs.passed, m_bookVecs.passed,
  // relns);
}

std::pair<str, str> BR_MainWindow::findSetting(std::ifstream &fs,
                                               const str &key) {
  str line;
  std::pair<str, str> pr(key, "0");
  while (getline(fs, line)) {
    std::istringstream strm(line);
    str s;
    if (getline(strm, s, '=') && s == key) {
      //      std::pair<str, str> pr(s, "");
      s.clear();
      if (getline(strm, s, '=')) {
        pr.second = s;
        return pr;
      }
    }
  }
  return pr;
}

// void BR_MainWindow::loadFileProperties() {
//   /** removed hardcoded settings here. picking of settings file using
//   filedialog
//    * to be implemented */
//   std::ifstream settingsFile{"settings.txt"};
//   if (settingsFile.is_open() && settingsFile.good()) {
//     m_settings.bank.numCols =
//         stoi(findSetting(settingsFile, bankFileHeaderAt).second);
//     txtBankNumCols->setValue(m_settings.bank.numCols);
//     m_settings.bank.headerAt =
//         stoi(findSetting(settingsFile, bank_first_row).second);
//     txtBankFirstRow->setValue(m_settings.bank.headerAt);
//
//     m_settings.books.numCols =
//         stoi(findSetting(settingsFile, booksFileHeaderAt).second);
//     txtBooksNumCols->setValue(m_settings.books.numCols);
//     m_settings.books.headerAt =
//         stoi(findSetting(settingsFile, books_first_row).second);
//     txtBooksFirstRow->setValue(m_settings.books.headerAt);
//     settingsFile.close();
//   } else {
//     qDebug() << "couldn't find settings file.";
//   }
// }
void BR_MainWindow::clearBankData() {
  if (!m_bankVecs.passed->empty())
    m_bankVecs.passed->clear();
  if (!m_bankVecs.failed->empty())
    m_bankVecs.failed->clear();
}

void BR_MainWindow::clearBooksData() {
  if (!m_bookVecs.passed->empty())
    m_bookVecs.passed->clear();
  if (!m_bookVecs.failed->empty())
    m_bookVecs.failed->clear();
}

void BR_MainWindow::btnClearClicked() {
  if (!m_results.matches.empty())
    m_results.matches.clear();
  if (!m_results.missingInBook.empty())
    m_results.missingInBank.clear();
  if (!m_results.missingInBook.empty())
    m_results.missingInBook.clear();

  updateTablesData();
}

void BR_MainWindow::showErrorMessage(const QString &title,
                                     const QString &message) {
  QMessageBox::critical(this, title, message);
}

void BR_MainWindow::setTitle() {
#if defined(PROJECT_VERSION_MAJOR) && defined(PROJECT_VERSION_MINOR)
  const int versionMajor = PROJECT_VERSION_MAJOR;
  const int versionMinor = PROJECT_VERSION_MINOR;
#else
  const int versionMajor = 0;
  const int versionMinor = 1;
#endif
  QString title = "Bank Reconciliation Tool ver: ";
  title.append(QString::number(versionMajor));
  title.append(".");
  title.append(QString::number(versionMinor));
  setWindowTitle(title);
}

void BR_MainWindow::setUpTables() {
  /* assign models to tableviews, and update behaviour */

  /* tblMissingInBank will display book entries missing in bank */
  tblMissingInBank->setModel(
      static_cast<QAbstractTableModel *>(&m_bankTableModel));

  /* tblMissingInBooks will display bank entries missing in books */
  tblMissingInBooks->setModel(
      static_cast<QAbstractTableModel *>(&m_bookTableModel));
  tblMissingInBank->setAlternatingRowColors(true);
  tblMissingInBooks->setAlternatingRowColors(true);
  tblMissingInBank->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);
  tblMissingInBooks->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);
  bankSelModel = tblMissingInBank->selectionModel();
  booksSelModel = tblMissingInBooks->selectionModel();

  tblMatches->setModel(&m_matchesTableModel);
  tblMatches->setAlternatingRowColors(false);
  tblMatches->setSelectionBehavior(
      QAbstractItemView::SelectionBehavior::SelectRows);

  tblBank->setModel(&m_bankDataModel);
  tblBooks->setModel(&m_booksDataModel);
}

void BR_MainWindow::connectSignals() {
  chkAutoParse->setChecked(m_options.isAutoParseEnabled());
  connect(actionExit_2, &QAction::triggered, this, &BR_MainWindow::onExit);
  connect(btnBankFile, &QPushButton::clicked, this,
          [&]() { openFileDialog(SettingFor::Bank); });
  connect(btnBookFile, &QPushButton::clicked, this,
          [&]() { openFileDialog(SettingFor::Books); });
  connect(btnRunReconciliation, &QPushButton::clicked, this,
          &BR_MainWindow::btnReconcileClicked);
  connect(btnClear, &QPushButton::clicked, this,
          &BR_MainWindow::btnClearClicked);

  connect(chkAutoParse, &QCheckBox::stateChanged, this,
          &BR_MainWindow::updateAutoParseSetting);
  connect(btnBankFileSettings, &QPushButton::clicked, this,
          [&]() { openFileSettingsDialog(SettingFor::Bank); });
  connect(btnBooksFileSettings, &QPushButton::clicked, this,
          [&]() { openFileSettingsDialog(SettingFor::Books); });

  connect(btnMatchSelected, &QPushButton::clicked, this,
          &BR_MainWindow::btnSaveMatchClicked);

  connect(actionAbout_2, &QAction::triggered, this,
          &BR_MainWindow::openAboutDialog);
  toggleSelectionConnections();
}

void BR_MainWindow::toggleSelectionConnections(bool mode) {
  if (mode) {
    m_bankSelConnection = connect(
        bankSelModel, &QItemSelectionModel::selectionChanged, this,
        [&](const QItemSelection &selected, const QItemSelection &deselected) {
          tblRowSelectionChanged(br_ui::SettingFor::Bank, deselected);
        });
    m_booksSelConnection = connect(
        booksSelModel, &QItemSelectionModel::selectionChanged, this,
        [&](const QItemSelection &selected, const QItemSelection &deselected) {
          tblRowSelectionChanged(br_ui::SettingFor::Books, deselected);
        });

  } else {
    QObject::disconnect(m_bankSelConnection);
    QObject::disconnect(m_booksSelConnection);
  }
}

void BR_MainWindow::updateTablesData(const bool afterSaveMatch) {
  const QDate &from = dtFrom->date();
  const QDate &to = dtTo->date();
  m_bankTableModel.updateVec(&from, &to);
  m_bookTableModel.updateVec(&from, &to);
  m_matchesTableModel.updateVec(&from, &to);

  /* don't update below tables unnecessarily */
  if (!afterSaveMatch) {
    m_bankDataModel.updateVec();
    m_booksDataModel.updateVec();

    QTableView *tables[] = {tblMissingInBank, tblMissingInBooks, tblMatches,
                            tblBank, tblBooks};
    for (QTableView *&t : tables) {
      if (t->model()->rowCount()) {
        t->resizeColumnsToContents();
      }
    }
  }
}

void BR_MainWindow::updateSettingsFile() const {
  const char origName[] = "settings.txt";
  std::filesystem::remove(origName);
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto t = std::localtime(&now_time_t);
  std::ofstream tmpFile{origName};
  tmpFile << "# autogen on " << std::put_time(t, "%Y-%m-%d %H:%M:%S")
          << std::endl;
  //  tmpFile << bankFileHeaderAt << '=' << m_settings.bank.headerAt <<
  //  std::endl; tmpFile << bankFileNumCols << '=' <<
  //  m_settings.bank.manual.numCols
  //          << std::endl;
  //  tmpFile << booksFileHeaderAt << '=' << m_settings.books.headerAt <<
  //  std::endl; tmpFile << booksFileNumCols << '=' <<
  //  m_settings.books.manual.numCols
  //          << std::endl;
  //  tmpFile << bank_date_format << '=' <<
  //  m_bankEntryParseOptions.bankDateFormat.value << std::endl; tmpFile <<
  //  "books_date_format=" << m_bankEntryParseOptions.booksDateFormat.value <<
  //  std::endl;
  tmpFile.close();
}

void BR_MainWindow::updateAutoParseSetting(bool state) {
  if (state) {
    btnBankFileSettings->setEnabled(false);
    btnBooksFileSettings->setEnabled(false);

    m_options.setAutoParse(true);

  } else {
    btnBankFileSettings->setEnabled(true);
    btnBooksFileSettings->setEnabled(true);

    m_options.setAutoParse(false);
  }
}

void BR_MainWindow::openFileSettingsDialog(SettingFor settingsFor) {
  brlib::ManualParseSettings *options = nullptr;
  if (settingsFor == SettingFor::Bank) {
    options = &m_options.bank;
  } else {
    options = &m_options.books;
  }
  FileSettingsDialog dialog(this, settingsFor, *options);
  if (dialog.exec() == QDialog::Accepted) {
    if (settingsFor == SettingFor::Bank) {
      m_options.bank = dialog.m_settings;
    } else if (settingsFor == SettingFor::Books) {
      m_options.books = dialog.m_settings;
    }
  }
}

void BR_MainWindow::updateDates(const brlib::entry_vec &entryVec) {
  const brlib::EntryBase &last = entryVec.back();
  const brlib::EntryBase &first = entryVec.front();
  const QDate lastDt = dateFromEntry(last);
  const QDate firstDt = dateFromEntry(first);
  if (dtFrom->date() > firstDt) {
    dtFrom->setDate(firstDt);
  }
  if (dtTo->date() < lastDt) {
    dtTo->setDate(lastDt);
  }
};

void BR_MainWindow::openAboutDialog() {
  AboutDialog aboutDialog(this);
  aboutDialog.exec();
}

} // namespace br_ui
