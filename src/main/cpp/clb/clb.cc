#include "lm/model.hh"
#include "lm/vocab.hh" // just for _misc

#include "util/string_piece.hh"

#include <iostream>

extern "C" {

#ifdef _MSC_VER
#define FEXPORT __declspec(dllexport)
#else
#define FEXPORT __attribute__((visibility("default")))
#endif

FEXPORT void
kenlm_misc() {
    char my_cstr [] = "a bc d"; // "a"
    StringPiece piece(my_cstr);

    std::cout << piece << std::endl;

    StringPiece::size_type prev_pos = 0;
    StringPiece::size_type pos;
    do {
        pos = piece.find_first_of(' ', prev_pos);
        std::cout << piece.substr(prev_pos, pos - prev_pos) << std::endl;
        prev_pos = pos + 1;
    }
    while (pos != StringPiece::npos);
}

FEXPORT void *
kenlm_init(size_t size, void *data, size_t ex_msg_size, char *ex_msg) {
    lm::ngram::ProbingModel *pModel = NULL;
    try {
        pModel = new lm::ngram::ProbingModel(size, data);
    } catch (const std::exception &ex) {
        if (ex_msg) {
            const char *what = ex.what();
            if (what) {
                size_t len = std::min(strlen(what), (size_t)(ex_msg_size - 1));
                memcpy(ex_msg, what, len + 1);
            }
        }
    }
    return pModel;
}

FEXPORT void
kenlm_clean(void *pHandle) {
    lm::ngram::ProbingModel *pModel = reinterpret_cast<lm::ngram::ProbingModel *>(pHandle);
    try {
        std::cout << "cleaning a model" << std::endl;
        delete pModel;
    } catch (...) {
    }
}

// lm/ngram_query.hh

FEXPORT float
kenlm_query(void *pHandle, const char *pTag) {
    float total = 0.0;
    if (!pHandle) {
        return total;
    }
    try {
        lm::ngram::ProbingModel *pModel = reinterpret_cast<lm::ngram::ProbingModel *>(pHandle);
        StringPiece piece(pTag);

        lm::ngram::ProbingModel::State out;
        lm::ngram::ProbingModel::State state = pModel->BeginSentenceState(); // : model.NullContextState(); if !sentence_context

        StringPiece::size_type prev_pos = 0;
        StringPiece::size_type pos;
        do {
            pos = piece.find_first_of(' ', prev_pos);
            StringPiece word(piece.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + 1;

            lm::WordIndex vocab = pModel->GetVocabulary().Index(word); // can hang !!!
            lm::FullScoreReturn ret = pModel->FullScore(state, vocab, out);
            total += ret.prob;
            state = out;
        }
        while (pos != StringPiece::npos);

        lm::FullScoreReturn ret = pModel->FullScore(state, pModel->GetVocabulary().EndSentence(), out);
        total += ret.prob;
    } catch (...) {
        total = 0.0;
    }
    return total;
}

}
