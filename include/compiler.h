#ifndef TINY_JS_COMPILER_H
#define TINY_JS_COMPILER_H

#include "vm.h"
#include "ast.h"

struct Local
{
    std::string name;
    int depth;
    bool isCaptured;
    bool isConst;
};

struct Upvalue
{
    uint8_t index;
    bool isLocal;
    bool isConst;
};

struct ClassCompiler
{
    ClassCompiler* enclosing = nullptr;
    bool hasSuperclass = false;
};

struct CompilerState
{
    CompilerState* enclosing = nullptr;
    ObjFunction* function{};
    std::vector<Local> locals;
    std::vector<Upvalue> upvalues;
    int scopeDepth = 0;
};

class Compiler
{
    VM& vm;
    CompilerState* current = nullptr;

    // 获取当前编译函数的Chunk
    [[nodiscard]] Chunk* currentChunk() const;

    // 发出单个字节指令
    void emitByte(uint8_t b) const;

    // 发出两个字节指令
    void emitBytes(uint8_t b1, uint8_t b2) const;

    // 发出跳转指令，返回跳转指令的偏移位置
    [[nodiscard]] int emitJump(OpCode op) const;

    // 修补跳转指令的偏移位置
    void patchJump(int off) const;

    // 发出循环指令
    void emitLoop(int start) const;

    // 解析局部变量，返回变量在栈中的位置，找不到返回-1
    static int resolveLocal(const CompilerState* s, const std::string& n);

    // 添加上值，返回上值在上值列表中的位置
    static int addUpvalue(CompilerState* s, uint8_t idx, bool isLocal, bool isConst);

    // 解析上值，返回上值在上值列表中的位置，找不到返回-1
    static int resolveUpvalue(CompilerState* s, std::string& n);

    // 编译函数声明
    void compileFunction(const std::shared_ptr<FunctionStmt>& s, bool isMethod);

    // 编译匿名函数表达式
    void compileFunctionExpression(const std::shared_ptr<FunctionExpr>& expr);

public:
    explicit Compiler(VM& v) : vm(v)
    {
    }

    // 编译语句列表，返回编译后的函数对象
    ObjFunction* compile(const std::vector<std::shared_ptr<Stmt>>& stmts);

    // 编译单个语句
    void compileStmt(const std::shared_ptr<Stmt>& stmt);

    // 编译单个表达式
    void compileExpr(const std::shared_ptr<Expr>& expr);
};


#endif //TINY_JS_COMPILER_H
