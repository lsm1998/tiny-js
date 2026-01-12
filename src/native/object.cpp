#include "sys_object.h"
#include "object.h"

Value objectKeys(VM& vm, int argc, Value* args)
{
    if (argc < 1)
    {
        throw std::runtime_error("Object.keys() requires at least one argument.");
    }

    // 获取对象
    const Value& objVal = args[0];

    // 检查是否是实例（对象字面量或类的实例）
    if (!std::holds_alternative<Obj*>(objVal))
    {
        throw std::runtime_error("Object.keys() argument must be an object.");
    }

    Obj* obj = std::get<Obj*>(objVal);

    // 检查是否是实例类型
    if (obj->type != ObjType::INSTANCE)
    {
        throw std::runtime_error("Object.keys() argument must be an object instance.");
    }

    auto* instance = dynamic_cast<ObjInstance*>(obj);

    // 创建包含所有键的列表
    auto* keysList = vm.allocate<ObjList>();
    keysList->elements.reserve(instance->fields.size());

    for (auto it = instance->fields.begin(); it != instance->fields.end(); ++it)
    {
        keysList->elements.push_back(vm.newString(it->first));
    }

    return keysList;
}

Value objectValues(VM& vm, int argc, Value* args)
{
    if (argc < 1)
    {
        throw std::runtime_error("Object.values() requires at least one argument.");
    }

    // 获取对象
    const Value& objVal = args[0];

    // 检查是否是实例
    if (!std::holds_alternative<Obj*>(objVal))
    {
        throw std::runtime_error("Object.values() argument must be an object.");
    }

    Obj* obj = std::get<Obj*>(objVal);

    if (obj->type != ObjType::INSTANCE)
    {
        throw std::runtime_error("Object.values() argument must be an object instance.");
    }

    auto* instance = dynamic_cast<ObjInstance*>(obj);

    // 创建包含所有值的列表
    auto* valuesList = vm.allocate<ObjList>();
    valuesList->elements.reserve(instance->fields.size());

    for (auto it = instance->fields.begin(); it != instance->fields.end(); ++it)
    {
        valuesList->elements.push_back(it->second);
    }

    return valuesList;
}

Value objectEntries(VM& vm, int argc, Value* args)
{
    if (argc < 1)
    {
        throw std::runtime_error("Object.entries() requires at least one argument.");
    }

    // 获取对象
    const Value& objVal = args[0];

    // 检查是否是实例
    if (!std::holds_alternative<Obj*>(objVal))
    {
        throw std::runtime_error("Object.entries() argument must be an object.");
    }

    Obj* obj = std::get<Obj*>(objVal);

    if (obj->type != ObjType::INSTANCE)
    {
        throw std::runtime_error("Object.entries() argument must be an object instance.");
    }

    auto* instance = dynamic_cast<ObjInstance*>(obj);

    // 创建包含所有 [key, value] 对的列表
    auto* entriesList = vm.allocate<ObjList>();
    entriesList->elements.reserve(instance->fields.size());

    for (auto it = instance->fields.begin(); it != instance->fields.end(); ++it)
    {
        // 为每个 entry 创建一个 [key, value] 数组
        auto* entryArray = vm.allocate<ObjList>();
        entryArray->elements.push_back(vm.newString(it->first));
        entryArray->elements.push_back(it->second);
        entriesList->elements.push_back(entryArray);
    }

    return entriesList;
}

void registerNativeObject(VM& vm)
{
    // 注册 Object 类及其静态方法
    std::map<std::string, NativeFn> methods;
    methods["keys"] = [&vm](int argc, Value* args) -> Value
    {
        return objectKeys(vm, argc, args);
    };
    methods["values"] = [&vm](int argc, Value* args) -> Value
    {
        return objectValues(vm, argc, args);
    };
    methods["entries"] = [&vm](int argc, Value* args) -> Value
    {
        return objectEntries(vm, argc, args);
    };

    vm.defineNativeClass("Object", methods);
}
