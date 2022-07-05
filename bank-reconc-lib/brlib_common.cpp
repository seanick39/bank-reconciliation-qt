#include "brlib_common.h"

namespace brlib {

bool dt_equal(const std::tm &lhs, const std::tm &rhs) {
  return (lhs.tm_mday == rhs.tm_mday) && (lhs.tm_mon == rhs.tm_mon) &&
         (lhs.tm_year == rhs.tm_year);
}

void ParseSettings::setAutoParse(bool value) {
  bank.autoParse = value;
  books.autoParse = value;
}

bool ParseSettings::isAutoParseEnabled() const { return m_autoParse; }

} // namespace brlib
