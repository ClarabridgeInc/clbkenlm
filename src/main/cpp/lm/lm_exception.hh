#ifndef LM_LM_EXCEPTION_H
#define LM_LM_EXCEPTION_H

// Named to avoid conflict with util/exception.hh.

#include "util/exception.hh"
#include "util/string_piece.hh"

#include <exception>
#include <string>

namespace lm {

class LoadException : public util::Exception {
   public:
      virtual ~LoadException() throw();

   protected:
      LoadException() throw();
};

class FormatLoadException : public LoadException {
  public:
    FormatLoadException() throw();
    ~FormatLoadException() throw();
};

} // namespace lm

#endif // LM_LM_EXCEPTION
