#ifndef BRLIB_RECONCILE_H
#define BRLIB_RECONCILE_H

#include <set>

#include "EntryBase.h"
#include "brlib_common.h"
#include "parse.h"

namespace brlib {

using namespace parse;

pr_vec_t findLastMatchingBalance(passedAndFailedVecs &lhs,
                                 passedAndFailedVecs &rhs);

void runReconciliation(passedAndFailedVecs &bank, entry_vec_sz_t bankBegin,
                       passedAndFailedVecs &book, entry_vec_sz_t booksBegin,
                       results_t &results);

struct PossibleRelation {
  entry_vec_sz_t parent;
  std::set<entry_vec_sz_t> children;
  bool isExact;
};
void findRelatedRecords(results_t &results, sp_vec_entry_t &bank,
                        sp_vec_entry_t &books, vec<PossibleRelation> &relns);

void printPossibleRelns(const vec<PossibleRelation> &relns,
                        sp_vec_entry_t &bank, sp_vec_entry_t &books);

void sortEntries(entry_vec &entries);

void sortMatches(vec<EntryMatch> &matches);

void saveManualMatch(results_t &results, EntryMatch &match);

} // namespace brlib

#endif // BRLIB_RECONCILE_H
