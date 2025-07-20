#include <gtest/gtest.h>
#include <HtmlParser/Parser.hpp>
#include <HtmlParser/DOM.hpp>

// Test parsing of a <link> tag in the head
TEST(HeadElementsTest, ParsesLinkTag)
{
    std::string html = "<html><head><link rel=\"stylesheet\" href=\"style.css\"></head><body></body></html>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);

    auto linkElement = dom.GetElementsByTagName("link");
    ASSERT_EQ(linkElement.size(), 1);
    ASSERT_EQ(linkElement[0]->GetAttribute("rel"), "stylesheet");
    ASSERT_EQ(linkElement[0]->GetAttribute("href"), "style.css");
}

// Test parsing of a <meta> tag in the head
TEST(HeadElementsTest, ParsesMetaTag)
{
    std::string html = "<html><head><meta charset=\"UTF-8\"></head><body></body></html>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);

    auto metaElement = dom.GetElementsByTagName("meta");
    ASSERT_EQ(metaElement.size(), 1);
    ASSERT_EQ(metaElement[0]->GetAttribute("charset"), "UTF-8");
}

// Test parsing of a <style> tag in the head
TEST(HeadElementsTest, ParsesStyleTag)
{
    std::string styleContent = "body { color: red; }";
    std::string html = "<html><head><style>" + styleContent + "</style></head><body></body></html>";
    HtmlParser::Parser parser;
    HtmlParser::DOM dom = parser.Parse(html);

    auto styleElement = dom.GetElementsByTagName("style");
    ASSERT_EQ(styleElement.size(), 1);
    ASSERT_EQ(styleElement[0]->GetTextContent(), styleContent);
}

// Test for whitespace handling inside the <head>
TEST(HeadElementsTest, HandlesWhitespaceInHead)
{
    std::string html = "<html><head> <title>Whitespace Test</title> </head><body></body></html>";
    HtmlParser::Parser parser;
    ASSERT_NO_THROW({
        HtmlParser::DOM dom = parser.Parse(html);
        auto title = dom.GetElementsByTagName("title");
        ASSERT_EQ(title.size(), 1);
        ASSERT_EQ(title[0]->GetTextContent(), "Whitespace Test");
    });
}