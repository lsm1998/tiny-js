#ifndef TINY_JS_VM_H
#define TINY_JS_VM_H

#include "object.h"
#include "jit.h"
#include <map>
#include <unordered_set>
#include <future>
#include <mutex>

// 调用栈帧结构体
struct CallFrame
{
    // 执行的闭包
    ObjClosure* closure;
    // 指令指针
    uint8_t* ip;
    // 栈起始位置
    int slots;
};

class VM
{
public:
    // 操作数栈
    std::vector<Value> stack;

    // 调用栈帧
    std::vector<CallFrame> frames;

    // 全局变量表
    std::map<std::string, Value> globals;

    // 全局常量集合
    std::unordered_set<std::string> globalConsts;

    // 已分配的对象链表
    Obj* objects = nullptr;

    // 垃圾回收标记栈
    std::vector<Obj*> grayStack;

    // 当前打开的上值链表
    ObjUpvalue* openUpvalues = nullptr;

    // 临时根对象列表
    std::vector<Obj*> tempRoots;

    // 内存管理 - 已分配的字节数
    size_t bytesAllocated = 0;

    // 下一次垃圾回收的阈值
    size_t nextGC = 1024 * 1024;

    // 模块系统
    std::map<std::string, Value> modules;

    // 编译器钩子，用于自定义编译过程
    std::function<ObjFunction*(std::string, std::string)> compilerHook;

    // 数组原生方法
    std::map<std::string, ObjNative*> listMethods;
    // 字符串原生方法
    std::map<std::string, ObjNative*> stringMethods;

    // JIT 编译器
    JitCompiler jit;

    // JIT 是否启用
    bool jitEnabled{true};

    // 异步任务列表（用于 setTimeout 等）
    std::vector<std::future<void>> asyncTasks;
    std::mutex asyncTasksMutex;

    VM()
    {
        stack.reserve(2048);
    }

    ~VM() { freeObjects(); }

    // 分配新对象并添加到对象链表
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

    // 初始化模块系统
    void initModule();

    // 绑定原生方法到指定类型
    void bindNativeMethod(ObjType type, const std::string& name, const NativeFn& fn);

    // 定义原生类及其方法
    void defineNativeClass(const std::string& className, std::map<std::string, NativeFn> methods);

    // 创建新字符串对象
    ObjString* newString(const std::string& s);

    // 释放所有对象
    void freeObjects();

    // 垃圾回收
    void collectGarbage();

    // 标记根对象
    void markRoots();

    // 标记值
    void markValue(const Value& v);

    // 标记对象
    void markObject(Obj* o);

    // 跟踪引用对象
    void traceReferences();

    // 清理未标记对象
    void sweep();

    // 注册内置原生函数
    void registerNative();

    // 捕获局部变量为上值
    ObjUpvalue* captureUpvalue(Value* local);

    // 关闭上值
    void closeUpvalues(const Value* last);

    // 解释执行脚本函数
    void interpret(ObjFunction* script);

    // 运行时错误处理
    void runtimeError(const char* format);

    // 调用并执行闭包
    void callAndRun(ObjClosure* closure);

    // 主运行循环
    void run();

    // 从文件运行脚本
    void runWithFile(const std::string& filename);

    // 等待所有异步任务完成
    void waitForAsyncTasks();

    // 启用或禁用 JIT 编译
    void enableJIT(const bool enable = true) { jitEnabled = enable; }

private:
    // 定义原生函数
    void defineNative(const std::string& name, const NativeFn& fn);
};

#endif //TINY_JS_VM_H
