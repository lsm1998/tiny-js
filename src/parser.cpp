#include "parser.h"

std::vector<std::shared_ptr<Stmt>> Parser::parse()
{
    std::vector<std::shared_ptr<Stmt>> s;
    while (!isAtEnd()) s.push_back(declaration());
    return s;
}

bool Parser::match(const TokenType t)
{
    if (check(t))
    {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(const TokenType t)
{
    return !isAtEnd() && peek().type == t;
}

Token Parser::advance()
{
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd()
{
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::peek()
{
    return tokens[current];
}

Token Parser::previous()
{
    return tokens[current - 1];
}

Token Parser::consume(const TokenType t, const std::string& m)
{
    if (check(t)) return advance();
    const Token wrongToken = previous();
    const std::string message = "[" + filename + ":" + std::to_string(wrongToken.line) + "] Error: " + m;
    throw std::runtime_error(message);
}

std::shared_ptr<Stmt> Parser::declaration()
{
    if (match(TokenType::CLASS)) return classDeclaration();
    if (match(TokenType::FUN)) return function("function");
    if (match(TokenType::VAR)) return varDeclaration(false);
    if (match(TokenType::CONST)) return varDeclaration(true);
    return statement();
}

std::shared_ptr<Stmt> Parser::function(const std::string& k)
{
    Token n = consume(TokenType::IDENTIFIER, "Expect name.");
    consume(TokenType::LEFT_PAREN, "Expect '('.");
    std::vector<Token> p;
    if (!check(TokenType::RIGHT_PAREN))
    {
        do { p.push_back(consume(TokenType::IDENTIFIER, "param")); }
        while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')'.");
    consume(TokenType::LEFT_BRACE, "Expect '{'.");
    const auto b = std::static_pointer_cast<BlockStmt>(block());
    return std::make_shared<FunctionStmt>(n, p, b->statements);
}

std::shared_ptr<Stmt> Parser::varDeclaration(bool isConst)
{
    Token n = consume(TokenType::IDENTIFIER, "Expect var name.");
    std::shared_ptr<Expr> i = nullptr;
    if (match(TokenType::EQUAL)) i = expression();
    consume(TokenType::SEMICOLON, "Expect ';'.");
    return std::make_shared<VarStmt>(n, i, isConst);
}

std::shared_ptr<Stmt> Parser::statement()
{
    if (match(TokenType::IF)) return ifStmt();
    if (match(TokenType::WHILE)) return whileStmt();
    if (match(TokenType::FOR)) return forStmt();
    if (match(TokenType::RETURN)) return returnStmt();
    if (match(TokenType::LEFT_BRACE)) return block();
    return exprStmt();
}

std::shared_ptr<Stmt> Parser::block()
{
    std::vector<std::shared_ptr<Stmt>> s;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) s.push_back(declaration());
    consume(TokenType::RIGHT_BRACE, "Expect '}'.");
    return std::make_shared<BlockStmt>(s);
}

std::shared_ptr<Stmt> Parser::exprStmt()
{
    auto e = expression();
    consume(TokenType::SEMICOLON, "Expect ';'.");
    return std::make_shared<ExpressionStmt>(e);
}

std::shared_ptr<Stmt> Parser::ifStmt()
{
    consume(TokenType::LEFT_PAREN, "Expect '('.");
    auto c = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')'.");
    auto t = statement();
    std::shared_ptr<Stmt> e = nullptr;
    if (match(TokenType::ELSE)) e = statement();
    return std::make_shared<IfStmt>(c, t, e);
}

std::shared_ptr<Stmt> Parser::whileStmt()
{
    consume(TokenType::LEFT_PAREN, "Expect '('.");
    auto c = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')'.");
    return std::make_shared<WhileStmt>(c, statement());
}


std::shared_ptr<Stmt> Parser::forStmt()
{
    consume(TokenType::LEFT_PAREN, "Expect '('.");
    std::shared_ptr<Stmt> init;
    if (match(TokenType::SEMICOLON)) init = nullptr;
    else if (match(TokenType::VAR)) init = varDeclaration(false);
    else init = exprStmt();
    std::shared_ptr<Expr> cond;
    if (!check(TokenType::SEMICOLON)) cond = expression();
    consume(TokenType::SEMICOLON, "Expect ';'.");
    std::shared_ptr<Expr> inc;
    if (!check(TokenType::RIGHT_PAREN)) inc = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')'.");
    auto body = statement();
    if (inc)
    {
        std::vector<std::shared_ptr<Stmt>> s = {body, std::make_shared<ExpressionStmt>(inc)};
        body = std::make_shared<BlockStmt>(s);
    }
    if (!cond) cond = std::make_shared<Literal>(true);
    body = std::make_shared<WhileStmt>(cond, body);
    if (init)
    {
        std::vector s = {init, body};
        body = std::make_shared<BlockStmt>(s);
    }
    return body;
}

std::shared_ptr<Stmt> Parser::returnStmt()
{
    Token k = previous();
    std::shared_ptr<Expr> v;
    if (!check(TokenType::SEMICOLON)) v = expression();
    consume(TokenType::SEMICOLON, "Expect ';'.");
    return std::make_shared<ReturnStmt>(k, v);
}

std::shared_ptr<Expr> Parser::expression()
{
    return conditional();
}

std::shared_ptr<Expr> Parser::conditional()
{
    auto e = assignment();
    if (match(TokenType::QUESTION))
    {
        auto thenExpr = assignment();
        consume(TokenType::COLON, "Expect ':' after then part of conditional expression.");
        auto elseExpr = conditional();
        return std::make_shared<Ternary>(e, thenExpr, elseExpr);
    }
    return e;
}

std::shared_ptr<Expr> Parser::logicalOr()
{
    auto e = logicalAnd();
    while (match(TokenType::OR_OR))
    {
        Token op = previous();
        e = std::make_shared<Binary>(e, op, logicalAnd());
    }
    return e;
}

std::shared_ptr<Expr> Parser::logicalAnd()
{
    auto e = equality();
    while (match(TokenType::AND_AND))
    {
        Token op = previous();
        e = std::make_shared<Binary>(e, op, equality());
    }
    return e;
}

std::shared_ptr<Expr> Parser::assignment()
{
    auto e = logicalOr();
    if (match(TokenType::EQUAL))
    {
        Token eq = previous();
        auto v = assignment();
        if (const auto get = std::dynamic_pointer_cast<GetExpr>(e))
        {
            return std::make_shared<SetExpr>(get->object, get->name, v);
        }
        if (const auto var = std::dynamic_pointer_cast<Variable>(e))
        {
            return std::make_shared<Assign>(var->name, v);
        }
        if (const auto sub = std::dynamic_pointer_cast<GetSubscriptExpr>(e))
        {
            return std::make_shared<SetSubscriptExpr>(sub->list, sub->index, v);
        }
        throw std::runtime_error("Invalid assignment target.");
    }
    if (match(TokenType::PLUS_EQUAL))
    {
        auto v = assignment();

        if (const auto var = std::dynamic_pointer_cast<Variable>(e))
        {
            auto right = std::make_shared<Binary>(
                e,
                Token{TokenType::PLUS, "+", var->name.line, {}},
                v
            );
            return std::make_shared<Assign>(var->name, right);
        }
        throw std::runtime_error("Invalid target for '+='.");
    }
    if (match(TokenType::MINUS_EQUAL))
    {
        auto v = assignment();
        if (const auto var = std::dynamic_pointer_cast<Variable>(e))
        {
            auto right = std::make_shared<Binary>(
                e,
                Token{TokenType::PLUS, "-", var->name.line, {}},
                v
            );
            return std::make_shared<Assign>(var->name, right);
        }
        throw std::runtime_error("Invalid target for '-='.");
    }
    if (match(TokenType::STAR_EQUAL))
    {
        auto v = assignment();
        if (const auto var = std::dynamic_pointer_cast<Variable>(e))
        {
            auto right = std::make_shared<Binary>(
                e,
                Token{TokenType::STAR, "*", var->name.line, {}},
                v
            );
            return std::make_shared<Assign>(var->name, right);
        }
        throw std::runtime_error("Invalid target for '*='.");
    }
    if (match(TokenType::SLASH_EQUAL))
    {
        auto v = assignment();
        if (const auto var = std::dynamic_pointer_cast<Variable>(e))
        {
            auto right = std::make_shared<Binary>(
                e,
                Token{TokenType::SLASH, "/", var->name.line, {}},
                v
            );
            return std::make_shared<Assign>(var->name, right);
        }
        throw std::runtime_error("Invalid target for '/='.");
    }
    if (match(TokenType::PERCENT_EQUAL))
    {
        auto v = assignment();
        if (const auto var = std::dynamic_pointer_cast<Variable>(e))
        {
            auto right = std::make_shared<Binary>(
                e,
                Token{TokenType::PERCENT, "%", var->name.line, {}},
                v
            );
            return std::make_shared<Assign>(var->name, right);
        }
        throw std::runtime_error("Invalid target for '%='.");
    }
    return e;
}

std::shared_ptr<Expr> Parser::equality()
{
    auto e = comparison();
    while (match(TokenType::BANG_EQUAL) || match(TokenType::EQUAL_EQUAL))
    {
        Token op = previous();
        e = std::make_shared<Binary>(e, op, comparison());
    }
    return e;
}

std::shared_ptr<Expr> Parser::comparison()
{
    auto e = term();
    while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) || match(TokenType::LESS) ||
        match(TokenType::LESS_EQUAL))
    {
        Token op = previous();
        e = std::make_shared<Binary>(e, op, term());
    }
    return e;
}

std::shared_ptr<Expr> Parser::term()
{
    auto e = factor();
    while (match(TokenType::MINUS) || match(TokenType::PLUS))
    {
        Token op = previous();
        e = std::make_shared<Binary>(e, op, factor());
    }
    return e;
}

std::shared_ptr<Expr> Parser::factor()
{
    auto e = unary();
    while (match(TokenType::SLASH) || match(TokenType::STAR) || match(TokenType::PERCENT))
    {
        Token op = previous();
        e = std::make_shared<Binary>(e, op, unary());
    }
    return e;
}

std::shared_ptr<Expr> Parser::unary()
{
    if (match(TokenType::BANG) || match(TokenType::MINUS)) return std::make_shared<Unary>(previous(), unary());

    if (match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS))
    {
        const Token op = previous();
        const auto right = primary();
        if (const auto var = std::dynamic_pointer_cast<Variable>(right))
        {
            bool isInc = (op.type == TokenType::PLUS_PLUS);
            return std::make_shared<UpdateExpr>(var->name, isInc, false);
        }
        throw std::runtime_error("[" + filename + ":" + std::to_string(op.line) + "] Error: Invalid target for prefix update.");
    }

    if (match(TokenType::NEW))
    {
        // 解析 new 表达式: new ClassName(args)
        auto callee = primary();
        if (match(TokenType::LEFT_PAREN))
        {
            std::vector<std::shared_ptr<Expr>> args;
            if (!check(TokenType::RIGHT_PAREN))
            {
                do args.push_back(expression()); while (match(TokenType::COMMA));
            }
            consume(TokenType::RIGHT_PAREN, "Expect ')'.");
            return std::make_shared<NewExpr>(callee, args);
        }
        throw std::runtime_error("Expect '(' after class name in 'new' expression.");
    }

    return call();
}

std::shared_ptr<Expr> Parser::call()
{
    auto e = primary();
    while (true)
    {
        if (match(TokenType::LEFT_PAREN))
        {
            std::vector<std::shared_ptr<Expr>> args;
            if (!check(TokenType::RIGHT_PAREN))
            {
                do args.push_back(expression()); while (match(TokenType::COMMA));
            }
            consume(TokenType::RIGHT_PAREN, "Expect ')'.");
            e = std::make_shared<Call>(e, args);
        }
        else if (match(TokenType::DOT))
        {
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            e = std::make_shared<GetExpr>(e, name);
        }
        else if (match(TokenType::LEFT_BRACKET))
        {
            auto index = expression();
            consume(TokenType::RIGHT_BRACKET, "Expect ']' after subscript.");
            e = std::make_shared<GetSubscriptExpr>(e, index);
        }
        else if (match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS))
        {
            Token op = previous();
            if (auto var = std::dynamic_pointer_cast<Variable>(e))
            {
                bool isInc = (op.type == TokenType::PLUS_PLUS);
                e = std::make_shared<UpdateExpr>(var->name, isInc, true);
            }
            else
            {
                throw std::runtime_error("Invalid target for postfix update.");
            }
        }
        else break;
    }
    return e;
}

std::shared_ptr<Expr> Parser::primary()
{
    if (match(TokenType::THIS)) return std::make_shared<ThisExpr>(previous());
    if (match(TokenType::FALSE)) return std::make_shared<Literal>(false);
    if (match(TokenType::TRUE)) return std::make_shared<Literal>(true);
    if (match(TokenType::NULLPTR)) return std::make_shared<Literal>(std::monostate{});
    if (match(TokenType::NUMBER)) return std::make_shared<Literal>(std::get<double>(previous().literal));
    if (match(TokenType::STRING)) return std::make_shared<Literal>(std::get<std::string>(previous().literal));
    if (match(TokenType::IDENTIFIER)) return std::make_shared<Variable>(previous());
    if (match(TokenType::LEFT_PAREN))
    {
        auto e = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')'.");
        return e;
    }
    if (match(TokenType::LEFT_BRACKET))
    {
        std::vector<std::shared_ptr<Expr>> elements;
        if (!check(TokenType::RIGHT_BRACKET))
        {
            do
            {
                elements.push_back(expression());
            }
            while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_BRACKET, "Expect ']' after list.");
        return std::make_shared<ListExpr>(elements);
    }
    const auto prev = previous();
    throw std::runtime_error("[" + filename + ":" + std::to_string(prev.line) + "] Error: Expect expression.");
}

std::shared_ptr<Stmt> Parser::classDeclaration()
{
    Token name = consume(TokenType::IDENTIFIER, "Expect class name.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before class body.");

    std::vector<std::shared_ptr<FunctionStmt>> methods;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
    {
        methods.push_back(std::static_pointer_cast<FunctionStmt>(function("method")));
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");

    return std::make_shared<ClassStmt>(name, methods);
}
