#ifndef BR_ENTRYTABLEMODEL_H
#define BR_ENTRYTABLEMODEL_H

#include <iomanip>

#include <EntryBase.h>
#include <QAbstractTableModel>

#include "brlib_common.h"
#include "helpers.h"

namespace br_ui
{

    class MissingEntryModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        enum EntryFrom
        {
            Bank,
            Books
        };
        using missing_t = vec<vec<brlib::EntryBase>::size_type>;

        explicit MissingEntryModel(QObject* parent, vec<brlib::EntryBase>* entries,
                                   missing_t* missing);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        [[nodiscard]] int columnCount(const QModelIndex& parent) const override;

        [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                          int role) const override;

        [[nodiscard]] QVariant data(const QModelIndex& index,
                                    int role) const override;

        bool updateVec(const QDate* from = nullptr, const QDate* to = nullptr);
        brlib::entry_vec_sz_t getIndex(const QModelIndex& idx) const;

    private:
        vec<brlib::EntryBase>* m_entries;
        missing_t* m_missingIndices;
        missing_t m_data;
    };

} // namespace br_ui
#endif // BR_ENTRYTABLEMODEL_H
