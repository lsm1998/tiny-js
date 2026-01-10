#ifndef TINY_JS_PARSER_H
#define TINY_JS_PARSER_H

#include "token.h"
#include "ast.h"
#include <vector>

class Parser
{
    // token 列表
    std::vector<Token> tokens;
    // 当前解析到的token索引
    int current = 0;
    // 当前解析的文件名
    std::string filename;

public:
    explicit Parser(const std::vector<Token>& t, std::string filename = "<script>") : tokens(t),
        filename(std::move(filename))
    {
    }

    std::vector<std::shared_ptr<Stmt>> parse();

private:
    // 匹配token类型
    bool match(TokenType t);

    // 检查当前token类型
    bool check(TokenType t);

    // 解析下一个token并返回
    Token advance();

    // 判断是否到达token末尾
    bool isAtEnd();

    // 返回当前token
    Token peek();

    // 查看下一个token
    Token peekNext();

    // 返回上一个token
    Token previous();

    // 消费指定类型的token，否则抛出错误
    Token consume(TokenType t, const std::string& m);

    // 解析声明
    std::shared_ptr<Stmt> declaration();

    // 解析函数声明
    std::shared_ptr<Stmt> function(const std::string& k);

    // 解析变量声明
    std::shared_ptr<Stmt> varDeclaration(bool isConst);

    // 解析语句
    std::shared_ptr<Stmt> statement();

    // 解析代码块
    std::shared_ptr<Stmt> block();

    // 解析表达式语句
    std::shared_ptr<Stmt> exprStmt();

    // 解析if语句
    std::shared_ptr<Stmt> ifStmt();

    // 解析while语句
    std::shared_ptr<Stmt> whileStmt();

    // 解析for语句
    std::shared_ptr<Stmt> forStmt();

    // 解析return语句
    std::shared_ptr<Stmt> returnStmt();

    // 解析表达式
    std::shared_ptr<Expr> expression();

    // 解析条件表达式（三元运算符）
    std::shared_ptr<Expr> conditional();

    // 解析逻辑或表达式（||）
    std::shared_ptr<Expr> logicalOr();

    // 解析逻辑与表达式（&&）
    std::shared_ptr<Expr> logicalAnd();

    // 解析赋值表达式
    std::shared_ptr<Expr> assignment();

    // 解析等式表达式
    std::shared_ptr<Expr> equality();

    // 解析比较表达式
    std::shared_ptr<Expr> comparison();

    // 解析加减表达式
    std::shared_ptr<Expr> term();

    // 解析乘除表达式
    std::shared_ptr<Expr> factor();

    // 解析一元表达式
    std::shared_ptr<Expr> unary();

    // 解析函数调用表达式
    std::shared_ptr<Expr> call();

    // 解析基础表达式
    std::shared_ptr<Expr> primary();

    // 解析类声明
    std::shared_ptr<Stmt> classDeclaration();

    // 解析import语句
    std::shared_ptr<Stmt> importDeclaration();

    // 解析export语句
    std::shared_ptr<Stmt> exportDeclaration();
};

#endif //TINY_JS_PARSER_H
