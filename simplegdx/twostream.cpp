#include "twostream.h"
#include <iostream>
#include <ios>
#include "../gdlib/xcompress.h"
using namespace twostream;

TwoSourceStream::TwoSourceStream(const std::string &filePath,
                                 std::ios_base::openmode mode)
    : fstream_(filePath, mode),
      mode_(mode),
      read_(0),
      write_(0),
      uncompressed_buffer_size_(0),
      compression_used_(false) {}

void TwoSourceStream::read(char *buf, std::streamsize size) {
  if (read_ == uncompressed_buffer_size_) {
    if (!compression_used_) {
      fstream_.read(buf, size);
    } else {
      UncompressAndLoad();
      read(buf, size);
    }
  } else if (read_ < uncompressed_buffer_size_) {
    auto beg = reinterpret_cast<char *>(&uncompressed_buffer[read_]);
    auto end =
        std::min(reinterpret_cast<char *>(&uncompressed_buffer[read_ + size]),
                 reinterpret_cast<char *>(
                     &uncompressed_buffer[uncompressed_buffer_size_]));
    std::copy(beg, end, buf);
    auto left = static_cast<int64_t>(uncompressed_buffer_size_) -
                static_cast<int64_t>(read_ + size);
    if (left < 0) {
      if (compression_used_) UncompressAndLoad();
      auto diff = end - beg;
      read(diff + buf, std::abs(left));
    } else {
      read_ += size;
    }
  }
}

void TwoSourceStream::write(const char *buf, std::streamsize size) {
  if (!compression_used_) {
    fstream_.write(buf, size);
  } else {
    auto free_space = buffer_size - write_;
    if (free_space >= size) {
      std::copy(buf, buf + size, uncompressed_buffer + write_);
      write_ += size;
    } else {
      CompressAndSave();
      write(buf, size);
    }
  }
}

int64_t TwoSourceStream::tellp() { return fstream_.tellp(); }

void TwoSourceStream::seekp(int64_t p) {
  fstream_.seekp(p);
  uncompressed_buffer_size_ = 0;
  read_ = 0;
}

void TwoSourceStream::SetCompression(bool use_compression) {
  if (compression_used_ && !use_compression &&
      (mode_ & std::ios_base::out) ==
          std::ios_base::out &&
      write_ > 0) {
    // we set compression to false when we want to flush

    CompressAndSave();
  }
  compression_used_ = use_compression;
}

void TwoSourceStream::UncompressAndLoad() {
  uint8_t compressHeader[3];
  fstream_.read(reinterpret_cast<char *>(compressHeader), 3);

  uint16_t s = (compressHeader[1] << 8) | compressHeader[2];
  if (compressHeader[0] != 0u) {
    fstream_.read(reinterpret_cast<char *>(compressed_buffer), s);
    uncompressed_buffer_size_ = buffer_size;
    read_ = 0;
    int result = gdlib::xcompress::uncompress(uncompressed_buffer, uncompressed_buffer_size_,
                            compressed_buffer, s);
    if (result != 0) {
      // we could not get our data, something is wrong
      throw twostream::CompressionError(result, false);
    }
  } else {
    fstream_.read(reinterpret_cast<char *>(uncompressed_buffer), s);
    read_ = 0;
    uncompressed_buffer_size_ = s;
  }
}

void TwoSourceStream::CompressAndSave() {
  compressed_buffer_size_ = buffer_size;
  int result = gdlib::xcompress::compress(compressed_buffer, compressed_buffer_size_,
                        uncompressed_buffer, write_);

  if (result == 0) {
    uint16_t c_buf_size = compressed_buffer_size_;

    uint8_t compression_header[3];
    compression_header[0] = 1;
    compression_header[1] = c_buf_size >> 8;
    compression_header[2] = c_buf_size & static_cast<uint16_t>(0xFF);

    fstream_.write(reinterpret_cast<char *>(compression_header), 3);

    fstream_.write(reinterpret_cast<char *>(compressed_buffer),
                   compressed_buffer_size_);
    write_ = 0;
  } else {
    uint16_t buf_size = write_;
    uint8_t compression_header[3];
    compression_header[0] = 0;
    compression_header[1] = buf_size >> 8;
    compression_header[2] = buf_size & static_cast<uint16_t>(0xFF);
    fstream_.write(reinterpret_cast<char *>(compression_header), 3);
    fstream_.write(reinterpret_cast<char *>(uncompressed_buffer), write_);
    write_ = 0;
  }
}
