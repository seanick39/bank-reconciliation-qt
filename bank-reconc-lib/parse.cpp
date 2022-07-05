#include <algorithm>
#include <map>
#include <sstream>

#include "EntryBase.h"
#include "parse.h"

namespace brlib::parse {

long checkZero(str &s) { return s.empty() ? 0 : toPaise(s); }

str::size_type wasFound(str::size_type pos) { return pos != str::npos; }

unsigned getDelimsBefore(str &s, str::size_type pos, const char delimChar) {
  unsigned cnt = 0;
  while (pos != str::npos && pos != 0) {
    if (s.at(pos) == '"') {
      pos = s.rfind('"', pos - 1);
    } else {
      if (isprint(s.at(pos))) {
        pos = s.rfind(delimChar, pos);
        if (pos == str::npos) {
          break;
        } else {
          ++cnt;
        }
      }
    }
    --pos;
  }
  return cnt;
}

str::size_type getDelimPos(str &s, unsigned cnt, const char &delimChar) {
  str::size_type pos = 0;
  unsigned _cnt = 0;
  while (pos < s.size() && cnt > 0) {
    if (s.at(pos) == '"') {
      pos = s.find('"', pos + 1);
    }
    if (s.at(pos) == delimChar) {
      ++_cnt;
      if (_cnt == cnt) {
        return pos;
      }
    }
    pos++;
  }
  return pos;
}

/** given the filestream, find the header row, look for special delim chars,
 * get the number of delim chars before each column name, save it to options,
 * so we know how many chars to skip while parsing value for each column name
 * while parsing each data row. look at 10 rows only, then seek the stream to 0
 * and leave it there. */
void configureAutoParse(std::fstream &filestrm, AutoParseSettings &options) {

  int cnt = 0;
  options.headerAt = -1;
  bool specialDelim = false;
  bool dateFormatRetrieved = false;

  /* grab frequently used refs */
  char &delim = options.delimChar;
  auto &delimsBefore = options.delimsBefore;

  using str_sz = str::size_type;

  str line;
  while (getline(filestrm, line) && cnt < 51 && !dateFormatRetrieved) {

    /** header logic here:
     * - find header row, so we can skip all rows till that position for data.
     * - in the header row, find the required hardcoded fields:
     *    - Date
     *    - Amount
     *    - either of:
     *      - Debit & Credit
     *      - Amount & Transaction Type [with a DR & CR flag]
     *    - look for description of trx in this order:
     *      - Narration
     *      - Account
     *      - Particulars
     *      - Description
     *  - once these positions are found in header row, get the count of delims
     *    before each word's first char, and store it in options. to get value
     *    for each col in a row, we'll skip past the number of delims stored for
     *    that col, and stream the first delimited string into value.
     * */
    if (options.headerAt == -1) {
      str_sz datePos = line.find("Date"), amountPos = line.find("Amount"),
             debitPos = line.find("Debit"), creditPos = line.find("Credit"),
             balancePos = line.find("Balance");
      if (!wasFound(debitPos)) {
        debitPos = line.find("Withdraw");
      }
      if (!wasFound(creditPos)) {
        creditPos = line.find("Deposit");
      }
      if (wasFound(datePos) && (wasFound(amountPos) ||
                                (wasFound(creditPos) && wasFound(debitPos)))) {
        /** figure out which delim char is being used. we're covering only:
         * - `|` : pipe
         * - `\t`: tab char
         * - `,` : comma (fallback)
         *  in that order
         */
        if (!specialDelim && line.find('|') != str::npos) {
          delim = '|';
          specialDelim = true;
        }
        if (!specialDelim && line.find('\t') != str::npos) {
          delim = '\t';
          specialDelim = true;
        }
        options.headerDelimsCount =
            getDelimsBefore(line, line.size() - 1, delim);
        options.headerAt = cnt;
        delimsBefore.date = getDelimsBefore(line, datePos, delim);
        if (wasFound(debitPos) && wasFound(creditPos)) {
          delimsBefore.debit = getDelimsBefore(line, debitPos, delim);
          delimsBefore.credit = getDelimsBefore(line, creditPos, delim);
        } else {
          options.singleAmountCol = true;
          delimsBefore.amount = getDelimsBefore(line, amountPos, delim);

          static const str trxTypeSearchWords[] = {"Cr/Dr", "Dr/Cr",
                                                   "Transaction Type"};
          unsigned c = 0;
          auto trxTypePos = line.find(trxTypeSearchWords[c]);
          while (!wasFound(trxTypePos) && (c++ < std::size(trxTypeSearchWords))) {
            trxTypePos = line.find(trxTypeSearchWords[c]);
          }
          if (!wasFound(trxTypePos)) {
            throw InvalidHeaderError("couldn't find transaction type for "
                                     "single col amount format.");
          } else {
            delimsBefore.transType = getDelimsBefore(line, trxTypePos, delim);
          }
        }
        static const str narrSearchWords[] = {"Narr", "Particulars", "Account",
                                              "Description", "Remarks"};
        unsigned c = 0;
        str_sz narrPos = line.find(narrSearchWords[c], datePos);
        while (!wasFound(narrPos) && c++ < std::size(narrSearchWords)) {
          narrPos = line.find(narrSearchWords[c], datePos);
        }
        if (!wasFound(narrPos)) {
          throw InvalidHeaderError("narration not found.");
        }
        delimsBefore.narr = getDelimsBefore(line, narrPos, delim);
        delimsBefore.balance = getDelimsBefore(line, balancePos, delim);
      }
    }

    /** for parsing date format, we try to construct a valid std::tm object by
     * iterating over formats in dateFormats.*/
    bool extractFromLine =
        options.headerAt != -1 && cnt > options.headerAt && !line.empty();
    auto dtPos = getDelimPos(line, delimsBefore.date, delim);
    if (extractFromLine && (line.find(delim) != str::npos) && wasFound(dtPos) &&
        !dateFormatRetrieved) {
      std::istringstream iss;
      if (dtPos) { // bad hack; normally we'd want to substr from delimPos + 1
        iss.str(line.substr(dtPos + 1));
      } else { // but if date is first char, we can't add 1;
        iss.str(line.substr(dtPos));
      }
      str dirty, cleaned;
      getline(iss, dirty, delim);
      iss.clear();
      iss.str(dirty);
      iss >> std::quoted(cleaned);
      iss.str(cleaned);
      std::tm t{};
      iss.clear();
      iss.str(cleaned);
      /** get_time() sets a badbit in stream if parsing fails.
       * we test the fail status of istringstream to stay in loop.
       * Set the badbit initially to enter loop. */
      iss.setstate(std::ios::badbit);
      for (auto &fmt : dateFormats) {
        /** the fist condition in below if is to bypass a bug(?) of get_time.
         * 01-01-20 was being assigned the format dd-mm-yyyy. Since the size of
         * label for %d-%m-%y [i.e. dd-mm-yy] will match the size of date string
         * to parse, we can ensure we have the correct date format. */
        if (fmt.label.size() == cleaned.size() &&
            (iss.fail() || t.tm_year == 0)) {
          /* reset the stream once inside loop */
          iss.clear();
          iss.str(cleaned);
          const str format(fmt.value);
          iss >> std::get_time(&t, format.data());
          if (!iss.fail()) {
            /** if stream makes through to here, we've got our format. */
            options.dateFormat = fmt;
            dateFormatRetrieved = true;
            break;
          }
        }
      }
    }
    ++cnt;
  }
  /** clear and reset the stream, so data parsing can begin from beginning. */
  filestrm.clear();
  filestrm.seekg(0, std::ios::beg);
  if (options.headerAt == -1) {
    throw InvalidHeaderError(
        "headers not found. looking for date, amount, balance.");
  }
  if (!dateFormatRetrieved) {
    throw DateParseError("Date format could not be parsed.");
  }
}

std::string_view rtrim(std::string_view &s) {
  s.remove_suffix(std::distance(
      s.crbegin(), std::find_if(s.crbegin(), s.crend(),
                                [](int c) { return !std::isspace(c); })));

  return s;
}

vec<str> parseDelimitedRecord(const str &s, char delimChar) {
  istringstream ss(s);
  vec<str> cols;
  while (ss.good()) {
    str substr;
    getline(ss >> std::ws, substr, delimChar);
    std::istringstream iss{substr};
    str x;
    iss >> std::quoted(x);
    std::string_view sv(x);
    rtrim(sv);
    cols.push_back(str(sv));
  }
  return cols;
}
std::tm parseDate(istringstream &iss, const str &fmt) {
  std::tm _tm{};
  iss >> std::get_time(&_tm, fmt.data());
  return _tm;
}

long toPaise(const str &s) {
  std::istringstream ss(s);

  /** it's crucial for cout to be using the locale with the custom moneypunct,
   * so get_money can function predictably */
  ss.imbue(std::cout.getloc());
  long double d;
  ss >> std::get_money(d);
  return long(d);
}

/** balance may have a Cr or Dr suffix in case of books data. */
long getBalance(str &str1) {
  auto pos = str1.find("Cr");
  if (pos <= str1.size()) {
    str1.erase(pos, str1.size() - 1);
    return checkZero(str1) * -1; // cr balance in books == -ve
  } else if (str1.find("Dr") < str1.size()) {
    pos = str1.find("Dr");
    str1.erase(pos, str1.size() - 1);
    return checkZero(str1);
  } else {
    return checkZero(str1);
  }
}

void checkTotalsRow(const str &s) {
  if (s.find("Total") != str::npos) {
    throw brlib::parse::TotalsRowError();
  }
}

EntryBase parseWithAutoConfig(str &s, EntryBase::EntryFrom from,
                              AutoParseSettings &options, bool &badDate) {

  /* get substring from position */
  auto getSubstr = [&](const unsigned int &pos) {
    str ret, sub;

    /* return empty string if we're trying to index out of bounds */
    if (pos >= s.size()) {
      return ret;
    }

    /* pos is delim's position; we need str from next char's pos */
    sub = s.substr(pos + 1);

    std::istringstream iss{sub};
    /* hard to debug this; if quotes exist, and aren't handled */
    if (iss.peek() == '"') {
      iss >> std::quoted(ret);
    } else {
      getline(iss, ret, options.delimChar);
    }
    iss.clear();
    return ret;
  };

  checkTotalsRow(s);
  unsigned rowDelimsCount = getDelimsBefore(s, s.size() - 1, options.delimChar);
  unsigned delimCountDiff = rowDelimsCount - options.headerDelimsCount;
  /** position of last delim char in string, given the number of delim chars as
   * param. */
  auto pos = [&](long cnt) { return getDelimPos(s, cnt, options.delimChar); };

  /* use common variables `lastDelimPos` and `valueStr` throughout */
  unsigned long lastDelimPos = pos(options.delimsBefore.date);

  const str &format = options.dateFormat.value;
  str valueStr = getSubstr(lastDelimPos);
  istringstream dtStrm{valueStr};
  std::tm date{parseDate(dtStrm, format)};
  badDate = dtStrm.fail();
#ifdef __linux__
  /** this directive is needed in parsing date.
   * When year component length of date is 2 [i.e. %y]; gcc/clang need addition
   * of 100y, while windows doesn't. catch formats with %y, let those with %Y
   * slide. */
  if (format.find('y') != str::npos) {
    date.tm_year += 100;
  }
#else
#endif

  long debit, credit, balance;
  if (!options.singleAmountCol) {
    /* debit */
    lastDelimPos = pos(options.delimsBefore.debit + delimCountDiff);
    valueStr = getSubstr(lastDelimPos);
    debit = checkZero(valueStr);

    /* credit */
    lastDelimPos = pos(options.delimsBefore.credit + delimCountDiff);
    valueStr = getSubstr(lastDelimPos);
    credit = checkZero(valueStr);
  } else {
    lastDelimPos = pos(options.delimsBefore.transType + delimCountDiff);
    valueStr = getSubstr(lastDelimPos);
    lastDelimPos = pos(options.delimsBefore.amount + delimCountDiff);
    if (valueStr.find('D') == str::npos) {
      valueStr = getSubstr(lastDelimPos);
      credit = checkZero(valueStr);
      debit = 0;
    } else {
      valueStr = getSubstr(lastDelimPos);
      debit = checkZero(valueStr);
      credit = 0;
    }
  }

  /* throw errors if needed */
  EntryBase::validate(debit, credit);

  /* balance */
  lastDelimPos = pos(options.delimsBefore.balance + delimCountDiff);
  valueStr = getSubstr(lastDelimPos);
  balance = getBalance(valueStr);

  /* narr */
  lastDelimPos = pos(options.delimsBefore.narr);
  str narr = getSubstr(lastDelimPos);

  return {from, date, narr, debit, credit, balance};
}

/** stream the value substrings into a vector, and use the provided positions to
 * extract values */
EntryBase parseWithManualConfig(str &s, EntryBase::EntryFrom from,
                                const ManualParseSettings &options,
                                bool &badDate) {
  vec<str> cols = parseDelimitedRecord(s, options.delimChar);
  using pr_t = ManualParseSettings::col_pr_t;
  typedef decltype(options.colIndices) map_t;
  const map_t &colIndices = options.colIndices;
  auto maxColPr = std::max_element(
      colIndices.cbegin(), colIndices.cend(),
      [&](const pr_t lhs, const pr_t rhs) { return lhs.second < rhs.second; });

  checkTotalsRow(s);
  if (maxColPr->second > cols.size()) {
    throw ColNumberError("col index exceeds parsed cols size.");
  }
  auto colIndex = [&](ManualParseSettings::Cols col) {
    return colIndices.find(col)->second;
  };
  /* extract the col number for each field */
  unsigned dateCol = colIndex(ManualParseSettings::Cols::Date),
           debitCol = colIndex(ManualParseSettings::Cols::Debit),
           creditCol = colIndex(ManualParseSettings::Cols::Credit),
           balanceCol = colIndex(ManualParseSettings::Cols::Balance),
           narrCol = colIndex(ManualParseSettings::Cols::Narr),
           amtCol = colIndex(ManualParseSettings::Cols::Amount),
           trxTypeCol = colIndex(ManualParseSettings::Cols::TransactionType);

  if (dateCol == -1 || narrCol == -1 || balanceCol == -1) {
    throw ColNumberError("required columns not assigned.");
  }

  bool debitColAvailable = debitCol != -1 && creditCol != -1,
       amtColAvailable = amtCol != -1 && trxTypeCol != -1;
  if (!debitColAvailable && !amtColAvailable) {
    throw ColNumberError("both debit and amt can't be unassigned.");
  }

  auto fromCols = [&](const unsigned &idx) {
    str _s;
    std::istringstream iss{cols.at(idx)};
    iss >> std::quoted(_s);
    return _s;
  };

  const char *format = options.dateFormat.value.data();
  str valueStr = fromCols(dateCol);
  istringstream dtStrm{valueStr};
  std::tm date{parseDate(dtStrm, format)};

  badDate = dtStrm.fail();

  /* see expln in func parseWithAutoConfig */
#if !defined(_WIN32)
  date.tm_year += 100;
#else
#endif

  str narr = fromCols(narrCol);

  long debit, credit, amount;
  if (debitColAvailable) {
    valueStr = fromCols(debitCol);
    debit = checkZero(valueStr);
    valueStr = fromCols(creditCol);
    credit = checkZero(valueStr);
  } else {
    valueStr = fromCols(amtCol);
    amount = checkZero(valueStr);
    valueStr = fromCols(trxTypeCol);
    /** Looking for the D in DR / CR flag. Should this be hardcoded? */
    if (valueStr.find('D') == str::npos) {
      credit = amount;
      debit = 0;
    } else {
      debit = amount;
      credit = 0;
    }
  }

  valueStr = fromCols(balanceCol);
  long balance = getBalance(valueStr);

  EntryBase::validate(debit, credit);

  return {from, date, narr, debit, credit, balance};
}

void parseEntries(EntryBase::EntryFrom from, std::fstream &file,
                  passedAndFailedVecs &vecs, bool autoParse,
                  ManualParseSettings &options) {

  /* if autoparse has been enabled, then parse data format, else, use user
   * provided settings. */
  AutoParseSettings autoSettings;
  int &headerAt = options.headerAt;
  if (autoParse) {
    configureAutoParse(file, autoSettings);
    headerAt = autoSettings.headerAt;
  }

  str raw_entry;
  int cnt;

  while (getline(file, raw_entry)) {
	// valgrind showed error here: 'conditional jump or move depends on uninitialized value(s)'
    if (cnt++ <= headerAt || raw_entry.find(options.delimChar) == str::npos) {
      continue;
    } else {
      try {
        /* for case where 2 separate lines form 1 entry
         * Since, date may be absent in line (2nd line onwards), pass a bool to
         * try and parse date; on failure, fetch the date from previous entry,
         * and copy it over. */
        bool badDate{false};
        EntryBase entry;
        if (autoParse) {
          entry = parseWithAutoConfig(raw_entry, from, autoSettings, badDate);
        } else {
          entry = parseWithManualConfig(raw_entry, from, options, badDate);
        }
        if (badDate) {
          if (!vecs.passed->empty()) {
            entry.date = vecs.passed->back().date;
          } else {
            vecs.failed->push_back(raw_entry);
            continue;
          }
        }
        vecs.passed->push_back(entry);
      } catch (TotalsRowError &e) {
        fprintf(stderr, "Stopping parse; found totals row: %s\n",
                raw_entry.data());
        break;
      } catch (ColNumberError &e) {
        fprintf(stderr, "parsing error: %s \n\t raw_entry: %s\n", e.what(),
                raw_entry.data());
        vecs.failed->push_back(raw_entry);
      } catch (DebitCreditError &e) {
        fprintf(stderr, "logic error: %s\n%s\n", e.what(), raw_entry.data());
        vecs.failed->push_back(raw_entry);
      } catch (BalanceParsingError &e) {
        fprintf(stderr, "parsing error: %s\n%s\n", e.what(), raw_entry.data());
        vecs.failed->push_back(raw_entry);
      } catch (std::exception &e) {
        file.close();
        fprintf(stderr, "unhandled error for entry: %s\n\t%s\n",
                raw_entry.data(), e.what());
        throw;
      }
    }
  }
  file.close();
}
} // namespace brlib::parse
