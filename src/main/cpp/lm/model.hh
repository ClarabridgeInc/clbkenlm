#ifndef LM_MODEL_H
#define LM_MODEL_H

#include "lm/config.hh"
#include "lm/search_hashed.hh"
#include "lm/state.hh"
#include "lm/value.hh"
#include "lm/vocab.hh"

#include <algorithm>
#include <vector>

namespace lm {
namespace base {

// Common model interface that depends on knowing the specific classes.
// Curiously recurring template pattern.
template <class Child, class StateT, class VocabularyT> class ModelFacade : public Model {
  public:
    typedef StateT State;
    typedef VocabularyT Vocabulary;

    /* Translate from void* to State */
    FullScoreReturn BaseFullScore(const void *in_state, const WordIndex new_word, void *out_state) const {
      return static_cast<const Child*>(this)->FullScore(
          *reinterpret_cast<const State*>(in_state),
          new_word,
          *reinterpret_cast<State*>(out_state));
    }

    // Default Score function calls FullScore.  Model can override this.
    float Score(const State &in_state, const WordIndex new_word, State &out_state) const {
      return static_cast<const Child*>(this)->FullScore(in_state, new_word, out_state).prob;
    }

    float BaseScore(const void *in_state, const WordIndex new_word, void *out_state) const {
      return static_cast<const Child*>(this)->Score(
          *reinterpret_cast<const State*>(in_state),
          new_word,
          *reinterpret_cast<State*>(out_state));
    }

    const State &BeginSentenceState() const { return begin_sentence_; }
    const State &NullContextState() const { return null_context_; }
    const Vocabulary &GetVocabulary() const { return *static_cast<const Vocabulary*>(&BaseVocabulary()); }

  protected:
    ModelFacade() : Model(sizeof(State)) {}

    virtual ~ModelFacade() {}

    // begin_sentence and null_context can disappear after.  vocab should stay.
    void Init(const State &begin_sentence, const State &null_context, const Vocabulary &vocab, unsigned char order) {
      begin_sentence_ = begin_sentence;
      null_context_ = null_context;
      begin_sentence_memory_ = &begin_sentence_;
      null_context_memory_ = &null_context_;
      base_vocab_ = &vocab;
      order_ = order;
    }

  private:
    State begin_sentence_, null_context_;
};

} // mamespace base

namespace ngram {
namespace detail {

// Should return the same results as SRI.
// ModelFacade typedefs Vocabulary so we use VocabularyT to avoid naming conflicts.
template <class Search, class VocabularyT>
class GenericModel : public base::ModelFacade<GenericModel<Search, VocabularyT>, State, VocabularyT> {
  private:
    typedef base::ModelFacade<GenericModel<Search, VocabularyT>, State, VocabularyT> P;
  public:
    // This is the model type returned by RecognizeBinary.
    static const ModelType kModelType;

    static const unsigned int kVersion = Search::kVersion;

    /* Get the size of memory that will be mapped given ngram counts.  This
     * does not include small non-mapped control structures, such as this class
     * itself.
     */
    static uint64_t Size(const std::vector<uint64_t> &counts, const Config &config = Config());

    /* Load the model from a file. Binary files must have the format expected by this class
     * or you'll get an exception.
     */
    explicit GenericModel(size_t file_size, void *data, const Config &config = Config());
    ~GenericModel();

    /* Score p(new_word | in_state) and incorporate new_word into out_state.
     * Note that in_state and out_state must be different references:
     * &in_state != &out_state.
     */
    FullScoreReturn FullScore(const State &in_state, const WordIndex new_word, State &out_state) const;

  private:
    FullScoreReturn ScoreExceptBackoff(const WordIndex *const context_rbegin, const WordIndex *const context_rend, const WordIndex new_word, State &out_state) const;

    // Score bigrams and above.  Do not include backoff.
    void ResumeScore(const WordIndex *context_rbegin, const WordIndex *const context_rend, unsigned char starting_order_minus_2, typename Search::Node &node, float *backoff_out, unsigned char &next_use, FullScoreReturn &ret) const;

    // Appears after Size in the cc file.
    void SetupMemory(void *start, const std::vector<uint64_t> &counts, const Config &config);

    VocabularyT vocab_;

    Search search_;

    uint8_t *readen_content;
};

} // namespace detail

class ProbingModel : public detail::GenericModel<detail::HashedSearch<BackoffValue>, ProbingVocabulary> {
public:
    ProbingModel(size_t file_size, void *data, const Config &config = Config())
    :
        detail::GenericModel<detail::HashedSearch<BackoffValue>, ProbingVocabulary>(file_size, data, config)
    {
    }
};

} // namespace ngram
} // namespace lm

#endif // LM_MODEL_H
