#include <HtmlParser/Node.hpp>
#include <HtmlParser/Parser.hpp>
#include <stdexcept>

#include "Utilities.hpp"

namespace HtmlParser
{
    Parser::Parser()
    {
    }

    Parser::InsertionMode Parser::CurrentInsertionMode() const
    {
        return InsertionModes.top();
    }

    DOM Parser::Parse(const std::string& Input)
    {
        Tokenizer Instance(Input);
        Instance.Tokenize();
        const auto& Tokens = Instance.GetTokens();
        while(!InsertionModes.empty()) {
            InsertionModes.pop();
        }

        Document = std::make_shared<Node>(NodeType::Document);
        OpenElements.clear();
        OpenElements.push_back(Document);
        InsertionModes.push(InsertionMode::Initial);

        for (const auto& Token : Tokens)
        {
            switch (CurrentInsertionMode())
            {
            case InsertionMode::Initial:
                InsertionModeInitial(Token);
                break;
            case InsertionMode::BeforeHtml:
                InsertionModeBeforeHtml(Token);
                break;
            case InsertionMode::BeforeHead:
                InsertionModeBeforeHead(Token);
                break;
            case InsertionMode::InHead:
                InsertionModeInHead(Token);
                break;
            case InsertionMode::AfterHead:
                InsertionModeAfterHead(Token);
                break;
            case InsertionMode::InBody:
                InsertionModeInBody(Token);
                break;
            case InsertionMode::Text:
                InsertionModeText(Token);
                break;                
            case InsertionMode::RawText:
                InsertionModeRawText(Token);
                break;                
            default:
                HandleError("Unsupported insertion mode");
                break;
            }
        }

        return DOM(Document);
    }

    void Parser::HandleError(const std::string& ErrorMessage)
    {
        if (m_IsStrict)
        {
            throw std::runtime_error("Parse error: " + ErrorMessage);
        }
        else
        {
            // For non-  mode, can log the error or ignore it
            // std::cerr << "Parse error: " << message << std::endl;
        }
    }

    std::shared_ptr<Node> Parser::CurrentNode()
    {
        return OpenElements.back();
    }

    void Parser::InsertElement(const Token& Token)
    {
        auto Element = std::make_shared<Node>(NodeType::Element);
        Element->Tag = Utils::ToLower(Token.Data);
        Element->Attributes = Token.Attributes;
        CurrentNode()->AppendChild(Element);
        if (!Token.SelfClosing)
        {
            OpenElements.push_back(Element);
        }
    }

    void Parser::InsertCharacter(const Token& Token)
    {
        auto TextNode = std::make_shared<Node>(NodeType::Text);
        TextNode->Text = Token.Data;
        CurrentNode()->AppendChild(TextNode);
    }

    void Parser::CloseElement(const Token& Token)
    {
        std::string TagName = Utils::ToLower(Token.Data);
        for (auto it = OpenElements.rbegin(); it != OpenElements.rend(); ++it)
        {
            if ((*it)->Tag == TagName)
            {
                OpenElements.erase(it.base() - 1, OpenElements.end());
                return;
            }
        }
        // If we didn't find the tag to close
        HandleError("No matching start tag for end tag: " + Token.Data);
    }

    void Parser::InsertionModeInitial(const Token& Token)
    {
        //This will be ignored and the parser will move to the next token
        if (Token.Type == TokenType::XML_DECLARATION)
        {
            return;
        }

        if (Token.Type == TokenType::DOCTYPE)
        {
            // Ignore for now
            InsertionModes.top() = InsertionMode::BeforeHtml;
        }
        else
        {
            // If no DOCTYPE is found, the parser defaults to BeforeHtml mode and re-processes the token.
            InsertionModes.top() = InsertionMode::BeforeHtml;
            InsertionModeBeforeHtml(Token);
        }
    }

    void Parser::InsertionModeBeforeHtml(const Token& Token)
    {
        if (Token.Type == TokenType::Character && isspace(Token.Data[0])) {
            return; // Ignore whitespace
        }
        if (Token.Type == TokenType::StartTag && Utils::ToLower(Token.Data) == "html")
        {
            InsertElement(Token);
            InsertionModes.top() = InsertionMode::BeforeHead;
        }
        else
        {
            // Implicitly create <html>
            HtmlParser::Token HtmlToken;
            HtmlToken.Type = TokenType::StartTag;
            HtmlToken.Data = "html";
            InsertElement(HtmlToken);
            InsertionModes.top() = InsertionMode::BeforeHead;
            InsertionModeBeforeHead(Token);
        }
    }

    void Parser::InsertionModeBeforeHead(const Token& Token)
    {
        if (Token.Type == TokenType::Character && isspace(Token.Data[0])) {
            return; // Ignore whitespace
        }
        if (Token.Type == TokenType::StartTag && Utils::ToLower(Token.Data) == "head")
        {
            InsertElement(Token);
            InsertionModes.top() = InsertionMode::InHead;
        }
        else
        {
            // Implicitly create <head>
            HtmlParser::Token HeadToken;
            HeadToken.Type = TokenType::StartTag;
            HeadToken.Data = "head";
            InsertElement(HeadToken);
            InsertionModes.top() = InsertionMode::InHead;
            InsertionModeInHead(Token);
        }
    }

    void Parser::InsertionModeInHead(const Token& Token)
    {
        if (Token.Type == TokenType::Character && isspace(Token.Data[0])) {
            return; // Ignore whitespace
        }
        switch (Token.Type)
        {
        case TokenType::StartTag:
        {
            std::string tag = Utils::ToLower(Token.Data);
            if (tag == "title")
            {
                InsertElement(Token);
                InsertionModes.push(InsertionMode::Text);
            }
            else if (tag == "style" || tag == "script")
            {
                InsertElement(Token);
                InsertionModes.push(InsertionMode::RawText);
            }
            else if (tag == "link" || tag == "meta" || tag == "base")
            {
                InsertElement(Token);
                OpenElements.pop_back(); // Self-closing tags
            }
            else if (tag == "head")
            {
                HandleError("Unexpected <head> tag.");
            }
            else
            {
                // Any other start tag implicitly closes the head
                OpenElements.pop_back();
                InsertionModes.top() = InsertionMode::AfterHead;
                InsertionModeAfterHead(Token);
            }
            break;
        }
        case TokenType::EndTag:
        {
            std::string tag = Utils::ToLower(Token.Data);
            if (tag == "head")
            {
                OpenElements.pop_back();
                InsertionModes.top() = InsertionMode::AfterHead;
            }
            else if (tag == "body" || tag == "html" || tag == "br")
            {
                // Implicitly close head and reprocess token in AfterHead
                OpenElements.pop_back();
                InsertionModes.top() = InsertionMode::AfterHead;
                InsertionModeAfterHead(Token);
            }
            else
            {
                HandleError("Unexpected end tag in head: " + Token.Data);
            }
            break;
        }
        case TokenType::Comment:
            // Comments can be ignored or inserted into the DOM
            break;
        default:
            // For other tokens, implicitly close head and reprocess
            OpenElements.pop_back();
            InsertionModes.top() = InsertionMode::AfterHead;
            InsertionModeAfterHead(Token);
            break;
        }
    }
    void Parser::InsertionModeAfterHead(const Token& Token)
    {
        if (Token.Type == TokenType::Character && isspace(Token.Data[0])) {
            return; // Ignore whitespace
        }
        if (Token.Type == TokenType::StartTag && Utils::ToLower(Token.Data) == "body")
        {
            InsertElement(Token);
            InsertionModes.top() = InsertionMode::InBody;
        }
        else
        {
            // Implicitly create <body>
            HtmlParser::Token BodyToken;
            BodyToken.Type = TokenType::StartTag;
            BodyToken.Data = "body";
            InsertElement(BodyToken);
            InsertionModes.top() = InsertionMode::InBody;
            InsertionModeInBody(Token);
        }
    }

    void Parser::InsertionModeInBody(const Token& Token)
    {
        if (Token.Type == TokenType::Character)
        {
            InsertCharacter(Token);
        }
        else if (Token.Type == TokenType::StartTag)
        {
            InsertElement(Token);
        }
        else if (Token.Type == TokenType::EndTag)
        {
            CloseElement(Token);
        }
        else
        {
            // Handle other token types as needed
        }
    }

    void Parser::InsertionModeRawText(const Token& token)
    {
        // The smart tokenizer gives us the script content as one big Character token.
        // So, any token that is NOT the specific end tag should just have its text appended.
        if (token.Type == TokenType::EndTag && Utils::ToLower(token.Data) == Utils::ToLower(CurrentNode()->Tag))
        {
            // We found the correct closing tag (e.g., </script>)
            OpenElements.pop_back();
            if (!InsertionModes.empty())
            {
                InsertionModes.pop();
            }
        }
        else
        {
            // This is the script content. Append it.
            InsertCharacter(token);
        }
    }

    void Parser::InsertionModeText(const Token& token)
    {
        // The handler for <title>. Same simple logic.
        if (token.Type == TokenType::EndTag && Utils::ToLower(token.Data) == "title")
        {
            OpenElements.pop_back();
            if (!InsertionModes.empty())
            {
                InsertionModes.pop();
            }
        }
        else
        {
            // This is the title content. Append it.
            InsertCharacter(token);
        }
    }
} // namespace HtmlParser
