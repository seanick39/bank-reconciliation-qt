#ifndef BRLIB_ENTRYMATCH_H
#define BRLIB_ENTRYMATCH_H

#include <set>
#include <utility>

#include "brlib_common.h"

namespace brlib {

struct EntryPointer {
  enum For { Bank, Books };
  explicit EntryPointer(entry_vec_sz_t _entryIdx, For matchFor)
      : entryIdx(_entryIdx), entryFor(matchFor) {}

  entry_vec_sz_t entryIdx;
  For entryFor;
};

class EntryMatch {
public:
  class EmptyVecError : std::runtime_error {
    static constexpr char bankEmpty[] = "bank";
    static constexpr char booksEmpty[] = "bank";

  public:
    enum Which { Bank, Books };
    explicit EmptyVecError(Which which)
        : std::runtime_error(which == Which::Bank ? bankEmpty : booksEmpty) {}
  };

  using entry_set = std::set<entry_vec_sz_t>;
  explicit EntryMatch(vec<EntryPointer> _data, sp<entry_vec> passedBankVec,
                      sp<entry_vec> passedBooksVec, bool isManual = false);

  [[nodiscard]] bool isManual() const;
  [[nodiscard]] bool bankIdxExists(entry_vec_sz_t entry_idx) const;
  [[nodiscard]] bool booksIdxExists(entry_vec_sz_t entry_idx) const;
  [[nodiscard]] unsigned long banksSize() const;
  [[nodiscard]] unsigned long booksSize() const;
  [[nodiscard]] unsigned long setsTotalSize() const;
  [[nodiscard]] long debitSum() const;
  [[nodiscard]] long creditSum() const;
  [[nodiscard]] long amountSum(bool forDebit = true) const;
  [[nodiscard]] long banksSum() const;
  [[nodiscard]] long booksSum() const;
  [[nodiscard]] bool containsBankIdx(entry_vec_sz_t i) const;
  [[nodiscard]] bool containsBooksIdx(entry_vec_sz_t i) const;
  bool insertIntoBank(entry_vec_sz_t i, const results_t &results);
  bool insertIntoBooks(entry_vec_sz_t i, const results_t &results);
  [[nodiscard]] entry_set banksIndices() const;
  [[nodiscard]] entry_set booksSet() const;

  void eraseBankIdx(entry_vec_sz_t bankIdx);
  void eraseBooksIdx(entry_vec_sz_t booksIdx);

  inline void clear() {
    m_data.clear();
    m_isValid = false;
  };

  void printData() const;

  [[nodiscard]] bool isValid();
  static void printMoney(unsigned long money, std::ostringstream &oss);

  [[nodiscard]] const vec<EntryPointer> &data() const;

private:
  bool insertionCheck(entry_vec_sz_t i, const vec<entry_vec_sz_t> &lookupVec,
                      sp<entry_vec> passedEntries);

  [[nodiscard]] const EntryBase &entryForBankIdx(entry_vec_sz_t bankIdx) const;

  [[nodiscard]] const EntryBase &
  entryForBooksIdx(entry_vec_sz_t booksIdx) const;

  [[nodiscard]] vec<EntryPointer>::const_iterator
  findIdx(EntryPointer::For entryFor, entry_vec_sz_t) const;

  void checkBankVecSize() const;
  void checkBooksVecSize() const;

  sp<entry_vec> m_bankPassedVec;
  sp<entry_vec> m_booksPassedVec;
  bool m_isValid;
  bool m_isManual;

  vec<EntryPointer> m_data;
};
} // namespace brlib

#endif // BRLIB_ENTRYMATCH_H
