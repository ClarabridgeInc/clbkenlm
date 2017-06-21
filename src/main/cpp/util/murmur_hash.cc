/* Downloaded from http://sites.google.com/site/murmurhash/ which says "All
 * code is released to the public domain. For business purposes, Murmurhash is
 * under the MIT license."
 * This is modified from the original:
 * ULL tag on 0xc6a4a7935bd1e995 so this will compile on 32-bit.
 * length changed to unsigned int.
 * placed in namespace util
 * add MurmurHashNative
 * default option = 0 for seed
 * ARM port from NICT
 */

#include "util/murmur_hash.hh"

namespace util {

//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms

uint64_t MurmurHash64A ( const void * key, std::size_t len, uint64_t seed )
{
  const uint64_t m = 0xc6a4a7935bd1e995ULL;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

#if defined(__arm) || defined(__arm__)
  const size_t ksize = sizeof(uint64_t);
  const unsigned char * data = (const unsigned char *)key;
  const unsigned char * end = data + (std::size_t)(len/8) * ksize;
#else
  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);
#endif

  while(data != end)
  {
#if defined(__arm) || defined(__arm__)
    uint64_t k;
    memcpy(&k, data, ksize);
    data += ksize;
#else
    uint64_t k = *data++;
#endif

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
  case 7: h ^= uint64_t(data2[6]) << 48;
  case 6: h ^= uint64_t(data2[5]) << 40;
  case 5: h ^= uint64_t(data2[4]) << 32;
  case 4: h ^= uint64_t(data2[3]) << 24;
  case 3: h ^= uint64_t(data2[2]) << 16;
  case 2: h ^= uint64_t(data2[1]) << 8;
  case 1: h ^= uint64_t(data2[0]);
          h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

} // namespace util
