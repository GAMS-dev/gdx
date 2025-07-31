#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "generated/gclgms.h"

namespace library {

class ShortString {
  std::array<char, GMS_SSSIZE> buffer{};

public:
  ShortString();
  explicit ShortString(const char *s);
  explicit ShortString(const std::string &s);

  char *data();
  [[nodiscard]] const char *data() const;
  [[nodiscard]] std::string string() const;
  [[nodiscard]] operator std::string() const;

  [[nodiscard]] uint8_t length() const;

  [[nodiscard]] char front() const;
  [[nodiscard]] char back() const;

  [[nodiscard]] char at(uint8_t i) const;
  [[nodiscard]] char operator[](uint8_t i) const;

  [[nodiscard]] bool empty() const;

  void clear();

  void append(char c);
  void append(const char *s);
  void append(const ShortString &s);
  void append(const std::string &s);

  void operator+=(char c);
  void operator+=(const char *s);
  void operator+=(const ShortString &s);
  void operator+=(const std::string &s);

  void to_upper_case();

  ShortString &operator=(const std::string &s);

  [[nodiscard]] bool operator==(const char *s) const;
  [[nodiscard]] bool operator==(const ShortString &s) const;
  [[nodiscard]] bool operator==(const std::string &s) const;

  [[nodiscard]] bool operator<(const ShortString &s) const;

  friend std::ostream &operator<<(std::ostream &os, const ShortString &s);

  friend std::string operator+(const std::string &lhs, const ShortString &rhs);
  friend std::string operator+(const ShortString &lhs, const std::string &rhs);

  friend std::string operator+(char lhs, const ShortString &rhs);
  friend std::string operator+(const ShortString &lhs, char rhs);
};

} // namespace library
