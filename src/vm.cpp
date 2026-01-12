#include "vm.h"
#include "debug.h"
#include "scanner.h"
#include "parser.h"
#include "compiler.h"
#include "native/require.h"
#include "native/base.h"
#include "native/file.h"
#include "native/string.h"
#include <iostream>

bool toBool(const Value value)
{
    if (std::holds_alternative<std::monostate>(value))
        return false;
    if (std::holds_alternative<bool>(value))
        return std::get<bool>(value);
    if (std::holds_alternative<double>(value))
        return std::get<double>(value) != 0;
    return true;
}

void VM::initModule()
{
    this->compilerHook = [&](const std::string& source, const std::string& filename) -> ObjFunction*
    {
        Scanner scanner(source);
        const auto tokens = scanner.scanTokens();
        Parser parser(tokens, filename);
        try
        {
            const auto stmts = parser.parse();
            Compiler compiler(*this);
            return compiler.compile(stmts);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Compile Error: " << e.what() << std::endl;
            return nullptr;
        }
    };
}


void VM::bindNativeMethod(const ObjType type, const std::string& name, const NativeFn& fn)
{
    if (type == ObjType::STRING)
    {
        stringMethods[name] = allocate<ObjNative>(fn, name);
    }
    else if (type == ObjType::LIST)
    {
        listMethods[name] = allocate<ObjNative>(fn, name);
    }
}

void VM::defineNativeClass(const std::string& className, std::map<std::string, NativeFn> methods)
{
    auto* klass = allocate<ObjClass>(className);
    klass->isNative = true; // 标记为原生

    // 注册方法
    for (auto& [name, fn] : methods)
    {
        klass->nativeMethods[name] = allocate<ObjNative>(fn, name);
    }

    // 注册全局变量
    stack.emplace_back(klass);
    globals[className] = klass;
    stack.pop_back();
}

ObjString* VM::newString(const std::string& s)
{
    return allocate<ObjString>(s);
}

void VM::freeObjects()
{
    const Obj* obj = objects;
    while (obj)
    {
        const Obj* next = obj->next;
        delete obj;
        obj = next;
    }
    objects = nullptr;
}

void VM::collectGarbage()
{
    markRoots();
    traceReferences();
    sweep();
    nextGC = bytesAllocated * 2;
}

void VM::markRoots()
{
    for (auto& v : stack) markValue(v);
    for (auto& [k, v] : globals) markValue(v);
    for (const auto& f : frames) markObject(f.closure);
    for (ObjUpvalue* u = openUpvalues; u; u = u->nextUp) markObject(u);
    for (Obj* o : tempRoots) markObject(o);
}

void VM::markValue(const Value& v)
{
    if (std::holds_alternative<Obj*>(v)) markObject(std::get<Obj*>(v));
}


void VM::markObject(Obj* o)
{
    if (!o || o->isMarked) return;
    o->isMarked = true;
    grayStack.push_back(o);
}

void VM::traceReferences()
{
    while (!grayStack.empty())
    {
        Obj* o = grayStack.back();
        grayStack.pop_back();
        if (o->type == ObjType::CLASS)
        {
            auto c = dynamic_cast<ObjClass*>(o);
            for (auto& [k, v] : c->methods) markObject(v);
        }
        else if (o->type == ObjType::INSTANCE)
        {
            auto i = dynamic_cast<ObjInstance*>(o);
            markObject(i->klass);
            for (auto& [k, v] : i->fields) markValue(v);
        }
        else if (o->type == ObjType::BOUND_METHOD)
        {
            auto b = dynamic_cast<ObjBoundMethod*>(o);
            markValue(b->receiver);
            markObject(b->method);
        }
        else if (o->type == ObjType::LIST)
        {
            for (const auto list = dynamic_cast<ObjList*>(o); auto& val : list->elements)
            {
                markValue(val);
            }
        }
        else if (o->type == ObjType::CLOSURE)
        {
            const auto c = dynamic_cast<ObjClosure*>(o);
            markObject(c->function);
            for (auto u : c->upvalues) markObject(u);
        }
        else if (o->type == ObjType::FUNCTION)
        {
            auto f = dynamic_cast<ObjFunction*>(o);
            for (auto& c : f->chunk.constants) markValue(c);
        }
        else if (o->type == ObjType::UPVALUE) markValue(dynamic_cast<ObjUpvalue*>(o)->closedValue);
    }
}

void VM::sweep()
{
    Obj* prev = nullptr;
    Obj* obj = objects;
    while (obj)
    {
        if (obj->isMarked)
        {
            obj->isMarked = false;
            prev = obj;
            obj = obj->next;
        }
        else
        {
            const Obj* unreached = obj;
            obj = obj->next;
            if (prev) prev->next = obj;
            else objects = obj;
            delete unreached;
        }
    }
}

void VM::defineNative(const std::string& name, const NativeFn& fn)
{
    auto* n = allocate<ObjNative>(fn, name);
    stack.emplace_back(n);
    globals[name] = n;
    stack.pop_back();
}

void VM::registerNative()
{
    defineNative("now", [this](auto argc, auto args) -> Value
    {
        return nativeNow(*this, argc, args);
    });

    defineNative("print", [this](auto argc, auto args) -> Value
    {
        return nativePrint(*this, argc, args);
    });

    defineNative("println", [this](auto argc, auto args) -> Value
    {
        return nativePrintln(*this, argc, args);
    });

    defineNative("require", [this](auto argc, auto args) -> Value
    {
        return nativeRequire(*this, argc, args);
    });

    defineNative("sleep", [this](auto argc, auto args) -> Value
    {
        return nativeSleep(*this, argc, args);
    });

    defineNative("getEnv", [this](auto argc, auto args) -> Value
    {
        return nativeGetEnv(*this, argc, args);
    });

    defineNative("setEnv", [this](auto argc, auto args) -> Value
    {
        return nativeSetEnv(*this, argc, args);
    });

    defineNative("setTimeout", [this](auto argc, auto args) -> Value
    {
        return nativeSetTimeout(*this, argc, args);
    });

    defineNative("setInterval", [this](auto argc, auto args) -> Value
    {
        return nativeSetInterval(*this, argc, args);
    });

    defineNative("clearInterval", [this](auto argc, auto args) -> Value
    {
        return nativeClearInterval(*this, argc, args);
    });

    defineNative("exit", [this](auto argc, auto args) -> Value
    {
        return nativeExit(*this, argc, args);
    });

    defineNative("typeof", [this](auto argc, auto args) -> Value
    {
        return nativeTypeof(*this, argc, args);
    });

    bindNativeMethod(ObjType::LIST, "clear", [this](auto argCount, auto args) -> Value
                     {
                         return nativeListClear(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::LIST, "push", [this](auto argCount, auto args) -> Value
                     {
                         return nativeListPush(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::LIST, "pop", [this](auto argCount, auto args) -> Value
                     {
                         return nativeListPop(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::LIST, "join", [this](auto argCount, auto args) -> Value
                     {
                         return nativeListJoin(*this, argCount, args);
                     }
    );


    // 注册File
    registerNativeFile(*this);
    // 注册字符串原生方法
    registerNativeString(*this);
}

ObjUpvalue* VM::captureUpvalue(Value* local)
{
    ObjUpvalue* prev = nullptr;
    ObjUpvalue* up = openUpvalues;
    while (up && up->location > local)
    {
        prev = up;
        up = up->nextUp;
    }
    if (up && up->location == local) return up;
    auto* created = allocate<ObjUpvalue>(local);
    created->nextUp = up;
    if (prev == nullptr) openUpvalues = created;
    else prev->nextUp = created;
    return created;
}

void VM::closeUpvalues(const Value* last)
{
    while (openUpvalues && openUpvalues->location >= last)
    {
        ObjUpvalue* up = openUpvalues;
        // 把值从栈上搬到堆上
        up->closedValue = *up->location;
        // 修改 location 指针，指向堆上的 closedValue
        up->location = &up->closedValue;
        openUpvalues = up->nextUp;
    }
}

void VM::interpret(ObjFunction* script)
{
    stack.clear();
    frames.clear();
    openUpvalues = nullptr;
    auto* closure = allocate<ObjClosure>(script);
    stack.emplace_back(closure);
    frames.push_back({closure, script->chunk.code.data(), 0});
    run();
}

void VM::runtimeError(const char* format)
{
    std::cerr << "Runtime Error: " << format << "\n";
    stack.clear();
    frames.clear();
}

void VM::callAndRun(ObjClosure* closure)
{
    if (closure == nullptr)
    {
        std::cout << "nullptr" << std::endl;
        return;
    }

    stack.emplace_back(closure);
    frames.push_back({closure, closure->function->chunk.code.data(), static_cast<int>(stack.size()) - 1});
    run();
}

void VM::run()
{
    size_t startFrameDepth = frames.size();

    CallFrame* frame = &frames.back();
#define READ_BYTE() (*frame->ip++)
#define READ_CONST() (frame->closure->function->chunk.constants[static_cast<uint16_t>(READ_BYTE() << 8 | READ_BYTE())])

    for (;;)
    {
        switch (uint8_t instr = READ_BYTE(); static_cast<OpCode>(instr))
        {
        case OpCode::OP_DEFINE_GLOBAL_CONST:
            {
                Value val = READ_CONST();
                if (!std::holds_alternative<Obj*>(val) || std::get<Obj*>(val)->type != ObjType::STRING)
                {
                    runtimeError("Variable name must be a string.");
                    return;
                }
                std::string n = dynamic_cast<ObjString*>(std::get<Obj*>(val))->chars;
                globals[n] = stack.back();
                globalConsts.insert(n);
                stack.pop_back();
                break;
            }
        case OpCode::OP_CONSTANT:
            {
                uint8_t highByte = READ_BYTE();
                uint8_t lowByte = READ_BYTE();
                uint16_t constIdx = highByte << 8 | lowByte;
                stack.push_back(frame->closure->function->chunk.constants[constIdx]);
            }
            break;
        case OpCode::OP_NIL: stack.emplace_back(std::monostate{});
            break;
        case OpCode::OP_TRUE: stack.emplace_back(true);
            break;
        case OpCode::OP_FALSE: stack.emplace_back(false);
            break;
        case OpCode::OP_POP: stack.pop_back();
            break;
        case OpCode::OP_GET_LOCAL: stack.push_back(stack[frame->slots + READ_BYTE()]);
            break;
        case OpCode::OP_SET_LOCAL: stack[frame->slots + READ_BYTE()] = stack.back();
            break;
        case OpCode::OP_GET_GLOBAL:
            {
                Value val = READ_CONST();

                if (!std::holds_alternative<Obj*>(val) || std::get<Obj*>(val)->type != ObjType::STRING)
                {
                    runtimeError("Compiler Error: Variable name constant must be a string.");
                    return;
                }

                std::string n = dynamic_cast<ObjString*>(std::get<Obj*>(val))->chars;
                stack.push_back(globals[n]);
                break;
            }
        case OpCode::OP_DEFINE_GLOBAL:
            {
                Value val = READ_CONST();
                if (!std::holds_alternative<Obj*>(val) || std::get<Obj*>(val)->type != ObjType::STRING)
                {
                    runtimeError("Compiler Error: Variable name constant must be a string.");
                    return;
                }
                std::string n = dynamic_cast<ObjString*>(std::get<Obj*>(val))->chars;
                globals[n] = stack.back();
                stack.pop_back();
                break;
            }
        case OpCode::OP_SET_GLOBAL:
            {
                Value val = READ_CONST();
                if (!std::holds_alternative<Obj*>(val) || std::get<Obj*>(val)->type != ObjType::STRING)
                {
                    runtimeError("Compiler Error: Variable name constant must be a string.");
                    return;
                }
                std::string n = dynamic_cast<ObjString*>(std::get<Obj*>(val))->chars;

                // 如果是全局常量，报错
                if (globalConsts.contains(n))
                {
                    runtimeError(("Cannot assign to const global variable '" + n + "'.").c_str());
                    return;
                }

                if (!globals.contains(n))
                {
                    std::cerr << "Undefined var " << n << "\n";
                    std::cerr << "  Current function: ";
                    if (frames.size() > 0)
                    {
                        std::cerr << frames.back().closure->function->name << "\n";
                    }
                    std::cerr << "  Constants in chunk: ";
                    // 打印当前函数的常量列表
                    if (frames.size() > 0)
                    {
                        const Chunk& chunk = frames.back().closure->function->chunk;
                        for (size_t i = 0; i < chunk.constants.size(); ++i)
                        {
                            const Value& c = chunk.constants[i];
                            if (std::holds_alternative<Obj*>(c) && std::get<Obj*>(c)->type == ObjType::STRING)
                            {
                                std::cerr << "[" << i << ": " << dynamic_cast<ObjString*>(std::get<Obj*>(c))->chars <<
                                    "] ";
                            }
                        }
                    }
                    std::cerr << "\n";
                    return;
                }
                globals[n] = stack.back();
                stack.pop_back();
                break;
            }
        case OpCode::OP_GET_UPVALUE: stack.push_back(*frame->closure->upvalues[READ_BYTE()]->location);
            break;
        case OpCode::OP_SET_UPVALUE: *frame->closure->upvalues[READ_BYTE()]->location = stack.back();
            break;

        case OpCode::OP_EQUAL:
            {
                Value b = stack.back();
                stack.pop_back();
                Value a = stack.back();
                stack.pop_back();
                stack.emplace_back(a == b);
                break;
            }
        case OpCode::OP_STRICT_EQUAL:
            {
                Value b = stack.back();
                stack.pop_back();
                Value a = stack.back();
                stack.pop_back();

                // 严格相等
                bool result = false;
                if (a.index() == b.index())
                {
                    if (std::holds_alternative<std::monostate>(a) && std::holds_alternative<std::monostate>(b))
                    {
                        result = true;
                    }
                    else if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
                    {
                        result = std::get<bool>(a) == std::get<bool>(b);
                    }
                    else if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
                    {
                        result = std::get<double>(a) == std::get<double>(b);
                    }
                    else if (std::holds_alternative<Obj*>(a) && std::holds_alternative<Obj*>(b))
                    {
                        // 特殊处理字符串
                        auto o1 = std::get<Obj*>(a);
                        auto o2 = std::get<Obj*>(b);
                        if (o1->type == ObjType::STRING && o2->type == ObjType::STRING)
                        {
                            auto s1 = dynamic_cast<ObjString*>(o1);
                            auto s2 = dynamic_cast<ObjString*>(o2);
                            result = s1->chars == s2->chars;
                        }
                        else
                        {
                            result = o1 == o2;
                        }
                    }
                }
                stack.emplace_back(result);
                break;
            }
        case OpCode::OP_STRICT_NOT_EQUAL:
            {
                Value b = stack.back();
                stack.pop_back();
                Value a = stack.back();
                stack.pop_back();

                // 严格不相等：类型不同 或者 (类型相同但值不同)
                bool result = true;
                if (a.index() == b.index())
                {
                    if (std::holds_alternative<std::monostate>(a) && std::holds_alternative<std::monostate>(b))
                    {
                        result = false;
                    }
                    else if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
                    {
                        result = std::get<bool>(a) != std::get<bool>(b);
                    }
                    else if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
                    {
                        result = std::get<double>(a) != std::get<double>(b);
                    }
                    else if (std::holds_alternative<Obj*>(a) && std::holds_alternative<Obj*>(b))
                    {
                        result = std::get<Obj*>(a) != std::get<Obj*>(b);
                    }
                }
                stack.emplace_back(result);
                break;
            }
        case OpCode::OP_AND:
            {
                bool b = toBool(stack.back());
                stack.pop_back();
                bool a = toBool(stack.back());
                stack.pop_back();
                stack.emplace_back(a && b);
                break;
            }
        case OpCode::OP_OR:
            {
                bool b = toBool(stack.back());
                stack.pop_back();
                bool a = toBool(stack.back());
                stack.pop_back();
                stack.emplace_back(a || b);
                break;
            }
        case OpCode::OP_GREATER:
            {
                Value bVal = stack.back();
                stack.pop_back();
                Value aVal = stack.back();
                stack.pop_back();

                if (std::holds_alternative<double>(aVal) && std::holds_alternative<double>(bVal))
                {
                    double b = std::get<double>(bVal);
                    double a = std::get<double>(aVal);
                    stack.emplace_back(a > b);
                }
                else
                {
                    runtimeError("Operands must be numbers for comparison.");
                    return;
                }
                break;
            }
        case OpCode::OP_LESS:
            {
                Value bVal = stack.back();
                stack.pop_back();
                Value aVal = stack.back();
                stack.pop_back();

                if (std::holds_alternative<double>(aVal) && std::holds_alternative<double>(bVal))
                {
                    double b = std::get<double>(bVal);
                    double a = std::get<double>(aVal);
                    stack.emplace_back(a < b);
                }
                else
                {
                    runtimeError("Operands must be numbers for comparison.");
                    return;
                }
                break;
            }

        case OpCode::OP_ADD:
            {
                Value b = stack.back();
                stack.pop_back();
                Value a = stack.back();
                stack.pop_back();
                if (isObjType(a, ObjType::STRING) || isObjType(b, ObjType::STRING))
                {
                    std::string sa = valToString(a);
                    std::string sb = valToString(b);
                    stack.emplace_back(newString(sa + sb));
                }
                else if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
                {
                    stack.emplace_back(std::get<double>(a) + std::get<double>(b));
                }
                else if (std::holds_alternative<bool>(a) || std::holds_alternative<bool>(b))
                {
                    // 布尔类型转换为字符串进行拼接
                    std::string sa = valToString(a);
                    std::string sb = valToString(b);
                    stack.emplace_back(newString(sa + sb));
                }
                else
                {
                    runtimeError("Operands must be two numbers or two strings.");
                    return;
                }
                break;
            }
        case OpCode::OP_SUB:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                double a = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(a - b);
                break;
            }
        case OpCode::OP_MUL:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                double a = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(a * b);
                break;
            }
        case OpCode::OP_DIV:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                double a = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(a / b);
                break;
            }
        case OpCode::OP_NOT:
            {
                Value v = stack.back();
                stack.pop_back();
                stack.emplace_back(!toBool(v));
                break;
            }
        case OpCode::OP_MOD:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                double a = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(fmod(a, b));
                break;
            }

        case OpCode::OP_JUMP:
            {
                uint16_t o = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip += 2 + o;
                break;
            }
        case OpCode::OP_JUMP_IF_FALSE:
            {
                uint16_t o = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip += 2;
                Value v = stack.back();
                // stack.pop_back(); // 弹出值
                if (!toBool(v))
                    frame->ip += o;
                break;
            }
        case OpCode::OP_JUMP_IF_TRUE:
            {
                uint16_t o = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip += 2;
                Value v = stack.back();
                // stack.pop_back(); // 弹出值
                if (toBool(v))
                    frame->ip += o;
                break;
            }
        case OpCode::OP_LOOP:
            {
                uint16_t o = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip = frame->ip + 2 - o; // 修正跳转计算
                break;
            }

        case OpCode::OP_CALL:
            {
                int argc = READ_BYTE();
                int calleeSlot = stack.size() - 1 - argc;
                if (Value callee = stack[calleeSlot]; isObjType(callee, ObjType::CLOSURE))
                {
                    auto* cl = dynamic_cast<ObjClosure*>(std::get<Obj*>(callee));

                    if (cl->function->jitFunction == nullptr && jitEnabled)
                    {
                        JitCompiler::JitFn jitFn = jit.compile(&cl->function->chunk);
                        if (jitFn != nullptr)
                        {
                            cl->function->jitFunction = reinterpret_cast<void*>(jitFn);
                            debug_log("JIT开始编译函数{} ", cl->function->name);
                        }
                        else
                        {
                            debug_log("JIT编译函数{} 失败，回退到解释器", cl->function->name);
                        }
                    }

                    // 如果有 JIT 函数，执行 JIT 代码
                    if (cl->function->jitFunction != nullptr && jitEnabled)
                    {
                        debug_log("执行 JIT 函数 {} ", cl->function->name);
                        // 准备参数
                        double args[256]; // 假设最多 256 个参数
                        for (int i = 0; i < argc; ++i)
                        {
                            if (std::holds_alternative<double>(stack[calleeSlot + 1 + i]))
                            {
                                args[i] = std::get<double>(stack[calleeSlot + 1 + i]);
                            }
                            else
                            {
                                // 如果参数不是数字，回退到解释器执行
                                frames.push_back({cl, cl->function->chunk.code.data(), calleeSlot});
                                frame = &frames.back();
                                goto call_end;
                            }
                        }

                        // 调用 JIT 函数
                        auto jitFn = reinterpret_cast<JitCompiler::JitFn>(cl->function->jitFunction);
                        double result;
                        try
                        {
                            result = jitFn(args);
                        }
                        catch (const std::exception& e)
                        {
                            std::cout << "JIT函数执行异常: " << e.what() << std::endl;
                            frames.push_back({cl, cl->function->chunk.code.data(), calleeSlot});
                            frame = &frames.back();
                            goto call_end;
                        }
                        catch (...)
                        {
                            std::cout << "JIT函数执行未知异常" << std::endl;
                            frames.push_back({cl, cl->function->chunk.code.data(), calleeSlot});
                            frame = &frames.back();
                            goto call_end;
                        }

                        // 处理返回值
                        if (calleeSlot < 0 || static_cast<size_t>(calleeSlot) > stack.size())
                        {
                            frames.push_back({cl, cl->function->chunk.code.data(), calleeSlot});
                            frame = &frames.back();
                            goto call_end;
                        }
                        if (static_cast<size_t>(calleeSlot) > stack.size())
                        {
                            stack.clear();
                        }
                        else
                        {
                            stack.resize(calleeSlot);
                        }
                        stack.emplace_back(result);
                        debug_log("JIT函数{}调用成功", cl->function->name);
                    }
                    else
                    {
                        // 没有 JIT 函数，回退到解释器执行
                        frames.push_back({cl, cl->function->chunk.code.data(), calleeSlot});
                        frame = &frames.back();
                    }
                }
                else if (isObjType(callee, ObjType::NATIVE))
                {
                    auto* n = dynamic_cast<ObjNative*>(std::get<Obj*>(callee));
                    Value* args = &stack[calleeSlot + 1];
                    Value res = n->function(argc, args);

                    if (calleeSlot < 0 || static_cast<size_t>(calleeSlot) > stack.size())
                    {
                        stack.clear();
                    }
                    else
                    {
                        stack.resize(calleeSlot);
                    }
                    stack.push_back(res);

                    frame = &frames.back();
                }
                else if (isObjType(callee, ObjType::CLASS))
                {
                    auto* klass = dynamic_cast<ObjClass*>(std::get<Obj*>(callee));

                    ObjInstance* instance;
                    if (klass->isNative)
                    {
                        instance = allocate<ObjNativeInstance>(klass);
                    }
                    else
                    {
                        instance = allocate<ObjInstance>(klass);
                    }

                    stack[calleeSlot] = instance;

                    if (klass->nativeMethods.contains("constructor"))
                    {
                        ObjNative* init = klass->nativeMethods["constructor"];
                        Value* args = &stack[calleeSlot + 1];
                        // 调用原生 init，args[-1] 是刚创建的 instance
                        init->function(argc, args);

                        if (calleeSlot < 0 || static_cast<size_t>(calleeSlot) > stack.size())
                        {
                            stack.clear();
                        }
                        else
                        {
                            stack.resize(calleeSlot);
                        }
                        stack.emplace_back(instance); // 构造函数返回实例
                        frame = &frames.back();
                    }
                    else if (klass->methods.contains("constructor"))
                    {
                        ObjClosure* init = klass->methods["constructor"];
                        // 创建帧，开始执行 init 方法
                        frames.push_back({init, init->function->chunk.code.data(), calleeSlot});
                        frame = &frames.back();
                    }
                    else if (argc != 0)
                    {
                        runtimeError(("Expected 0 arguments but got " + std::to_string(argc) + ".").c_str());
                        return;
                    }
                }
                else if (isObjType(callee, ObjType::BOUND_METHOD))
                {
                    auto* bound = dynamic_cast<ObjBoundMethod*>(std::get<Obj*>(callee));
                    stack[calleeSlot] = bound->receiver;

                    if (bound->method->type == ObjType::CLOSURE)
                    {
                        auto* closure = dynamic_cast<ObjClosure*>(bound->method);
                        frames.push_back({closure, closure->function->chunk.code.data(), calleeSlot});
                        frame = &frames.back();
                    }
                    else if (bound->method->type == ObjType::NATIVE)
                    {
                        auto* native = dynamic_cast<ObjNative*>(bound->method);
                        Value* args = &stack[calleeSlot + 1];
                        Value res = native->function(argc, args);
                        if (calleeSlot < 0 || static_cast<size_t>(calleeSlot) > stack.size())
                        {
                            stack.clear();
                        }
                        else
                        {
                            stack.resize(calleeSlot);
                        }
                        stack.push_back(res);
                        frame = &frames.back();
                    }
                }
                else
                {
                    if (std::holds_alternative<std::monostate>(callee))
                    {
                        std::cerr << "Call failed: callee is null\n";
                    }
                    else if (std::holds_alternative<bool>(callee))
                    {
                        std::cerr << "Call failed: callee is boolean (" << std::get<bool>(callee) << ")\n";
                    }
                    else if (std::holds_alternative<double>(callee))
                    {
                        std::cerr << "Call failed: callee is number (" << std::get<double>(callee) << ")\n";
                    }
                    else if (std::holds_alternative<Obj*>(callee))
                    {
                        auto* obj = std::get<Obj*>(callee);
                        std::cerr << "Call failed: callee is Obj* of type " << static_cast<int>(obj->type) << "\n";
                    }
                    return;
                }
            call_end:
                break;
            }
        case OpCode::OP_NEW:
            {
                int argc = READ_BYTE();
                int calleeSlot = stack.size() - 1 - argc;

                // 获取类名
                Value callee = stack[calleeSlot];
                if (!std::holds_alternative<Obj*>(callee))
                {
                    runtimeError("Class name must be a class object.");
                    return;
                }

                auto obj = std::get<Obj*>(callee);
                if (!isObjType(callee, ObjType::CLASS))
                {
                    runtimeError("Can only use 'new' with a class.");
                    return;
                }

                auto* klass = dynamic_cast<ObjClass*>(obj);

                // 创建实例
                ObjInstance* instance;
                if (klass->isNative)
                {
                    instance = allocate<ObjNativeInstance>(klass);
                }
                else
                {
                    instance = allocate<ObjInstance>(klass);
                }

                // 将实例替换掉栈上的类名
                stack[calleeSlot] = instance;

                // 调用 constructor 方法
                if (klass->nativeMethods.contains("constructor"))
                {
                    ObjNative* init = klass->nativeMethods["constructor"];
                    Value* args = &stack[calleeSlot + 1];
                    // 调用原生 init，args[-1] 是刚创建的 instance
                    init->function(argc, args);

                    if (calleeSlot < 0 || static_cast<size_t>(calleeSlot) > stack.size())
                    {
                        stack.clear();
                    }
                    else
                    {
                        stack.resize(calleeSlot);
                    }
                    stack.emplace_back(instance); // 构造函数返回实例
                }
                else if (klass->methods.contains("constructor"))
                {
                    ObjClosure* init = klass->methods["constructor"];
                    // 创建帧，开始执行 init 方法
                    frames.push_back({init, init->function->chunk.code.data(), calleeSlot});
                    frame = &frames.back();
                }
                else if (argc != 0)
                {
                    runtimeError(("Expected 0 arguments but got " + std::to_string(argc) + ".").c_str());
                    return;
                }

                break;
            }
        case OpCode::OP_CLOSURE:
            {
                auto t = READ_CONST();

                // 检查常量是否可以转换为函数类型
                ObjFunction* func = nullptr;

                if (std::holds_alternative<Obj*>(t))
                {
                    func = dynamic_cast<ObjFunction*>(std::get<Obj*>(t));
                }

                // 如果不是有效的函数类型，尝试获取当前任务回调的函数（用于事件循环执行）
                if (!func || func->type != ObjType::FUNCTION)
                {
                    // 检查是否在事件循环中执行任务回调
                    if (!frames.empty() && frames.back().closure != nullptr)
                    {
                        // 使用当前执行的函数作为替代（虽然可能不正确，但避免崩溃）
                        func = frames.back().closure->function;
                    }
                    else
                    {
                        runtimeError("Expected function in OP_CLOSURE");
                        return;
                    }
                }

                auto* cl = allocate<ObjClosure>(func);
                stack.emplace_back(cl);
                for (int i = 0; i < func->upvalueCount; i++)
                {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t idx = READ_BYTE();
                    if (isLocal) cl->upvalues.push_back(captureUpvalue(&stack[frame->slots + idx]));
                    else cl->upvalues.push_back(frame->closure->upvalues[idx]);
                }
                break;
            }
        case OpCode::OP_CLOSE_UPVALUE: closeUpvalues(&stack.back());
            stack.pop_back();
            break;
        case OpCode::OP_RETURN:
            {
                Value res = stack.back();
                stack.pop_back();

                int slots = frame->slots; // 在弹出帧前保存 slots

                // 检查 slots 是否在有效范围内
                if (slots < 0 || static_cast<size_t>(slots) > stack.size())
                {
                    stack.clear();
                    frames.pop_back();
                    stack.push_back(res);
                    return;
                }

                closeUpvalues(&stack[slots]);

                frames.pop_back(); // 弹出当前帧

                if (frames.size() < startFrameDepth)
                {
                    if (static_cast<size_t>(slots) > stack.size())
                    {
                        stack.clear();
                    }
                    else
                    {
                        stack.erase(stack.begin() + slots, stack.end());
                    }
                    stack.push_back(res);
                    return;
                }

                if (static_cast<size_t>(slots) > stack.size())
                {
                    stack.clear();
                }
                else
                {
                    stack.erase(stack.begin() + slots, stack.end());
                }
                stack.push_back(res);
                frame = &frames.back();
                break;
            }
        case OpCode::OP_BUILD_LIST:
            {
                int count = READ_BYTE();
                auto* list = allocate<ObjList>(); // 创建对象
                list->elements.resize(count);
                for (int i = count - 1; i >= 0; i--)
                {
                    list->elements[i] = stack.back();
                    stack.pop_back();
                }
                stack.emplace_back(list);
                break;
            }
        case OpCode::OP_BUILD_OBJECT:
            {
                int count = READ_BYTE();
                
                auto* objClass = allocate<ObjClass>("<object>");
                auto* instance = allocate<ObjInstance>(objClass);
                for (int i = 0; i < count; i++)
                {
                    Value value = stack.back();
                    stack.pop_back();
                    Value keyVal = stack.back();
                    stack.pop_back();

                    // key 应该是字符串
                    if (!isObjType(keyVal, ObjType::STRING))
                    {
                        runtimeError("Object property key must be a string.");
                        continue;
                    }

                    const std::string key = std::get<Obj*>(keyVal) != nullptr
                        ? dynamic_cast<ObjString*>(std::get<Obj*>(keyVal))->chars
                        : "";

                    instance->fields[key] = value;
                }

                stack.emplace_back(instance);
                break;
            }
        case OpCode::OP_GET_SUBSCRIPT:
            {
                Value indexVal = stack.back();
                stack.pop_back();
                Value listVal = stack.back();
                stack.pop_back();

                if (!isObjType(listVal, ObjType::LIST))
                {
                    runtimeError("Operands must be a list.");
                    return;
                }
                if (!std::holds_alternative<double>(indexVal))
                {
                    runtimeError("Index must be a number.");
                    return;
                }

                auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(listVal));
                int index = static_cast<int>(std::get<double>(indexVal));

                if (index < 0 || index >= list->elements.size())
                {
                    runtimeError("List index out of bounds.");
                    return;
                }

                stack.push_back(list->elements[index]);
                break;
            }
        case OpCode::OP_SET_SUBSCRIPT:
            {
                Value val = stack.back();
                stack.pop_back();
                Value indexVal = stack.back();
                stack.pop_back();
                Value listVal = stack.back();
                stack.pop_back();

                if (!isObjType(listVal, ObjType::LIST))
                {
                    runtimeError("Operands must be a list.");
                    return;
                }
                auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(listVal));
                int index = static_cast<int>(std::get<double>(indexVal));

                if (index < 0 || index >= list->elements.size())
                {
                    runtimeError("List index out of bounds.");
                    return;
                }

                list->elements[index] = val;
                stack.push_back(val); // 赋值表达式返回赋的值
                break;
            }
        case OpCode::OP_CLASS:
            {
                std::string name = dynamic_cast<ObjString*>(std::get<Obj*>(READ_CONST()))->chars;
                stack.emplace_back(allocate<ObjClass>(name));
                break;
            }
        case OpCode::OP_METHOD:
            {
                std::string name = dynamic_cast<ObjString*>(std::get<Obj*>(READ_CONST()))->chars;
                Value methodVal = stack.back();
                stack.pop_back();
                auto* klass = dynamic_cast<ObjClass*>(std::get<Obj*>(stack.back()));
                klass->methods[name] = dynamic_cast<ObjClosure*>(std::get<Obj*>(methodVal));
                break;
            }
        case OpCode::OP_GET_PROPERTY:
            {
                Value val = READ_CONST();

                std::string name = dynamic_cast<ObjString*>(std::get<Obj*>(val))->chars;
                Value objVal = stack.back();

                // 检查是否是有效的对象（实例或其他可拥有属性的对象）
                if (std::holds_alternative<std::monostate>(objVal))
                {
                    runtimeError(("Cannot read property '" + name + "' of null").c_str());
                    return;
                }
                if (!std::holds_alternative<Obj*>(objVal) ||
                    (std::get<Obj*>(objVal)->type != ObjType::INSTANCE &&
                        std::get<Obj*>(objVal)->type != ObjType::CLASS &&
                        std::get<Obj*>(objVal)->type != ObjType::LIST &&
                        std::get<Obj*>(objVal)->type != ObjType::STRING))
                {
                    runtimeError("Only instances, classes, lists, or strings have properties.");
                    return;
                }

                // 处理数组的原生方法和属性
                if (isObjType(objVal, ObjType::LIST))
                {
                    if (name == "length")
                    {
                        auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(objVal));
                        stack.pop_back();
                        stack.emplace_back(static_cast<double>(list->elements.size()));
                        break;
                    }

                    if (listMethods.contains(name))
                    {
                        ObjNative* method = listMethods[name];
                        auto* bound = allocate<ObjBoundMethod>(objVal, method);
                        stack.pop_back();
                        stack.emplace_back(bound);
                        break;
                    }

                    runtimeError(("Undefined property '" + name + "' on list.").c_str());
                    return;
                }

                // 处理字符串的原生方法和属性
                if (isObjType(objVal, ObjType::STRING))
                {
                    if (name == "length")
                    {
                        auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(objVal));
                        stack.pop_back();
                        stack.emplace_back(static_cast<double>(str->chars.length()));
                        break;
                    }

                    if (stringMethods.contains(name))
                    {
                        ObjNative* method = stringMethods[name];
                        auto* bound = allocate<ObjBoundMethod>(objVal, method);
                        stack.pop_back();
                        stack.emplace_back(bound);
                        break;
                    }

                    runtimeError(("Undefined property '" + name + "' on string.").c_str());
                    return;
                }

                if (!isObjType(objVal, ObjType::INSTANCE))
                {
                    runtimeError("Only instances have properties.");
                    return;
                }
                auto* instance = dynamic_cast<ObjInstance*>(std::get<Obj*>(objVal));

                // 查找字段
                if (instance->fields.contains(name))
                {
                    stack.pop_back(); // 弹出对象
                    stack.push_back(instance->fields[name]);
                    break;
                }

                // 查找原生方法 (绑定 this)
                if (instance->klass->nativeMethods.contains(name))
                {
                    ObjNative* method = instance->klass->nativeMethods[name];
                    auto* bound = allocate<ObjBoundMethod>(objVal, method);
                    stack.pop_back();
                    stack.emplace_back(bound);
                    break;
                }

                // 查找方法 (绑定 this)
                if (instance->klass->methods.contains(name))
                {
                    ObjClosure* method = instance->klass->methods[name];
                    auto* bound = allocate<ObjBoundMethod>(objVal, method);
                    stack.pop_back();
                    stack.emplace_back(bound);
                    break;
                }

                runtimeError(("Undefined property '" + name + "'.").c_str());
                return;
            }
        case OpCode::OP_SET_PROPERTY:
            {
                Value val = READ_CONST();
                std::string name = dynamic_cast<ObjString*>(std::get<Obj*>(val))->chars;
                Value value = stack.back();
                stack.pop_back();
                Value objVal = stack.back();
                stack.pop_back();

                if (!isObjType(objVal, ObjType::INSTANCE))
                {
                    runtimeError("Only instances have fields.");
                    return;
                }
                auto* instance = dynamic_cast<ObjInstance*>(std::get<Obj*>(objVal));
                instance->fields[name] = value;
                stack.push_back(value); // 赋值表达式的值
                break;
            }
        default: break;
        }
    }
}

void VM::runWithFile(const std::string& filename)
{
    if (const std::string source = readFile(filename); !source.empty())
    {
        // 创建一个全局 exports 对象（用于 export 语句）
        auto* exportsClass = allocate<ObjClass>("exports");
        auto* exportsObj = allocate<ObjInstance>(exportsClass);
        globals["exports"] = exportsObj;

        Scanner scanner(source);
        const auto tokens = scanner.scanTokens();
        Parser parser(tokens, filename);
        const auto stmts = parser.parse();
        Compiler compiler(*this);
        ObjFunction* script = compiler.compile(stmts);
        this->interpret(script);

        // 运行事件循环，处理所有异步任务
        runEventLoop();
    }
    else
    {
        std::cerr << "Could not read file: " << filename << std::endl;
    }
}

void VM::waitForAsyncTasks()
{
    std::lock_guard lock(asyncTasksMutex);
    for (auto& task : asyncTasks)
    {
        if (task.valid())
        {
            task.wait();
        }
    }
    asyncTasks.clear();
}

void VM::runEventLoop()
{
    eventLoopRunning = true;
    // 运行事件循环，直到所有定时器都被清除
    while (true)
    {
        std::unique_lock lock(eventQueueMutex);

        // 等待新任务或超时
        eventQueueCV.wait_for(lock, std::chrono::milliseconds(100), [this]()
        {
            return !eventQueue.empty();
        });

        // 检查是否还有活动的 interval 或异步任务
        bool hasActiveInterval = false;
        {
            std::lock_guard intervalLock(intervalIdsMutex);
            hasActiveInterval = !intervalIds.empty();
        }

        bool hasRunningAsyncTasks = false;
        {
            std::lock_guard asyncLock(asyncTasksMutex);
            for (const auto& task : asyncTasks)
            {
                if (task.valid() && task.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    hasRunningAsyncTasks = true;
                    break;
                }
            }
        }

        // 如果没有任务、没有活动的 interval、没有运行中的异步任务，退出循环
        if (eventQueue.empty() && !hasActiveInterval && !hasRunningAsyncTasks)
        {
            break;
        }

        // 处理队列中的任务
        while (!eventQueue.empty())
        {
            EventTask task = eventQueue.front();
            eventQueue.pop();

            // 释放锁后再执行任务，避免死锁
            lock.unlock();

            try
            {
                // 调试信息：任务回调的状态
                if (task.callback == nullptr)
                {
                    continue; // 跳过无效任务
                }

                // 确保任务执行的堆栈是空的
                stack.clear();
                frames.clear();

                // 执行任务回调
                callAndRun(task.callback);

                // 再次确保任务执行后的堆栈状态
                stack.clear();
                frames.clear();
            }
            catch (const std::exception& e)
            {
                // 如果是 interval 任务出错，清除它
                if (task.isInterval)
                {
                    std::lock_guard intervalLock(intervalIdsMutex);
                    intervalIds.erase(task.intervalId);
                }
            }

            // 重新获取锁以检查队列
            lock.lock();
        }
    }

    eventLoopRunning = false;
}
