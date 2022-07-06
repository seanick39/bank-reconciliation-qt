#include "EntryDataModel.h"

namespace br_ui
{

    int EntryDataModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid() || !m_entries || m_entries->empty())
        {
            return 0;
        }
        return static_cast<int>(m_entries->size());
    }

    int EntryDataModel::columnCount(const QModelIndex& parent) const
    {
        (void)parent;
        return 5;
    }

    QVariant EntryDataModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const
    {
        QVariant ret;
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                    case ED_Date:
                        ret = "Date";
                        break;
                    case ED_Narr:
                        ret = "Narration";
                        break;
                    case ED_Debit:
                        ret = "Debit";
                        break;
                    case ED_Credit:
                        ret = "Credit";
                        break;
                    case ED_Balance:
                        ret = "Balance";
                        break;
                    default:
                        break;
                }
            }
            else if (role == Qt::TextAlignmentRole)
            {
                return alignmentData(section);
            }
        }
        return ret;
    }

    QVariant EntryDataModel::data(const QModelIndex& index, int role) const
    {
        QVariant ret;
        if (!index.isValid() || !m_entries || m_entries->empty() || index.row() < 0 ||
            index.row() >= static_cast<int>(m_entries->size()))
        {
            return ret;
        }
        const brlib::EntryBase& entry = m_entries->at(index.row());
        std::ostringstream oss{};
        oss.imbue(std::cout.getloc());

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
                case ED_Date:
                    entry.printDate(oss);
                    break;
                case ED_Narr:
                    entry.printNarr(oss);
                    break;
                case ED_Debit:
                    entry.printDebit(oss);
                    break;
                case ED_Credit:
                    entry.printCredit(oss);
                    break;
                case ED_Balance:
                    entry.printBalance(oss);
                    break;
                default:
                    qDebug() << "entrydatamodel data() default switch case index: " << index;
                    break;
            }
            return QString(oss.str().data());
        }
        else if (role == Qt::TextAlignmentRole)
        {
            int col = index.column();
            return alignmentData(col);
        }
        return ret;
    }

    QVariant EntryDataModel::alignmentData(int column)
    {
        QVariant ret(Qt::AlignVCenter | Qt::AlignLeft);
        if (column == ED_Debit || column == ED_Credit || column == ED_Balance)
        {
            ret = QVariant(Qt::AlignVCenter | Qt::AlignRight);
        }
        return ret;
    }

    bool EntryDataModel::updateVec()
    {
        beginResetModel();
        const int rows = rowCount();
        if (rows)
        {
            beginRemoveRows(QModelIndex(), 0, rows - 1);
            removeRows(0, rows);
            endRemoveRows();
        }
        if (!m_entries)
        {
            /* bail out in case of nullptr */
            endResetModel();
            return false;
        }
        const int entryCount = static_cast<int>(m_entries->size());
        beginInsertRows(QModelIndex(), 0, entryCount - 1);
        insertRows(0, entryCount);
        endInsertRows();
        endResetModel();
        return true;
    }

} // namespace br_ui
