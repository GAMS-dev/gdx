#pragma once
#include <string>
#include "twostream.h"

namespace mistream {

class IncompatibleEncoding : public std::exception {
 public:
  const char *what() { return "Encoding of the file could not be understood"; }
};

class EndianCorrectedStream {
 public:
  static const uint8_t kPatternBadSize = 255;
  static const uint8_t kPatternBadOrder = 254;
  static constexpr uint16_t kPatternWord = 0x1234;
  static constexpr int32_t kPatternInteger = 0x12345678;
  static constexpr double kPatternDouble = 3.1415926535897932385;

  EndianCorrectedStream(const std::string &file_path,
                        std::ios_base::openmode mode);
  EndianCorrectedStream &operator<<(uint8_t b);
  EndianCorrectedStream &operator<<(uint16_t word);
  EndianCorrectedStream &operator<<(int32_t i);
  EndianCorrectedStream &operator<<(double d);
  EndianCorrectedStream &operator<<(const std::string &str);
  EndianCorrectedStream &operator<<(int64_t l);

  EndianCorrectedStream &operator>>(uint8_t &b);
  EndianCorrectedStream &operator>>(uint16_t &word);
  EndianCorrectedStream &operator>>(int32_t &i);
  EndianCorrectedStream &operator>>(double &d);
  EndianCorrectedStream &operator>>(std::string &str);
  EndianCorrectedStream &operator>>(int64_t &l);

  inline int64_t Tell() { return ts_stream_.tellp(); }
  inline void Seek(int64_t p) { ts_stream_.seekp(p); }
  inline void SetCompression(bool use_compression) {
    ts_stream_.SetCompression(use_compression);
  }

 private:
  twostream::TwoSourceStream ts_stream_;
  uint8_t order_word_;
  uint8_t order_integer_;
  uint8_t order_double_;
  void DetermineByteOrder();
  template <class T>
  uint8_t DetermineOrder(T pattern);
  void WriteEndiannessSignature();
  static inline uint16_t bswap16(uint16_t);
  static inline uint32_t bswap32(uint32_t);
  static inline uint64_t bswap64(uint64_t);
};

}  // namespace mistream
