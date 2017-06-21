#include "lm/lm_exception.hh"

#include <cerrno>
#include <cstdio>

namespace lm {

LoadException::LoadException() throw() {}
LoadException::~LoadException() throw() {}

FormatLoadException::FormatLoadException() throw() {}
FormatLoadException::~FormatLoadException() throw() {}

} // namespace lm
