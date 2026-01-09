#include "scanner.h"
#include <iostream>

std::vector<Token> Scanner::scanTokens()
{
    std::vector<Token> tokens;
    while (!isAtEnd())
    {
        start = current;
        scanToken(tokens);
    }
    tokens.push_back({TokenType::END_OF_FILE, "", line, std::monostate{}});
    return tokens;
}

bool Scanner::isAtEnd() const
{
    return current >= source.length();
}

char Scanner::advance()
{
    return source[current++];
}

char Scanner::peek() const
{
    return isAtEnd() ? '\0' : source[current];
}

void Scanner::scanToken(std::vector<Token>& t)
{
    switch (const char c = advance())
    {
    case '(': addToken(t, TokenType::LEFT_PAREN);
        break;
    case ')': addToken(t, TokenType::RIGHT_PAREN);
        break;
    case '{': addToken(t, TokenType::LEFT_BRACE);
        break;
    case '}': addToken(t, TokenType::RIGHT_BRACE);
        break;
    case '[': addToken(t, TokenType::LEFT_BRACKET);
        break;
    case ']': addToken(t, TokenType::RIGHT_BRACKET);
        break;
    case ',': addToken(t, TokenType::COMMA);
        break;
    case '.': addToken(t, TokenType::DOT);
        break;
    case '-':
        if (match('-'))
            addToken(t, TokenType::MINUS_MINUS);
        else if (match('='))
            addToken(t, TokenType::MINUS_EQUAL);
        else
            addToken(t, TokenType::MINUS);
        break;
    case '+':
        if (match('+'))
            addToken(t, TokenType::PLUS_PLUS);
        else if (match('='))
            addToken(t, TokenType::PLUS_EQUAL);
        else
            addToken(t, TokenType::PLUS);
        break;
    case '%':
        if (match('='))
            addToken(t, TokenType::PERCENT_EQUAL);
        else
            addToken(t, TokenType::PERCENT);
        break;
    case ';': addToken(t, TokenType::SEMICOLON);
        break;
    case '*':
        if (match('='))
            addToken(t, TokenType::STAR_EQUAL);
        else
            addToken(t, TokenType::STAR);
        break;
    case '!': addToken(t, match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
        break;
    case '=': addToken(t, match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        break;
    case '<': addToken(t, match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        break;
    case '>': addToken(t, match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
        break;
    case '/':
        if (match('/'))
        {
            while (peek() != '\n' && !isAtEnd()) advance();
        }
        else if (match('*'))
        {
            while (!isAtEnd())
            {
                if (peek() == '*' && peekNext() == '/')
                {
                    advance();
                    advance();
                    break;
                }
                if (peek() == '\n') line++;
                advance();
            }
            if (isAtEnd())
            {
                std::cerr << "[Line " << line << "] Error: Unterminated multi-line comment.\n";
            }
        }
        else if (match('='))
        {
            addToken(t, TokenType::SLASH_EQUAL);
        }
        else
        {
            addToken(t, TokenType::SLASH);
        }
        break;
    case ' ':
    case '\r':
    case '\t': break;
    case '\n': line++;
        break;
    case '"': string(t, '"');
        break;
    case '\'': string(t, '\'');
        break;
    default: if (isdigit(c)) number(t);
        if (isalpha(c) || c == '_') identifier(t);
        break;
    }
}

void Scanner::identifier(std::vector<Token>& t)
{
    while (isalnum(peek()) || peek() == '_') advance();
    const std::string text = source.substr(start, current - start);
    const TokenType type = keywords.contains(text) ? keywords.at(text) : TokenType::IDENTIFIER;
    addToken(t, type);
}
