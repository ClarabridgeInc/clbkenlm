#include "lm/model.hh"

#include "lm/max_order.hh"
#include "lm/lm_exception.hh"
#include "util/exception.hh"

#include <algorithm>
#include <functional>
#include <numeric>
#include <cmath>
#include <limits>

namespace lm {
namespace ngram {

/* Suppose "foo bar" appears with zero backoff but there is no trigram
 * beginning with these words.  Then, when scoring "foo bar", the model could
 * return out_state containing "bar" or even null context if "bar" also has no
 * backoff and is never followed by another word.  Then the backoff is set to
 * kNoExtensionBackoff.  If the n-gram might be extended, then out_state must
 * contain the full n-gram, in which case kExtensionBackoff is set.  In any
 * case, if an n-gram has non-zero backoff, the full state is returned so
 * backoff can be properly charged.
 * These differ only in sign bit because the backoff is in fact zero in either
 * case.
 */
const float kNoExtensionBackoff = -0.0;

// This compiles down nicely.
inline bool HasExtension(const float &backoff) {
  typedef union { float f; uint32_t i; } UnionValue;
  UnionValue compare, interpret;
  compare.f = kNoExtensionBackoff;
  interpret.f = backoff;
  return compare.i != interpret.i;
}

namespace detail {

template <class Search, class VocabularyT> const ModelType GenericModel<Search, VocabularyT>::kModelType = Search::kModelType;

template <class Search, class VocabularyT>
uint64_t GenericModel<Search, VocabularyT>::Size(const std::vector<uint64_t> &counts, const Config &config) {
  return VocabularyT::Size(counts[0], config) + Search::Size(counts, config);
}

template <class Search, class VocabularyT>
void GenericModel<Search, VocabularyT>::SetupMemory(void *base, const std::vector<uint64_t> &counts, const Config &config) {
  size_t goal_size = ::util::CheckOverflow(Size(counts, config));
  uint8_t *start = static_cast<uint8_t*>(base);
  size_t allocated = VocabularyT::Size(counts[0], config);
  vocab_.SetupMemory(start, allocated); // , counts[0], config
  start += allocated;
  start = search_.SetupMemory(start, counts, config);
  if (static_cast<std::size_t>(start - static_cast<uint8_t*>(base)) != goal_size) UTIL_THROW(FormatLoadException, "The data structures took " << (start - static_cast<uint8_t*>(base)) << " but Size says they should take " << goal_size);
}

namespace {

void CheckCounts(const std::vector<uint64_t> &counts) {
  UTIL_THROW_IF(counts.size() > KENLM_MAX_ORDER, FormatLoadException, "This model has order " << counts.size() << " but KenLM was compiled to support up to " << KENLM_MAX_ORDER << ".  " << KENLM_ORDER_MESSAGE);
  if (sizeof(uint64_t) > sizeof(std::size_t)) {
    for (std::vector<uint64_t>::const_iterator i = counts.begin(); i != counts.end(); ++i) {
      UTIL_THROW_IF(*i > static_cast<uint64_t>(std::numeric_limits<size_t>::max()), util::OverflowException, "This model has " << *i << " " << (i - counts.begin() + 1) << "-grams which is too many for 32-bit machines.");
    }
  }
}

const char *kModelNames[1] = {
    "probing hash tables"
};

const char kMagicBeforeVersion[] = "mmap lm http://kheafield.com/code format version";
const char kMagicBytes[] = "mmap lm http://kheafield.com/code format version 5\n\0";
// This must be shorter than kMagicBytes and indicates an incomplete binary file (i.e. build failed).
const char kMagicIncomplete[] = "mmap lm http://kheafield.com/code incomplete\n";
const long int kMagicVersion = 5;

// Old binary files built on 32-bit machines have this header.
// TODO: eliminate with next binary release.
struct OldSanity {
  char magic[sizeof(kMagicBytes)];
  float zero_f, one_f, minus_half_f;
  WordIndex one_word_index, max_word_index;
  uint64_t one_uint64;

  void SetToReference() {
    std::memset(this, 0, sizeof(OldSanity));
    std::memcpy(magic, kMagicBytes, sizeof(magic));
    zero_f = 0.0; one_f = 1.0; minus_half_f = -0.5;
    one_word_index = 1;
    max_word_index = std::numeric_limits<WordIndex>::max();
    one_uint64 = 1;
  }
};


// Test values aligned to 8 bytes.
struct Sanity {
  char magic[ALIGN8(sizeof(kMagicBytes))];
  float zero_f, one_f, minus_half_f;
  WordIndex one_word_index, max_word_index, padding_to_8;
  uint64_t one_uint64;

  void SetToReference() {
    std::memset(this, 0, sizeof(Sanity));
    std::memcpy(magic, kMagicBytes, sizeof(kMagicBytes));
    zero_f = 0.0; one_f = 1.0; minus_half_f = -0.5;
    one_word_index = 1;
    max_word_index = std::numeric_limits<WordIndex>::max();
    padding_to_8 = 0;
    one_uint64 = 1;
  }
};

struct FixedWidthParameters {
  unsigned char order;
  float probing_multiplier;
  // What type of model is this?
  ModelType model_type;
  // Does the end of the file have the actual strings in the vocabulary?
  bool has_vocabulary;
  unsigned int search_version;
};

// Parameters stored in the header of a binary file.
struct Parameters {
  FixedWidthParameters fixed;
  std::vector<uint64_t> counts;
};

} // namespace

std::size_t TotalHeaderSize(unsigned char order) {
  return ALIGN8(sizeof(Sanity) + sizeof(FixedWidthParameters) + sizeof(uint64_t) * order);
}

void CheckHeader(Parameters &out) {
  if (out.fixed.probing_multiplier < 1.0)
    UTIL_THROW(FormatLoadException, "Binary format claims to have a probing multiplier of ... which is < 1.0."); // << out.fixed.probing_multiplier 
  out.counts.resize(static_cast<std::size_t>(out.fixed.order));
}

namespace util {
  const size_t kBadSize = (size_t)-1;
}

bool IsBinaryFormat(size_t file_size, void *data) {
  if (file_size == util::kBadSize || (file_size <= sizeof(Sanity))) return false;
  Sanity reference_header = Sanity();
  reference_header.SetToReference();
  if (!std::memcmp(data, &reference_header, sizeof(Sanity))) return true;
  if (!std::memcmp(data, kMagicIncomplete, strlen(kMagicIncomplete))) {
    UTIL_THROW(FormatLoadException, "This binary file did not finish building");
  }
  if (!std::memcmp(data, kMagicBeforeVersion, strlen(kMagicBeforeVersion))) {
    char *end_ptr;
    const char *begin_version = static_cast<const char*>(data) + strlen(kMagicBeforeVersion);
    long int version = std::strtol(begin_version, &end_ptr, 10);
    if ((end_ptr != begin_version) && version != kMagicVersion) {
      UTIL_THROW(FormatLoadException, "Binary file has version " << version << " but this implementation expects version " << kMagicVersion << " so you'll have to use the ARPA to rebuild your binary");
    }
    OldSanity old_sanity = OldSanity();
    old_sanity.SetToReference();
    UTIL_THROW_IF(!std::memcmp(data, &old_sanity, sizeof(OldSanity)), FormatLoadException, "Looks like this is an old 32-bit format.  The old 32-bit format has been removed so that 64-bit and 32-bit files are exchangeable.");
    UTIL_THROW(FormatLoadException, "File looks like it should be loaded with mmap, but the test values don't match.  Try rebuilding the binary format LM using the same code revision, compiler, and architecture");
  }
  return false;
}

const std::size_t kInvalidSize = static_cast<std::size_t>(-1);

void MatchCheck(ModelType model_type, unsigned int search_version, const Parameters &params) {
  if (params.fixed.model_type != model_type) {
    if (static_cast<unsigned int>(params.fixed.model_type) >= (sizeof(kModelNames) / sizeof(const char *)))
      UTIL_THROW(FormatLoadException, "The binary file claims to be model type " << static_cast<unsigned int>(params.fixed.model_type) << " but this is not implemented for in this inference code.");
    UTIL_THROW(FormatLoadException, "The binary file was built for " << kModelNames[params.fixed.model_type] << " but the inference code is trying to load " << kModelNames[model_type]);
  }
  UTIL_THROW_IF(search_version != params.fixed.search_version, FormatLoadException, "The binary file has " << kModelNames[params.fixed.model_type] << " version " << params.fixed.search_version << " but this code expects " << kModelNames[params.fixed.model_type] << " version " << search_version);
}


template <class Search, class VocabularyT>
GenericModel<Search, VocabularyT>::GenericModel(size_t file_size, void *data, const Config &init_config)
:
    readen_content((uint8_t *)std::malloc(file_size))
{
  std::memcpy(readen_content, data, file_size);

  bool isBinaryFormat = IsBinaryFormat(file_size, readen_content);
  UTIL_THROW_IF(!isBinaryFormat, FormatLoadException, "Not a binary format of a file");

  Parameters parameters;

  memcpy(&parameters.fixed, readen_content + sizeof(Sanity), sizeof(FixedWidthParameters));

  CheckHeader(parameters);
  if (parameters.fixed.order) {
    memcpy(&*parameters.counts.begin(), readen_content + sizeof(Sanity) + sizeof(FixedWidthParameters), sizeof(uint64_t) * parameters.fixed.order);
  }

  MatchCheck(kModelType, kVersion, parameters);
  std::size_t header_size = TotalHeaderSize(parameters.counts.size());
  assert(header_size != kInvalidSize);

  CheckCounts(parameters.counts);

  Config new_config(init_config);
  new_config.probing_multiplier = parameters.fixed.probing_multiplier;

  UTIL_THROW_IF(!parameters.fixed.has_vocabulary, FormatLoadException, "The decoder requested all the vocabulary strings, but this binary does not have them.  You may need to rebuild the binary with an updated version of build_binary.");

  std::size_t size = Size(parameters.counts, new_config);
  // The header is smaller than a page, so we have to map the whole header as well.
  uint64_t total_map = static_cast<uint64_t>(header_size) + static_cast<uint64_t>(size);

  UTIL_THROW_IF(file_size != util::kBadSize && file_size < total_map, FormatLoadException, "Binary file has size " << file_size << " but the headers say it should be at least " << total_map);
  SetupMemory(readen_content + header_size, parameters.counts, new_config);
  // isBinaryFormat

  // g++ prints warnings unless these are fully initialized.
  State begin_sentence = State();
  begin_sentence.length = 1;
  begin_sentence.words[0] = vocab_.BeginSentence();
  typename Search::Node ignored_node;
  bool ignored_independent_left;
  uint64_t ignored_extend_left;
  begin_sentence.backoff[0] = search_.LookupUnigram(begin_sentence.words[0], ignored_node, ignored_independent_left, ignored_extend_left).Backoff();
  State null_context = State();
  null_context.length = 0;
  P::Init(begin_sentence, null_context, vocab_, search_.Order());
}

template <class Search, class VocabularyT>
GenericModel<Search, VocabularyT>::~GenericModel() {
  if (readen_content) {
      std::free(readen_content);
  }
}

template <class Search, class VocabularyT>
FullScoreReturn GenericModel<Search, VocabularyT>::FullScore(const State &in_state, const WordIndex new_word, State &out_state) const {
  FullScoreReturn ret = ScoreExceptBackoff(in_state.words, in_state.words + in_state.length, new_word, out_state);
  for (const float *i = in_state.backoff + ret.ngram_length - 1; i < in_state.backoff + in_state.length; ++i) {
    ret.prob += *i;
  }
  return ret;
}

namespace {
// Do a paraonoid copy of history, assuming new_word has already been copied
// (hence the -1).  out_state.length could be zero so I avoided using
// std::copy.
void CopyRemainingHistory(const WordIndex *from, State &out_state) {
  WordIndex *out = out_state.words + 1;
  const WordIndex *in_end = from + static_cast<ptrdiff_t>(out_state.length) - 1;
  for (const WordIndex *in = from; in < in_end; ++in, ++out) *out = *in;
}
} // namespace

/* Ugly optimized function.  Produce a score excluding backoff.
 * The search goes in increasing order of ngram length.
 * Context goes backward, so context_begin is the word immediately preceeding
 * new_word.
 */
template <class Search, class VocabularyT>
FullScoreReturn GenericModel<Search, VocabularyT>::ScoreExceptBackoff(
    const WordIndex *const context_rbegin,
    const WordIndex *const context_rend,
    const WordIndex new_word,
    State &out_state) const {
  assert(new_word < vocab_.Bound());
  FullScoreReturn ret;
  // ret.ngram_length contains the last known non-blank ngram length.
  ret.ngram_length = 1;

  typename Search::Node node;
  typename Search::UnigramPointer uni(search_.LookupUnigram(new_word, node, ret.independent_left, ret.extend_left));
  out_state.backoff[0] = uni.Backoff();
  ret.prob = uni.Prob();
  ret.rest = uni.Rest();

  // This is the length of the context that should be used for continuation to the right.
  out_state.length = HasExtension(out_state.backoff[0]) ? 1 : 0;
  // We'll write the word anyway since it will probably be used and does no harm being there.
  out_state.words[0] = new_word;
  if (context_rbegin == context_rend) return ret;

  ResumeScore(context_rbegin, context_rend, 0, node, out_state.backoff + 1, out_state.length, ret);
  CopyRemainingHistory(context_rbegin, out_state);
  return ret;
}

template <class Search, class VocabularyT>
void GenericModel<Search, VocabularyT>::ResumeScore(const WordIndex *hist_iter, const WordIndex *const context_rend, unsigned char order_minus_2, typename Search::Node &node, float *backoff_out, unsigned char &next_use, FullScoreReturn &ret) const {
  for (; ; ++order_minus_2, ++hist_iter, ++backoff_out) {
    if (hist_iter == context_rend) return;
    if (ret.independent_left) return;
    if (order_minus_2 == P::Order() - 2) break;

    typename Search::MiddlePointer pointer(search_.LookupMiddle(order_minus_2, *hist_iter, node, ret.independent_left, ret.extend_left));
    if (!pointer.Found()) return;
    *backoff_out = pointer.Backoff();
    ret.prob = pointer.Prob();
    ret.rest = pointer.Rest();
    ret.ngram_length = order_minus_2 + 2;
    if (HasExtension(*backoff_out)) {
      next_use = ret.ngram_length;
    }
  }
  ret.independent_left = true;
  typename Search::LongestPointer longest(search_.LookupLongest(*hist_iter, node));
  if (longest.Found()) {
    ret.prob = longest.Prob();
    ret.rest = ret.prob;
    // There is no blank in longest_.
    ret.ngram_length = P::Order();
  }
}

template class GenericModel<HashedSearch<BackoffValue>, ProbingVocabulary>;

} // namespace detail

} // namespace ngram
} // namespace lm
