#include <iomanip>
#include <sstream>
#include <boost/format.hpp>

#include "EntryBase.h"
#include "parse.h"

namespace brlib {

EntryBase::EntryBase(EntryFrom from, const std::tm &tm, const str &_narr,
                     long dr, long cr, long bal)
    : m_entryFrom(from), date(tm), narr(std::move(_narr)), debit(dr),
      credit(cr), balance(bal) {
  if (from == EntryFrom::Bank) {
    std::swap(debit, credit);
  }
}

void EntryBase::printDate(ostringstream &os) const {
  os << std::put_time(&date, "%d-%m-%Y\0");
}

void EntryBase::printNarr(ostringstream &os) const { os << narr; }

void EntryBase::printMoney(ostringstream &os, moneyType mt) const {
  const long m = mt == moneyType::bal  ? balance
                 : mt == moneyType::dr ? debit
                                       : credit;
  os << std::setw(12) << std::right << std::put_money(m);
}

void EntryBase::printDebit(ostringstream &os) const {
  printMoney(os, moneyType::dr);
}

void EntryBase::printCredit(ostringstream &os) const {
  printMoney(os, moneyType::cr);
}

void EntryBase::printBalance(ostringstream &os) const {
  printMoney(os, moneyType::bal);
}

long EntryBase::toPaise(const str &s) {
  std::istringstream ss(s);
  double d;
  ss >> d;
  return long(d * 100);
}

void EntryBase::print(ostringstream &os) const {

  if (m_entryFrom == EntryFrom::Bank)
    os << "Bank: ";
  else
    os << "Books: ";
  printDate(os);
  printNarr(os);
  printDebit(os);
  printCredit(os);
  printBalance(os);
}

void EntryBase::validate(long debit, long credit) {
  if (debit > 0 && credit > 0) {
    throw parse::DebitCreditError(parse::DebitCreditError::errorType::gt_zero);
  }
  if (debit == 0 && credit == 0) {
    throw parse::DebitCreditError(parse::DebitCreditError::errorType::eq_zero);
  }
}

EntryBase::EntryFrom EntryBase::entryFrom() const { return m_entryFrom; }

bool EntryBase::operator==(const EntryBase &rhs) const {
  bool dateEqual = dt_equal(date, rhs.date);
  return dateEqual && credit == rhs.credit && debit == rhs.debit;
}

} // namespace brlib
