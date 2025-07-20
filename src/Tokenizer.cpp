#include <HtmlParser/Tokenizer.hpp>
#include "Utilities.hpp"

namespace HtmlParser
{
    Tokenizer::Tokenizer(const std::string& InputStr) : m_Input(InputStr), m_Position(0), m_CurrentState(State::Data)
    {
       m_AppropriateEndTag.clear();
    }

    void Tokenizer::Tokenize()
    {
        while (m_Position < m_Input.size())
        {
            char c = m_Input[m_Position++];
            switch (m_CurrentState)
            {
            case State::Data:
                HandleDataState(c);
                break;
            case State::TagOpen:
                HandleTagOpenState(c);
                break;
            case State::TagName:
                HandleTagNameState(c);
                break;
            case State::EndTagOpen:
                HandleEndTagOpenState(c);
                break;
            case State::SelfClosingStartTag:
                HandleSelfClosingStartTagState(c);
                break;
            case State::BeforeAttributeName:
                HandleBeforeAttributeNameState(c);
                break;
            case State::AttributeName:
                HandleAttributeNameState(c);
                break;
            case State::AfterAttributeName:
                HandleAfterAttributeNameState(c);
                break;
            case State::BeforeAttributeValue:
                HandleBeforeAttributeValueState(c);
                break;
            case State::AttributeValueDoubleQuoted:
                HandleAttributeValueDoubleQuotedState(c);
                break;
            case State::AttributeValueSingleQuoted:
                HandleAttributeValueSingleQuotedState(c);
                break;
            case State::AttributeValueUnquoted:
                HandleAttributeValueUnquotedState(c);
                break;
            case State::AfterAttributeValueQuoted:
                HandleAfterAttributeValueQuotedState(c);
                break;
            case State::AfterAttributeValueUnquoted:
                HandleAfterAttributeValueUnquotedState(c);
                break;
            case State::XMLDeclaration:
                HandleXMLDeclarationState(c);
                break;
            case State::DOCTYPEDeclaration:
                HandleDOCTYPEDeclarationState(c);
                break;                
            case State::RawText:
                HandleRawTextState(c);
                break;
            }
        }
    }

    const std::vector<Token>& Tokenizer::GetTokens() const
    {
        return m_Tokens;
    }

    void Tokenizer::EmitToken(const Token& Token)
    {
        m_Tokens.push_back(Token);
    }

    void Tokenizer::ReconsumeChar()
    {
        --m_Position;
    }

    bool Tokenizer::IsWhitespace(char c) const
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
    }

    bool Tokenizer::IsAlpha(char c) const
    {
        return std::isalpha(static_cast<unsigned char>(c));
    }

    // Implement state handling functions
    void Tokenizer::HandleDataState(char c)
    {
        if (c == '<')
        {
            m_CurrentState = State::TagOpen;
        }
        else
        {
            Token Token;
            Token.Type = TokenType::Character;
            Token.Data = c;
            EmitToken(Token);
        }
    }

    void Tokenizer::HandleTagOpenState(char c)
    {
        if (c == '!')
        {
            m_CurrentState = State::DOCTYPEDeclaration;
            return;
        }

        if (c == '?')
        {
            m_CurrentState = State::XMLDeclaration;
            return;
        }

        if (c == '/')
        {
            m_CurrentState = State::EndTagOpen;
        }
        else if (IsAlpha(c))
        {
            m_CurrentToken = Token();
            m_CurrentToken.Type = TokenType::StartTag;
            m_CurrentToken.Data = c;
            m_CurrentState = State::TagName;
        }
        else
        {
            // Parse error
            m_CurrentState = State::Data;
            Token Token;
            Token.Type = TokenType::Character;
            Token.Data = '<';
            EmitToken(Token);
            ReconsumeChar();
        }
    }

   void Tokenizer::HandleTagNameState(char c)
    {
        if (c == '>')
        {
            EmitToken(m_CurrentToken); // Emit the tag, e.g., <p> or <script>

            std::string lower_tag_name = Utils::ToLower(m_CurrentToken.Data);
            if (lower_tag_name == "script" || lower_tag_name == "style")
            {
                // If it's a script or style tag, set the appropriate end tag
                // and switch to the special raw text state.
                m_AppropriateEndTag = "</" + lower_tag_name;
                m_CurrentState = State::RawText;
            }
            else
            {
                // Otherwise, go back to normal data processing.
                m_CurrentState = State::Data;
            }
        }
        else if (IsWhitespace(c))
        {
            m_CurrentState = State::BeforeAttributeName;
        }
        else if (c == '/')
        {
            m_CurrentState = State::SelfClosingStartTag;
        }
        else
        {
            m_CurrentToken.Data += c;
        }
    }

    // 4. ADD THE NEW HandleRawTextState FUNCTION
    void Tokenizer::HandleRawTextState(char c)
    {
        // In this state, we simply buffer characters until we see the start of our end tag.
        ReconsumeChar(); // Re-process the current character in the logic below.

        size_t end_tag_pos = m_Input.find(m_AppropriateEndTag, m_Position);

        std::string raw_text;
        if (end_tag_pos != std::string::npos)
        {
            // We found the end tag. The content is everything up to it.
            raw_text = m_Input.substr(m_Position, end_tag_pos - m_Position);
            m_Position = end_tag_pos; // Move our position to the start of the end tag.
        }
        else
        {
            // No end tag found. The rest of the document is the content.
            raw_text = m_Input.substr(m_Position);
            m_Position = m_Input.size();
        }

        if (!raw_text.empty())
        {
            Token text_token;
            text_token.Type = TokenType::Character;
            text_token.Data = raw_text;
            EmitToken(text_token);
        }

        // Done with raw text mode. Return to the normal data state.
        // The main loop will then naturally tokenize the `</script>` or `</style>` tag.
        m_CurrentState = State::Data;
    }

    void Tokenizer::HandleEndTagOpenState(char c)
    {
        if (IsAlpha(c))
        {
            m_CurrentToken = Token();
            m_CurrentToken.Type = TokenType::EndTag;
            m_CurrentToken.Data = c;
            m_CurrentState = State::TagName;
        }
        else
        {
            // Parse error
            m_CurrentState = State::Data;
        }
    }

    void Tokenizer::HandleSelfClosingStartTagState(char c)
    {
        if (c == '>')
        {
            m_CurrentToken.SelfClosing = true;
            EmitToken(m_CurrentToken);
            m_CurrentState = State::Data;
        }
        else
        {
            // Parse error
            m_CurrentState = State::BeforeAttributeName;
            ReconsumeChar();
        }
    }

    void Tokenizer::HandleBeforeAttributeNameState(char c)
    {
        if (IsWhitespace(c))
        {
            // Ignore
        }
        else if (c == '/' || c == '>')
        {
            m_CurrentState = State::AfterAttributeName;
            ReconsumeChar();
        }
        else
        {
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            m_CurrentState = State::AttributeName;
            ReconsumeChar();
        }
    }

    void Tokenizer::HandleAttributeNameState(char c)
    {
        if (IsWhitespace(c) || c == '/' || c == '>')
        {
            m_CurrentState = State::AfterAttributeName;
            ReconsumeChar();
        }
        else if (c == '=')
        {
            m_CurrentState = State::BeforeAttributeValue;
        }
        else
        {
            m_CurrentAttributeName += c;
        }
    }

    void Tokenizer::HandleAfterAttributeNameState(char c)
    {
        if (IsWhitespace(c))
        {
            // Ignore
        }
        else if (c == '/')
        {
            m_CurrentState = State::SelfClosingStartTag;
        }
        else if (c == '=')
        {
            m_CurrentState = State::BeforeAttributeValue;
        }
        else if (c == '>')
        {
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            EmitToken(m_CurrentToken);
            m_CurrentState = State::Data;
        }
        else
        {
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            m_CurrentState = State::AttributeName;
            ReconsumeChar();
        }
    }

    void Tokenizer::HandleBeforeAttributeValueState(char c)
    {
        if (IsWhitespace(c))
        {
            // Ignore
        }
        else if (c == '"')
        {
            m_CurrentState = State::AttributeValueDoubleQuoted;
        }
        else if (c == '\'')
        {
            m_CurrentState = State::AttributeValueSingleQuoted;
        }
        else if (c == '>')
        {
            // Parse error
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            EmitToken(m_CurrentToken);
            m_CurrentState = State::Data;
        }
        else
        {
            m_CurrentState = State::AttributeValueUnquoted;
            ReconsumeChar();
        }
    }

    void Tokenizer::HandleAttributeValueDoubleQuotedState(char c)
    {
        if (c == '"')
        {
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            m_CurrentState = State::AfterAttributeValueQuoted;
        }
        else
        {
            m_CurrentAttributeValue += c;
        }
    }

    void Tokenizer::HandleAttributeValueSingleQuotedState(char c)
    {
        if (c == '\'')
        {
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            m_CurrentState = State::AfterAttributeValueQuoted;
        }
        else
        {
            m_CurrentAttributeValue += c;
        }
    }

    void Tokenizer::HandleAttributeValueUnquotedState(char c)
    {
        if (IsWhitespace(c))
        {
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            m_CurrentState = State::AfterAttributeValueUnquoted;
        }
        else if (c == '>')
        {
            m_CurrentToken.Attributes[m_CurrentAttributeName] = m_CurrentAttributeValue;
            m_CurrentAttributeName.clear();
            m_CurrentAttributeValue.clear();
            EmitToken(m_CurrentToken);
            m_CurrentState = State::Data;
        }
        else if (c == '&')
        {
            m_CurrentAttributeValue += c;
        }
        else if (c == '\0')
        {
            // Parse error
        }
        else if (c == '"' || c == '\'' || c == '<' || c == '=' || c == '`')
        {
            // Parse error
            m_CurrentAttributeValue += c;
        }
        else
        {
            m_CurrentAttributeValue += c;
        }
    }

    void Tokenizer::HandleAfterAttributeValueQuotedState(char c)
    {
        if (IsWhitespace(c))
        {
            m_CurrentState = State::BeforeAttributeName;
        }
        else if (c == '/')
        {
            m_CurrentState = State::SelfClosingStartTag;
        }
        else if (c == '>')
        {
            EmitToken(m_CurrentToken);
            m_CurrentState = State::Data;
        }
        else
        {
            // Parse error
            m_CurrentState = State::BeforeAttributeName;
            ReconsumeChar();
        }
    }

    void Tokenizer::HandleAfterAttributeValueUnquotedState(char c)
    {
        if (IsWhitespace(c))
        {
            m_CurrentState = State::BeforeAttributeName;
        }
        else if (c == '/')
        {
            m_CurrentState = State::SelfClosingStartTag;
        }
        else if (c == '>')
        {
            EmitToken(m_CurrentToken);
            m_CurrentState = State::Data;
        }
        else
        {
            // Parse error
            m_CurrentState = State::BeforeAttributeName;
            ReconsumeChar();
        }
    }

    void Tokenizer::HandleXMLDeclarationState(char c)
    {
        if (c == '>')
        {
            m_CurrentState = State::Data;
        }
    }

    void Tokenizer::HandleDOCTYPEDeclarationState(char c)
    {
        if (c == '>')
        {
            m_CurrentState = State::Data;
        }
    }  
} // namespace HtmlParser
