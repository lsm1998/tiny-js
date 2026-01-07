#ifndef TINY_JS_FILE_H
#define TINY_JS_FILE_H

#include "vm.h"

template <typename T>
T* getNativeData(const Value obj)
{
    auto* instance = dynamic_cast<ObjNativeInstance*>(std::get<Obj*>(obj));
    return static_cast<T*>(instance->data);
}

void registerNativeFile(VM& vm);

#endif //TINY_JS_FILE_H
