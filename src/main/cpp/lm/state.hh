#ifndef LM_STATE_H
#define LM_STATE_H

namespace lm {
namespace ngram {

// This is a POD but if you want memcmp to return the same as operator==, call
// ZeroRemaining first.
class State {
  public:
    bool operator==(const State &other) const {
      if (length != other.length) return false;
      return !memcmp(words, other.words, length * sizeof(WordIndex));
    }

    // Three way comparison function.
    int Compare(const State &other) const {
      if (length != other.length) return length < other.length ? -1 : 1;
      return memcmp(words, other.words, length * sizeof(WordIndex));
    }

    bool operator<(const State &other) const {
      if (length != other.length) return length < other.length;
      return memcmp(words, other.words, length * sizeof(WordIndex)) < 0;
    }

    // Call this before using raw memcmp.
    void ZeroRemaining() {
      for (unsigned char i = length; i < KENLM_MAX_ORDER - 1; ++i) {
        words[i] = 0;
        backoff[i] = 0.0;
      }
    }

    unsigned char Length() const { return length; }

    // You shouldn't need to touch anything below this line, but the members are public so FullState will qualify as a POD.
    // This order minimizes total size of the struct if WordIndex is 64 bit, float is 32 bit, and alignment of 64 bit integers is 64 bit.
    WordIndex words[KENLM_MAX_ORDER - 1];
    float backoff[KENLM_MAX_ORDER - 1];
    unsigned char length;
};

} // namespace ngram
} // namespace lm

#endif // LM_STATE_H
