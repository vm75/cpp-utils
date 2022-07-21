#include <tiny-json.hxx>
using namespace std::literals::string_view_literals;

static int testCount = 1;

void testParse(std::string_view testStr, bool expectError)
{
  std::string error;
  TinyJson::JsonValue json = TinyJson::JsonValue::parse(testStr, error);
  auto str = json.toString();

  std::cout << "Test #" << testCount++ << ": " << (error.empty() != expectError ? "Success" : "Failure");
  if (!error.empty()) {
    std::cout << ": " << error;
  }
  std::cout << std::endl;
}

void testEquality(std::string_view str1, std::string_view str2, bool expectEqual)
{
  std::string error;
  TinyJson::JsonValue json1 = TinyJson::JsonValue::parse(str1, error);
  TinyJson::JsonValue json2 = TinyJson::JsonValue::parse(str2, error);

  std::cout << "Test #" << testCount++ << ": " << (((json1 == json2) == expectEqual) ? "Success" : "Failure") << std::endl;
}

void testNullValue(std::string_view str)
{
  std::string error;
  TinyJson::JsonValue json = TinyJson::JsonValue::parse(str, error);
  auto object = json.asObject();

  std::cout << "Test #" << testCount++ << ": " << (object["value"].isNull() ? "Success" : "Failure") << std::endl;
}

template <typename T>
void testValue(std::string_view str, T expected)
{
  std::string error;
  TinyJson::JsonValue json = TinyJson::JsonValue::parse(str, error);
  auto object = json.asObject();
  auto value = object["value"].as<T>();

  std::cout << "Test #" << testCount++ << ": " << ((value == expected) ? "Success" : "Failure") << std::endl;
}

int main(int argc, char **argv)
{
  testParse(R"("abc":123)", true /*expectError */);
  testParse(R"({})", false /*expectError */);
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

  testEquality(R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])"sv, R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])"sv, true /*expectEqual*/);
  testEquality(R"({"a":1, "b": ["c"]})"sv, R"({"b": ["c"], "a":1})"sv, true /*expectEqual*/);
  testEquality(R"({"a":1, "b": ["c", "d"]})"sv, R"({"b": ["d", "c"], "a":1})"sv, false /*expectEqual*/);

  testNullValue(R"({"value": null})");
  testValue<TinyJson::JsonBool>(R"({"value": true})", true);
  testValue<TinyJson::JsonBool>(R"({"value": false})", false);
  testValue<TinyJson::JsonInt>(R"({"value": 123})", 123);
  testValue<TinyJson::JsonFloat>(R"({"value": 123.45})", 123.45l);
  testValue<TinyJson::JsonString>(R"({"value": "str"})", "str");
  testValue<TinyJson::JsonObject>(R"({"value": {}})", TinyJson::JsonObject());
  testValue<TinyJson::JsonArray>(R"({"value": []})", TinyJson::JsonArray());

  return 0;
}