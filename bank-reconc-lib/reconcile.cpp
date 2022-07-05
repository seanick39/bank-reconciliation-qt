#include <algorithm>
#include <fstream>
#include <iomanip>
#include <set>

#include "EntryMatch.h"
#include "reconcile.h"

namespace brlib {

/* Todo: edge cases:
 *  - no entries in file
 */

pr_vec_t findLastMatchingBalance(passedAndFailedVecs &lhs,
                                 passedAndFailedVecs &rhs) {
  pr_vec_t pr = std::make_pair(0, 0);
  if (!lhs.passed->empty() && !rhs.passed->empty()) {
    entry_vec_sz_t bank_r_idx = lhs.passed->size() - 1, bank_r_end = 0,
                   book_r_idx = rhs.passed->size() - 1, book_r_end = 0;
    for (; bank_r_idx != bank_r_end; --bank_r_idx) {
      for (; book_r_idx != book_r_end; --book_r_idx) {
        if (lhs.passed->at(bank_r_idx).balance ==
            rhs.passed->at(book_r_idx).balance) {
          pr.first = bank_r_idx;
          pr.second = book_r_idx;
          return pr;
        }
      }
    }
  }
  return pr;
}

/**
 * - match passed entries in both bank and book,
 * - push matches to results.matches
 * - push bank entries not found in books to results.missingInBooks
 * - push books entries not found in bank to results.missingInBank
 * */
void runReconciliation(passedAndFailedVecs &bank, entry_vec_sz_t bankBegin,
                       passedAndFailedVecs &book, entry_vec_sz_t booksBegin,
                       results_t &results) {

  if (bank.passed && book.passed && !bank.passed->empty()) {
    sp<entry_vec> bankPassedVec(bank.passed);
    sp<entry_vec> booksPassedVec(book.passed);
    std::set<entry_vec_sz_t>
        skipBooksIds{}; // set to skip book_ids that have already been found.

    results.matches.reserve(bank.passed->size() + book.passed->size());

    for (entry_vec_sz_t bankIdx = bankBegin, bank_sz = bank.passed->size();
         bankIdx != bank_sz; ++bankIdx) {
      bool match_found(false);
      for (entry_vec_sz_t bookIdx = booksBegin, book_sz = book.passed->size();
           (bookIdx != book_sz); ++bookIdx) {

        /** skip if book_id is in set */
        if (skipBooksIds.find(bookIdx) != skipBooksIds.end()) {
          continue;
        }

        const EntryBase &bankObj = bank.passed->at(bankIdx);
        const EntryBase &booksObj = book.passed->at(bookIdx);
        if (bankObj == booksObj) {
          /* earlier we were constructing an EntryMatch with an idx of one books
           * entry and an idx of one bank entry. Now, we're constructing an
           * EntryMatch with a vector of indices of bank entries, and set for
           * books entries.
           */
          EntryMatch m({}, bankPassedVec, booksPassedVec);
          m.insertIntoBank(bankIdx, results);
          m.insertIntoBooks(bookIdx, results);
          results.matches.push_back(m);
          skipBooksIds.insert(bookIdx);
          break; // break inner book vector loop
        }
      }
      if (!match_found) {
        results.missingInBook.push_back(bankIdx);
      }
    }
  }
  if (book.passed && bank.passed && !book.passed->empty()) {
    entry_vec_sz_t booksIdx = 0, booksEnd = book.passed->size();
    auto isBooksMatch = [&booksIdx](EntryMatch &m) {
      return m.booksIdxExists(booksIdx);
    };
    for (; booksIdx != booksEnd; ++booksIdx) {
      bool skip_book =
          std::find_if(results.matches.begin(), results.matches.end(),
                       isBooksMatch) != results.matches.end();
      if (!skip_book) {
        results.missingInBank.push_back(booksIdx);
      }
    }
  }
}

void printPossibleRelns(const vec<PossibleRelation> &relns,
                        sp_vec_entry_t &bank, sp_vec_entry_t &books) {
  std::cout << "------- relns -------" << std::endl;
  for (const PossibleRelation &rel : relns) {
    const EntryBase &par = bank->at(rel.parent);
    const long &trxAmt = !par.debit ? par.credit : par.debit;
    std::cout << "\tparent: " << trxAmt << std::endl;
    for (const entry_vec_sz_t &c : rel.children) {
      const EntryBase &ch = books->at(c);
      const long &amt = &trxAmt == &par.debit ? ch.debit : ch.credit;
      std::cout << "\t\tchild: " << amt << std::endl;
    }
  }
  std::cout << "--------------------" << std::endl;
}

void findRelatedRecords(results_t &results, sp_vec_entry_t &bank,
                        sp_vec_entry_t &books, vec<PossibleRelation> &relns) {
  auto isTrxDirSame = [&](const EntryBase &lhs, const EntryBase &rhs) {
    if (!lhs.credit) {
      return rhs.credit == 0;
    } else {
      return rhs.debit == 0;
    }
  };

  auto isInDaysDelta = [&](const std::time_t &future, EntryBase &e) {
    std::tm checkDate = e.date;
    std::time_t checkTm = std::mktime(&checkDate);
    return checkTm <= future;
  };

  const unsigned daysDelta = 5;
  const long diffdelta = 60 * 60 * 24 * daysDelta;
  vec<entry_vec_sz_t> &missingInBook = results.missingInBook;
  vec<entry_vec_sz_t> &missingInBank = results.missingInBank;
  if (!missingInBook.empty() && !bank->empty()) {
    for (entry_vec_sz_t &currIdx : missingInBook) {
      EntryBase &currEntry = bank->at(currIdx);
      std::tm currDate = currEntry.date;
      // check for possible failure of mktime
      std::time_t future = std::mktime(&currDate) + diffdelta;
      long &trxAmt = !currEntry.debit ? currEntry.credit : currEntry.debit;
      std::set<entry_vec_sz_t> perfectMatches;
      PossibleRelation reln{currIdx, {}, false};
      for (entry_vec_sz_t &checkIdx : missingInBank) {
        EntryBase &checkEntry = books->at(checkIdx);
        /* now we're in allowed date range. */
        long &checkAmt =
            &trxAmt == &currEntry.debit ? checkEntry.debit : checkEntry.credit;
        if (!perfectMatches.contains(checkIdx) &&
            isTrxDirSame(currEntry, checkEntry) &&
            isInDaysDelta(future, checkEntry) && checkAmt <= trxAmt) {
          if (checkAmt == trxAmt) {
            reln.isExact = true;
            if (!reln.children.empty()) {
              reln.children.clear();
            }
            reln.children.insert(checkIdx);
            perfectMatches.insert(checkIdx);
            break;

          } else {
            reln.children.insert(checkIdx);
          }
        }
      }
      relns.push_back(reln);
      break;
    }
  }
  printPossibleRelns(relns, bank, books);
}
void sortEntries(entry_vec &entries) {
  std::sort(entries.begin(), entries.end(),
            [&](const EntryBase &lhs, const EntryBase &rhs) {
              const std::tm &lDt = lhs.date;
              const std::tm &rDt = rhs.date;
              return lDt.tm_mday < rDt.tm_mday && lDt.tm_mon < rDt.tm_mon &&
                     lDt.tm_year < rDt.tm_year;
            });
}

/** match isn't const because we're clearing it after pushing a copy to vector.
 */
void saveManualMatch(results_t &results, EntryMatch &match) {
  vec<entry_vec_sz_t> &missingInBank = results.missingInBank;
  vec<entry_vec_sz_t> &missingInBooks = results.missingInBook;
  if (match.isValid()) {
    for (const entry_vec_sz_t &bankIdx : match.banksIndices()) {
      auto it =
          std::find(missingInBooks.begin(), missingInBooks.end(), bankIdx);
      if (it != missingInBooks.end()) {
        missingInBooks.erase(it);
      } else {
        std::cerr
            << "saveManualMatch() missingInBooks bankIdx not found for erase.";
      }
    }
    for (const entry_vec_sz_t &booksIdx : match.booksSet()) {
      auto it = std::find(missingInBank.begin(), missingInBank.end(), booksIdx);
      if (it != missingInBank.end()) {
        missingInBank.erase(it);
      } else {
        std::cerr
            << "saveManualMatch() missingInBank booksIdx not found for erase.";
      }
    }
    results.matches.push_back(match);
  }
  match.clear();
}

} // namespace brlib
