#include <QPalette>

#include <EntryBase.h>
#include <EntryMatch.h>

#include "EntryMatchModel.h"
#include "helpers.h"

namespace br_ui {
EntryMatchModel::EntryMatchModel(QObject *parent,
                                 vec<brlib::EntryMatch> *matches,
                                 vec<brlib::EntryBase> *bankEntries,
                                 vec<brlib::EntryBase> *booksEntries)
    : QAbstractTableModel(parent), m_matches(matches),
      m_bankEntries(bankEntries), m_bookEntries(booksEntries) {}

int EntryMatchModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !m_matches) {
    return 0;
  }
  return static_cast<int>(m_data.size());
}

int EntryMatchModel::columnCount(const QModelIndex &parent) const {
  (void)parent;
  return 5;
}

QVariant EntryMatchModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {

  QVariant ret;
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
      case EM_From:
        return "From";
      case EM_Date:
        return "Date";
      case EM_Narr:
        return "Narration / Account";
      case EM_Debit:
        return "Debit";
      case EM_Credit:
        return "Credit";
      default:
        return ret;
      }
    } else if (role == Qt::TextAlignmentRole) {
      if (section == EM_Debit || section == EM_Credit) {
        return {Qt::AlignVCenter | Qt::AlignRight};
      } else {
        return {Qt::AlignVCenter | Qt::AlignLeft};
      }
    }
  }
  return ret;
}

brlib::EntryBase *
EntryMatchModel::entry(const brlib::EntryPointer &entryPtr) const {
  brlib::entry_vec_sz_t entryIdx = entryPtr.entryIdx;
  brlib::EntryBase *entry = nullptr;
  if (entryPtr.entryFor == brlib::EntryPointer::For::Bank) {
    entry = &m_bankEntries->at(entryIdx);
  } else {
    entry = &m_bookEntries->at(entryIdx);
  }
  return entry;
}

QVariant EntryMatchModel::data(const QModelIndex &index, int role) const {
  QVariant ret;
  bool isDataUnavailable =
      m_data.empty() || !m_matches || (m_matches->empty()) || !m_bankEntries ||
      (m_bankEntries->empty()) || !m_bookEntries || (m_bookEntries->empty());
  bool isIndexFromDataInvalid =
      (index.row() >= m_data.size() || index.row() < 0);
  if (!index.isValid() || isDataUnavailable || isIndexFromDataInvalid) {
    return ret;
  }

  auto entryFromIdx = [&]() -> brlib::EntryBase * {
    const brlib::EntryPointer *ePtr = m_data.at(index.row());
    if (!ePtr) {
      return nullptr;
    }
    return entry(*ePtr);
  };

  if (role == Qt::DisplayRole) {
    brlib::EntryBase *entry = entryFromIdx();
    if (!entry) {
      return ret;
    }
    std::ostringstream oss{""};
    oss.imbue(std::cout.getloc());
    switch (index.column()) {
    case EM_From:
      if (entry->entryFrom() == brlib::EntryBase::EntryFrom::Bank) {
        oss << "Bank";
      } else {
        oss << "Books";
      }
      break;
    case EM_Date:
      entry->printDate(oss);
      break;
    case EM_Narr:
      entry->printNarr(oss);
      break;
    case EM_Debit:
      entry->printDebit(oss);
      break;
    case EM_Credit:
      entry->printCredit(oss);
      break;
    default:
      qDebug() << "matchedentrymodel data() default switch case index: "
               << index;
      break;
    }
    return QString(oss.str().data());
  } else if (role == Qt::TextAlignmentRole) {
    ret = QVariant(Qt::AlignVCenter | Qt::AlignLeft);
    switch (index.column()) {
    case EM_From:
    case EM_Date:
    case EM_Narr:
      break;
    case EM_Debit:
    case EM_Credit:
      ret = QVariant(Qt::AlignVCenter | Qt::AlignRight);
      break;
    }
    return ret;
  } else if (role == Qt::BackgroundRole) { /* bg color */
    const brlib::EntryBase *entry = entryFromIdx();
    ret = QBrush(QColorConstants::DarkGray);
    if (!entry) {
      return ret;
    }
    QBrush bankBg, booksBg;
#if defined(WIN32) /* lighter tones for Windows */
    bankBg = QBrush(QColorConstants::Svg::lightblue);
    booksBg = QBrush(QColorConstants::Svg::lightpink);
#else
    bankBg = QBrush(QColorConstants::Svg::slategray);
    booksBg = QBrush(QColorConstants::Svg::gray);
#endif
    switch (entry->entryFrom()) {
    case brlib::EntryBase::EntryFrom::Bank:
      ret = bankBg;
      break;
    case brlib::EntryBase::EntryFrom::Books:
      ret = booksBg;
      break;
    default:
      break;
    }
    return ret;
  } else if (role == Qt::ForegroundRole) {
    const brlib::EntryBase *entry = entryFromIdx();
    if (!entry) {
      return ret;
    } else {
      return QBrush(QColorConstants::Black);
    }
  }
  return ret;
}

void EntryMatchModel::clearManualMatches() {
  auto endIt = m_matches->end();
  for (auto it = m_matches->begin(); it != endIt; ++it) {
    if (it->isManual()) {
      m_matches->erase(it);
    }
  }
}

bool EntryMatchModel::updateVec(const QDate *from, const QDate *to) {
  beginResetModel();
  const int rows = rowCount();
  if (rows) {
    beginRemoveRows(QModelIndex(), 0, rows - 1);
    removeRows(0, rows);
    endRemoveRows();
    m_data.clear();
  }
  if (!m_matches) {
    endResetModel();
    return false;
  }
  const int matchesCount = static_cast<int>(m_matches->size());
  if (matchesCount) {
    auto it = m_matches->cbegin();
    while (it != m_matches->cend()) {
      for (const brlib::EntryPointer &e : it->data()) {
        if (from && to) {
          const brlib::EntryBase *entr = entry(e);
          if (entr) {
            const QDate dt(dateFromEntry(*entr));
            if (*from <= dt && dt <= *to) {
              m_data.push_back(&e);
            }
          }
        } else {
          m_data.push_back(&e);
        }
      }
      ++it;
      m_data.push_back(nullptr); // for separator blank row
    }

    /* resize the vector, as it will have unnecessary nullptr in the end.
     * In case a date range is provided, it will have nullptrs to match
     * m_matches.size() */
    auto rIt = m_data.end();

    // end iterator pre incremented, dereferenced, and nullptr-checked
    while (!*--rIt && rIt != m_data.begin()) {
      m_data.erase(rIt);
    }

    const int dataSize = static_cast<int>(m_data.size());
    beginInsertRows(QModelIndex(), 0, dataSize - 1);
    insertRows(0, dataSize);
    endInsertRows();
    endResetModel();
    return true;
  }
  endResetModel();
  return true;
}

} // namespace br_ui
