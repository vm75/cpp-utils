#include <tiny-json.hxx>
using namespace std::literals::string_view_literals;

void test(std::string_view testStr)
{
  static int count = 1;
  std::string error;
  TinyJson::JsonValue json = TinyJson::JsonValue::parse(testStr, error);
  auto str = json.toString();
  std::cout << "Test #" << count++ << ": " << (error.empty() ? "Success" : error) << std::endl;
}

int main(int argc, char **argv)
{
  test(R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})");
  test(R"({
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
    })");
  test("{\"a\": 1}//trailing line comment");
  test("{\"a\": 1}/*trailing multi-line comment*/");
  test("{\n/* unterminated comment\n\"a\": 1,\n}");
  test("{\n/* unterminated trailing comment }");
  test("{\n/ / bad comment }");
  test("{// bad comment }");
  test("{\n\"a\": 1\n}/");
  test("{/* bad\ncomment *}");
  test(R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])");

  std::string error;
  TinyJson::JsonValue json1 = TinyJson::JsonValue::parse(R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])"sv, error);
  TinyJson::JsonValue json2 = TinyJson::JsonValue::parse(R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])"sv, error);
  std::cout << (json1 == json2 ? "Success" : "Failure") << std::endl;

  return 0;
}