#include "vm.h"
#include "scanner.h"
#include "parser.h"
#include "compiler.h"
#include "native/require.h"
#include "native/base.h"
#include <iostream>

void VM::initModule()
{
    this->compilerHook = [&](const std::string& source) -> ObjFunction*
    {
        Scanner scanner(source);
        const auto tokens = scanner.scanTokens();
        Parser parser(tokens);
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
            Obj* unreached = obj;
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

    defineNative("require", [this](auto argc, auto args) -> Value
    {
        return nativeRequire(*this, argc, args);
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

    bindNativeMethod(ObjType::STRING, "length", [this](auto argCount, auto args) -> Value
                     {
                         return nativeStringLength(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::STRING, "indexOf", [this](auto argCount, auto args) -> Value
                     {
                         return nativeStringIndexOf(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::STRING, "substring", [this](auto argCount, auto args) -> Value
                     {
                         return nativeStringSubstring(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::STRING, "toUpperCase", [this](auto argCount, auto args) -> Value
                     {
                         return nativeStringToUpper(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::STRING, "toLowerCase", [this](auto argCount, auto args) -> Value
                     {
                         return nativeStringToLower(*this, argCount, args);
                     }
    );

    bindNativeMethod(ObjType::STRING, "trim", [this](auto argCount, auto args) -> Value
                     {
                         return nativeStringTrim(*this, argCount, args);
                     }
    );
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
        up->closedValue = *up->location;
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
    stack.emplace_back(closure);
    frames.push_back({closure, closure->function->chunk.code.data(), static_cast<int>(stack.size()) - 1});
    run();
}

void VM::run()
{
    size_t startFrameDepth = frames.size();
    CallFrame* frame = &frames.back();
#define READ_BYTE() (*frame->ip++)
#define READ_CONST() (frame->closure->function->chunk.constants[READ_BYTE()])

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
        case OpCode::OP_CONSTANT: stack.push_back(READ_CONST());
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
                if (!globals.contains(n))
                {
                    std::cerr << "Undefined var " << n << "\n";
                    return;
                }
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
                    return;
                }
                globals[n] = stack.back();
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
        case OpCode::OP_GREATER:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                double a = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(a > b);
                break;
            }
        case OpCode::OP_LESS:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                double a = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(a < b);
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
                stack.emplace_back(std::get<double>(stack.back()) - b);
                break;
            }
        case OpCode::OP_MUL:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(std::get<double>(stack.back()) * b);
                break;
            }
        case OpCode::OP_DIV:
            {
                double b = std::get<double>(stack.back());
                stack.pop_back();
                stack.emplace_back(std::get<double>(stack.back()) / b);
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
                if (std::holds_alternative<std::monostate>(v) || (std::holds_alternative<bool>(v) && !std::get<
                    bool>(v)))
                    frame->ip += o;
                break;
            }
        case OpCode::OP_LOOP:
            {
                uint16_t o = (frame->ip[0] << 8) | frame->ip[1];
                frame->ip -= (o - 2);
                break;
            }

        case OpCode::OP_CALL:
            {
                int argc = READ_BYTE();
                int calleeSlot = stack.size() - 1 - argc;
                if (Value callee = stack[calleeSlot]; isObjType(callee, ObjType::CLOSURE))
                {
                    auto* cl = dynamic_cast<ObjClosure*>(std::get<Obj*>(callee));
                    frames.push_back({cl, cl->function->chunk.code.data(), calleeSlot});
                    frame = &frames.back();
                }
                else if (isObjType(callee, ObjType::NATIVE))
                {
                    auto* n = dynamic_cast<ObjNative*>(std::get<Obj*>(callee));
                    Value* args = &stack[calleeSlot + 1];
                    Value res = n->function(argc, args);

                    stack.resize(calleeSlot);
                    stack.push_back(res);

                    frame = &frames.back();
                }
                else if (isObjType(callee, ObjType::CLASS))
                {
                    auto* klass = dynamic_cast<ObjClass*>(std::get<Obj*>(callee));
                    auto* instance = allocate<ObjInstance>(klass);

                    // 将栈上的 Class 替换为 Instance (作为 this)
                    stack[calleeSlot] = instance;

                    // 检查是否有构造函数 'constructor'
                    if (klass->methods.contains("constructor"))
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
                        stack.resize(calleeSlot);
                        stack.push_back(res);
                        frame = &frames.back();
                    }
                }
                else
                {
                    std::cerr << "Call failed\n";
                    return;
                }
                break;
            }
        case OpCode::OP_CLOSURE:
            {
                auto* func = dynamic_cast<ObjFunction*>(std::get<Obj*>(READ_CONST()));
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

                closeUpvalues(&stack[frame->slots]);

                frames.pop_back(); // 弹出当前帧

                if (frames.size() < startFrameDepth)
                {
                    stack.erase(stack.begin() + frame->slots, stack.end());
                    stack.push_back(res);
                    return;
                }

                stack.erase(stack.begin() + frame->slots, stack.end());
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

                // 查找方法 (绑定 this)
                if (instance->klass->methods.contains(name))
                {
                    ObjClosure* method = instance->klass->methods[name];
                    auto* bound = allocate<ObjBoundMethod>(objVal, method);
                    stack.pop_back(); // 弹出对象
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
        Scanner scanner(source);
        const auto tokens = scanner.scanTokens();
        Parser parser(tokens);
        const auto stmts = parser.parse();
        Compiler compiler(*this);
        ObjFunction* script = compiler.compile(stmts);
        this->interpret(script);
    }
    else
    {
        std::cerr << "Could not read file: " << filename << std::endl;
    }
}
