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

Token Parser::peekNext()
{
    if (current + 1 >= tokens.size()) return tokens[current];
    return tokens[current + 1];
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
    if (match(TokenType::IMPORT)) return importDeclaration();
    if (match(TokenType::EXPORT)) return exportDeclaration();
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

    // 箭头函数作为初始化器时，分号是可选的
    if (std::dynamic_pointer_cast<ArrowFunctionExpr>(i))
    {
        match(TokenType::SEMICOLON);
    }
    else
    {
        consume(TokenType::SEMICOLON, "Expect ';'.");
    }

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
                Token{TokenType::MINUS, "-", var->name.line, {}},
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
    while (match(TokenType::BANG_EQUAL) || match(TokenType::EQUAL_EQUAL) ||
           match(TokenType::BANG_EQUAL_EQUAL) || match(TokenType::EQUAL_EQUAL_EQUAL))
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
    if (match(TokenType::BANG) || match(TokenType::MINUS))
    {
        const Token op = previous();
        return std::make_shared<Unary>(op, unary());
    }

    if (match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS))
    {
        const Token op = previous();
        const auto right = primary();
        if (const auto var = std::dynamic_pointer_cast<Variable>(right))
        {
            bool isInc = op.type == TokenType::PLUS_PLUS;
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
        else if (match(TokenType::ARROW))
        {
            // 箭头函数: params => body
            std::vector<Token> params;
            if (auto var = std::dynamic_pointer_cast<Variable>(e))
            {
                // 单个参数，无括号: x => expr
                params.push_back(var->name);
            }
            else if (auto paren = std::dynamic_pointer_cast<Expr>(e))
            {
                // 括号内的参数列表: (x, y) => expr
                // 此时 e 是括号内的表达式，应该是参数列表
                // 实际上这个情况会在 primary() 中处理，当遇到 '(' 时
                // 我们需要回溯，但更简单的方法是在 primary() 中直接检测箭头函数
                throw std::runtime_error("[" + filename + ":" + std::to_string(previous().line) + "] Error: Arrow function parsing error.");
            }
            else
            {
                throw std::runtime_error("[" + filename + ":" + std::to_string(previous().line) + "] Error: Invalid arrow function syntax.");
            }

            // 解析函数体
            std::vector<std::shared_ptr<Stmt>> body;
            if (check(TokenType::LEFT_BRACE))
            {
                // 块体: => { stmts }
                consume(TokenType::LEFT_BRACE, "Expect '{'.");
                auto blockStmt = std::static_pointer_cast<BlockStmt>(block());
                body = blockStmt->statements;
            }
            else
            {
                // 简写: => expr
                // 将表达式包装在 return 语句中
                auto expr = expression();
                body.push_back(std::make_shared<ReturnStmt>(Token{TokenType::RETURN, "return", previous().line, {}}, expr));
            }

            e = std::make_shared<ArrowFunctionExpr>(params, body);
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
        // 检查是否是箭头函数的参数列表
        // 只有 (IDENTIFIER COMMA ...) 或 (IDENTIFIER RIGHT_PAREN ARROW) 或 () ARROW 时才是箭头函数

        // 检查是否是 () => 的情况
        // match(LEFT_PAREN) 已经推进了 current，所以现在 current 指向 )
        // 如果 ) 后面是 =>，则是无参数箭头函数
        if (check(TokenType::RIGHT_PAREN) && current + 1 < tokens.size() && tokens[current + 1].type == TokenType::ARROW)
        {
            // 是 () => 形式的箭头函数
            advance(); // 消费 )
            match(TokenType::ARROW); // 消费 =>
            std::vector<std::shared_ptr<Stmt>> body;
            if (check(TokenType::LEFT_BRACE))
            {
                // 块体
                consume(TokenType::LEFT_BRACE, "Expect '{'.");
                auto blockStmt = std::static_pointer_cast<BlockStmt>(block());
                body = blockStmt->statements;
            }
            else
            {
                // 简写: => expr
                // 将表达式包装在 return 语句中
                auto expr = expression();
                body.push_back(std::make_shared<ReturnStmt>(Token{TokenType::RETURN, "return", previous().line, {}}, expr));
            }
            return std::make_shared<ArrowFunctionExpr>(std::vector<Token>(), body);
        }

        // 检查是否是 (params) => 的情况
        if (check(TokenType::IDENTIFIER))
        {
            // 检查标识符后面是否是逗号或右括号
            Token next = peekNext();
            if (next.type == TokenType::COMMA || next.type == TokenType::RIGHT_PAREN)
            {
                // 可能是箭头函数: (params) => body
                // 先解析参数列表
                std::vector<Token> params;
                do { params.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name.")); }
                while (match(TokenType::COMMA));
                consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");

                // 检查是否有 =>
                if (match(TokenType::ARROW))
                {
                    // 是箭头函数
                    std::vector<std::shared_ptr<Stmt>> body;
                    if (check(TokenType::LEFT_BRACE))
                    {
                        // 块体
                        consume(TokenType::LEFT_BRACE, "Expect '{'.");
                        auto blockStmt = std::static_pointer_cast<BlockStmt>(block());
                        body = blockStmt->statements;
                    }
                    else
                    {
                        // 简写: => expr
                        // 将表达式包装在 return 语句中
                        auto expr = expression();
                        body.push_back(std::make_shared<ReturnStmt>(Token{TokenType::RETURN, "return", previous().line, {}}, expr));
                    }
                    return std::make_shared<ArrowFunctionExpr>(params, body);
                }
            }
        }

        // 普通括号表达式
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
    if (match(TokenType::LEFT_BRACE))
    {
        // 对象字面量: { key: value, ... }
        std::vector<ObjectExpr::Property> properties;
        if (!check(TokenType::RIGHT_BRACE))
        {
            do
            {
                // 解析 key (可以是标识符或字符串)
                Token key;
                if (match(TokenType::IDENTIFIER))
                {
                    key = previous();
                }
                else if (match(TokenType::STRING))
                {
                    key = previous();
                }
                else
                {
                    throw std::runtime_error("[" + filename + ":" + std::to_string(peek().line) + "] Error: Expect property name.");
                }

                consume(TokenType::COLON, "Expect ':' after property name.");
                std::shared_ptr<Expr> value = expression();
                properties.push_back({key, value});
            }
            while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' after object literal.");
        return std::make_shared<ObjectExpr>(properties);
    }
    if (match(TokenType::FUN))
    {
        auto name = Token{}; // 空名称表示匿名函数
        if (check(TokenType::IDENTIFIER))
        {
            name = consume(TokenType::IDENTIFIER, "Expect function name.");
        }
        consume(TokenType::LEFT_PAREN, "Expect '('.");
        std::vector<Token> params;
        if (!check(TokenType::RIGHT_PAREN))
        {
            do { params.push_back(consume(TokenType::IDENTIFIER, "param")); }
            while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')'.");
        consume(TokenType::LEFT_BRACE, "Expect '{'.");
        const auto body = std::static_pointer_cast<BlockStmt>(block())->statements;
        return std::make_shared<FunctionExpr>(name, params, body);
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

std::shared_ptr<Stmt> Parser::importDeclaration()
{
    // import { add, PI } from "util.js";
    consume(TokenType::LEFT_BRACE, "Expect '{' after import.");

    std::vector<Token> specifiers;
    if (!check(TokenType::RIGHT_BRACE))
    {
        do
        {
            specifiers.push_back(consume(TokenType::IDENTIFIER, "Expect identifier in import list."));
        }
        while (match(TokenType::COMMA));
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after import list.");
    consume(TokenType::FROM, "Expect 'from' after import list.");
    Token source = consume(TokenType::STRING, "Expect module path string.");
    consume(TokenType::SEMICOLON, "Expect ';' after import statement.");

    return std::make_shared<ImportStmt>(specifiers, source);
}

std::shared_ptr<Stmt> Parser::exportDeclaration()
{
    consume(TokenType::LEFT_BRACE, "Expect '{' after export.");

    std::vector<Token> specifiers;
    if (!check(TokenType::RIGHT_BRACE))
    {
        do
        {
            specifiers.push_back(consume(TokenType::IDENTIFIER, "Expect identifier in export list."));
        }
        while (match(TokenType::COMMA));
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after export list.");
    consume(TokenType::SEMICOLON, "Expect ';' after export statement.");

    return std::make_shared<ExportStmt>(specifiers);
}
