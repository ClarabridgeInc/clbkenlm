#include "util/exception.hh"

#ifdef __GXX_RTTI
#include <typeinfo>
#endif

namespace util {

Exception::Exception() throw() {}
Exception::~Exception() throw() {}

void Exception::SetLocation(const char *file, unsigned int line, const char *func, const char *child_name, const char *condition) {
  /* The child class might have set some text, but we want this to come first.
   * Another option would be passing this information to the constructor, but
   * then child classes would have to accept constructor arguments and pass
   * them down.
   */
  std::string old_text;
  what_.swap(old_text);
  what_ << file << ':' << line;
  if (func) what_ << " in " << func << " threw ";
  if (child_name) {
    what_ << child_name;
  } else {
#ifdef __GXX_RTTI
    what_ << typeid(this).name();
#else
    what_ << "an exception";
#endif
  }
  if (condition) {
    what_ << " because `" << condition << '\'';
  }
  what_ << ".\n";
  what_ << old_text;
}

OverflowException::OverflowException() throw() {}
OverflowException::~OverflowException() throw() {}

} // namespace util
