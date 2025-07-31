#include <cassert>
#include <cstring>
#include <ostream>

#include "gdlib/utils.hpp"
#include "short_string.hpp"

// #define ENABLE_ASSERTIONS

namespace library {

ShortString::ShortString() {
  buffer[0] = '\0';
}

ShortString::ShortString(const char *s) {
  const size_t s_length{std::strlen(s)};

#if defined(ENABLE_ASSERTIONS)
  assert(s_length < GMS_SSSIZE);
#endif

  std::memcpy(buffer.data(), s, s_length);
  buffer[s_length] = '\0';
}

ShortString::ShortString(const std::string &s)
    : ShortString(s.data()) {}

char *ShortString::data() {
  return buffer.data();
}

const char *ShortString::data() const {
  return buffer.data();
}

std::string ShortString::string() const {
  return buffer.data();
}

ShortString::operator std::string() const {
  return buffer.data();
}

uint8_t ShortString::length() const {
  return static_cast<uint8_t>(strlen(buffer.data()));
}

char ShortString::front() const {
#if defined(ENABLE_ASSERTIONS)
  assert(buffer[0] != '\0');
#endif

  return buffer[0];
}

char ShortString::back() const {
  const uint8_t length{this->length()};

#if defined(ENABLE_ASSERTIONS)
  assert(length > 0);
#endif

  return buffer[length - 1];
}

char ShortString::at(const uint8_t i) const {
#if defined(ENABLE_ASSERTIONS)
  const uint8_t length{this->length()};

  if (i > 0) {
    assert(i < length);
  } else {
    assert(i <= length);
  }
#endif

  return buffer[i];
}

char ShortString::operator[](const uint8_t i) const {
  return buffer[i];
}

bool ShortString::empty() const {
  return buffer[0] == '\0';
}

void ShortString::clear() {
  buffer[0] = '\0';
}

void ShortString::append(const char c) {
  const uint8_t length{this->length()};

#if defined(ENABLE_ASSERTIONS)
  assert(length + 1 < GMS_SSSIZE);
#endif

  buffer[length] = c;
  buffer[length + 1] = '\0';
}

void ShortString::append(const char *s) {
  const uint8_t length{this->length()};
  const size_t s_length{std::strlen(s)};

#if defined(ENABLE_ASSERTIONS)
  assert(length + s_length < GMS_SSSIZE);
#endif

  std::memcpy(buffer.data() + length, s, s_length);
  buffer[length + s_length] = '\0';
}

void ShortString::append(const ShortString &s) {
  append(s.data());
}

void ShortString::append(const std::string &s) {
  append(s.data());
}

void ShortString::operator+=(const char c) {
  append(c);
}

void ShortString::operator+=(const char *s) {
  append(s);
}

void ShortString::operator+=(const ShortString &s) {
  append(s);
}

void ShortString::operator+=(const std::string &s) {
  append(s);
}

void ShortString::to_upper_case() {
  for (uint8_t i{}; buffer[i] != '\0'; i++) {
    buffer[i] = utils::toupper(buffer[i]);
  }
}

ShortString &ShortString::operator=(const std::string &s) {
  const size_t s_length{s.length()};

#if defined(ENABLE_ASSERTIONS)
  assert(s_length < GMS_SSSIZE);
#endif

  std::memcpy(buffer.data(), s.data(), s_length);
  buffer[s_length] = '\0';
  return *this;
}

bool ShortString::operator==(const char *s) const {
  return strcmp(buffer.data(), s) == 0;
}

bool ShortString::operator==(const ShortString &s) const {
  return *this == s.data();
}

bool ShortString::operator==(const std::string &s) const {
  return *this == s.data();
}

bool ShortString::operator<(const ShortString &s) const {
  return strcmp(buffer.data(), s.data()) < 0;
}

std::ostream &operator<<(std::ostream &os, const ShortString &s) {
  return os << s.data();
}

std::string operator+(const std::string &lhs, const ShortString &rhs) {
  return lhs + rhs.data();
}

std::string operator+(const ShortString &lhs, const std::string &rhs) {
  return lhs.data() + rhs;
}

std::string operator+(const char lhs, const ShortString &rhs) {
  return lhs + rhs.string();
}

std::string operator+(const ShortString &lhs, const char rhs) {
  return lhs.string() + rhs;
}

} // namespace library
