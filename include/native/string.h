#ifndef TINY_JS_STRING_H
#define TINY_JS_STRING_H

#include "vm.h"

Value nativeStringLength(VM& vm, int argc, const Value* args);

Value nativeListLength(VM& vm, int argc, const Value* args);

Value nativeListPush(VM& vm, int argc, const Value* args);

Value nativeListJoin(VM& vm, int argc, const Value* args);

Value nativeListClear(VM& vm, int argc, const Value* args);

Value nativeListPop(VM& vm, int argc, const Value* args);

Value nativeListAt(VM& vm, int argc, const Value* args);

Value nativeStringAt(VM& vm, int argc, const Value* args);

Value nativeStringIndexOf(VM& vm, int argc, const Value* args);

Value nativeStringSubstring(VM& vm, int argc, const Value* args);

Value nativeStringToUpper(VM& vm, int argc, const Value* args);

Value nativeStringToLower(VM& vm, int argc, const Value* args);

Value nativeStringTrim(VM& vm, int argc, const Value* args);

void registerNativeString(VM& vm);

#endif //TINY_JS_STRING_H