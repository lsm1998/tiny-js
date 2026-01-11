#include "native/require.h"
#include "object.h"
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
        std::cerr << "Could not open file: " << path << " (tried '" << path << "' and 'scripts/" << path << "')\n";
        return std::monostate{};
    }

    if (!vm.compilerHook)
    {
        std::cerr << "Compiler hook not set.\n";
        return std::monostate{};
    }

    // 保存旧的 exports 对象
    Value oldExports = std::monostate{};
    const bool hadExports = vm.globals.contains("exports");
    if (hadExports)
    {
        oldExports = vm.globals["exports"];
    }

    // 创建一个空的 exports 类和实例
    auto* exportsClass = vm.allocate<ObjClass>("exports");
    auto* exportsObj = vm.allocate<ObjInstance>(exportsClass);
    vm.tempRoots.push_back(exportsObj);

    // 将 exports 注入到全局变量中
    vm.globals["exports"] = exportsObj;

    // 编译模块
    ObjFunction* moduleScript = vm.compilerHook(source, path);
    if (!moduleScript)
    {
        // 恢复旧的 exports 对象
        if (hadExports)
        {
            vm.globals["exports"] = oldExports;
        }
        else
        {
            vm.globals.erase("exports");
        }
        vm.tempRoots.pop_back();
        return std::monostate{};
    }

    // 创建模块闭包并使用 callAndRun
    auto* moduleClosure = vm.allocate<ObjClosure>(moduleScript);
    vm.callAndRun(moduleClosure);

    // 恢复旧的 exports 对象
    if (hadExports)
    {
        vm.globals["exports"] = oldExports;
    }
    else
    {
        vm.globals.erase("exports");
    }

    vm.tempRoots.pop_back();

    // 返回 exports 对象
    vm.modules[path] = exportsObj;
    return exportsObj;
}
