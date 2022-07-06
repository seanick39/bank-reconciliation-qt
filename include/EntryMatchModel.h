#ifndef BR_MATCHEDENTRYMODEL_H
#define BR_MATCHEDENTRYMODEL_H

#include <QAbstractItemModel>

#include <EntryMatch.h>

#include "helpers.h"

namespace br_ui
{

    class EntryMatchModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum MatchCols
        {
            EM_From,
            EM_Date,
            EM_Narr,
            EM_Debit,
            EM_Credit
        };
        explicit EntryMatchModel(QObject* parent, vec<brlib::EntryMatch>* matches,
                                 vec<brlib::EntryBase>* bankEntries,
                                 vec<brlib::EntryBase>* booksEntries);
        [[nodiscard]] int
          rowCount(const QModelIndex& parent = QModelIndex()) const override;
        [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
        [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                          int role) const override;
        [[nodiscard]] QVariant data(const QModelIndex& index,
                                    int role) const override;

        bool updateVec(const QDate* from = nullptr, const QDate* to = nullptr);
        void clearManualMatches();

    private:
        vec<brlib::EntryMatch>* m_matches;
        vec<brlib::EntryBase>*m_bankEntries, *m_bookEntries;

        brlib::EntryBase* entry(const brlib::EntryPointer& entryPtr) const;

        vec<const brlib::EntryPointer*> m_data;
    };

} // namespace br_ui

#endif // BR_MATCHEDENTRYMODEL_H
