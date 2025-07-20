#include <gtest/gtest.h>

#include <HtmlParser/DOM.hpp>
#include <HtmlParser/Parser.hpp>

TEST(ParserTest, ParseWithXMLDeclaration)
{
    HtmlParser::Parser parser;
    std::string html = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE html><html><body></body></html>";
    HtmlParser::DOM dom = parser.Parse(html);
    auto htmlElement = dom.GetElementsByTagName("html");
    ASSERT_FALSE(htmlElement.empty());
}

TEST(ParserTest, BasicParse)
{
    HtmlParser::Parser Parser;
    HtmlParser::DOM DOM = Parser.Parse("<html><body><p>Hello World</p></body></html>");

    auto Body = DOM.GetElementsByTagName("body");
    ASSERT_EQ(Body.size(), 1);

    auto Paragraph = DOM.GetElementsByTagName("p");
    ASSERT_EQ(Paragraph.size(), 1);

    auto Text = Paragraph.front()->GetTextContent();
    ASSERT_EQ(Text, "Hello World");
}
