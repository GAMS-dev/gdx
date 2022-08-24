#pragma once
#include <fstream>
#include <iostream>
#include <string>

namespace twostream {
struct CompressionError : public std::exception {
  int error_code;
  bool compressing;
  const char *what() {
    return reinterpret_cast<const char *>("Error while using compression");
  }

  CompressionError(int errCode, bool compress)
      : error_code(errCode), compressing(compress) {}
};

class TwoSourceStream {
 private:
  static constexpr uint16_t buffer_size = 32768;

 private:
  std::fstream fstream_;
  std::ios_base::openmode mode_;
  uint8_t uncompressed_buffer[buffer_size];
  uint8_t compressed_buffer[buffer_size];
  uint32_t read_;
  uint32_t write_;
  unsigned long uncompressed_buffer_size_;
  unsigned long compressed_buffer_size_;
  bool compression_used_;
  void UncompressAndLoad();
  void CompressAndSave();

 public:
  TwoSourceStream(const std::string &filePath, std::ios_base::openmode mode);
  void read(char *s, std::streamsize n);
  void write(const char *s, std::streamsize n);
  int64_t tellp();
  void seekp(int64_t p);
  void SetCompression(bool useCompression);
};

}  // namespace twostream
