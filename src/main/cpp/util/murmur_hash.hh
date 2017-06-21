#ifndef UTIL_MURMUR_HASH_H
#define UTIL_MURMUR_HASH_H
#include <cstddef>
#include <stdint.h>

namespace util {

// 64-bit machine version
uint64_t MurmurHash64A(const void * key, std::size_t len, uint64_t seed = 0);

} // namespace util

#endif // UTIL_MURMUR_HASH_H
