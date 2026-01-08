#ifndef TINY_JS_VM_H
#define TINY_JS_VM_H

#include "object.h"
#include <map>
#include <unordered_set>

#include "jit.h"

struct CallFrame
{
    ObjClosure* closure;
    uint8_t* ip;
    int slots;
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

    JitCompiler jit;

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

    void defineNativeClass(const std::string& className, std::map<std::string, NativeFn> methods);

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
