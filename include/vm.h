#ifndef TINY_JS_VM_H
#define TINY_JS_VM_H

#include "object.h"
#include <map>
#include <unordered_set>

struct CallFrame
{
    ObjClosure* closure;
    uint8_t* ip;
    int slots;
};

enum class OpCode : uint8_t
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_NEGATE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_BUILD_LIST,
    OP_GET_SUBSCRIPT,
    OP_SET_SUBSCRIPT,
    OP_DEFINE_GLOBAL_CONST,
    OP_CLASS,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_METHOD,
};

class VM
{
public:
    std::vector<Value> stack;
    std::vector<CallFrame> frames;
    std::map<std::string, Value> globals;
    std::unordered_set<std::string> globalConsts;

    Obj* objects = nullptr;
    std::vector<Obj*> grayStack;
    ObjUpvalue* openUpvalues = nullptr;
    std::vector<Obj*> tempRoots;

    size_t bytesAllocated = 0;
    size_t nextGC = 1024 * 1024;

    std::map<std::string, Value> modules;

    std::function<ObjFunction*(std::string)> compilerHook;

    // 数组原生方法
    std::map<std::string, ObjNative*> listMethods;
    // 字符串原生方法
    std::map<std::string, ObjNative*> stringMethods;

    VM()
    {
        stack.reserve(2048);
    }

    ~VM() { freeObjects(); }

    template <typename T, typename... Args>
    T* allocate(Args&&... args)
    {
        if (bytesAllocated > nextGC) collectGarbage();
        T* obj = new T(std::forward<Args>(args)...);
        obj->next = objects;
        objects = obj;
        bytesAllocated += sizeof(T);
        return obj;
    }

    void initModule();

    void bindNativeMethod(ObjType type, const std::string& name, const NativeFn& fn);

    ObjString* newString(const std::string& s);

    void freeObjects();

    void collectGarbage();

    void markRoots();

    void markValue(const Value& v);

    void markObject(Obj* o);

    void traceReferences();

    void sweep();

    void registerNative();

    ObjUpvalue* captureUpvalue(Value* local);

    void closeUpvalues(const Value* last);

    void interpret(ObjFunction* script);

    void runtimeError(const char* format);

    void callAndRun(ObjClosure* closure);

    void run();

    void runWithFile(const std::string& filename);

private:
    void defineNative(const std::string& name, const NativeFn& fn);
};

#endif //TINY_JS_VM_H
