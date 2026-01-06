#include "native/require.h"
#include <iostream>

Value nativeRequire(VM& vm, const int argc, const Value* args)
{
    if (argc != 1 || !std::holds_alternative<Obj*>(args[0]) ||
        std::get<Obj*>(args[0])->type != ObjType::STRING)
    {
        std::cerr << "require expects a file path string.\n";
        return std::monostate{};
    }

    const std::string path = dynamic_cast<ObjString*>(std::get<Obj*>(args[0]))->chars;

    // 是否已加载模块
    if (vm.modules.contains(path))
    {
        return vm.modules[path];
    }

    const std::string source = readFile(path);
    if (source.empty())
    {
        std::cerr << "Could not open file: " << path << "\n";
        return std::monostate{};
    }

    if (!vm.compilerHook)
    {
        std::cerr << "Compiler hook not set.\n";
        return std::monostate{};
    }

    ObjFunction* moduleScript = vm.compilerHook(source);
    if (!moduleScript) return std::monostate{};

    auto* moduleClosure = vm.allocate<ObjClosure>(moduleScript);

    vm.stack.emplace_back(moduleClosure);

    vm.frames.push_back({moduleClosure, moduleScript->chunk.code.data(), static_cast<int>(vm.stack.size()) - 1});
    vm.run();

    const Value result = vm.stack.back();
    vm.stack.pop_back();
    vm.stack.pop_back();

    // 缓存模块结果
    vm.modules[path] = result;
    return result;
}
