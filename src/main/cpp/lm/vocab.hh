#ifndef LM_VOCAB_H
#define LM_VOCAB_H

#include "lm/virtual_interface.hh"
#include "util/probing_hash_table.hh"

#include <limits>
#include <string>
#include <vector>

namespace lm {
namespace ngram {
struct Config;

namespace detail {
uint64_t HashForVocab(const char *str, std::size_t len);
inline uint64_t HashForVocab(const StringPiece &str) {
  return HashForVocab(str.data(), str.length());
}
struct ProbingVocabularyHeader;
} // namespace detail

#pragma pack(push)
#pragma pack(4)
struct ProbingVocabularyEntry {
  uint64_t key;
  WordIndex value; // lm/word_index.hh:  typedef unsigned int WordIndex;

  typedef uint64_t Key;
  uint64_t GetKey() const { return key; }
  void SetKey(uint64_t to) { key = to; }

  static ProbingVocabularyEntry Make(uint64_t key, WordIndex value) {
    ProbingVocabularyEntry ret;
    ret.key = key;
    ret.value = value;
    return ret;
  }
};
#pragma pack(pop)

// Vocabulary storing a map from uint64_t to WordIndex.
class ProbingVocabulary : public base::Vocabulary {
  public:
    ProbingVocabulary() {};

    WordIndex Index(const StringPiece &str) const {
      Lookup::ConstIterator i;
      return lookup_.Find(detail::HashForVocab(str), i) ? i->value : 0;
    }

    static uint64_t Size(uint64_t entries, float probing_multiplier);
    // This just unwraps Config to get the probing_multiplier.
    static uint64_t Size(uint64_t entries, const Config &config);

    // Vocab words are [0, Bound()).
    WordIndex Bound() const { return bound_; }

    // Everything else is for populating.  I'm too lazy to hide and friend these, but you'll only get a const reference anyway.
    void SetupMemory(void *start, std::size_t allocated); // + LoadedBinary

  private:
    typedef util::ProbingHashTable<ProbingVocabularyEntry, util::IdentityHash> Lookup;

    Lookup lookup_;

    WordIndex bound_;

    bool saw_unk_;
};

} // namespace ngram
} // namespace lm

#endif // LM_VOCAB_H
