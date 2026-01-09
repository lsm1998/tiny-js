#include "compiler.h"
#include "debug.h"

Chunk* Compiler::currentChunk() const
{
    return &current->function->chunk;
}

void Compiler::emitByte(const uint8_t b) const
{
    currentChunk()->write(b);
}

void Compiler::emitBytes(const uint8_t b1, const uint8_t b2) const
{
    emitByte(b1);
    emitByte(b2);
}

int Compiler::emitJump(OpCode op) const
{
    emitByte(static_cast<uint8_t>(op));
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->code.size() - 2;
}

void Compiler::patchJump(const int off) const
{
    int j = currentChunk()->code.size() - off - 2;
    currentChunk()->code[off] = (j >> 8) & 0xff;
    currentChunk()->code[off + 1] = j & 0xff;
}

void Compiler::emitLoop(const int start) const
{
    emitByte(static_cast<uint8_t>(OpCode::OP_LOOP));
    const size_t o = currentChunk()->code.size() - start + 2;
    emitByte(o >> 8 & 0xff);
    emitByte(o & 0xff);
}

int Compiler::resolveLocal(const CompilerState* s, const std::string& n)
{
    for (int i = s->locals.size() - 1; i >= 0; i--) if (s->locals[i].name == n) return i;
    return -1;
}

int Compiler::addUpvalue(CompilerState* s, const uint8_t idx, const bool isLocal, const bool isConst)
{
    for (int i = 0; i < s->upvalues.size(); i++)
        if (s->upvalues[i].index == idx && s->upvalues[i].isLocal == isLocal)
            return i;
    s->upvalues.push_back({idx, isLocal, isConst});
    return s->function->upvalueCount++;
}

int Compiler::resolveUpvalue(CompilerState* s, std::string& n)
{
    if (!s->enclosing) return -1;
    int l = resolveLocal(s->enclosing, n);
    if (l != -1)
    {
        s->enclosing->locals[l].isCaptured = true;
        return addUpvalue(s, static_cast<uint8_t>(l), true, s->enclosing->locals[l].isConst);
    }
    int u = resolveUpvalue(s->enclosing, n);
    if (u != -1) return addUpvalue(s, static_cast<uint8_t>(u), false, s->enclosing->locals[l].isConst);
    return -1;
}

void Compiler::compileFunction(const std::shared_ptr<FunctionStmt>& s, const bool isMethod)
{
    int gIdx = -1;

    if (!isMethod)
    {
        if (current->scopeDepth > 0)
        {
            current->locals.push_back({s->name.lexeme, current->scopeDepth, false, false});
        }
        else
        {
            gIdx = currentChunk()->addConstant(vm.newString(s->name.lexeme));
        }
    }

    auto* next = new CompilerState();
    next->enclosing = current;
    next->function = vm.allocate<ObjFunction>();
    vm.tempRoots.push_back(next->function);

    // 设置函数基本信息
    next->function->name = s->name.lexeme;
    next->function->arity = static_cast<int>(s->params.size());

    if (isMethod)
    {
        next->locals.push_back({"this", 0, false, true});
    }
    else
    {
        next->locals.push_back({"", 0, false, false});
    }

    current = next;
    current->scopeDepth++;

    for (const auto& p : s->params)
    {
        // 参数作为局部变量加入作用域
        current->locals.push_back({p.lexeme, current->scopeDepth, false, false});
    }

    bool hasReturn = false;
    for (auto& b : s->body)
    {
        if (std::dynamic_pointer_cast<ReturnStmt>(b))
        {
            hasReturn = true;
        }
        compileStmt(b);
    }

    if (!hasReturn)
    {
        if (isMethod && s->name.lexeme == "constructor")
        {
            emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL), 0);
        }
        else
        {
            emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
        }

        emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
    }

    ObjFunction* f = current->function;
    const auto ups = current->upvalues;

    vm.tempRoots.pop_back();

    current = current->enclosing;
    delete next;

    const int idx = currentChunk()->addConstant(f);

    emitBytes(static_cast<uint8_t>(OpCode::OP_CLOSURE), static_cast<uint8_t>(idx));

    for (const auto& u : ups)
    {
        emitByte(u.isLocal ? 1 : 0);
        emitByte(u.index);
    }
    if (!isMethod && current->scopeDepth == 0)
    {
        emitBytes(static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL), static_cast<uint8_t>(gIdx));
    }
}

ObjFunction* Compiler::compile(const std::vector<std::shared_ptr<Stmt>>& stmts)
{
    current = new CompilerState();
    current->function = vm.allocate<ObjFunction>();
    vm.tempRoots.push_back(current->function); // GC Protect

    current->function->name = "<script>";
    current->locals.push_back({"", 0, false});
    for (auto& s : stmts) compileStmt(s);
    emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
    emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
    ObjFunction* f = current->function;

    vm.tempRoots.pop_back();
    delete current;
    return f;
}

void Compiler::compileStmt(const std::shared_ptr<Stmt>& stmt)
{
    if (const auto s = std::dynamic_pointer_cast<ExpressionStmt>(stmt))
    {
        compileExpr(s->expression);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    }
    else if (const auto var_stmt = std::dynamic_pointer_cast<VarStmt>(stmt))
    {
        if (var_stmt->initializer) compileExpr(var_stmt->initializer);
        else emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
        if (current->scopeDepth > 0)
        {
            current->locals.push_back({var_stmt->name.lexeme, current->scopeDepth, false, var_stmt->isConst});
        }
        else
        {
            // 全局变量
            const int i = currentChunk()->addConstant(vm.newString(var_stmt->name.lexeme));
            OpCode opCode = var_stmt->isConst ? OpCode::OP_DEFINE_GLOBAL_CONST : OpCode::OP_DEFINE_GLOBAL;
            emitBytes(static_cast<uint8_t>(opCode), static_cast<uint8_t>(i));
        }
    }
    else if (const auto block_stmt = std::dynamic_pointer_cast<BlockStmt>(stmt))
    {
        current->scopeDepth++;
        for (const auto& st : block_stmt->statements) compileStmt(st);
        current->scopeDepth--;
        while (!current->locals.empty() && current->locals.back().depth > current->scopeDepth)
        {
            if (current->locals.back().isCaptured) emitByte(static_cast<uint8_t>(OpCode::OP_CLOSE_UPVALUE));
            else emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            current->locals.pop_back();
        }
    }
    else if (const auto if_stmt = std::dynamic_pointer_cast<IfStmt>(stmt))
    {
        compileExpr(if_stmt->condition);
        const int tj = emitJump(OpCode::OP_JUMP_IF_FALSE);
        compileStmt(if_stmt->thenBranch);
        const int ej = emitJump(OpCode::OP_JUMP);
        patchJump(tj);
        if (if_stmt->elseBranch) compileStmt(if_stmt->elseBranch);
        patchJump(ej);
    }
    else if (const auto while_stmt = std::dynamic_pointer_cast<WhileStmt>(stmt))
    {
        const size_t ls = currentChunk()->code.size();
        compileExpr(while_stmt->condition);
        const int ej = emitJump(OpCode::OP_JUMP_IF_FALSE);
        compileStmt(while_stmt->body);
        emitLoop(static_cast<int>(ls));
        patchJump(ej);
    }
    else if (const auto function_stmt = std::dynamic_pointer_cast<FunctionStmt>(stmt))
    {
        compileFunction(function_stmt, false);
    }
    else if (const auto return_stmt = std::dynamic_pointer_cast<ReturnStmt>(stmt))
    {
        if (return_stmt->value) compileExpr(return_stmt->value);
        else emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
        emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
    }
    else if (const auto class_stmt = std::dynamic_pointer_cast<ClassStmt>(stmt))
    {
        const int nameIdx = currentChunk()->addConstant(vm.newString(class_stmt->name.lexeme));
        emitBytes(static_cast<uint8_t>(OpCode::OP_CLASS), static_cast<uint8_t>(nameIdx));
        emitBytes(static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL), static_cast<uint8_t>(nameIdx)); // 定义类名
        emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL), static_cast<uint8_t>(nameIdx));
        for (auto& method : class_stmt->methods)
        {
            const int constIdx = currentChunk()->addConstant(vm.newString(method->name.lexeme));

            // 编译方法体
            compileFunction(method, true);

            emitBytes(static_cast<uint8_t>(OpCode::OP_METHOD), static_cast<uint8_t>(constIdx));
        }
        emitByte(static_cast<uint8_t>(OpCode::OP_POP)); // 弹出类对象
    }
}

void Compiler::compileExpr(const std::shared_ptr<Expr>& expr)
{
    if (const auto e = std::dynamic_pointer_cast<Literal>(expr))
    {
        if (std::holds_alternative<double>(e->value))
            emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT),
                      static_cast<uint8_t>(currentChunk()->addConstant(
                          std::get<double>(e->value))));
        else if (std::holds_alternative<std::string>(e->value))
            emitBytes(
                static_cast<uint8_t>(OpCode::OP_CONSTANT),
                static_cast<uint8_t>(currentChunk()->addConstant(vm.newString(std::get<std::string>(e->value)))));
        else if (std::holds_alternative<bool>(e->value))
            emitByte(
                std::get<bool>(e->value)
                    ? static_cast<uint8_t>(OpCode::OP_TRUE)
                    : static_cast<uint8_t>(OpCode::OP_FALSE));
        else emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
    }
    else if (const auto ternary = std::dynamic_pointer_cast<Ternary>(expr))
    {
        compileExpr(ternary->condition);
        const int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
        compileExpr(ternary->thenExpr);
        const int endifJump = emitJump(OpCode::OP_JUMP);
        patchJump(elseJump);
        compileExpr(ternary->elseExpr);
        patchJump(endifJump);
    }
    else if (const auto binary = std::dynamic_pointer_cast<Binary>(expr))
    {
        compileExpr(binary->left);
        compileExpr(binary->right);
        if (const TokenType t = binary->op.type; t == TokenType::PLUS) emitByte(static_cast<uint8_t>(OpCode::OP_ADD));
        else if (t == TokenType::MINUS) emitByte(static_cast<uint8_t>(OpCode::OP_SUB));
        else if (t == TokenType::STAR) emitByte(static_cast<uint8_t>(OpCode::OP_MUL));
        else if (t == TokenType::SLASH) emitByte(static_cast<uint8_t>(OpCode::OP_DIV));
        else if (t == TokenType::PERCENT) emitByte(static_cast<uint8_t>(OpCode::OP_MOD));
        else if (t == TokenType::EQUAL_EQUAL) emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL));
        else if (t == TokenType::LESS) emitByte(static_cast<uint8_t>(OpCode::OP_LESS));
        else if (t == TokenType::LESS_EQUAL)
        {
            compileExpr(binary->right);
            compileExpr(binary->left);
            emitByte(static_cast<uint8_t>(OpCode::OP_GREATER));
        }
        else if (t == TokenType::GREATER) emitByte(static_cast<uint8_t>(OpCode::OP_GREATER));
        else if (t == TokenType::GREATER_EQUAL)
        {
            compileExpr(binary->right);
            compileExpr(binary->left);
            emitByte(static_cast<uint8_t>(OpCode::OP_LESS));
        }
        else if (t == TokenType::AND_AND)
        {
            compileExpr(binary->left);
            const int falseJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            compileExpr(binary->right);
            patchJump(falseJump);
        }
        else if (t == TokenType::OR_OR)
        {
            compileExpr(binary->left);
            const int trueJump = emitJump(OpCode::OP_JUMP_IF_TRUE);
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            compileExpr(binary->right);
            patchJump(trueJump);
        }
    }
    else if (const auto variable = std::dynamic_pointer_cast<Variable>(expr))
    {
        if (int arg = resolveLocal(current, variable->name.lexeme); arg != -1)
        {
            emitBytes(static_cast<uint8_t>(OpCode::OP_GET_LOCAL), static_cast<uint8_t>(arg));
        }
        else if ((arg = resolveUpvalue(current, variable->name.lexeme)) != -1)
            emitBytes(static_cast<uint8_t>(OpCode::OP_GET_UPVALUE), static_cast<uint8_t>(arg));
        else
            emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL),
                      static_cast<uint8_t>(currentChunk()->addConstant(vm.newString(variable->name.lexeme))));
    }
    else if (const auto assign = std::dynamic_pointer_cast<Assign>(expr))
    {
        compileExpr(assign->value);
        if (int arg = resolveLocal(current, assign->name.lexeme); arg != -1)
        {
            if (current->locals[arg].isConst)
            {
                throw std::runtime_error("Cannot assign to const variable '" + assign->name.lexeme + "'.");
            }
            emitBytes(static_cast<uint8_t>(OpCode::OP_SET_LOCAL), static_cast<uint8_t>(arg));
        }
        else if ((arg = resolveUpvalue(current, assign->name.lexeme)) != -1)
        {
            if (current->upvalues[arg].isConst)
            {
                throw std::runtime_error("Cannot assign to const variable '" + assign->name.lexeme + "'.");
            }
            emitBytes(static_cast<uint8_t>(OpCode::OP_SET_UPVALUE), static_cast<uint8_t>(arg));
        }
        else
        {
            emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL),
                      static_cast<uint8_t>(currentChunk()->addConstant(vm.newString(assign->name.lexeme))));
        }
    }
    else if (const auto call = std::dynamic_pointer_cast<Call>(expr))
    {
        compileExpr(call->callee);
        for (const auto& a : call->args) compileExpr(a);
        emitBytes(static_cast<uint8_t>(OpCode::OP_CALL), static_cast<uint8_t>(call->args.size()));
    }
    else if (const auto new_expr = std::dynamic_pointer_cast<NewExpr>(expr))
    {
        compileExpr(new_expr->callee);
        for (const auto& a : new_expr->args) compileExpr(a);
        emitBytes(static_cast<uint8_t>(OpCode::OP_NEW), static_cast<uint8_t>(new_expr->args.size()));
    }
    else if (const auto list_expr = std::dynamic_pointer_cast<ListExpr>(expr))
    {
        for (auto& element : list_expr->elements)
        {
            compileExpr(element);
        }
        emitBytes(static_cast<uint8_t>(OpCode::OP_BUILD_LIST), static_cast<uint8_t>(list_expr->elements.size()));
    }
    else if (const auto get_subscript_expr = std::dynamic_pointer_cast<GetSubscriptExpr>(expr))
    {
        compileExpr(get_subscript_expr->list);
        compileExpr(get_subscript_expr->index);
        emitByte(static_cast<uint8_t>(OpCode::OP_GET_SUBSCRIPT));
    }
    else if (const auto set_subscript_expr = std::dynamic_pointer_cast<SetSubscriptExpr>(expr))
    {
        compileExpr(set_subscript_expr->list);
        compileExpr(set_subscript_expr->index);
        compileExpr(set_subscript_expr->value);
        emitByte(static_cast<uint8_t>(OpCode::OP_SET_SUBSCRIPT));
    }
    else if (const auto this_expr = std::dynamic_pointer_cast<ThisExpr>(expr))
    {
        compileExpr(std::make_shared<Variable>(this_expr->keyword));
    }
    else if (const auto get_expr = std::dynamic_pointer_cast<GetExpr>(expr))
    {
        compileExpr(get_expr->object);
        const int nameIdx = currentChunk()->addConstant(vm.newString(get_expr->name.lexeme));
        emitBytes(static_cast<uint8_t>(OpCode::OP_GET_PROPERTY), static_cast<uint8_t>(nameIdx));
    }
    else if (const auto set_expr = std::dynamic_pointer_cast<SetExpr>(expr))
    {
        compileExpr(set_expr->object);
        compileExpr(set_expr->value);
        const int nameIdx = currentChunk()->addConstant(vm.newString(set_expr->name.lexeme));
        emitBytes(static_cast<uint8_t>(OpCode::OP_SET_PROPERTY), static_cast<uint8_t>(nameIdx));
    }
    else if (const auto update = std::dynamic_pointer_cast<UpdateExpr>(expr))
    {
        int arg = resolveLocal(current, update->name.lexeme);
        OpCode getOp, setOp;
        int index = arg;

        if (arg != -1)
        {
            getOp = OpCode::OP_GET_LOCAL;
            setOp = OpCode::OP_SET_LOCAL;
        }
        else if ((arg = resolveUpvalue(current, update->name.lexeme)) != -1)
        {
            getOp = OpCode::OP_GET_UPVALUE;
            setOp = OpCode::OP_SET_UPVALUE;
            index = arg;
        }
        else
        {
            getOp = OpCode::OP_GET_GLOBAL;
            setOp = OpCode::OP_SET_GLOBAL;
            index = currentChunk()->addConstant(vm.newString(update->name.lexeme));
        }

        emitBytes(static_cast<uint8_t>(getOp), static_cast<uint8_t>(index));
        emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT),
                  static_cast<uint8_t>(currentChunk()->addConstant(1.0)));

        if (update->isIncrement) emitByte(static_cast<uint8_t>(OpCode::OP_ADD));
        else emitByte(static_cast<uint8_t>(OpCode::OP_SUB));

        emitBytes(static_cast<uint8_t>(setOp), static_cast<uint8_t>(index));

        if (update->isPostfix)
        {
            emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT),
                      static_cast<uint8_t>(currentChunk()->addConstant(1.0)));

            if (update->isIncrement) emitByte(static_cast<uint8_t>(OpCode::OP_SUB)); // i++: new - 1 = old
            else emitByte(static_cast<uint8_t>(OpCode::OP_ADD)); // i--: new + 1 = old
        }
    }
}
