// Copyright 2022 VM75. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

#include <tiny-json.hxx>
using namespace std::literals::string_view_literals;

static int testCount = 1;

void testParse(std::string_view testStr, bool expectError)
{
  std::string error{};
  auto json = TinyJson::JsonValue::parse(testStr, error);
  auto str = json.toString();

  std::cout << "Test #" << testCount++ << ": " << (error.empty() != expectError ? "Success" : "Failure");
  if (!error.empty()) {
    std::cout << ": " << error;
  }
  std::cout << std::endl;
}

void testEquality(std::string_view str1, std::string_view str2, bool expectEqual)
{
  std::string error{};
  auto json1 = TinyJson::JsonValue::parse(str1, error);
  auto json2 = TinyJson::JsonValue::parse(str2, error);

  std::cout << "Test #" << testCount++ << ": " << (((json1 == json2) == expectEqual) ? "Success" : "Failure") << std::endl;
}

void testNullValue(std::string_view str)
{
  std::string error{};
  auto json = TinyJson::JsonValue::parse(str, error);
  auto object = json.asObject();

  std::cout << "Test #" << testCount++ << ": " << (object["value"].isNull() ? "Success" : "Failure") << std::endl;
}

template <typename T>
void testValue(std::string_view str, T expected)
{
  std::string error{};
  auto json = TinyJson::JsonValue::parse(str, error);
  auto object = json.asObject();
  auto value = object["value"].as<T>();

  std::cout << "Test #" << testCount++ << ": " << ((value == expected) ? "Success" : "Failure") << std::endl;
}

void testQuery(std::string_view str1, const std::string &query, std::string_view expected)
{
  std::string error{};
  auto json1 = TinyJson::JsonValue::parse(str1, error).find(query);
  auto json2 = TinyJson::JsonValue::parse(expected, error);

  std::cout << "Test #" << testCount++ << ": " << ((json1 == json2) ? "Success" : "Failure") << std::endl;
}

void testRemove(std::string_view str1, const std::string &query, std::string_view expected)
{
  std::string error{};
  auto json1 = TinyJson::JsonValue::parse(str1, error);
  auto json2 = TinyJson::JsonValue::parse(expected, error);
  auto res = json1.erase(query);

  std::cout << "Test #" << testCount++ << ": " << ((res && (json1 == json2)) ? "Success" : "Failure") << std::endl;
}

void testDeepCopy(std::string_view str1, const std::string &query)
{
  std::string error{};
  auto json1 = TinyJson::JsonValue::parse(str1, error);
  auto json2 = json1;
  auto res = json1.erase(query);

  std::cout << "Test #" << testCount++ << ": " << ((json1 != json2) ? "Success" : "Failure") << std::endl;
}

int main(int argc, char **argv)
{
  testNullValue(R"({"value": null})");
  testValue<TinyJson::JsonBool>(R"({"value": true})", true);
  testValue<TinyJson::JsonBool>(R"({"value": false})", false);
  testValue<TinyJson::JsonInt>(R"({"value": 123})", 123);
  testValue<TinyJson::JsonFloat>(R"({"value": 123.45})", 123.45l);
  testValue<TinyJson::JsonString>(R"({"value": "str"})", "str");
  testValue<TinyJson::JsonObject>(R"({"value": {}})", TinyJson::JsonObject());
  testValue<TinyJson::JsonArray>(R"({"value": []})", TinyJson::JsonArray());

  testParse(R"({})", false /*expectError */);
  testParse(R"(   {}   )", false /*expectError */);
  testParse(R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})", false /*expectError */);
  testParse(R"({
      // comment /* with nested comment */
      "a": 1,
      // comment
      // continued
      "b": "text",
      /* multi
         line
         comment
        // line-comment-inside-multiline-comment
      */
      // and single-line comment
      // and single-line comment /* multiline inside single line */
      "c": [1, 2, 3]
      // and single-line comment at end of object
    })",
            false /*expectError */);
  testParse("{\"a\": 1}//trailing line comment", false /*expectError */);
  testParse("{\"a\": 1}/*trailing multi-line comment*/", false /*expectError */);

  testParse("{\n/* unterminated comment\n\"a\": 1,\n}", true /*expectError */);
  testParse("{\n/* unterminated trailing comment }", true /*expectError */);
  testParse("{\n/ / bad comment }", true /*expectError */);
  testParse("{// bad comment }", true /*expectError */);
  testParse("{\n\"a\": 1\n}/", true /*expectError */);
  testParse("{/* bad\ncomment *}", true /*expectError */);
  testParse(R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])", false /*expectError */);
  testParse(R"({"k1":"v1"])", true /*expectError */);
  testParse(R"(["k1":"v1"})", true /*expectError */);
  testParse(R"({"k1":"v1	v2"})", true /*expectError */);
  testParse(R"("abc":123)", true /*expectError */);

  testEquality(R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])"sv, R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])"sv, true /*expectEqual*/);
  testEquality(R"({"a":1, "b": ["c"]})"sv, R"({"b": ["c"], "a":1})"sv, true /*expectEqual*/);
  testEquality(R"({"a":1, "b": ["c", "d"]})"sv, R"({"b": ["d", "c"], "a":1})"sv, false /*expectEqual*/);

  testQuery(R"({"a":1, "b": ["c", "d"]})"sv, "b", R"(["c", "d"])"sv);
  testQuery(R"({"a":1, "b": ["c", "d"]})"sv, "b.1", R"("d")"sv);
  testQuery(R"({"a":1, "b": ["c", { "d" : 5 }]})"sv, "b.1.d", R"(5)"sv);
  testQuery(R"({"a":1, "b": ["c", "d"]})"sv, "b.5", R"(null)"sv);

  testRemove(R"({"a":1, "b": ["c", "d"]})"sv, "b", R"({"a":1})"sv);
  testRemove(R"({"a":1, "b": ["c", "d"]})"sv, "b.0", R"({"a":1, "b": ["d"]})"sv);
  testRemove(R"({"a":1, "b": ["c", { "d" : 5 }]})"sv, "b.1.d", R"({"a":1, "b": ["c", { }]})"sv);

  testDeepCopy(R"({"a":1, "b": ["c", { "d" : 5 }]})"sv, "b.1.d");

  return 0;
}