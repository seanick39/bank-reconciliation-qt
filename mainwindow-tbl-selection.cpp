#include <QMessageBox>
#include <qabstractitemmodel.h>
#include <qitemselectionmodel.h>

#include "BR_MainWindow.h"

namespace br_ui {

void BR_MainWindow::updateMatchesText() {
  auto numSelectionText = [&](const unsigned long &sel) {
    return QString("%1 selected").arg(sel);
  };
  if (!currEntryMatch) {
    txtMatchesBankSum->setText("-");
    txtMatchesBooksSum->setText("-");
    txtBankSelectedNumRows->setText(numSelectionText(0));
    txtBooksSelectedNumRows->setText(numSelectionText(0));
    return;
  }
  std::ostringstream oss{};
  brlib::EntryMatch::printMoney(currEntryMatch->banksSum(), oss);

  txtMatchesBankSum->setText(oss.str().data());
  oss.clear();
  oss.str("");
  brlib::EntryMatch::printMoney(currEntryMatch->booksSum(), oss);
  txtMatchesBooksSum->setText(oss.str().data());

  const auto bankSel = currEntryMatch->banksSize();
  const auto booksSel = currEntryMatch->booksSize();
  txtBankSelectedNumRows->setText(numSelectionText(bankSel));
  txtBooksSelectedNumRows->setText(numSelectionText(booksSel));
  updateBtnSaveMatch();
}

void BR_MainWindow::deselectSelection(QItemSelectionModel *selModel,
                                      const QModelIndex &row) {
  if (selModel) {
    QItemSelection selection(
        row, selModel->model()->index(
                 row.row(), selModel->model()->columnCount(QModelIndex()) - 1));
    selModel->select(selection, QItemSelectionModel::SelectionFlag::Deselect);
  }
}

void BR_MainWindow::deselectSelection(QItemSelectionModel *selModel) {
  if (selModel) {
    for (auto &i : selModel->selectedRows()) {
      deselectSelection(selModel, i);
    }
  }
}

void BR_MainWindow::updateSelState(SelectionState::Init init,
                                   SelectionState::Side side) {
  selState.init = init;
  selState.side = side;
}

void BR_MainWindow::tblRowSelectionChanged(br_ui::SettingFor settingFor,
                                           const QItemSelection &deselected) {

  /** erase indices that have been deselected */

  /* set to hold removable indices */
  std::set<brlib::entry_vec_sz_t> removable;
  if (settingFor == SettingFor::Bank) {
    /* insert indices into set */
    for (const QModelIndex &i : deselected.indexes()) {
      removable.insert(m_bankTableModel.getIndex(i));
    }
    for (const brlib::entry_vec_sz_t &idx : removable) {
      currEntryMatch->eraseBooksIdx(idx);
    }
  } else {
    for (const QModelIndex &idx : deselected.indexes()) {
      removable.insert(m_bookTableModel.getIndex(idx));
    }
    for (const brlib::entry_vec_sz_t idx : removable) {
      currEntryMatch->eraseBankIdx(idx);
    }
  }

  auto selRows = [&](const QList<QModelIndex> &indexes) {
    QList<int> _rows;
    for (const QModelIndex &r : indexes) {
      _rows.push_back(r.row());
    }
    return _rows;
  };

  auto insertIntoMatch = [&](const brlib::entry_vec_sz_t &idx,
                             const SettingFor &sFor) {
    if (sFor == SettingFor::Bank) {
      return currEntryMatch->insertIntoBank(idx, m_results);
    }
    return currEntryMatch->insertIntoBooks(idx, m_results);
  };

  auto getEntryFromRow = [&](const QModelIndex &idx, const SettingFor &sFor) {
    if (sFor == br_ui::SettingFor::Bank) {
      const brlib::entry_vec_sz_t entryIdx = m_bankTableModel.getIndex(idx);
      return m_bookVecs.passed->at(entryIdx);
    } else {
      const brlib::entry_vec_sz_t entryIdx = m_bookTableModel.getIndex(idx);
      return m_bankVecs.passed->at(entryIdx);
    }
  };

  auto entrySide = [&](const brlib::EntryBase &entry) {
    return !entry.debit ? SelectionState::Side::SelCredit
                        : SelectionState::Side::SelDebit;
  };

  /* selected rows of books entries [tblMisingInBank] */
  QModelIndexList bankRows = bankSelModel->selectedRows();

  /* selected rows of bank entries [tblMisingInBooks] */
  QModelIndexList booksRows = booksSelModel->selectedRows();

  /* loop over selected rows in tblMissingInBank */
  auto checkBankRows = [&]() {
    for (const QModelIndex &r : bankRows) {
      const brlib::EntryBase entry = getEntryFromRow(r, SettingFor::Bank);

      /* deselect the entry if it's not consistent with selection state, i.e.
       * inward vs outward */
      if (entrySide(entry) != selState.side) {
        deselectSelection(bankSelModel, r);
        return;
      }

      /* insert g */
      const brlib::entry_vec_sz_t idx = m_bankTableModel.getIndex(r);
      if (!currEntryMatch->containsBooksIdx(idx)) {
        if (!insertIntoMatch(idx, SettingFor::Books)) {
          deselectSelection(bankSelModel, r);
          QMessageBox::warning(this, "insertion error.",
                               "duplicate books entry.");
          return;
        };
      }
    }
  };

  auto checkBooksRows = [&]() {
    for (const QModelIndex &r : booksRows) {
      const brlib::EntryBase entry = getEntryFromRow(r, SettingFor::Books);
      if (entrySide(entry) != selState.side) {
        deselectSelection(booksSelModel, r);
        return;
      }
      const brlib::entry_vec_sz_t idx = m_bookTableModel.getIndex(r);
      if (!currEntryMatch->containsBankIdx(idx)) {
        if (!insertIntoMatch(idx, SettingFor::Bank)) {
          deselectSelection(booksSelModel, r);
          QMessageBox::warning(this, "insertion error.",
                               "duplicate bank entry.");
          return;
        };
      }
    }
  };

  if (!bankRows.empty()) {
    if (!booksRows.empty()) {
      /** both have selection */
      checkBankRows();
      checkBooksRows();

    } else {
      /** only bankRows has selection.  */
      if (bankRows.size() == 1) {
        /** init selection state from bankRows[0] */

        const brlib::EntryBase entry =
            getEntryFromRow(bankRows[0], SettingFor::Bank);
        const long &sum = entry.debit ? entry.debit : entry.credit;
        const SelectionState::Side side = entry.debit
                                              ? SelectionState::Side::SelDebit
                                              : SelectionState::Side::SelCredit;
        updateSelState(SelectionState::Init::SelBank, side);
      }
      checkBankRows();
    }
  } else {
    if (!booksRows.empty()) {
      /** only booksRows has selection.  */

      if (booksRows.size() == 1) {
        /** init selection state from booksRows[0] */

        const brlib::EntryBase entry =
            getEntryFromRow(booksRows[0], SettingFor::Books);
        const SelectionState::Side side = entry.debit
                                              ? SelectionState::Side::SelDebit
                                              : SelectionState::Side::SelCredit;
        updateSelState(SelectionState::Init::SelBooks, side);
      }
      checkBooksRows();
    }
  }
  currEntryMatch->printData();
  updateMatchesText();
}

void BR_MainWindow::updateBtnSaveMatch() {
  if (currEntryMatch->isValid())
    btnMatchSelected->setEnabled(true);
  else
    btnMatchSelected->setEnabled(false);
}

void BR_MainWindow::btnSaveMatchClicked() {
  /** disconnect selectionChanged signals from tablemodels while we save an
   * entry match, and deselect selection */
  toggleSelectionConnections(false);

  deselectSelection(bankSelModel);
  deselectSelection(booksSelModel);

  brlib::saveManualMatch(m_results, *currEntryMatch);
  updateMatchesText();
  toggleSelectionConnections();
  /* currEntryMatch.clearAll() called by saveManualMatch() */
  updateTablesData(true);
  btnMatchSelected->setEnabled(false);
}

} // namespace br_ui
