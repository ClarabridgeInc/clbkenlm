#include "lm/vocab.hh"

#include "lm/lm_exception.hh"
#include "lm/config.hh"
#include "util/exception.hh"
#include "util/murmur_hash.hh"

namespace lm {
namespace ngram {

namespace detail {
uint64_t HashForVocab(const char *str, std::size_t len) {
  // This proved faster than Boost's hash in speed trials: total load time Murmur 67090000, Boost 72210000
  // Chose to use 64A instead of native so binary format will be portable across 64 and 32 bit.
  return util::MurmurHash64A(str, len, 0);
}
} // namespace detail

namespace {
// Normally static initialization is a bad idea but MurmurHash is pure arithmetic, so this is ok.
const uint64_t kUnknownHash = detail::HashForVocab("<unk>", 5);
// Sadly some LMs have <UNK>.
const uint64_t kUnknownCapHash = detail::HashForVocab("<UNK>", 5);

} // namespace

namespace {
const unsigned int kProbingVocabularyVersion = 0;
} // namespace

namespace detail {
struct ProbingVocabularyHeader {
  // Lowest unused vocab id.  This is also the number of words, including <unk>.
  unsigned int version;
  WordIndex bound;
};
} // namespace detail

uint64_t ProbingVocabulary::Size(uint64_t entries, float probing_multiplier) {
  return ALIGN8(sizeof(detail::ProbingVocabularyHeader)) + Lookup::Size(entries, probing_multiplier);
}

uint64_t ProbingVocabulary::Size(uint64_t entries, const Config &config) {
  return Size(entries, config.probing_multiplier);
}

void ProbingVocabulary::SetupMemory(void *start, std::size_t allocated) {
  detail::ProbingVocabularyHeader *header_ = static_cast<detail::ProbingVocabularyHeader*>(start);
  lookup_ = Lookup(static_cast<uint8_t*>(start) + ALIGN8(sizeof(detail::ProbingVocabularyHeader)), allocated);
  bound_ = 1;
  saw_unk_ = false;

  UTIL_THROW_IF(header_->version != kProbingVocabularyVersion, FormatLoadException, "The binary file has probing version " << header_->version << " but the code expects version " << kProbingVocabularyVersion << ".  Please rerun build_binary using the same version of the code.");
  bound_ = header_->bound;
  SetSpecial(Index("<s>"), Index("</s>"), 0);
  // just my addition, not present in original version, use for debug
  //lookup_.CheckConsistency(); 
}

} // namespace ngram
} // namespace lm
