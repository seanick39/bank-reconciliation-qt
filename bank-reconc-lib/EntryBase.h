#ifndef BRLIB_ENTRYBASE_H
#define BRLIB_ENTRYBASE_H

#include "brlib_common.h"
#include <ctime>
#include <utility>

namespace brlib {

class EntryBase {
public:
  enum EntryFrom { Bank, Books };
  EntryBase(EntryFrom from, const std::tm &tm, const str &_narr, long dr,
            long cr = 0, long bal = 0);
  EntryBase() : EntryBase(EntryFrom::Bank, {}, "", 0) {}

  std::tm date;
  str narr;
  long debit, credit, balance;

  void printDate(ostringstream &) const;
  void printNarr(ostringstream &) const;
  void printDebit(ostringstream &) const;
  void printCredit(ostringstream &) const;
  void printBalance(ostringstream &) const;

  void print(ostringstream &) const;
  [[nodiscard]] EntryFrom entryFrom() const;

  bool operator==(const EntryBase &rhs) const;

  static long toPaise(const str &);
  //  static EntryBase fromString(const str &, const EntryFrom &, const
  //  EntryParseOptions &options);
  static void validate(long debit, long credit);

private:
  enum moneyType { dr = 0, cr = 1, bal = 2 };
  void printMoney(ostringstream &, moneyType) const;

  EntryFrom m_entryFrom;
  static constexpr char books_dt_fmt[] = "%d-%m-%Y\0";
  static constexpr char bank_dt_fmt[] = "%d/%m/%y\0"; // 01/01/21
};
} // namespace brlib

#endif // BRLIB_ENTRYBASE_H
