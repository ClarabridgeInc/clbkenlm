#include "lm/search_hashed.hh"

#include "lm/value.hh"

namespace lm {
namespace ngram {
namespace detail {

template <class Value> uint8_t *HashedSearch<Value>::SetupMemory(uint8_t *start, const std::vector<uint64_t> &counts, const Config &config) {
  unigram_ = Unigram(start, counts[0]);
  start += Unigram::Size(counts[0]);
  std::size_t allocated;
  middle_.clear();
  for (unsigned int n = 2; n < counts.size(); ++n) {
    allocated = Middle::Size(counts[n - 1], config.probing_multiplier);
    middle_.push_back(Middle(start, allocated));
    start += allocated;
  }
  allocated = Longest::Size(counts.back(), config.probing_multiplier);
  longest_ = Longest(start, allocated);
  start += allocated;
  return start;
}

template class HashedSearch<BackoffValue>;

} // namespace detail
} // namespace ngram
} // namespace lm
