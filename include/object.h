#ifndef TINY_JS_OBJECT_H
#define TINY_JS_OBJECT_H

#include "common.h"
#include "functional"
#include <map>
#include <utility>

enum class ObjType
{
    // 字符串
    STRING,
    // 函数
    FUNCTION,
    // 闭包
    CLOSURE,
    // 上值
    UPVALUE,
    // 原生函数
    NATIVE,
    // 数组
    LIST,
    // 类
    CLASS,
    // 实例
    INSTANCE,
    // 绑定方法
    BOUND_METHOD
};

struct Chunk
{
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    void write(const uint8_t byte) { code.push_back(byte); }

    int addConstant(const Value value)
    {
        constants.push_back(value);
        return static_cast<int>(constants.size()) - 1;
    }
};

enum class OpCode : uint8_t
{
    // 加载常量
    OP_CONSTANT,
    // 加载null
    OP_NIL,
    // 加载布尔值真
    OP_TRUE,
    // 加载布尔值假
    OP_FALSE,
    // 弹出栈顶
    OP_POP,
    // 读取局部变量
    OP_GET_LOCAL,
    // 设置局部变量
    OP_SET_LOCAL,
    // 读取全局变量
    OP_GET_GLOBAL,
    // 定义全局变量
    OP_DEFINE_GLOBAL,
    // 设置全局变量
    OP_SET_GLOBAL,
    // 读取上值
    OP_GET_UPVALUE,
    // 设置上值
    OP_SET_UPVALUE,
    // 相等比较
    OP_EQUAL,
    // 大于比较
    OP_GREATER,
    // 小于比较
    OP_LESS,
    // 加法
    OP_ADD,
    // 减法
    OP_SUB,
    // 乘法
    OP_MUL,
    // 除法
    OP_DIV,
    // 逻辑非
    OP_NOT,
    // 取负
    OP_NEGATE,
    // 无条件跳转
    OP_JUMP,
    // 条件跳转
    OP_JUMP_IF_FALSE,
    // 循环跳转
    OP_LOOP,
    // 函数调用
    OP_CALL,
    // 创建闭包
    OP_CLOSURE,
    // 关闭上值
    OP_CLOSE_UPVALUE,
    // 函数返回
    OP_RETURN,
    // 构建列表
    OP_BUILD_LIST,
    // 获取下标
    OP_GET_SUBSCRIPT,
    // 设置下标
    OP_SET_SUBSCRIPT,
    // 定义全局常量
    OP_DEFINE_GLOBAL_CONST,
    // 定义类
    OP_CLASS,
    // 获取属性
    OP_GET_PROPERTY,
    // 设置属性
    OP_SET_PROPERTY,
    // 方法定义
    OP_METHOD,
};

static constexpr std::array<std::string_view, 36> opCodeNames = {
    "OP_CONSTANT",
    "OP_NIL",
    "OP_TRUE",
    "OP_FALSE",
    "OP_POP",
    "OP_GET_LOCAL",
    "OP_SET_LOCAL",
    "OP_GET_GLOBAL",
    "OP_DEFINE_GLOBAL",
    "OP_SET_GLOBAL",
    "OP_GET_UPVALUE",
    "OP_SET_UPVALUE",
    "OP_EQUAL",
    "OP_GREATER",
    "OP_LESS",
    "OP_ADD",
    "OP_SUB",
    "OP_MUL",
    "OP_DIV",
    "OP_NOT",
    "OP_NEGATE",
    "OP_JUMP",
    "OP_JUMP_IF_FALSE",
    "OP_LOOP",
    "OP_CALL",
    "OP_CLOSURE",
    "OP_CLOSE_UPVALUE",
    "OP_RETURN",
    "OP_BUILD_LIST",
    "OP_GET_SUBSCRIPT",
    "OP_SET_SUBSCRIPT",
    "OP_DEFINE_GLOBAL_CONST",
    "OP_CLASS",
    "OP_GET_PROPERTY",
    "OP_SET_PROPERTY",
    "OP_METHOD"
};

struct Obj
{
    // 对象类型
    ObjType type;
    // 标记垃圾回收
    bool isMarked = false;
    // 链表指针，指向下一个对象
    Obj* next = nullptr;

    explicit Obj(const ObjType t) : type(t)
    {
    }

    virtual ~Obj() = default;
};

struct ObjString : Obj
{
    // 字符串内容
    std::string chars;

    explicit ObjString(std::string s) : Obj(ObjType::STRING), chars(std::move(s))
    {
    }
};

struct ObjFunction : Obj
{
    int arity = 0;
    int upvalueCount = 0;
    Chunk chunk;
    std::string name;
    void* jitFunction = nullptr; // 存储编译后的 JIT 函数指针

    ObjFunction() : Obj(ObjType::FUNCTION)
    {
    }
};

struct ObjUpvalue : Obj
{
    Value* location;
    Value closedValue;
    ObjUpvalue* nextUp = nullptr;

    explicit ObjUpvalue(Value* loc) : Obj(ObjType::UPVALUE), location(loc)
    {
    }
};

struct ObjClosure : Obj
{
    ObjFunction* function;
    std::vector<ObjUpvalue*> upvalues;

    explicit ObjClosure(ObjFunction* func) : Obj(ObjType::CLOSURE), function(func)
    {
    }
};


using NativeFn = std::function<Value(int argCount, Value* args)>;

struct ObjNative : Obj
{
    NativeFn function;
    std::string name;

    ObjNative(NativeFn fn, std::string n) : Obj(ObjType::NATIVE), function(std::move(fn)), name(std::move(n))
    {
    }
};

struct ObjList : Obj
{
    // 使用 vector 存储元素
    std::vector<Value> elements;

    ObjList() : Obj(ObjType::LIST)
    {
    }
};

struct ObjClass : Obj
{
    // 类名
    std::string name;
    // 方法表
    std::map<std::string, ObjClosure*> methods;
    // 原生方法
    std::map<std::string, ObjNative*> nativeMethods;
    // 是否为原生类
    bool isNative = false;

    explicit ObjClass(std::string n) : Obj(ObjType::CLASS), name(std::move(n))
    {
    }
};

struct ObjInstance : Obj
{
    ObjClass* klass;
    std::map<std::string, Value> fields; // 字段表

    explicit ObjInstance(ObjClass* c) : Obj(ObjType::INSTANCE), klass(c)
    {
    }
};

struct ObjBoundMethod : Obj
{
    Value receiver; // 绑定的 this 对象
    Obj* method;

    ObjBoundMethod(const Value r, Obj* m) : Obj(ObjType::BOUND_METHOD), receiver(r), method(m)
    {
    }
};

struct FileHandle
{
    std::fstream stream;
    std::string path;
};

struct ObjNativeInstance : ObjInstance
{
    // 原生数据指针
    void* data = nullptr;
    // 清理函数
    std::function<void(void*)> deleter;

    explicit ObjNativeInstance(ObjClass* k) : ObjInstance(k)
    {
    }

    ~ObjNativeInstance() override
    {
        if (data && deleter)
        {
            deleter(data);
            data = nullptr;
        }
    }
};

// 检查 Value 是否为指定类型的对象
inline bool isObjType(const Value val, const ObjType type)
{
    return std::holds_alternative<Obj*>(val) && std::get<Obj*>(val)->type == type;
}

// 将 Value 转换为字符串表示
inline std::string valToString(const Value val)
{
    if (std::holds_alternative<std::monostate>(val)) return "null";
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
    if (std::holds_alternative<double>(val))
    {
        double d = std::get<double>(val);
        double intPart;
        if (modf(d, &intPart) == 0.0) return std::to_string(static_cast<long long>(d));
        std::string s = std::to_string(d);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    if (std::holds_alternative<Obj*>(val))
    {
        const auto o = std::get<Obj*>(val);
        if (o->type == ObjType::STRING) return dynamic_cast<ObjString*>(o)->chars;
        if (o->type == ObjType::FUNCTION) return "<fn " + dynamic_cast<ObjFunction*>(o)->name + ">";
        if (o->type == ObjType::CLOSURE) return "<fn " + dynamic_cast<ObjClosure*>(o)->function->name + ">";
        if (o->type == ObjType::NATIVE) return "<native fn " + dynamic_cast<ObjNative*>(o)->name + ">";
        if (o->type == ObjType::LIST)
        {
            const auto list = dynamic_cast<ObjList*>(o);
            std::string result = "[";
            for (size_t i = 0; i < list->elements.size(); i++)
            {
                result += valToString(list->elements[i]);
                if (i < list->elements.size() - 1) result += ", ";
            }
            result += "]";
            return result;
        }
        if (o->type == ObjType::CLASS) return "<class " + dynamic_cast<ObjClass*>(o)->name + ">";
        if (o->type == ObjType::INSTANCE) return "<instance " + dynamic_cast<ObjInstance*>(o)->klass->name + ">";
        if (o->type == ObjType::BOUND_METHOD)
        {
            const auto* bound = dynamic_cast<ObjBoundMethod*>(o);
            if (bound->method->type == ObjType::NATIVE)
            {
                return "<native fn " + dynamic_cast<ObjNative*>(bound->method)->name + ">";
            }

            if (bound->method->type == ObjType::CLOSURE)
            {
                return "<fn " + dynamic_cast<ObjClosure*>(bound->method)->function->name + ">";
            }
            return "<bound method>";
        }
        return "object";
    }
    return "";
}

#endif //TINY_JS_OBJECT_H
