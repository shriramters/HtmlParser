#include <gtest/gtest.h>
#include <HtmlParser/Parser.hpp>
#include <HtmlParser/DOM.hpp>

// Test to ensure the parser correctly transitions to and from the Text insertion mode for <title>
TEST(ParserStateTest, TitleTagTextMode)
{
    std::string html = "<html><head><title>Hello & Welcome</title></head><body></body></html>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);

    auto titleElement = dom.GetElementsByTagName("title");
    ASSERT_EQ(titleElement.size(), 1);
    ASSERT_EQ(titleElement[0]->GetTextContent(), "Hello & Welcome");
}

// Test to ensure the parser correctly transitions to and from the RawText insertion mode for <script>
TEST(ParserStateTest, ScriptTagRawTextMode)
{
    std::string html = "<html><head><script>if (a < b) { console.log('hello'); }</script></head><body></body></html>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);

    auto scriptElement = dom.GetElementsByTagName("script");
    ASSERT_EQ(scriptElement.size(), 1);
    ASSERT_EQ(scriptElement[0]->GetTextContent(), "if (a < b) { console.log('hello'); }");
}

// Test that a non-matching end tag inside a script is treated as text
TEST(ParserStateTest, NonMatchingEndTagInScript)
{
    std::string html = "<script>var x = '</script>';</script>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);
    auto scriptElement = dom.GetElementsByTagName("script");
    ASSERT_EQ(scriptElement.size(), 1);
    ASSERT_EQ(scriptElement[0]->GetTextContent(), "var x = '");
}

// Test to ensure RawText mode handles other end tags as plain text.
TEST(ParserStateTest, HandlesOtherEndTagsAsTextInRawTextMode)
{
    std::string html = "<script><div></p></span></script>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);
    auto scripts = dom.GetElementsByTagName("script");
    ASSERT_EQ(scripts.size(), 1);
    ASSERT_EQ(scripts[0]->GetTextContent(), "<div></p></span>");
}
