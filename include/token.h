#ifndef TINY_JS_TOKEN_H
#define TINY_JS_TOKEN_H

#include <string>
#include <variant>

enum class TokenType
{
    // 左括号
    LEFT_PAREN,
    // 右括号
    RIGHT_PAREN,
    // 左花括号
    LEFT_BRACE,
    // 右花括号
    RIGHT_BRACE,
    // 逗号
    COMMA,
    // 点号
    DOT,
    // 破折号
    MINUS,
    // 加号
    PLUS,
    // 分号
    SEMICOLON,
    // 斜杠
    SLASH,
    // 星号
    STAR,
    // 感叹号
    BANG,
    // 等于号
    BANG_EQUAL,
    // 等号
    EQUAL,
    // 双等号
    EQUAL_EQUAL,
    // 大于号
    GREATER,
    // 大于等于号
    GREATER_EQUAL,
    // 小于号
    LESS,
    // 小于等于号
    LESS_EQUAL,
    // 标识符
    IDENTIFIER,
    // 字符串
    STRING,
    // 数字
    NUMBER,
    // 中括号 左
    LEFT_BRACKET,
    // 中括号 右
    RIGHT_BRACKET,
    /** 关键字 开始 **/
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NULLPTR, OR, RETURN, SUPER, THIS, TRUE, VAR, WHILE, CONST,
    /** 关键字 结束 **/
    END_OF_FILE
};

struct Token
{
    TokenType type;
    std::string lexeme;
    int line;
    std::variant<std::monostate, double, std::string> literal;
};

#endif //TINY_JS_TOKEN_H
