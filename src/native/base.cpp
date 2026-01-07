#include "native/base.h"
#include <iostream>
#include <chrono>

Value nativeNow(VM& vm, int argc, const Value* args)
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return static_cast<double>(ms);
}

Value nativePrint(VM& vm, const int argc, const Value* args)
{
    for (int i = 0; i < argc; i++) std::cout << valToString(args[i]) << (i < argc - 1 ? " " : "");
    return std::monostate{};
}

Value nativePrintln(VM& vm, const int argc, const Value* args)
{
    nativePrint(vm, argc, args);
    std::cout << std::endl;
    return std::monostate{};
}

Value nativeStringLength(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    return static_cast<double>(str->chars.size());
}

Value nativeListLength(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(receiver));
    return static_cast<double>(list->elements.size());
}

Value nativeListClear(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(receiver));
    list->elements.clear();
    return std::monostate{};
}

Value nativeListPush(VM& vm, const int argc, const Value* args)
{
    const Value receiver = args[-1];
    auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(receiver));
    for (int i = 0; i < argc; i++)
    {
        list->elements.push_back(args[i]);
    }
    return std::monostate{};
}

Value nativeListPop(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(receiver));
    if (list->elements.empty())
    {
        throw std::runtime_error("Cannot pop from an empty list.");
    }
    Value val = list->elements.back();
    list->elements.pop_back();
    return val;
}

Value nativeListJoin(VM& vm, const int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(receiver));

    std::string sep = ",";
    if (argc > 0 && std::holds_alternative<Obj*>(args[0]))
    {
        if (const auto o = std::get<Obj*>(args[0]); o->type == ObjType::STRING)
        {
            sep = dynamic_cast<ObjString*>(o)->chars;
        }
    }

    std::string res;
    for (size_t i = 0; i < list->elements.size(); i++)
    {
        res += valToString(list->elements[i]);
        if (i < list->elements.size() - 1) res += sep;
    }
    return Value{new ObjString(res)};
}

Value nativeListAt(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* list = dynamic_cast<ObjList*>(std::get<Obj*>(receiver));
    if (argc < 1 || !std::holds_alternative<double>(args[0]))
    {
        throw std::runtime_error("Index must be a number.");
    }
    const int index = static_cast<int>(std::get<double>(args[0]));
    if (index < 0 || index >= list->elements.size())
    {
        throw std::runtime_error("List index out of bounds.");
    }
    return list->elements[index];
}

Value nativeStringAt(VM& vm, const int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    if (argc < 1 || !std::holds_alternative<double>(args[0]))
    {
        throw std::runtime_error("Index must be a number.");
    }
    const int index = static_cast<int>(std::get<double>(args[0]));
    if (index < 0 || index >= str->chars.size())
    {
        throw std::runtime_error("String index out of bounds.");
    }
    return Value{new ObjString(std::string(1, str->chars[index]))};
}

Value nativeStringIndexOf(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    if (argc < 1 || !std::holds_alternative<Obj*>(args[0]) ||
        dynamic_cast<ObjString*>(std::get<Obj*>(args[0])) == nullptr)
    {
        throw std::runtime_error("Argument must be a string.");
    }
    const auto* substr = dynamic_cast<ObjString*>(std::get<Obj*>(args[0]));
    const size_t pos = str->chars.find(substr->chars);
    if (pos == std::string::npos)
    {
        return -1.0;
    }
    return static_cast<double>(pos);
}

Value nativeStringSubstring(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    if (argc < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
    {
        throw std::runtime_error("Arguments must be numbers.");
    }
    const int start = static_cast<int>(std::get<double>(args[0]));
    const int end = static_cast<int>(std::get<double>(args[1]));
    if (start < 0 || end > str->chars.size() || start > end)
    {
        throw std::runtime_error("Invalid substring indices.");
    }
    return Value{new ObjString(str->chars.substr(start, end - start))};
}

Value nativeStringToUpper(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    std::string upperStr = str->chars;
    std::transform(
        upperStr.begin(),
        upperStr.end(),
        upperStr.begin(),
        [](const unsigned char c)
        {
            return std::tolower(c);
        }
    );
    return Value{new ObjString(upperStr)};
}

Value nativeStringToLower(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    std::string lowerStr = str->chars;
    std::transform(
        lowerStr.begin(),
        lowerStr.end(),
        lowerStr.begin(),
        [](const unsigned char c)
        {
            return std::tolower(c);
        }
    );
    return Value{new ObjString(lowerStr)};
}

Value nativeStringTrim(VM& vm, int argc, const Value* args)
{
    const Value receiver = args[-1];
    const auto* str = dynamic_cast<ObjString*>(std::get<Obj*>(receiver));
    const std::string& s = str->chars;

    const size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
    {
        return vm.newString("");
    }
    const size_t end = s.find_last_not_of(" \t\n\r");
    return vm.newString(s.substr(start, end - start + 1));
}
