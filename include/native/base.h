#ifndef TINY_JS_BASE_H
#define TINY_JS_BASE_H

#include "vm.h"

Value nativePrint(VM& vm, int argc, const Value* args);

Value nativeNow(VM& vm, int argc, const Value* args);

Value nativeStringLength(VM& vm, int argc, const Value* args);

Value nativeListLength(VM& vm, int argc, const Value* args);

Value nativeListPush(VM& vm, int argc, const Value* args);

Value nativeListJoin(VM& vm, int argc, const Value* args);

Value nativeListClear(VM& vm, int argc, const Value* args);

Value nativeListPop(VM& vm, int argc, const Value* args);

Value nativeListAt(VM& vm, int argc, const Value* args);

Value nativeStringIndexOf(VM& vm, int argc, const Value* args);

Value nativeStringSubstring(VM& vm, int argc, const Value* args);

Value nativeStringToUpper(VM& vm, int argc, const Value* args);

Value nativeStringToLower(VM& vm, int argc, const Value* args);

Value nativeStringTrim(VM& vm, int argc, const Value* args);

#endif //TINY_JS_BASE_H
