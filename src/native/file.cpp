#include "native/file.h"
#include <filesystem>

void registerNativeFile(VM& vm)
{
    auto methods = std::map<std::string, NativeFn>{};
    methods["constructor"] = [](const int argc, const Value* args) -> Value
    {
        if (argc < 1) return std::monostate{};

        const std::string path = valToString(args[0]);
        const std::string modeStr = (argc > 1) ? valToString(args[1]) : "r";

        auto* instance = dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]));

        auto* handle = new FileHandle();
        handle->path = path;

        std::ios_base::openmode mode = std::ios::in;
        if (modeStr == "w") mode = std::ios::out;
        else if (modeStr == "a") mode = std::ios::app;

        // 打开流
        handle->stream.open(path, mode);

        // 绑定数据
        instance->data = handle;

        instance->deleter = [](void* p)
        {
            auto* h = static_cast<FileHandle*>(p);
            if (h->stream.is_open()) h->stream.close();
            delete h;
        };
        return std::monostate{};
    };
    methods["write"] = [](const int argc, const Value* args) -> Value
    {
        if (auto* handle = static_cast<FileHandle*>(dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]))->data);
            handle && handle->stream.is_open() && argc > 0)
        {
            handle->stream << valToString(args[0]);
        }
        return std::monostate{};
    };
    methods["read"] = [&vm](int argc, const Value* args) -> Value
    {
        const auto* handle = static_cast<FileHandle*>(dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]))->data);

        if (!handle || !handle->stream.is_open()) return std::monostate{};

        std::stringstream buffer;
        buffer << handle->stream.rdbuf();
        return vm.newString(buffer.str());
    };
    methods["close"] = [](const int argc, const Value* args) -> Value
    {
        if (auto* handle = static_cast<FileHandle*>(dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]))->data);
            handle && handle->stream.is_open())
            handle->stream.close();
        return std::monostate{};
    };
    methods["isOpen"] = [](const int argc, const Value* args) -> Value
    {
        if (const auto* handle = static_cast<FileHandle*>(dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]))->
                data);
            handle)
        {
            return handle->stream.is_open();
        }
        return false;
    };

    methods["size"] = [](const int argc, const Value* args) -> Value
    {
        auto* handle = static_cast<FileHandle*>(dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]))->data);

        if (!handle) return -1.0;

        if (handle->stream.is_open())
        {
            handle->stream.flush();
        }

        std::error_code ec;
        const auto size = std::filesystem::file_size(handle->path, ec);

        if (ec)
        {
            return -1.0;
        }

        return static_cast<double>(size);
    };

    methods["remove"] = [](const int argc, const Value* args) -> Value
    {
        auto* handle = static_cast<FileHandle*>(dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(args[-1]))->data);

        if (!handle) return false;

        if (handle->stream.is_open())
        {
            handle->stream.close();
        }
        return std::remove(handle->path.c_str()) == 0;
    };

    vm.defineNativeClass("File", methods);
}
