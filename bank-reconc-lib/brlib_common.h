#ifndef BRLIB_COMMON_H
#define BRLIB_COMMON_H

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace brlib {

using str = std::string;
using std::istringstream;
using std::ostringstream;

template <typename T> using vec = std::vector<T>;

template <typename T> using sp = std::shared_ptr<T>;

/* fwd decl */
class EntryBase;
class EntryMatch;

using entry_vec = vec<EntryBase>;
using entry_vec_sz_t = vec<EntryBase>::size_type;

struct results_t {
  vec<EntryMatch> matches;
  vec<entry_vec_sz_t> missingInBook;
  vec<entry_vec_sz_t> missingInBank;
};

using pr_vec_t = std::pair<entry_vec_sz_t, entry_vec_sz_t>;

bool dt_equal(const std::tm &lhs, const std::tm &rhs);

struct DateFormat {
  str value;
  str label;
};

/* things are messy with parsing dates. dd-mm-yy is parsed to 20th century. */
static const DateFormat dateFormats[] = {
    {"%d-%m-%y", "dd-mm-yy"},
    {"%d/%m/%y", "dd/mm/yy"},
    {"%d-%m-%Y", "dd-mm-yyyy"},
    {"%d/%m/%Y", "dd/mm/yyyy"},
};

struct Delim {
  char value;
  str label;
};

static const Delim delims[] = {
    {'\t', "tab ( ->| )"},
    {'|', "pipe ( | )"},
    {',', "comma ( , )"},
};

/* positions of fields in a row defined by number of delims present before each
 * field in header row. */
struct Pos {
  long date{-1}, debit{-1}, credit{-1}, balance{-1}, narr{-1}, amount{-1},
      transType{-1};
};

struct SettingsBase {
  int headerAt{-1};
  DateFormat dateFormat{dateFormats[0]};
  char delimChar{','};
  bool singleAmountCol{false};
  virtual ~SettingsBase() = default;
};

struct AutoParseSettings : SettingsBase {
  AutoParseSettings() : SettingsBase() {}

  Pos delimsBefore;
  unsigned headerDelimsCount{0};
};

struct ManualParseSettings : SettingsBase {
  ManualParseSettings() : SettingsBase() {}

  enum Cols { Date = 0, Narr, Debit, Credit, Balance, Amount, TransactionType };

  bool autoParse{false};
  unsigned short numCols{5}, firstRowAt{1};

  using col_pr_t = std::pair<Cols, int>;
  using col_map_t = std::map<col_pr_t::first_type, col_pr_t::second_type>;
  col_map_t colIndices{
      {Date, 0},    {Narr, 1},    {Debit, 2},           {Credit, 3},
      {Balance, 4}, {Amount, -1}, {TransactionType, -1}};
};

class ParseSettings {
public:
  ManualParseSettings bank, books;

  void setAutoParse(bool value);
  [[nodiscard]] bool isAutoParseEnabled() const;

private:
  bool m_autoParse{true};
};

struct indianMoneyPunct : std::moneypunct<char> {
  pattern do_pos_format() const override {
    return {{value, none, none, none}};
  };
  char_type do_decimal_point() const override { return '.'; }
  pattern do_neg_format() const override { return {{sign, value, none, none}}; }
  int do_frac_digits() const override { return 2; }
  char_type do_thousands_sep() const override { return ','; }
  string_type do_grouping() const override { return "\003\002"; }
};

} // namespace brlib

#endif // BRLIB_COMMON_H
