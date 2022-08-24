#include "mistream.h"
#include <iostream>

using namespace mistream;

inline uint16_t EndianCorrectedStream::bswap16(uint16_t w) {
#if defined(__GNUC__)
  return __builtin_bswap16(w);
#elif defined(_MSC_VER)
  return _byteswap_ushort(w);
#endif
}

inline uint32_t EndianCorrectedStream::bswap32(uint32_t i) {
#if defined(__GNUC__)
  return __builtin_bswap32(i);
#elif defined(_MSC_VER)
  return _byteswap_ulong(i);
#endif
}

inline uint64_t EndianCorrectedStream::bswap64(uint64_t l) {
#if defined(__GNUC__)
  return __builtin_bswap64(l);
#elif defined(_MSC_VER)
  return _byteswap_uint64(l);
#endif
}

EndianCorrectedStream::EndianCorrectedStream(const std::string &filePath,
                                             std::ios_base::openmode mode)
    : ts_stream_(filePath, mode) {
  if ((mode & std::ios_base::out) == std::ios_base::out) {
    WriteEndiannessSignature();
  } else if ((mode & std::ios_base::in) == std::ios_base::in) {
    DetermineByteOrder();
    if (order_word_ + order_integer_ + order_double_ > 3)
      throw IncompatibleEncoding();
  }
}

void EndianCorrectedStream::DetermineByteOrder() {
  order_word_ = DetermineOrder<uint16_t>(kPatternWord);
  order_integer_ = DetermineOrder<int>(kPatternInteger);
  order_double_ = DetermineOrder<double>(kPatternDouble);
}

EndianCorrectedStream &EndianCorrectedStream::operator<<(uint16_t word) {
  if (order_word_) word = bswap16(word);
  ts_stream_.write(reinterpret_cast<char *>(&word), sizeof(uint16_t));
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator<<(int i) {
  if (order_integer_) i = bswap32(i);
  ts_stream_.write(reinterpret_cast<char *>(&i), sizeof(int));
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator<<(double d) {
  if (order_double_) d = bswap64(d);
  ts_stream_.write(reinterpret_cast<char *>(&d), sizeof(double));
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator<<(
    const std::string &str) {
  uint8_t len = (uint8_t)str.length();
  ts_stream_.write(reinterpret_cast<char *>(&len), 1);
  ts_stream_.write(str.c_str(), len);
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator<<(uint8_t b) {
  ts_stream_.write(reinterpret_cast<char *>(&b), 1);
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator<<(int64_t l) {
  if (order_integer_) l = bswap64(l);
  ts_stream_.write(reinterpret_cast<char *>(&l), sizeof(int64_t));
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator>>(uint8_t &b) {
  uint8_t loc;
  ts_stream_.read(reinterpret_cast<char *>(&loc), 1);
  b = loc;
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator>>(uint16_t &word) {
  uint16_t loc;
  ts_stream_.read(reinterpret_cast<char *>(&loc), sizeof(uint16_t));
  if (order_word_) loc = bswap16(loc);
  word = loc;
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator>>(int &i) {
  int loc;
  ts_stream_.read(reinterpret_cast<char *>(&loc), sizeof(int));
  if (order_integer_) loc = bswap32(loc);
  i = loc;
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator>>(double &d) {
  double loc;
  ts_stream_.read(reinterpret_cast<char *>(&loc), sizeof(double));
  if (order_double_) loc = bswap64(loc);
  d = loc;
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator>>(std::string &str) {
  uint8_t buf[256] = {0};
  uint8_t len;
  this->operator>>(len);
  ts_stream_.read(reinterpret_cast<char *>(buf), len);
  str.append(reinterpret_cast<char *>(buf));
  return *this;
}

EndianCorrectedStream &EndianCorrectedStream::operator>>(int64_t &l) {
  int64_t loc;
  ts_stream_.read(reinterpret_cast<char *>(&loc), sizeof(int64_t));
  if (order_integer_) loc = bswap64(loc);
  l = loc;
  return *this;
}

template <class T>
uint8_t EndianCorrectedStream::DetermineOrder(T pattern) {
  uint8_t size;
  ts_stream_.read(reinterpret_cast<char *>(&size), 1);
  if (size != sizeof(T)) return kPatternBadSize;

  uint8_t order = 0;
  T data;
  ts_stream_.read(reinterpret_cast<char *>(&data), sizeof(data));
  if (data != pattern) {
    order = 1;
    if (sizeof(T) == 2 && bswap16(data) != pattern)
      return kPatternBadOrder;
    else if (sizeof(T) == 4 && static_cast<T>(bswap32(data)) != pattern)
      return kPatternBadOrder;
    else if (sizeof(T) == 8 && static_cast<T>(bswap64(data)) != pattern)
      return kPatternBadOrder;
  }
  return order;
}

// Writes a signature that can be used to determine
// endianness of the current machine
// result file is compatible with current machine
// so all orders are set to 0(compatible)
void EndianCorrectedStream::WriteEndiannessSignature() {
  order_word_ = 0;
  order_integer_ = 0;
  order_double_ = 0;

  (this->operator<<((uint8_t)sizeof(uint16_t)))
      << kPatternWord << (uint8_t)sizeof(int) << kPatternInteger
      << (uint8_t)sizeof(double) << kPatternDouble;
}
