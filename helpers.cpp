#include <EntryBase.h>

#include "brlib_common.h"
#include "helpers.h"

using namespace brlib;

namespace br_ui {

QHash<int, QByteArray> getCommonRoleNames() {
  QHash<int, QByteArray> roles;
  roles.insert(Qt::DisplayRole, "display");
  roles.insert(Qt::TextAlignmentRole, "txtAlign");
  return roles;
}

QVariant getCommonTextAlignment(const int &col) {
  auto ret = QVariant(Qt::AlignVCenter | Qt::AlignLeft);
  switch (col) {
  case TableCols::Date:
    break;
  case TableCols::Narr:
    break;
  case TableCols::Debit:
    ret = QVariant(Qt::AlignVCenter | Qt::AlignRight);
    break;
  case TableCols::Credit:
    ret = QVariant(Qt::AlignVCenter | Qt::AlignRight);
    break;
  case TableCols::Balance:
    ret = QVariant(Qt::AlignVCenter | Qt::AlignRight);
    break;
  default:
    ret = QVariant(Qt::AlignVCenter | Qt::AlignRight);
    break;
  }
  return ret;
}

QDate dateFromEntry(const brlib::EntryBase &entry) {
  const std::tm &t = entry.date;
  const int &y = t.tm_year + 1900, &m = t.tm_mon + 1, &d = t.tm_mday;
  return {y, m, d};
}

} // namespace br_ui