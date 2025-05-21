#ifndef GDX_SHORT_STRING_H
#define GDX_SHORT_STRING_H

#include <array>
#include <cstdint>
#include <string>

#include "generated/gclgms.h"

namespace library {

class ShortString_t {
  std::array<char, GMS_SSSIZE> buffer;

public:
  ShortString_t();
  explicit ShortString_t(const char *s);
  explicit ShortString_t(const std::string &s);

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
  void append(const ShortString_t &s);
  void append(const std::string &s);

  void operator+=(char c);
  void operator+=(const char *s);
  void operator+=(const ShortString_t &s);
  void operator+=(const std::string &s);

  void to_upper_case();

  ShortString_t &operator=(const std::string &s);

  [[nodiscard]] bool operator==(const char *s) const;
  [[nodiscard]] bool operator==(const ShortString_t &s) const;
  [[nodiscard]] bool operator==(const std::string &s) const;

  [[nodiscard]] bool operator<(const ShortString_t &s) const;

  friend std::ostream &operator<<(std::ostream &os, const ShortString_t &s);

  friend std::string operator+(const std::string &lhs, const ShortString_t &rhs);
  friend std::string operator+(const ShortString_t &lhs, const std::string &rhs);

  friend std::string operator+(char lhs, const ShortString_t &rhs);
  friend std::string operator+(const ShortString_t &lhs, char rhs);
};

} // namespace library

#endif // GDX_SHORT_STRING_H
