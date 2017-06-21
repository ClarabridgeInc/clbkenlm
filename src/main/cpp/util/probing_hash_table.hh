#ifndef UTIL_PROBING_HASH_TABLE_H
#define UTIL_PROBING_HASH_TABLE_H

#include "util/exception.hh"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>
#include <memory>

#include <cassert>
#include <stdint.h>

#include <iostream>

namespace util {

/* Thrown when table grows too large */
class ProbingSizeException : public Exception {
  public:
    ProbingSizeException() throw() {}
    ~ProbingSizeException() throw() {}
};

// std::identity is an SGI extension :-(
// needed by others
struct IdentityHash {
  template <class T> T operator()(T arg) const { return arg; }
};

class DivMod {
  public:
    explicit DivMod(std::size_t buckets)
    : buckets_(buckets) {
        //std::cout << "DivMod c-tor buckets: " << std::dec << buckets_ << std::endl;
    }

    static std::size_t RoundBuckets(std::size_t from) {
      return from;
    }

    template <class It> It Ideal(It begin, uint64_t hash) const {
      It result = begin + (hash % buckets_);
      //std::cout << "Ideal begin: " << std::hex << begin << " hash: " << hash << " buckets_: " << buckets_
      //    << " (hash mod buckets_): " << (hash % buckets_) << " result: " << result << std::endl;
      return result;
    }

    template <class BaseIt, class OutIt> void Next(BaseIt begin, BaseIt end, OutIt &it) const {
      if (++it == end) it = begin;
    }

  private:
    std::size_t buckets_;
};

/* Non-standard hash table
 * Buckets must be set at the beginning and must be greater than maximum number
 * of elements, else it throws ProbingSizeException.
 * Memory management and initialization is externalized to make it easier to
 * serialize these to disk and load them quickly.
 * Uses linear probing to find value.
 * Only insert and lookup operations.
 */
template <class EntryT, class HashT, class EqualT = std::equal_to<typename EntryT::Key>, class ModT = DivMod> class ProbingHashTable {
  public:
    typedef EntryT Entry;
    typedef typename Entry::Key Key;
    typedef const Entry *ConstIterator;
    typedef Entry *MutableIterator;
    typedef HashT Hash;
    typedef EqualT Equal;
    typedef ModT Mod;

    static uint64_t Size(uint64_t entries, float multiplier) {
      uint64_t buckets = Mod::RoundBuckets(std::max(entries + 1, static_cast<uint64_t>(multiplier * static_cast<float>(entries))));
      return buckets * sizeof(Entry);
    }

    // !!! Please, do not hold any mem-mgr stuff or override copy c-tor and assignment

    // Must be assigned to later.
    ProbingHashTable()
    :
        buckets_(0),
        mod_(1),
        entries_(0)
#ifdef DEBUG
      , initialized_(false)
#endif
    {
        //std::cout << "pht c-tor 0 this: " << std::hex << reinterpret_cast<void *>(this) << std::endl;
    }

    ProbingHashTable(void *start, std::size_t allocated, const Key &invalid = Key(), const Hash &hash_func = Hash(), const Equal &equal_func = Equal())
    : 
        buckets_(allocated / sizeof(Entry)),
        begin_(reinterpret_cast<MutableIterator>(start)),
        end_(begin_ + (allocated / sizeof(Entry))),
        mod_(allocated / sizeof(Entry)),
        invalid_(invalid),
        hash_(hash_func),
        equal_(equal_func),
        entries_(0)
#ifdef DEBUG
        , initialized_(true)
#endif
    {
    }

    MutableIterator Ideal(const Key key) {
      return mod_.Ideal(begin_, hash_(key));
    }
    ConstIterator Ideal(const Key key) const {
      return mod_.Ideal(begin_, hash_(key));
    }

    // Iterator is both input and output.
    template <class Key> bool FindFromIdeal(const Key key, ConstIterator &i) const {
#ifdef DEBUG
      assert(initialized_);
#endif
      for (;; mod_.Next(begin_, end_, i)) {
        Key got(i->GetKey());
        //std::cout << "pht.FindFromIdeal got: " << std::hex << got << " begin: " << begin_ << " end: " << end_ << " i: " << i << std::endl;
        if (equal_(got, key)) {
            //std::cout << "found." << std::endl;
            return true;
        }
        if (equal_(got, invalid_)) {
            //std::cout << "not found." << std::endl;
            return false;
        }
      }
    }

    template <class Key> bool Find(const Key key, ConstIterator &out) const {
      out = Ideal(key);
      //std::cout << "pht.Find key: " << std::hex << key << " begin: " << begin_ << " end: " << end_ << " out: " << out
      //    << " out.key: " << out->key << " begin_.key: " << begin_->key << std::endl;

      bool result = FindFromIdeal(key, out);
      //std::cout << "pht.Find result: " << std::dec << (result ? 1 : 0) << std::endl;
      return result;
    }

    // Mostly for tests, check consistency of every entry.
    void CheckConsistency() {
      MutableIterator last;
      for (last = end_ - 1; last >= begin_ && !equal_(last->GetKey(), invalid_); --last) {}
      UTIL_THROW_IF(last == begin_, ProbingSizeException, "Completely full");
      MutableIterator i;
      // Beginning can be wrap-arounds.
      for (i = begin_; !equal_(i->GetKey(), invalid_); ++i) {
        MutableIterator ideal = Ideal(i->GetKey());
        UTIL_THROW_IF(ideal > i && ideal <= last, Exception, "Inconsistency at position " << (i - begin_) << " should be at " << (ideal - begin_));
      }
      MutableIterator pre_gap = i;
      for (; i != end_; ++i) {
        if (equal_(i->GetKey(), invalid_)) {
          pre_gap = i;
          continue;
        }
        MutableIterator ideal = Ideal(i->GetKey());
        UTIL_THROW_IF(ideal > i || ideal <= pre_gap, Exception, "Inconsistency at position " << (i - begin_) << " with ideal " << (ideal - begin_));
      }
    }

  private:
    MutableIterator begin_;
    MutableIterator end_;
    std::size_t buckets_;
    Key invalid_;
    Hash hash_;
    Equal equal_;
    Mod mod_;

    std::size_t entries_;
#ifdef DEBUG
    bool initialized_;
#endif
};

} // namespace util

#endif // UTIL_PROBING_HASH_TABLE_H
