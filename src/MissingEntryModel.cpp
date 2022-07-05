#include "MissingEntryModel.h"
#include "helpers.h"

namespace br_ui {

MissingEntryModel::MissingEntryModel(QObject *parent,
                                     vec<brlib::EntryBase> *entries,
                                     missing_t *missing)
    : QAbstractTableModel(parent), m_entries(entries),
      m_missingIndices(missing) {}

int MissingEntryModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !m_missingIndices || m_missingIndices->empty() ||
      m_data.empty())
    return 0;
  return static_cast<int>(m_data.size());
}

int MissingEntryModel::columnCount(const QModelIndex &parent) const {
  (void)parent;
  return 4;
}

QVariant MissingEntryModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  QVariant ret;
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
      case Date:
        ret = "Date";
        break;
      case Narr:
        ret = "Narr";
        break;
      case Debit:
        ret = "Debit";
        break;
      case Credit:
        ret = "Credit";
        break;
      default:
        ret = "";
        break;
      }
    } else if (role == Qt::TextAlignmentRole) {
      ret = getCommonTextAlignment(section);
    }
  }
  return ret;
}

QVariant MissingEntryModel::data(const QModelIndex &index, int role) const {

  auto ret = QVariant();
  bool isDataUnavailable = !m_entries || (m_entries->empty()) ||
                           !m_missingIndices || m_missingIndices->empty() ||
                           m_data.empty();
  bool isIndexFromDataInvalid =
      (index.row() >= m_data.size()) || (index.row() < 0);
  if (!index.isValid() || isDataUnavailable || isIndexFromDataInvalid) {
    return ret;
  }
  try {

    auto dataIndex = m_data.at(index.row());
    const auto &entry =
        static_cast<brlib::EntryBase &>(m_entries->at(dataIndex));
    std::ostringstream oss{""};

    /* at this point, std::cout has a locale with custom moneypunct
     * (indian), so oss can imbue that locale  */
    oss.imbue(std::cout.getloc());

    if (role == Qt::DisplayRole) {
      switch (index.column()) {
      case Date:
        entry.printDate(oss);
        break;
      case Narr:
        entry.printNarr(oss);
        break;
      case Credit:
        entry.printCredit(oss);
        break;
      case Debit:
        entry.printDebit(oss);
        break;
      default:
        qDebug() << "missingentrymodel data() default switch case index: "
                 << index;
        break;
      }
      return QString(oss.str().data());
    } else if (role == Qt::TextAlignmentRole) {
      ret = getCommonTextAlignment(index.column());
    }
  } catch (std::out_of_range &e) {
    std::cerr << "bankentrytablemodel out_of_range index: \t" << index.row()
              << std::endl;
    return ret;
  }
  return ret;
}

bool MissingEntryModel::updateVec(const QDate *from, const QDate *to) {
  beginResetModel();
  const int rows = rowCount();
  if (rows) {
    beginRemoveRows(QModelIndex(), 0, rows - 1);
    removeRows(0, rows);
    endRemoveRows();
  }
  if (!m_entries || !m_missingIndices) {
    /* bail out in case of nullptrs */
    endResetModel();
    qDebug() << "missingentrymodel updateVec() nullptrs received.";
    return false;
  }
  m_data.clear();
  for (const auto &i : *m_missingIndices) {
    if (from && to) {
      const brlib::EntryBase &e = m_entries->at(i);
      const QDate dt(dateFromEntry(e));
      if (*from <= dt && dt <= *to) {
        m_data.push_back(i);
      }
    } else {
      m_data.push_back(i);
    }
  }

  const int dataSize = static_cast<int>(m_data.size());
  beginInsertRows(QModelIndex(), 0, dataSize - 1);
  insertRows(0, dataSize);
  endInsertRows();
  endResetModel();
  return true;
}

brlib::entry_vec_sz_t
MissingEntryModel::getIndex(const QModelIndex &idx) const {
  return m_data.at(idx.row());
}
} // namespace br_ui
