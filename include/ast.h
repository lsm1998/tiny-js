#ifndef TINY_JS_AST_H
#define TINY_JS_AST_H

#include "token.h"
#include <utility>
#include <variant>
#include <vector>

struct Stmt;
struct Expr;

// AST Node
struct Node
{
    virtual ~Node() = default;
};

// 表达式
struct Expr : Node
{
    ~Expr() override = default;
};

// 语句
struct Stmt : Node
{
    ~Stmt() override = default;
};

// 二元表达式
struct Binary : Expr
{
    std::shared_ptr<Expr> left, right;
    Token op;

    Binary(auto l, auto o, auto r) : left(l), op(o), right(r)
    {
    }
};

// 字面量表达式
struct Literal : Expr
{
    std::variant<std::monostate, double, std::string, bool> value;

    explicit Literal(auto v) : value(v)
    {
    }
};

// 一元表达式
struct Unary : Expr
{
    Token op;
    std::shared_ptr<Expr> right;

    Unary(auto o, auto r) : op(o), right(r)
    {
    }
};

// 变量表达式
struct Variable : Expr
{
    Token name;

    Variable(auto n) : name(n)
    {
    }
};

// 赋值表达式
struct Assign : Expr
{
    Token name;
    std::shared_ptr<Expr> value;

    Assign(auto n, auto v) : name(n), value(v)
    {
    }
};

// 函数调用表达式
struct Call : Expr
{
    std::shared_ptr<Expr> callee;
    std::vector<std::shared_ptr<Expr>> args;

    Call(auto c, auto a) : callee(c), args(a)
    {
    }
};

// 自增自减表达式
struct UpdateExpr : Expr
{
    Token name;
    bool isIncrement;
    bool isPostfix;

    UpdateExpr(Token n, bool inc, bool post)
        : name(std::move(n)), isIncrement(inc), isPostfix(post)
    {
    }
};

// 表达式语句
struct ExpressionStmt : Stmt
{
    std::shared_ptr<Expr> expression;

    explicit ExpressionStmt(auto e) : expression(e)
    {
    }
};

// 变量声明语句
struct VarStmt : Stmt
{
    Token name;
    std::shared_ptr<Expr> initializer;
    bool isConst = false;

    VarStmt(auto n, auto i, auto c) : name(n), initializer(i), isConst(c)
    {
    }
};

// 代码块语句
struct BlockStmt : Stmt
{
    std::vector<std::shared_ptr<Stmt>> statements;

    explicit BlockStmt(auto s) : statements(s)
    {
    }
};

// If语句
struct IfStmt : Stmt
{
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> thenBranch, elseBranch;

    IfStmt(auto c, auto t, auto e) : condition(c), thenBranch(t), elseBranch(e)
    {
    }
};

// While语句
struct WhileStmt : Stmt
{
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;

    WhileStmt(auto c, auto b) : condition(c), body(b)
    {
    }
};

// 函数声明语句
struct FunctionStmt : Stmt
{
    Token name;
    std::vector<Token> params;
    std::vector<std::shared_ptr<Stmt>> body;

    FunctionStmt(auto n, auto p, auto b) : name(n), params(p), body(b)
    {
    }
};

// 返回语句
struct ReturnStmt : Stmt
{
    Token keyword;
    std::shared_ptr<Expr> value;

    ReturnStmt(auto k, auto v) : keyword(k), value(v)
    {
    }
};

// 数组表达式
struct ListExpr : Expr
{
    std::vector<std::shared_ptr<Expr>> elements;

    explicit ListExpr(auto e) : elements(e)
    {
    }
};

// 获取下标表达式
struct GetSubscriptExpr : Expr
{
    std::shared_ptr<Expr> list;
    std::shared_ptr<Expr> index;

    GetSubscriptExpr(auto l, auto i) : list(l), index(i)
    {
    }
};

// 设置下标表达式
struct SetSubscriptExpr : Expr
{
    std::shared_ptr<Expr> list;
    std::shared_ptr<Expr> index;
    std::shared_ptr<Expr> value;

    SetSubscriptExpr(auto l, auto i, auto v) : list(l), index(i), value(v)
    {
    }
};

// this 表达式
struct ThisExpr : Expr
{
    Token keyword;

    explicit ThisExpr(Token k) : keyword(std::move(k))
    {
    }
};

// 获取属性表达式: obj.field
struct GetExpr : Expr
{
    std::shared_ptr<Expr> object;
    Token name;

    GetExpr(auto o, auto n) : object(o), name(n)
    {
    }
};

// 设置属性表达式: obj.field = value
struct SetExpr : Expr
{
    std::shared_ptr<Expr> object;
    Token name;
    std::shared_ptr<Expr> value;

    SetExpr(auto o, auto n, auto v) : object(o), name(n), value(v)
    {
    }
};

// Class 声明语句
struct ClassStmt : Stmt
{
    Token name;
    std::vector<std::shared_ptr<FunctionStmt>> methods; // 方法列表
    ClassStmt(auto n, auto m) : name(n), methods(m)
    {
    }
};

#endif //TINY_JS_AST_H
