// Copyright 2022 VM75. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace TinyJson {

class JsonError : public std::string
{
private:
  JsonError(std::string &&val) noexcept : std::string(std::move(val)){};

  friend class AbstractJsonValue;
};

class AbstractJsonValue
{
public:
  virtual ~AbstractJsonValue() = default;

  virtual void serialize(std::ostream &outStream) const noexcept = 0;

  std::string toString() const noexcept
  {
    std::stringstream ss;
    serialize(ss);
    return ss.str();
  }

protected:
  static inline bool isInRange(int val, int min, int max) noexcept { return val >= min && val <= max; }

  static inline bool isIn(const std::string &val, const std::vector<std::string_view> &list) noexcept
  {
    for (auto &entry : list) {
      if (entry.starts_with(val)) {
        return true;
      }
    }
    return false;
  }

  // Skip all whitespaces & comments
  static bool skipSpaces(std::istream &inStream) noexcept
  {
    constexpr std::string_view spaces{" \t\n\r"};
    while (!inStream.eof()) {
      auto nextChar = inStream.peek();
      if (nextChar == '/') {
        inStream.ignore();
        nextChar = inStream.peek();
        if (nextChar == '/') {
          while (!inStream.eof() && nextChar != '\n') {
            nextChar = inStream.get();
          }
        } else if (nextChar == '*') {
          inStream.ignore();
          while (!inStream.eof()) {
            nextChar = inStream.get();
            if (nextChar != '*') {
              continue;
            }
            if (inStream.peek() == '/') {
              nextChar = inStream.get();
              break;
            }
          }
        } else {
          inStream.unget();
          break;
        }
      } else if (spaces.find(inStream.peek()) != spaces.npos) {
        inStream.ignore();
      } else {
        break;
      }
    }

    return !inStream.eof();
  }

  static bool expectAndConsume(std::istream &inStream, char expected) noexcept
  {
    if (!skipSpaces(inStream) || inStream.peek() != expected) {
      return false;
    }
    inStream.ignore(); // Consume
    return skipSpaces(inStream);
  }

  static JsonError getError(std::string_view error, std::istream &is, const std::string &currentStr = "") noexcept
  {
    auto errorPos = is.tellg();
    if (errorPos == -1) {
      is.clear();
      is.seekg(0, is.end);
      errorPos = is.tellg();
    }

    errorPos -= currentStr.length();

    std::stringstream ss;
    ss << "Parse error at position " << errorPos << ": " << error;
    return JsonError(ss.str());
  }
};

template <bool b>
class JsonString_T;

template <bool b>
class JsonObject_T;

template <bool b>
class JsonArray_T;

template <bool b>
using ValueTypes = std::variant<void *, bool, int64_t, long double, JsonString_T<b>, JsonArray_T<b>, JsonObject_T<b>, JsonError>;
template <bool b>
using ReturnableValueTypes = std::variant<bool, int64_t, long double, JsonString_T<b>, JsonArray_T<b>, JsonObject_T<b>>;

template <bool b>
class JsonValue_T : public ValueTypes<b>, public AbstractJsonValue
{
private:
  using super = ValueTypes<b>;

public:
  static JsonValue_T<b> parse(std::istream &inStream, std::string &error) noexcept
  {
    skipSpaces(inStream);
    auto parseResult = parse(inStream);
    if (!parseResult.isValid()) {
      error = parseResult.asError();
      return JsonValue_T<b>();
    }
    return parseResult;
  }

  static JsonValue_T<b> parse(const std::string &str, std::string &error) noexcept
  {
    std::stringstream inStream(str);
    return parse(inStream, error);
  }

  static JsonValue_T<b> parse(std::string_view str, std::string &error) noexcept
  {
    std::stringstream inStream(std::string(str.begin(), str.end()));
    return parse(inStream, error);
  }

  JsonValue_T() noexcept : super(NullValue) {}
  JsonValue_T(bool value) noexcept : super(value) {}
  JsonValue_T(int64_t value) noexcept : super(value) {}
  JsonValue_T(long double value) noexcept : super(value) {}
  JsonValue_T(JsonString_T<b> value) noexcept : super(value) {}
  JsonValue_T(JsonArray_T<b> value) noexcept : super(value) {}
  JsonValue_T(JsonObject_T<b> value) noexcept : super(value) {}

  void serialize(std::ostream &outStream) const noexcept override
  {
    if (isNull()) {
      outStream << "null";
    } else if (isBool()) {
      outStream << (asBool() ? "true" : "false");
    } else if (isInt()) {
      outStream << asInt();
    } else if (isFloat()) {
      outStream << asFloat();
    } else if (isString()) {
      asString().serialize(outStream);
    } else if (isObject()) {
      asObject().serialize(outStream);
    } else if (isArray()) {
      asArray().serialize(outStream);
    }
  }

  inline bool isNull() const noexcept { return std::holds_alternative<void *>(*this); }
  inline bool isBool() const noexcept { return std::holds_alternative<bool>(*this); }
  inline bool isInt() const noexcept { return std::holds_alternative<int64_t>(*this); }
  inline bool isFloat() const noexcept { return std::holds_alternative<long double>(*this); }
  inline bool isString() const noexcept { return std::holds_alternative<JsonString_T<b>>(*this); }
  inline bool isArray() const noexcept { return std::holds_alternative<JsonArray_T<b>>(*this); }
  inline bool isObject() const noexcept { return std::holds_alternative<JsonObject_T<b>>(*this); }
  inline bool isValid() const noexcept { return !std::holds_alternative<JsonError>(*this); }

  inline bool asBool() const noexcept { return std::get<bool>(*this); }
  inline int64_t asInt() const noexcept { return std::get<int64_t>(*this); }
  inline long double asFloat() const noexcept { return std::get<long double>(*this); }
  inline JsonString_T<b> &asString() noexcept { return std::get<JsonString_T<b>>(*this); }
  inline const JsonString_T<b> &asString() const noexcept { return std::get<JsonString_T<b>>(*this); }
  inline JsonObject_T<b> &asObject() noexcept { return std::get<JsonObject_T<b>>(*this); }
  inline const JsonObject_T<b> &asObject() const noexcept { return std::get<JsonObject_T<b>>(*this); }
  inline JsonArray_T<b> &asArray() noexcept { return std::get<JsonArray_T<b>>(*this); }
  inline const JsonArray_T<b> &asArray() const noexcept { return std::get<JsonArray_T<b>>(*this); }
  inline JsonError &asError() noexcept { return std::get<JsonError>(*this); }

  // bool operator==(const JsonValue_T<b> &other) const noexcept
  // {
  //   if (this->index() != other.index()) {
  //     return false;
  //   }
  //   if (isBool()) {
  //     return asBool() == other.asBool();
  //   }
  //   if (isInt()) {
  //     return asInt() == other.asInt();
  //   }
  //   if (isFloat()) {
  //     return asFloat() == other.asFloat();
  //   }
  //   if (isString()) {
  //     return asString() == other.asString();
  //   }
  //   if (isObject()) {
  //     return asObject() == other.asObject();
  //   }
  //   if (isArray()) {
  //     return asArray() == other.asArray();
  //   }
  //   return true;
  // }

private:
  friend class JsonString_T<b>;
  friend class JsonObject_T<b>;
  friend class JsonArray_T<b>;
  JsonValue_T(JsonError value) : super(value) {}

  static JsonValue_T<b> parse(std::istream &inStream) noexcept
  {
    if (!skipSpaces(inStream)) {
      return getError("Unexpected end of JSON input", inStream);
    }

    auto nextChar = inStream.peek();
    std::string str{};
    if (nextChar == '[') {
      return JsonArray_T<b>::parse(inStream);
    } else if (nextChar == '{') {
      return JsonObject_T<b>::parse(inStream);
    } else if (nextChar == '"') {
      return JsonString_T<b>::parse(inStream);
    } else if (isInRange(nextChar, '0', '9') || nextChar == '-') {
      return parseNumber(inStream);
    } else {
      using namespace std::literals::string_view_literals;
      const std::vector<std::string_view> list = {"true"sv, "false"sv, "null"sv};

      str += inStream.get();
      while (isIn(str, list)) {
        if (inStream.eof()) {
          return getError("Unexpected end of JSON input", inStream, str);
        }
        str += inStream.get();
        if (str == "true") {
          return true;
        }
        if (str == "false") {
          return false;
        }
        if (str == "null") {
          return NullValue;
        }
      }
    }
    return getError("Unexpected token in JSON", inStream, str);
  }

  static bool parseDigits(std::istream &inStream, std::string &str) noexcept
  {
    size_t initialLen{str.length()};
    while (!inStream.eof() && (isInRange(inStream.peek(), '0', '9'))) {
      str += inStream.get();
    }
    return str.length() > initialLen;
  }

  static JsonValue_T<b> parseNumber(std::istream &inStream) noexcept
  {
    std::string str{};
    if (inStream.peek() == '-') {
      str += inStream.get();
    }

    // leading int
    if (!parseDigits(inStream, str) || (str.length() > 2 && str.starts_with("-0")) || (str.length() > 1 && str.starts_with("0"))) {
      return getError("Error in JSON number", inStream, str);
    }

    // integer
    if (inStream.peek() != '.') {
      return static_cast<int64_t>(std::stoll(str));
    }

    // decimal
    str += inStream.get();
    if (!parseDigits(inStream, str)) {
      return getError("Error in JSON number", inStream, str);
    }

    // exponent
    if (inStream.peek() == 'e' || inStream.peek() == 'E') {
      str += inStream.get();
      if (!parseDigits(inStream, str)) {
        return getError("Error in JSON number", inStream, str);
      }
    }
    return std::stold(str);
  }

  static constexpr void *NullValue{nullptr};
};

template <bool b>
class JsonString_T : public std::string, public AbstractJsonValue
{
public:
  JsonString_T(const std::string &value) : std::string(value) {}
  JsonString_T(std::string &&value) : std::string(std::move(value)) {}
  JsonString_T(const char *value) : std::string(value) {}

  void serialize(std::ostream &outStream) const noexcept override
  {
    outStream << '"';
    int index = 0;
    while (index < this->length()) {
      auto ch = this->at(index++);
      if (ch == '\n') {
        outStream << "\\n";
      } else if (ch == '\r') {
        outStream << "\\r";
      } else if (ch == '\t') {
        outStream << "\\t";
      } else if (ch == '\b') {
        outStream << "\\b";
      } else if (ch == '\f') {
        outStream << "\\f";
      } else if (ch == '"') {
        outStream << "\\\"";
      } else if (static_cast<uint8_t>(ch) <= 0x1f) {
        char buf[8];
        snprintf(buf, sizeof buf, "\\u%04x", ch);
        outStream << buf;
      } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(this->at(index + 1)) == 0x80 && static_cast<uint8_t>(this->at(index + 2)) == 0xa8) {
        outStream << "\\u2028";
        index += 2;
      } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(this->at(index + 1)) == 0x80 && static_cast<uint8_t>(this->at(index + 2)) == 0xa9) {
        outStream << "\\u2029";
        index += 2;
      } else {
        outStream << ch;
      }
    }
    outStream << '"';
  }

  static JsonValue_T<b> parse(std::istream &inStream) noexcept
  {
    if (!expectAndConsume(inStream, '"')) {
      return getError("Expecting \" for JSON string", inStream);
    }

    long codepointRemainder = -1;
    std::string str;
    while (!inStream.eof()) {
      auto nextChar = inStream.get();
      if (inStream.eof()) {
        break;
      }
      if (nextChar == '"') {
        encodeToUtf8(codepointRemainder, str);
        return JsonString_T<b>(str);
      }
      if (isInRange(nextChar, 0, 0x1f)) {
        return getError("Unescaped char in JSON string", inStream);
      }

      if (nextChar != '\\') {
        encodeToUtf8(codepointRemainder, str);
        str += nextChar;
        continue;
      }

      // escaped char
      inStream.ignore(); // Consume '\'
      if (inStream.eof()) {
        return getError("Unescaped end of JSON input", inStream);
      }

      nextChar = inStream.get();
      if (nextChar != 'u') { // escaped char other than u
        encodeToUtf8(codepointRemainder, str);

        nextChar = inStream.get();
        if (nextChar == 'n') {
          str += '\n';
        } else if (nextChar == 'r') {
          str += '\r';
        } else if (nextChar == 't') {
          str += '\t';
        } else if (nextChar == 'b') {
          str += '\b';
        } else if (nextChar == 'f') {
          str += '\f';
        } else {
          str += nextChar;
        }
        continue;
      }

      char code[5]{0, 0, 0, 0, 0};
      for (int i = 0; i < 4; i++) {
        if (inStream.eof()) {
          return getError("Unescaped end of JSON input", inStream);
        }
        auto c = inStream.get();
        if (!isInRange(c, 'a', 'f') && !isInRange(c, 'A', 'F') && !isInRange(c, '0', '9')) {
          return getError("Invalid unicode escape in JSON", inStream);
        }
      }
      auto codepoint = std::strtol(code, nullptr, 16);

      // JSON specifies that characters outside the BMP shall be encoded as a pair
      // of 4-hex-digit \u escapes encoding their surrogate pair components. Check
      // whether we're in the middle of such a beast: the previous codepoint was an
      // escaped lead (high) surrogate, and this is a trail (low) surrogate.
      if (isInRange(codepointRemainder, 0xD800, 0xDBFF) && isInRange(codepoint, 0xDC00, 0xDFFF)) {
        // Reassemble the two surrogate pairs into one astral-plane character, per
        // the UTF-16 algorithm.
        codepointRemainder = (((codepointRemainder - 0xD800) << 10) | (codepoint - 0xDC00)) + 0x10000;
        encodeToUtf8(codepointRemainder, str);
      } else {
        encodeToUtf8(codepointRemainder, str);
        codepointRemainder = codepoint;
      }
    }
    return getError("Unexpected end of JSON input", inStream);
  }

private:
  /* encodeToUtf8(ch, str)
   *
   * Encode ch as UTF-8 and add it to str.
   */
  static void encodeToUtf8(long &ch, std::string &str)
  {
    if (ch < 0)
      return;

    if (ch < 0x80) {
      str += static_cast<char>(ch);
    } else if (ch < 0x800) {
      str += static_cast<char>((ch >> 6) | 0xC0);
      str += static_cast<char>((ch & 0x3F) | 0x80);
    } else if (ch < 0x10000) {
      str += static_cast<char>((ch >> 12) | 0xE0);
      str += static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
      str += static_cast<char>((ch & 0x3F) | 0x80);
    } else {
      str += static_cast<char>((ch >> 18) | 0xF0);
      str += static_cast<char>(((ch >> 12) & 0x3F) | 0x80);
      str += static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
      str += static_cast<char>((ch & 0x3F) | 0x80);
    }

    ch = -1;
  }
};

template <bool b>
class JsonObject_T : public std::map<JsonString_T<b>, JsonValue_T<b>>, public AbstractJsonValue
{
public:
  void serialize(std::ostream &outStream) const noexcept override
  {
    bool first = true;
    outStream << '{';
    for (const auto &entry : *this) {
      if (!first) {
        outStream << ',';
      }
      first = false;
      outStream << '"';
      outStream << entry.first;
      outStream << '"';
      outStream << ':';
      entry.second.serialize(outStream);
    }
    outStream << '}';
  }

  static JsonValue_T<b> parse(std::istream &inStream) noexcept
  {
    if (!expectAndConsume(inStream, '{')) {
      return getError("Expecting { for JSON object", inStream);
    }
    JsonObject_T<b> object;
    bool first = true;
    while (skipSpaces(inStream) && inStream.peek() != '}') {
      if (!first) {
        if (!expectAndConsume(inStream, ',')) {
          break;
        }
      }
      first = false;
      auto key = JsonString_T<b>::parse(inStream);
      if (!key.isValid()) {
        return key; // Error
      }
      if (!expectAndConsume(inStream, ':')) {
        return getError("Expecting ':' here", inStream);
      }
      auto parsedValue = JsonValue_T<b>::parse(inStream);
      if (!parsedValue.isValid()) {
        return parsedValue; // Error
      }
      object.emplace(std::make_pair(std::move(key.asString()), std::move(parsedValue)));
    }

    if (inStream.peek() != '}') {
      return getError("Expecting '}' here", inStream);
    }

    inStream.ignore(); // Consume '}'
    return object;
  }
};

template <bool b>
class JsonArray_T : public std::vector<JsonValue_T<b>>, public AbstractJsonValue
{
public:
  void serialize(std::ostream &outStream) const noexcept override
  {
    bool first = true;
    outStream << '[';
    for (auto &entry : *this) {
      if (!first) {
        outStream << ',';
      }
      first = false;
      entry.serialize(outStream);
    }
    outStream << ']';
  }

  static JsonValue_T<b> parse(std::istream &inStream) noexcept
  {
    if (!expectAndConsume(inStream, '[')) {
      return getError("Expecting [ for JSON array", inStream);
    }

    skipSpaces(inStream);
    JsonArray_T<b> array;
    bool first = true;
    while (skipSpaces(inStream) && inStream.peek() != ']') {
      if (!first) {
        if (!expectAndConsume(inStream, ',')) {
          break;
        }
      }
      first = false;
      auto parsedValue = JsonValue_T<b>::parse(inStream);
      if (!parsedValue.isValid()) {
        return parsedValue; // Error
      }
      array.emplace_back(std::move(parsedValue));
    }

    if (inStream.peek() != ']') {
      return getError("Expecting ']' here", inStream);
    }

    inStream.ignore(); // Consume ']'
    return array;
  }
};

using JsonString = JsonString_T<true>;
using JsonObject = JsonObject_T<true>;
using JsonArray = JsonArray_T<true>;
using JsonValue = JsonValue_T<true>;

} // namespace TinyJson