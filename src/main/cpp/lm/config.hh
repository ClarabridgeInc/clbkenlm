#ifndef LM_CONFIG_H
#define LM_CONFIG_H

#include <stdint.h>

// This is a macro instead of an inline function so constants can be assigned using it.
#define ALIGN8(a) ((std::ptrdiff_t(((a)-1)/8)+1)*8)

/* Configuration for ngram model.  Separate header to reduce pollution. */

namespace lm {
namespace ngram {

struct Config {
  // Size multiplier for probing hash table.  Must be > 1.  Space is linear in
  // this.  Time is probing_multiplier / (probing_multiplier - 1).  No effect
  // for sorted variant.
  // If you find yourself setting this to a low number, consider using the
  // TrieModel which has lower memory consumption.
  float probing_multiplier;

  // Set defaults.
  Config();
};

} /* namespace ngram */
} /* namespace lm */

#endif // LM_CONFIG_H
