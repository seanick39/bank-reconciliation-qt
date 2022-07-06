#include "EntryBase.h"
#include <algorithm>
#include <sstream>

#include "EntryMatch.h"
#include <stdexcept>
#include <utility>

namespace brlib
{

    EntryMatch::EntryMatch(vec<EntryPointer> _data, sp<entry_vec> passedBankVec,
                           sp<entry_vec> passedBooksVec, bool isManual):
        m_data(std::move(_data)),
        m_bankPassedVec(std::move(passedBankVec)),
        m_booksPassedVec(std::move(passedBooksVec)), m_isManual(isManual)
    {
        if (!isManual)
        {
            m_isValid = true;
        }
    }

    bool EntryMatch::isManual() const { return m_isManual; }

    bool EntryMatch::bankIdxExists(entry_vec_sz_t entry_idx) const
    {
        return findIdx(EntryPointer::For::Bank, entry_idx) != m_data.end();
    }

    bool EntryMatch::booksIdxExists(entry_vec_sz_t entry_idx) const
    {
        return findIdx(EntryPointer::For::Books, entry_idx) != m_data.end();
    }

    vec<EntryPointer>::const_iterator
      EntryMatch::findIdx(EntryPointer::For entryFor, entry_vec_sz_t idx) const
    {
        return std::find_if(m_data.begin(), m_data.end(), [&](const EntryPointer p) {
            return p.entryFor == entryFor && p.entryIdx == idx;
        });
    }

    unsigned long EntryMatch::banksSize() const
    {
        return std::count_if(m_data.begin(), m_data.end(), [&](const EntryPointer p) {
            return p.entryFor == EntryPointer::For::Bank;
        });
    }

    unsigned long EntryMatch::booksSize() const
    {
        return std::count_if(m_data.begin(), m_data.end(), [&](const EntryPointer p) {
            return p.entryFor == EntryPointer::For::Books;
        });
    }

    unsigned long EntryMatch::setsTotalSize() const { return m_data.size(); }

    long EntryMatch::debitSum() const { return amountSum(); }

    long EntryMatch::creditSum() const { return amountSum(false); }

    long EntryMatch::amountSum(bool forDebit) const
    {
        checkBankVecSize();
        long sum = 0;
        for (const decltype(m_data)::value_type& entryPointer : m_data)
        {
            if (entryPointer.entryFor == EntryPointer::For::Bank)
            {
                const auto& bankIdx = entryPointer.entryIdx;
                const EntryBase& entry = m_bankPassedVec->at(bankIdx);
                if (!forDebit)
                {
                    sum += entry.credit;
                }
                else
                {
                    sum += entry.debit;
                }
            }
        }
        return sum;
    }

    long EntryMatch::banksSum() const
    {
        checkBankVecSize();
        long sum = 0;
        for (const decltype(m_data)::value_type& entryPointer : m_data)
        {
            if (entryPointer.entryFor == EntryPointer::For::Bank)
            {
                const auto& bankIdx = entryPointer.entryIdx;
                const EntryBase& entry = m_bankPassedVec->at(bankIdx);
                sum += !entry.debit ? entry.credit : entry.debit;
            }
        }
        return sum;
    }

    long EntryMatch::booksSum() const
    {
        checkBooksVecSize();
        long sum = 0;
        for (const decltype(m_data)::value_type& entryPointer : m_data)
        {
            if (entryPointer.entryFor == EntryPointer::For::Books)
            {
                const auto& booksIdx = entryPointer.entryIdx;
                const EntryBase& entry = m_booksPassedVec->at(booksIdx);
                sum += !entry.debit ? entry.credit : entry.debit;
            }
        }
        return sum;
    }

    void EntryMatch::printMoney(unsigned long money, std::ostringstream& oss)
    {
        oss.imbue(std::cout.getloc());
        oss << std::setw(12) << std::right << std::put_money(money);
    }

    /* insert bankVecIdx into set for a valid match. */
    bool EntryMatch::insertIntoBank(entry_vec_sz_t bankIdx,
                                    const results_t& results)
    {
        checkBankVecSize();
        if (!insertionCheck(bankIdx, results.missingInBook, m_bankPassedVec))
        {
            return false;
        }
        m_data.push_back(EntryPointer(bankIdx, EntryPointer::For::Bank));
        return true;
    }

    /* insert booksVecIdx into set for a valid match. */
    bool EntryMatch::insertIntoBooks(entry_vec_sz_t bookIdx,
                                     const results_t& results)
    {
        checkBooksVecSize();
        if (!insertionCheck(bookIdx, results.missingInBank, m_booksPassedVec))
        {
            return false;
        }
        m_data.push_back(EntryPointer(bookIdx, EntryPointer::For::Books));
        return true;
    }

    bool EntryMatch::insertionCheck(entry_vec_sz_t i,
                                    const vec<entry_vec_sz_t>& lookupVec,
                                    sp<entry_vec> passedEntries)
    {
        if (i >= passedEntries->size())
        {
            std::cerr << "EntryMatch insertionCheck() i >= passedEntries.size()" << i
                      << std::endl;
            return false;
        }
        if (m_isManual)
        {
            auto endIt = lookupVec.cend();
            auto it = std::find(lookupVec.cbegin(), endIt, i);
            if (it == endIt)
            {
                std::cerr << "EntryMatch insertionCheck() i not found in lookupVec" << i
                          << std::endl;
                return false;
            }
        }
        return true;
    }

    bool EntryMatch::containsBankIdx(entry_vec_sz_t i) const
    {
        return std::find_if(m_data.begin(), m_data.end(), [&](const EntryPointer p) {
                   return p.entryFor == EntryPointer::For::Bank && p.entryIdx == i;
               }) != m_data.end();
    }

    bool EntryMatch::containsBooksIdx(entry_vec_sz_t i) const
    {
        return std::find_if(m_data.begin(), m_data.end(), [&](const EntryPointer p) {
                   return p.entryFor == EntryPointer::For::Books && p.entryIdx == i;
               }) != m_data.end();
    };

    void EntryMatch::printData() const
    {
        std::cout << "entrymatch \tbanks: [ ";
        for (const auto& ep : m_data)
        {
            if (ep.entryFor == EntryPointer::For::Bank)
            {
                std::cout << ep.entryIdx << " ";
            }
        }
        std::cout << "] \tbooks: [ ";
        for (const auto& ep : m_data)
        {
            if (ep.entryFor == EntryPointer::For::Books)
            {
                std::cout << ep.entryIdx << " ";
            }
        }
        std::cout << "]\n";
    }

    void EntryMatch::eraseBankIdx(entry_vec_sz_t bankIdx)
    {
        erase_if(m_data, [&](EntryPointer& ep) {
            return ep.entryFor == EntryPointer::For::Bank && ep.entryIdx == bankIdx;
        });
    }

    void EntryMatch::eraseBooksIdx(entry_vec_sz_t booksIdx)
    {
        erase_if(m_data, [&](EntryPointer& ep) {
            return ep.entryFor == EntryPointer::For::Books && ep.entryIdx == booksIdx;
        });
    }

    const EntryBase& EntryMatch::entryForBankIdx(entry_vec_sz_t bankIdx) const
    {
        return m_bankPassedVec->at(bankIdx);
    }
    const EntryBase& EntryMatch::entryForBooksIdx(entry_vec_sz_t booksIdx) const
    {
        return m_booksPassedVec->at(booksIdx);
    }

    bool EntryMatch::isValid()
    {
        long bankSum = 0;
        long booksSum = 0;
        for (const EntryPointer& ep : m_data)
        {
            if (ep.entryFor == EntryPointer::For::Bank)
            {
                if (!containsBankIdx(ep.entryIdx))
                {
                    return false;
                }
                const EntryBase& entry = entryForBankIdx(ep.entryIdx);
                bankSum += !entry.debit ? entry.credit : entry.debit;
            }
            else
            {
                if (!containsBooksIdx(ep.entryIdx))
                {
                    return false;
                }
                const EntryBase& entry = entryForBooksIdx(ep.entryIdx);
                booksSum += !entry.debit ? entry.credit : entry.debit;
            }
        }
        m_isValid = (bankSum > 0) && (booksSum == bankSum);
        return m_isValid;
    }

    void EntryMatch::checkBankVecSize() const
    {
        if (m_bankPassedVec->empty())
            throw EmptyVecError(EntryMatch::EmptyVecError::Which::Bank);
    }

    void EntryMatch::checkBooksVecSize() const
    {
        if (m_booksPassedVec->empty())
            throw EmptyVecError(EntryMatch::EmptyVecError::Which::Books);
    }

    EntryMatch::entry_set EntryMatch::banksIndices() const
    {
        entry_set s;
        for (const auto& ep : m_data)
        {
            if (ep.entryFor == EntryPointer::For::Bank)
            {
                s.insert(ep.entryIdx);
            }
        }
        return s;
    }

    EntryMatch::entry_set EntryMatch::booksSet() const
    {
        entry_set s;
        for (const auto& ep : m_data)
        {
            if (ep.entryFor == EntryPointer::For::Books)
            {
                s.insert(ep.entryIdx);
            }
        }
        return s;
    }

    const vec<EntryPointer>& EntryMatch::data() const { return m_data; }

} // namespace brlib
