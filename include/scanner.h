#ifndef TINY_JS_SCANNER_H
#define TINY_JS_SCANNER_H

#include "token.h"
#include <map>
#include <vector>

class Scanner
{
    std::string source;
    int start = 0, current = 0, line = 1;
    const std::map<std::string, TokenType> keywords = {
        {"class", TokenType::CLASS},
        {"and", TokenType::AND},
        {"else", TokenType::ELSE},
        {"false", TokenType::FALSE},
        {"for", TokenType::FOR},
        {"fun", TokenType::FUN},
        {"function", TokenType::FUN},
        {"if", TokenType::IF},
        {"null", TokenType::NULLPTR},
        {"or", TokenType::OR},
        {"return", TokenType::RETURN},
        {"true", TokenType::TRUE},
        {"var", TokenType::VAR},
        {"let", TokenType::VAR},
        {"while", TokenType::WHILE},
        {"const", TokenType::CONST},
    };

public:
    explicit Scanner(std::string s) : source(std::move(s))
    {
    }

    std::vector<Token> scanTokens();

private:
    [[nodiscard]] bool isAtEnd() const;
    char advance();
    [[nodiscard]] char peek() const;
    [[nodiscard]] char peekNext() const { return (current + 1 >= source.length()) ? '\0' : source[current + 1]; }

    bool match(const char expected)
    {
        if (isAtEnd() || source[current] != expected) return false;
        current++;
        return true;
    }

    void addToken(std::vector<Token>& t, TokenType type,
                  std::variant<std::monostate, double, std::string> l = std::monostate{}) const
    {
        t.push_back({type, source.substr(start, current - start), line, l});
    }

    void scanToken(std::vector<Token>& t);

    void string(std::vector<Token>& t, char quote)
    {
        while (peek() != quote && !isAtEnd())
        {
            if (peek() == '\n') line++;
            advance();
        }
        if (isAtEnd()) return;
        advance();
        addToken(t, TokenType::STRING, source.substr(start + 1, current - start - 2));
    }

    void number(std::vector<Token>& t)
    {
        while (isdigit(peek())) advance();
        if (peek() == '.' && isdigit(peekNext()))
        {
            advance();
            while (isdigit(peek())) advance();
        }
        addToken(t, TokenType::NUMBER, std::stod(source.substr(start, current - start)));
    }

    void identifier(std::vector<Token>& t);
};

#endif //TINY_JS_SCANNER_H
