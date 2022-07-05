#ifndef BRLIB_PARSE_H
#define BRLIB_PARSE_H

#include "EntryBase.h"
#include "brlib_common.h"

namespace brlib::parse {

class DebitCreditError : public std::invalid_argument {
public:
  enum errorType { eq_zero = 0, gt_zero = 1 };
  explicit DebitCreditError(errorType et)
      : std::invalid_argument(
            et == errorType::eq_zero
                ? "Both debit and credit can't be zero"
                : "Both debit and credit can't be non-zero.") {}
};

class ColNumberError : public std::runtime_error {
public:
  explicit ColNumberError(const str &s) : std::runtime_error(s) {}
};

class BalanceParsingError : public std::runtime_error {
public:
  explicit BalanceParsingError()
      : std::runtime_error("balance doesn't have Dr or Cr") {}
};

class InvalidHeaderError : public std::invalid_argument {
public:
  explicit InvalidHeaderError(const str &s) : std::invalid_argument(s) {}
};

class TotalsRowError : public std::runtime_error {
public:
  explicit TotalsRowError() : std::runtime_error("") {}
};

class DateParseError : public std::invalid_argument {
public:
  explicit DateParseError(const str &e) : std::invalid_argument(e) {}
};

str::size_type wasFound(str::size_type pos);

std::string_view rtrim(std::string_view &s);

std::tm parseDate(istringstream &iss, const str &fmt);

void configureAutoParse(std::fstream &file, AutoParseSettings &options);

vec<str> parseDelimitedRecord(const str &s, char delimChar);

long toPaise(const str &s);

long checkZero(str &str1);

using sp_vec_entry_t = sp<vec<EntryBase>>;

struct passedAndFailedVecs {
  passedAndFailedVecs()
      : passed(std::make_shared<vec<EntryBase>>()),
        failed(std::make_shared<vec<str>>()) {}
  sp_vec_entry_t passed;
  sp<vec<str>> failed;
};
EntryBase parseWithAutoConfig(str &s, EntryBase::EntryFrom from,
                              AutoParseSettings &options, bool &badDate);

EntryBase parseWithManualConfig(str &s, EntryBase::EntryFrom from,
                                const ManualParseSettings &options,
                                bool &badDate);

void parseEntries(EntryBase::EntryFrom from, std::fstream &file,
                  passedAndFailedVecs &vecs, bool autoParse,
                  ManualParseSettings &options);

void checkTotalsRow(const str &s);

} // namespace brlib::parse

#endif // BRLIB_PARSE_H
