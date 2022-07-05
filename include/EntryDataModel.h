#ifndef BR_ENTRYDATAMODEL_H
#define BR_ENTRYDATAMODEL_H

#include <QAbstractTableModel>
#include <utility>

#include <EntryBase.h>

#include "helpers.h"

namespace br_ui {

class EntryDataModel : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit EntryDataModel(QObject *parent = nullptr,
                          sp<vec<brlib::EntryBase>> entries = nullptr)
      : QAbstractTableModel(parent), m_entries(std::move(entries)) {}
  [[nodiscard]] int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  bool updateVec();

private:
  enum Cols { ED_Date, ED_Narr, ED_Debit, ED_Credit, ED_Balance };
  sp<vec<brlib::EntryBase>> m_entries;
  static QVariant alignmentData(int column);
};

} // namespace br_ui

#endif // BR_ENTRYDATAMODEL_H
