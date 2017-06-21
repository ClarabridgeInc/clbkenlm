#ifndef LM_VALUE_H
#define LM_VALUE_H

#include "lm/config.hh"

#include <stdint.h>

namespace util {

typedef union { float f; uint32_t i; } FloatEnc;

const uint32_t kSignBit = 0x80000000;

} // namespace util

namespace lm {
namespace ngram {

// Template proxy for probing unigrams and middle.
template <class Weights> class GenericProbingProxy {
  public:
    explicit GenericProbingProxy(const Weights &to) : to_(&to) {}

    GenericProbingProxy() : to_(0) {}

    bool Found() const { return to_ != 0; }

    float Prob() const {
      util::FloatEnc enc;
      enc.f = to_->prob;
      enc.i |= util::kSignBit;
      return enc.f;
    }

    float Backoff() const { return to_->backoff; }

    bool IndependentLeft() const {
      util::FloatEnc enc;
      enc.f = to_->prob;
      return enc.i & util::kSignBit;
    }

  protected:
    const Weights *to_;
};

struct BackoffValue {
  typedef ProbBackoff Weights;
  static const ModelType kProbingModelType = PROBING;

  class ProbingProxy : public GenericProbingProxy<Weights> {
    public:
      explicit ProbingProxy(const Weights &to) : GenericProbingProxy<Weights>(to) {}
      ProbingProxy() {}
      float Rest() const { return Prob(); }
  };

  struct ProbingEntry {
    typedef uint64_t Key;
    typedef Weights Value;
    uint64_t key;
    ProbBackoff value;
    uint64_t GetKey() const { return key; }
  };
};

} // namespace ngram
} // namespace lm

#endif // LM_VALUE_H
