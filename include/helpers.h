#ifndef BR_HELPERS_H
#define BR_HELPERS_H

#include <QAbstractTableModel>
#include <QDate>

#include <brlib_common.h>

namespace br_ui {

using str = brlib::str;

template <typename T> using vec = brlib::vec<T>;

template <typename T> using sp = brlib::sp<T>;

enum TableCols { Date, Narr, Debit, Credit, Balance };

QHash<int, QByteArray> getCommonRoleNames();

QVariant getCommonTextAlignment(const int &col);

class EmptyDataError : public std::invalid_argument {
public:
  explicit EmptyDataError(const str &e) : std::invalid_argument(e){};
};
static const QDate today(QDateTime::currentDateTime().date());

QDate dateFromEntry(const brlib::EntryBase &entry);

} // namespace br_ui

#endif // BR_HELPERS_H
